#include "acmacs-base/argv.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-tal/newick.hh"
#include "acmacs-tal/export.hh"

// ----------------------------------------------------------------------

using namespace acmacs::argv;
struct Options : public argv
{
    Options(int a_argc, const char* const a_argv[], on_error on_err = on_error::exit) : argv() { parse(a_argc, a_argv, on_err); }

    option<str> db_dir{*this, "db-dir"};
    option<str> seqdb{*this, "seqdb"};

    option<str_array> settings_files{*this, 's'};
    option<str_array> defines{*this, 'D', "--define"};
    option<bool>      no_whocc{*this, "no-whocc", desc{"init settings without whocc defaults (clades, hz sections)"}};
    option<str>       report_cumulative{*this, "report-cumulative"};
    option<bool>      report_hz_section_antigens{*this, "report-hz-section-antigens"};
    option<bool>      show_aa_at_pos{*this, "show-aa-at-pos", desc{"show aa_at_pos section if --init-settings was used"}};
    option<size_t>    aa_at_pos_hz_section_threshold{*this, "aa-at-pos-hz-section-threshold", dflt{100UL}, desc{"if --init-settings and --show-aa-at-pos, detect hz sections with this threshold"}};
    option<size_t>    aa_at_pos_small_section_threshold{*this, "aa-at-pos-small-section-threshold", dflt{3UL}, desc{"if --init-settings and --show-aa-at-pos, elminate small sections having no more leaf nodes than this value"}};
    option<bool>      not_show_hz_sections{*this, "not-show-hz-sections"};
    option<bool>      hz_sections_report{*this, "hz-sections-report"};
    option<str>       report_first_node_of_subtree{*this, "report-first-node-of-subtree", desc{"filename or - to report subtree data"}};
    option<size_t>    subtree_threshold{*this, "subtree-threshold", desc{"min number of leaf nodes in a subtree for --report-first-node-of-subtree"}};
    option<str>       list_ladderized{*this, "list-ladderized"};
    option<bool>      no_draw{*this, "no-draw", desc{"do not generate pdf"}};
    option<str>       chart{*this, "chart", desc{"path to a chart for the signature page"}};
    option<bool>      open{*this, "open"};
    option<bool>      ql{*this, "ql"};
    option<bool>      verbose{*this, 'v', "verbose"};

    argument<str> tree_file{*this, arg_name{"tree.newick|tree.json[.xz]"}, mandatory};
    argument<str_array> outputs{*this, arg_name{".pdf, .json[.xz], .html, .txt"}, mandatory};
};

int main(int argc, const char *argv[])
{
    try {
        Options opt(argc, argv);
        acmacs::seqdb::setup(opt.seqdb);

        auto tree = acmacs::tal::newick_import(opt.tree_file);
        tree.cumulative_calculate();
        for (const auto& output : *opt.outputs)
            acmacs::tal::export_tree(tree, output);

        return 0;
    }
    catch (std::exception& err) {
        fmt::print(stderr, "ERROR: {}\n", err);
        return 1;
    }
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
