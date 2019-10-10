#include "acmacs-base/read-file.hh"
#include "acmacs-tal/settings.hh"

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Settings::apply_built_in(std::string_view name) const
{
    // printenv();
    if (name == "report-cumulative") {
        if (const auto output_filename = getenv("report-cumulative-output", ""); !output_filename.empty())
            acmacs::file::write(output_filename, tree_.report_cumulative(getenv("all", false) ? Tree::CumulativeReport::all : Tree::CumulativeReport::clusters));
    }
    else if (name == "report-selected") {
        select_and_report_nodes(getenv("select"), true);
    }
    else if (name == "seqdb") {
        if (getenv("apply", false))
            tree_.match_seqdb(getenv("filename", ""));
    }
    else if (name == "ladderize") {
        if (const auto method = getenv("method", "number-of-leaves"); method == "number-of-leaves")
            tree_.ladderize(Tree::Ladderize::NumberOfLeaves);
        else if (method == "max-edge-length")
            tree_.ladderize(Tree::Ladderize::MaxEdgeLength);
        else
            throw acmacs::settings::error{fmt::format("unsupported ladderize method: {}", method)};
    }
    else if (name == "re-root") {
        tree_.re_root(SeqId{getenv("new-root", "re-root: new-root not specified")});
    }
    else
        return false;
    return true;

} // acmacs::tal::v3::Settings::apply_built_in

// ----------------------------------------------------------------------

acmacs::tal::v3::NodeConstSet acmacs::tal::v3::Settings::select_and_report_nodes(const rjson::value& criteria, bool report) const
{
    const auto selected = select_nodes(criteria);
    if (report)
        report_nodes(fmt::format("INFO: {} selected nodes {}\n", selected.size(), criteria), "  ", selected);
    return selected;

} // acmacs::tal::v3::Settings::select_and_report_nodes

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::report_nodes(std::string_view prefix, std::string_view indent, const NodeConstSet& nodes) const
{
    fmt::print("{}", prefix);
    for (const auto* node : nodes)
        fmt::print("{}{} cumul:{}\n", indent, node->seq_id, node->cumulative_edge_length);

} // acmacs::tal::v3::Settings::report_nodes

// ----------------------------------------------------------------------

acmacs::tal::v3::NodeConstSet acmacs::tal::v3::Settings::select_nodes(const rjson::value& criteria) const
{
    NodeConstSet nodes;
    rjson::for_each(criteria, [&nodes,this,update=Tree::Select::Init](const std::string& key, const rjson::value& val) mutable {
        if (key == "cumulative >=") {
            tree_.select_cumulative(nodes, update, val.to<double>());
        }
        else
            throw acmacs::settings::error{fmt::format("unrecognized select node criterium: {}", key)};
            update = Tree::Select::Update;
    });
    return nodes;

} // acmacs::tal::v3::Settings::select_nodes

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
