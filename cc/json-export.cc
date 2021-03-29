#include "acmacs-base/to-json.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-tal/json-export.hh"
#include "acmacs-tal/tree.hh"

static to_json::object export_node(const acmacs::tal::v3::Node& node);

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::json_export(const Tree& tree, size_t indent)
{
    // Timeit ti{"exporting tree to json"};
    tree.cumulative_calculate();
    auto json = to_json::object(to_json::key_val("_", fmt::format("-*- js-indent-level: {} -*-", indent)),
                                to_json::key_val("  version", "phylogenetic-tree-v3"),
                                to_json::key_val("  date", date::current_date_time()));
    json << to_json::key_val_if_not_empty("v", tree.virus_type())
         << to_json::key_val_if_not_empty("l", tree.lineage())
         << to_json::key_val("tree", export_node(tree));
    return fmt::format(fmt::format("{{:{}}}", indent), json);

} // acmacs::tal::v3::json_export

// ----------------------------------------------------------------------

to_json::object export_node(const acmacs::tal::v3::Node& node)
{
    to_json::object result;
    if (node.is_leaf()) {
        result << to_json::key_val("n", *node.seq_id)
               << to_json::key_val_if_true("H", node.hidden)
               << to_json::key_val_if_not_empty("a", *node.aa_sequence)
               << to_json::key_val_if_not_empty("N", *node.nuc_sequence)
               << to_json::key_val_if_not_empty("d", node.date)
               << to_json::key_val_if_not_empty("C", node.continent)
               << to_json::key_val_if_not_empty("D", node.country);
        if (!node.hi_names.empty())
            result << to_json::key_val("h", to_json::array(std::begin(node.hi_names), std::end(node.hi_names)));
    }
    if (!node.edge_length.is_zero())
        result << to_json::key_val("l", to_json::raw(node.edge_length.as_string()));
    if (node.cumulative_edge_length >= acmacs::tal::v3::EdgeLength{0.0})
        result << to_json::key_val("c", to_json::raw(node.cumulative_edge_length.as_string()));
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
