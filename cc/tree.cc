#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_calculate()
{
    if (cumulative_edge_length == EdgeLengthNotSet) {
        EdgeLength cumulative{0.0};
        const auto leaf = [&cumulative](Node& node) { node.cumulative_edge_length = cumulative + node.edge_length; };
        const auto pre = [&cumulative](Node& node) {
            node.cumulative_edge_length = cumulative + node.edge_length;
            cumulative = node.cumulative_edge_length;
        };
        const auto post = [&cumulative](Node& node) { cumulative -= node.edge_length; };

        tree::iterate_leaf_pre_post(*this, leaf, pre, post);
    }

} // acmacs::tal::v3::Tree::cumulative_calculate

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_reset()
{
    tree::iterate_leaf(*this, [](Node& node) { node.cumulative_edge_length = EdgeLengthNotSet; });

} // acmacs::tal::v3::Tree::cumulative_reset

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::Tree::report_cumulative(CumulativeReport report)
{
    cumulative_calculate();

    std::vector<std::pair<SeqId, EdgeLength>> nodes;
    tree::iterate_leaf(*this, [&nodes](Node& node) { nodes.emplace_back(node.seq_id, node.cumulative_edge_length); });
    std::sort(std::begin(nodes), std::end(nodes), [](const auto& e1, const auto& e2) { return e1.second > e2.second; });

    std::string result;
    if (report == CumulativeReport::all) {
        for (const auto& entry : nodes)
            result.append(fmt::format("{:.10f} {}\n", entry.second.as_number(), entry.first));
    }
    return result;

} // acmacs::tal::v3::Tree::report_cumulative

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
