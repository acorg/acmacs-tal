#include "acmacs-base/to-json.hh"
#include "acmacs-base/date.hh"
#include "acmacs-tal/json-export.hh"
#include "acmacs-tal/tree.hh"

static to_json::object export_node(const acmacs::tal::v3::Node& node);

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::json_export(const Tree& tree, size_t indent)
{
    const auto json = to_json::object(to_json::key_val("_", fmt::format("-*- js-indent-level: {} -*-", indent)),
                                      to_json::key_val("  version", "phylogenetic-tree-v3"),
                                      to_json::key_val("  date", date::current_date_time()),
                                      to_json::key_val("tree", export_node(tree)));
    return fmt::format(fmt::format("{{:{}}}", indent), json);

} // acmacs::tal::v3::json_export

// ----------------------------------------------------------------------

to_json::object export_node(const acmacs::tal::v3::Node& node)
{
    to_json::object result;
    if (!node.seq_id.empty())
        result << to_json::key_val("n", node.seq_id);
    if (!node.edge_length.is_zero())
        result << to_json::key_val("l", node.edge_length.as_string());
    if (!node.subtree.empty()) {
        to_json::array subtree;
        for (const auto& sub_node : node.subtree)
            subtree << export_node(sub_node);
        result << to_json::key_val("t", std::move(subtree));
    }
    return result;

} // export_node

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
