#include <algorithm>
#include <regex>

#include "seqdb-3/seqdb.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

const acmacs::tal::v3::Node& acmacs::tal::v3::Node::first_leaf() const
{
    if (is_leaf())
        return *this;
    else
        return subtree.front().first_leaf();

} // acmacs::tal::v3::Node::first_leaf

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_calculate(bool recalculate) const
{
    if (recalculate || cumulative_edge_length == EdgeLengthNotSet) {
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

acmacs::tal::v3::EdgeLength acmacs::tal::v3::Tree::max_cumulative_shown() const
{
    cumulative_calculate();
    EdgeLength max_cumulative{0.0};
    tree::iterate_leaf(*this, [&max_cumulative](const Node& node) {
        if (!node.hidden)
            max_cumulative = std::max(max_cumulative, node.cumulative_edge_length);
    });
    return max_cumulative;

} // acmacs::tal::v3::Tree::max_cumulative_shown

// ----------------------------------------------------------------------

// void acmacs::tal::v3::Tree::cumulative_reset() const
// {
//     tree::iterate_leaf(*this, [](const Node& node) { node.cumulative_edge_length = EdgeLengthNotSet; });

// } // acmacs::tal::v3::Tree::cumulative_reset

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

template <typename F> inline void select_update(acmacs::tal::v3::NodeSet& nodes, acmacs::tal::v3::Tree::Select update, acmacs::tal::v3::Tree::Descent descent, acmacs::tal::v3::Node& root, F func)
{
    using namespace acmacs::tal::v3;
    switch (update) {
        case Tree::Select::init: {
            const auto add = [&nodes, func](Node& node) {
                const auto do_add = func(node);
                if (do_add)
                    nodes.push_back(&node);
                return !do_add;
            };
            switch (descent) {
                case Tree::Descent::no:
                    tree::iterate_leaf_pre_stop(root, add, add);
                    break;
                case Tree::Descent::yes:
                    tree::iterate_leaf_pre(root, add, add);
                    break;
            }
        } break;
        case Tree::Select::update:
            nodes.erase(std::remove_if(std::begin(nodes), std::end(nodes), [func](Node* node) { return !func(*node); }), std::end(nodes));
            break;
    }
}

void acmacs::tal::v3::Tree::select_if_cumulative_more_than(NodeSet& nodes, Select update, double cumulative_min, Descent descent)
{
    cumulative_calculate();
    select_update(nodes, update, descent, *this, [cumulative_min=EdgeLength{cumulative_min}](Node& node) { return node.cumulative_edge_length >= cumulative_min; });

} // acmacs::tal::v3::Tree::select_cumulative

void acmacs::tal::v3::Tree::select_all(NodeSet& nodes, Select update)
{
    select_update(nodes, update, Descent::yes, *this, [](Node& node) { return node.is_leaf(); });
}

void acmacs::tal::v3::Tree::select_by_date(NodeSet& nodes, Select update, std::string_view start, std::string_view end)
{
    select_update(nodes, update, Descent::yes, *this, [start,end](Node& node) { return node.is_leaf() && (start.empty() || node.date >= start) && (end.empty() || node.date < end); });

} // acmacs::tal::v3::Tree::select_by_date

void acmacs::tal::v3::Tree::select_by_seq_id(NodeSet& nodes, Select update, std::string_view regexp)
{
    std::regex re{std::begin(regexp), std::end(regexp), std::regex_constants::ECMAScript|std::regex_constants::icase|std::regex_constants::optimize};
    select_update(nodes, update, Descent::yes, *this, [&re](Node& node) {return node.is_leaf() && std::regex_search(node.seq_id->begin(), node.seq_id->end(), re); });
}

void acmacs::tal::v3::Tree::select_by_aa(NodeSet& nodes, Select update, const acmacs::seqdb::amino_acid_at_pos1_list_t& aa_at_pos1)
{
    fmt::print(stderr, "DEBUG: select_by_aa {}\n", aa_at_pos1);
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::match_seqdb(std::string_view seqdb_filename)
{
    acmacs::seqdb::setup(seqdb_filename);
    const auto& seqdb = acmacs::seqdb::get();

    tree::iterate_leaf(*this, [this,&seqdb](Node& node) {
        if (const auto subset = seqdb.select_by_seq_id(node.seq_id); !subset.empty()) {
            const auto& ref = subset.front();
            node.aa_sequence = ref.seq().aa_aligned();
            node.date = ref.entry->date();
            node.continent = ref.entry->continent;
            node.country = ref.entry->country;
            node.hi_names = ref.seq().hi_names;
            if (virus_type_.empty())
                virus_type_ = ref.entry->virus_type;
            else if (virus_type_ != ref.entry->virus_type)
                fmt::print(stderr, "WARNING: multiple virus_types from seqdb for \"{}\": {} and {}\n", node.seq_id, virus_type_, ref.entry->virus_type);
            if (lineage_.empty())
                lineage_ = ref.entry->lineage;
            else if (lineage_ != ref.entry->lineage)
                fmt::print(stderr, "WARNING: multiple lineages from seqdb for \"{}\": {} and {}\n", node.seq_id, lineage_, ref.entry->lineage);
        }
        else {
            fmt::print(stderr, "WARNING: seq_id \"{}\" not found in seqdb\n", node.seq_id);
        }
    });

} // acmacs::tal::v3::Tree::match_seqdb

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::ladderize(Ladderize method)
{
    const auto set_leaf = [](Node& node) {
        node.ladderize_helper_ = {node.edge_length, node.date, node.seq_id};
    };

    const auto set_parent = [](Node& node) {
        const auto max_subtree_edge_node = std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return std::get<EdgeLength>(node1.ladderize_helper_) < std::get<EdgeLength>(node2.ladderize_helper_); });
        const auto max_subtree_date_node = std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return std::get<1>(node1.ladderize_helper_) < std::get<1>(node2.ladderize_helper_); });
        const auto max_subtree_name_node = std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return std::get<2>(node1.ladderize_helper_) < std::get<2>(node2.ladderize_helper_); });
        node.ladderize_helper_ = {
            node.edge_length + std::get<EdgeLength>(max_subtree_edge_node->ladderize_helper_),
            std::get<1>(max_subtree_date_node->ladderize_helper_),
            std::get<2>(max_subtree_name_node->ladderize_helper_)
        };
    };

      // set max_edge_length field for every node
    tree::iterate_leaf_post(*this, set_leaf, set_parent);

    const auto reorder_by_max_edge_length = [](const Node& node1, const Node& node2) -> bool {
        return node1.ladderize_helper_ < node2.ladderize_helper_;
    };

    const auto reorder_by_number_of_leaves = [reorder_by_max_edge_length](const Node& node1, const Node& node2) -> bool {
        if (node1.number_strains_in_subtree_ == node2.number_strains_in_subtree_)
            return reorder_by_max_edge_length(node1, node2);
        else
            return node1.number_strains_in_subtree_ < node2.number_strains_in_subtree_;
    };

    switch (method) {
      case Ladderize::MaxEdgeLength:
          fmt::print(stderr, "INFO: ladderizing by MaxEdgeLength\n");
          tree::iterate_post(*this, [reorder_by_max_edge_length](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_max_edge_length); });
          break;
      case Ladderize::NumberOfLeaves:
          fmt::print(stderr, "INFO: ladderizing by NumberOfLeaves\n");
          number_strains_in_subtree();
          tree::iterate_post(*this, [reorder_by_number_of_leaves](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_number_of_leaves); });
          break;
      case Ladderize::None:
          fmt::print(stderr, "WARNING: no ladderizing\n");
          break;
    }

} // acmacs::tal::v3::Tree::ladderize

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::number_strains_in_subtree() const
{
    if (number_strains_in_subtree_ <= subtree.size()) {
        const auto set_number_strains = [](const Node& node) {
            node.number_strains_in_subtree_ = std::accumulate(std::begin(node.subtree), std::end(node.subtree), 0UL, [](size_t sum, const auto& subnode) {
                if (!subnode.hidden)
                    return sum + subnode.number_strains_in_subtree_;
                else
                    return sum;
            });
        };
        tree::iterate_post(*this, set_number_strains);
    }

} // acmacs::tal::v3::Tree::number_strains_in_subtree

