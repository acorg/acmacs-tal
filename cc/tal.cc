#include <unistd.h>
#include <signal.h>

#include "acmacs-base/string-compare.hh"
#include "acmacs-base/argv.hh"
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

    option<bool>      interactive{*this, 'i', "interactive"};
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

static void signal_handler(int sig_num);

int main(int argc, const char* argv[])
{
    using namespace std::string_view_literals;
    try {
        acmacs::log::register_enabler_acmacs_base();
        acmacs::log::register_enabler("clades"sv, acmacs::log::clades);
        acmacs::log::register_enabler("coloring"sv, acmacs::log::coloring);
        acmacs::log::register_enabler("tree"sv, acmacs::log::tree);
        acmacs::log::register_enabler("hz-sections"sv, acmacs::log::hz_sections);
        acmacs::log::register_enabler("time-series"sv, acmacs::log::time_series);
        AD_INFO("-v arguments: {}", acmacs::log::registered_enablers());

        Options opt(argc, argv);
        acmacs::seqdb::setup(opt.seqdb);
        acmacs::log::enable(opt.verbose);
        acmacs::log::enable("hz-sections"sv);

        acmacs::tal::Tal tal;
        tal.import_tree(opt.tree_file);
        tal.import_chart(opt.chart_file);

        acmacs::tal::Settings settings{tal};
        settings.load_from_conf({"tal.json"sv, "clades.json"sv, "vaccines.json"sv});
        settings.load(opt.settings_files);
        settings.set_defines(opt.defines);

        if (opt.interactive)
            signal(SIGHUP, signal_handler);

        for (;;) {
            tal.import_tree(opt.tree_file);

            settings.apply("tal-default"sv);

            if (opt.chart_file) {
                acmacs::tal::AntigenicMaps* maps{tal.draw().layout().find<acmacs::tal::AntigenicMaps>()};
                if (!maps)
                    throw std::runtime_error{"internal: AntigenicMaps not found in layout"};
                acmacs::tal::MapsSettings& maps_settings{maps->maps_settings()};
                maps_settings.load_from_conf({"mapi.json"sv, "tal.json"sv, "clades.json"sv, "vaccines.json"sv});
                maps_settings.load(opt.settings_files);
                maps_settings.set_defines(opt.defines);
            }

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
                    if (acmacs::string::endswith(output, ".pdf"sv) || acmacs::string::endswith(output, ".html"sv) || acmacs::string::endswith(output, ".txt"sv) ||
                        acmacs::string::endswith(output, ".text"sv))
                        acmacs::open_or_quicklook(opt.open, opt.ql, output, 2);
                }
            }

            if (!opt.interactive)
                break;

            // ----------------------------------------------------------------------
            // interactive

            fmt::print(stderr, "tal-i >> ");

            fd_set set;
            FD_ZERO(&set);
            FD_SET(0, &set);
            if (select(FD_SETSIZE, &set, nullptr, nullptr, nullptr) > 0) {
                // fmt::print(stderr, " (reading)\n");
                std::string input(20, ' ');
                if (const auto bytes = ::read(0, input.data(), input.size()); bytes > 0) {
                    input.resize(static_cast<size_t>(bytes));
                    // fmt::print(stderr, " (read {} bytes)\n", bytes);
                }
            }

            acmacs::run_and_detach({"tink"}, 0);

            fmt::print(stderr, "\n> {sep}\n> {date}\n> {sep}\n\n", fmt::arg("sep", "======================================================================================================================================================"),
                       fmt::arg("date", date::current_date_time()));
            tal.reset();
            try {
                settings.reload();
            }
            catch (std::exception& err) {
                AD_ERROR("{}", err);
                acmacs::run_and_detach({"submarine"}, 0);
            }
        }

        // AD_INFO("tal configuration docs: {}/share/doc/tal-conf.org", acmacs::acmacsd_root());

        return 0;
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR {}\n", err);
        return 1;
    }
}

// ----------------------------------------------------------------------

void signal_handler(int sig_num)
{
    fmt::print("SIGNAL {}\n", sig_num);

} // signal_handler

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
