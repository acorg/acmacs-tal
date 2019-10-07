#include <algorithm>

#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_calculate() const
{
    if (cumulative_edge_length == EdgeLengthNotSet) {
        EdgeLength cumulative{0.0};
        const auto leaf = [&cumulative](const Node& node) { node.cumulative_edge_length = cumulative + node.edge_length; };
        const auto pre = [&cumulative](const Node& node) {
            node.cumulative_edge_length = cumulative + node.edge_length;
            cumulative = node.cumulative_edge_length;
        };
        const auto post = [&cumulative](const Node& node) { cumulative -= node.edge_length; };

        tree::iterate_leaf_pre_post(*this, leaf, pre, post);
    }

} // acmacs::tal::v3::Tree::cumulative_calculate

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_reset() const
{
    tree::iterate_leaf(*this, [](const Node& node) { node.cumulative_edge_length = EdgeLengthNotSet; });

} // acmacs::tal::v3::Tree::cumulative_reset

// ----------------------------------------------------------------------

struct CumulativeEntry
{
    acmacs::tal::SeqId seq_id;
    acmacs::tal::EdgeLength edge_length;
    acmacs::tal::EdgeLength gap_to_next{acmacs::tal::EdgeLengthNotSet};

    CumulativeEntry(const acmacs::tal::SeqId& a_seq_id, acmacs::tal::EdgeLength a_edge) : seq_id{a_seq_id}, edge_length{a_edge} {}
    constexpr bool operator<(const CumulativeEntry& rhs) const { return edge_length > rhs.edge_length; }
    void set_gap(acmacs::tal::EdgeLength next_edge) { gap_to_next = edge_length - next_edge; }
};

std::string acmacs::tal::v3::Tree::report_cumulative(CumulativeReport report) const
{
    cumulative_calculate();

    std::vector<CumulativeEntry> nodes;
    tree::iterate_leaf(*this, [&nodes](const Node& node) { nodes.emplace_back(node.seq_id, node.cumulative_edge_length); });
    std::sort(std::begin(nodes), std::end(nodes));
    for (auto it = std::begin(nodes); it != std::prev(std::end(nodes)); ++it)
        it->set_gap(std::next(it)->edge_length);
    // const auto gap_average = std::accumulate(std::begin(nodes), std::prev(std::end(nodes)), EdgeLength{0.0}, [](EdgeLength sum, const auto& en) { return sum + en.gap_to_next; }) / (nodes.size() - 1);

    std::sort(std::begin(nodes), std::end(nodes), [](const auto& e1, const auto& e2) { return e1.gap_to_next > e2.gap_to_next; });

    EdgeLength sum_gap_top{0.0};
    const auto top_size = std::min(20L, static_cast<ssize_t>(nodes.size()));
    for (auto it = std::begin(nodes); it != std::next(std::begin(nodes), top_size); ++it)
        sum_gap_top += it->gap_to_next;
    const EdgeLength ave_gap_top = sum_gap_top / top_size;
    const EdgeLength gap_cut = (nodes.front().gap_to_next - ave_gap_top) * 0.1 + ave_gap_top;
    const auto* to_cut = &nodes.front();
    for (auto it = std::next(std::begin(nodes)); it != std::next(std::begin(nodes), top_size); ++it) {
        if (it->gap_to_next > gap_cut && it->edge_length < to_cut->edge_length)
            to_cut = &*it;
    }

    std::string result;
    // result.append(fmt::format("Ave Gap: {}\n", gap_average.as_number()));
    result.append(fmt::format("Ave Top Gap: {}\n", ave_gap_top.as_number()));
    result.append(fmt::format("Cut: {:.10f} {:.10f} {}\n", to_cut->edge_length.as_number(), to_cut->gap_to_next.as_number(), to_cut->seq_id));
    for (auto it = std::begin(nodes); it != std::next(std::begin(nodes), top_size); ++it)
        result.append(fmt::format("{:.10f} {:.10f} {}\n", it->edge_length.as_number(), it->gap_to_next.as_number(), it->seq_id));

    if (report == CumulativeReport::all) {
        result.append(1, '\n');
        std::sort(std::begin(nodes), std::end(nodes));
        for (const auto& entry : nodes)
            result.append(fmt::format("{:.10f} {:.10f} {}\n", entry.edge_length.as_number(), entry.gap_to_next.as_number(), entry.seq_id));
    }
    return result;

} // acmacs::tal::v3::Tree::report_cumulative

// ----------------------------------------------------------------------

template <typename F> inline void select_update(acmacs::tal::v3::NodeConstSet& nodes, acmacs::tal::v3::Tree::Select update, const acmacs::tal::v3::Node& root, F func)
{
    using namespace acmacs::tal::v3;
    switch (update) {
      case Tree::Select::Init:
          tree::iterate_leaf(root, [&nodes,func](const Node& node) { if (func(node)) nodes.push_back(&node); });
          break;
      case Tree::Select::Update:
          nodes.erase(std::remove_if(std::begin(nodes), std::end(nodes), [func](const Node* node) { return !func(*node); }), std::end(nodes));
          break;
    }
}

void acmacs::tal::v3::Tree::select_cumulative(NodeConstSet& nodes, Select update, double cumulative_min) const
{
    cumulative_calculate();
    select_update(nodes, update, *this, [cumulative_min=EdgeLength{cumulative_min}](const Node& node) { return node.cumulative_edge_length >= cumulative_min; });

} // acmacs::tal::v3::Tree::select_cumulative

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