// ----------------------------------------------------------------------

acmacs::tal::v3::NodePath acmacs::tal::v3::Tree::find_name(SeqId look_for) const
{
    NodePath path;
    bool found{false};

    const auto stop = [&found](const Node&) -> bool { return found; };

    const auto leaf = [&found, &path, look_for](const Node& node) {
        if (node.seq_id == look_for) {
            found = true;
            path.push_back(&node);
        }
    };

    const auto pre = [&path](const Node& node) { path.push_back(&node); };

    const auto post = [&path, &found](const Node&) {
        if (!found)
            path.get().pop_back();
    };

    tree::iterate_stop_leaf_pre_post(*this, stop, leaf, pre, post);
    if (!found)
        throw error(fmt::format("seq-id \"{}\" not found in the tree", look_for));
    return path;

} // acmacs::tal::v3::Tree::find_name

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::re_root(SeqId new_root)
{
    auto path = find_name(new_root);
    path.get().pop_back();
    re_root(path);

} // acmacs::tal::v3::Tree::re_root

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::re_root(const NodePath& new_root)
{
    if (new_root->front() != this)
        throw error("Invalid path passed to Tree::re_root");

    std::vector<Node> nodes;
    for (size_t item_no = 0; item_no < (new_root.size() - 1); ++item_no) {
        const auto& source = *new_root[item_no];
        nodes.push_back(Node());
        std::copy_if(source.subtree.begin(), source.subtree.end(), std::back_inserter(nodes.back().subtree), [&](const Node& to_copy) -> bool { return &to_copy != new_root[item_no + 1]; });
        nodes.back().edge_length = new_root[item_no + 1]->edge_length;
    }

    std::vector<Node> new_subtree(new_root->back()->subtree.begin(), new_root->back()->subtree.end());
    Subtree* append_to = &new_subtree;
    for (auto child = nodes.rbegin(); child != nodes.rend(); ++child) {
        append_to->push_back(*child);
        append_to = &append_to->back().subtree;
    }
    subtree = new_subtree;
    edge_length = EdgeLength{0.0};

    if (cumulative_edge_length != EdgeLengthNotSet)
        cumulative_calculate(true);

} // acmacs::tal::v3::Tree::re_root

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
