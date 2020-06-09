#include "acmacs-base/argv.hh"
#include "acmacs-base/acmacsd.hh"
#include "acmacs-base/filesystem.hh"
#include "acmacs-base/quicklook.hh"
#include "acmacs-base/timeit.hh"
// #include "acmacs-base/string-split.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/settings.hh"
#include "acmacs-tal/antigenic-maps.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    // option<str> db_dir{*this, "db-dir"};
    option<str> seqdb{*this, "seqdb"};

    option<str_array> settings_files{*this, 's'};
    option<str_array> defines{*this, 'D', "--define", desc{"see $ACMACSD_ROOT/share/doc/tal-conf.org"}};
    option<str>       chart_file{*this, "chart", desc{"path to a chart for the signature page"}};
    option<size_t>    first_last_leaves{*this, "first-last-leaves", desc{"min num of leaves per node to print"}};
    option<bool>      open{*this, "open"};
    option<bool>      ql{*this, "ql"};
    option<str_array> verbose{*this, 'v', "verbose", desc{"comma separated list (or multiple switches) of enablers"}};

    argument<str> tree_file{*this, arg_name{"tree.newick|tree.phy|tree.json[.xz]|tjz"}, mandatory};
    argument<str_array> outputs{*this, arg_name{".pdf, .json[.xz], .html, .names, /json, /names"}}; // , mandatory};

    // option<bool>      no_whocc{*this, "no-whocc", desc{"init settings without whocc defaults (clades, hz sections)"}};
    // option<str>       report_cumulative{*this, "report-cumulative"};
    // option<bool>      report_hz_section_antigens{*this, "report-hz-section-antigens"};
    // option<bool>      show_aa_at_pos{*this, "show-aa-at-pos", desc{"show aa_at_pos section if --init-settings was used"}};
    // option<size_t>    aa_at_pos_hz_section_threshold{*this, "aa-at-pos-hz-section-threshold", dflt{100UL}, desc{"if --init-settings and --show-aa-at-pos, detect hz sections with this threshold"}};
    // option<size_t>    aa_at_pos_small_section_threshold{*this, "aa-at-pos-small-section-threshold", dflt{3UL}, desc{"if --init-settings and --show-aa-at-pos, elminate small sections having no more leaf nodes than this value"}};
    // option<bool>      not_show_hz_sections{*this, "not-show-hz-sections"};
    // option<bool>      hz_sections_report{*this, "hz-sections-report"};
    // option<str>       report_first_node_of_subtree{*this, "report-first-node-of-subtree", desc{"filename or - to report subtree data"}};
    // option<size_t>    subtree_threshold{*this, "subtree-threshold", desc{"min number of leaf nodes in a subtree for --report-first-node-of-subtree"}};
    // option<str>       list_ladderized{*this, "list-ladderized"};
    // option<bool>      no_draw{*this, "no-draw", desc{"do not generate pdf"}};
};

int main(int argc, const char *argv[])
{
    using namespace std::string_view_literals;
    try {
        acmacs::log::register_enabler_acmacs_base();
        acmacs::log::register_enabler("clades"sv, acmacs::log::clades);
        acmacs::log::register_enabler("coloring"sv, acmacs::log::coloring);
        acmacs::log::register_enabler("tree"sv, acmacs::log::tree);
        acmacs::log::register_enabler("hz-sections"sv, acmacs::log::hz_sections);
        AD_INFO("-v arguments: {}", acmacs::log::registered_enablers());

        Options opt(argc, argv);
        acmacs::seqdb::setup(opt.seqdb);
        acmacs::log::enable(opt.verbose);
        acmacs::log::enable("hz-sections"sv);

        const report_time report{do_report_time(acmacs::log::is_enabled(acmacs::log::timer))};

        acmacs::tal::Tal tal;
        Timeit time_loading_tree(">>>> Loading tree: ", report);
        tal.import_tree(opt.tree_file);
        time_loading_tree.report();
        Timeit time_loading_chart(">>>> Loading chart: ", report);
        tal.import_chart(opt.chart_file);
        time_loading_chart.report();

        Timeit time_loading_settings(">>>> Loading settings: ", report);
        acmacs::tal::Settings settings{tal};
        for (const auto& settings_file_name : {"tal.json"sv, "clades.json"sv, "vaccines.json"sv}) {
            if (const auto filename = fmt::format("{}/share/conf/{}", acmacs::acmacsd_root(), settings_file_name); fs::exists(filename)) {
                AD_LOG(acmacs::log::settings, "loading {}", filename);
                settings.load(filename);
            }
            else
                fmt::print(stderr, ">> WARNING cannot load \"{}\": file not found\n", filename);
        }
        settings.load(opt.settings_files);
        for (const auto& def : *opt.defines) {
            if (const auto pos = def.find('='); pos != std::string_view::npos) {
                const auto val_s = def.substr(pos + 1);
                if (val_s == "-") { // parsed as -0
                    settings.setenv(def.substr(0, pos), rjson::v3::parse_string(fmt::format("\"{}\"", val_s)));
                }
                else {
                    try {
                        settings.setenv(def.substr(0, pos), rjson::v3::parse_string(val_s));
                    }
                    catch (std::exception&) {
                        settings.setenv(def.substr(0, pos), rjson::v3::parse_string(fmt::format("\"{}\"", val_s)));
                    }
                }
            }
            else
                settings.setenv(def, "true"sv);
        }
        time_loading_settings.report();

        // Timeit time_applying_settings(">>>> Applying settings: ", report);
        settings.apply("tal-default"sv);
        // time_applying_settings.report();

        if (opt.chart_file)
            tal.draw().layout().find<acmacs::tal::AntigenicMaps>()->chart_draw_settings().load(opt.settings_files, opt.defines);

        // Timeit time_preparing(">>>> preparing: ", report);
        tal.prepare();
        // time_preparing.report();

        if (opt.first_last_leaves.has_value())
            tal.tree().report_first_last_leaves(opt.first_last_leaves);

        // Timeit time_exporting(">>>> exporting: ", report);
        for (const auto& output : *opt.outputs)
            tal.export_tree(output);
        // time_exporting.report();

        if (opt.open || opt.ql) {
            for (const auto& output : *opt.outputs) {
                if (output.substr(output.size() - 4) == ".pdf")
                    acmacs::open_or_quicklook(opt.open, opt.ql, output, 2);
            }
        }

        AD_INFO("tal configuration docs: {}/share/doc/tal-conf.org", acmacs::acmacsd_root());
        return 0;
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR {}\n", err);
        return 1;
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
