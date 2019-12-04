#include <algorithm>
#include <regex>
#include <set>

#include "acmacs-base/statistics.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-chart-2/chart.hh"
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

void acmacs::tal::v3::Node::remove_aa_transition(seqdb::pos0_t pos, char right) const
{
    auto remove = [pos,right](const Node& node) -> bool {
        const bool present_any = node.aa_transitions_.find(pos);
        node.aa_transitions_.remove(pos, right);
        return !present_any;
    };
    tree::iterate_leaf_pre_stop(*this, remove, remove);

} // acmacs::tal::v3::Node::remove_aa_transition

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

std::string acmacs::tal::v3::Tree::report_cumulative() const
{
    cumulative_calculate();

    std::vector<const Node*> nodes;
    tree::iterate_leaf(*this, [&nodes](const Node& node) { nodes.push_back(&node); });
    std::sort(std::begin(nodes), std::end(nodes), [](const auto& en1, const auto& en2) { return en1->cumulative_edge_length > en2->cumulative_edge_length; });

    std::string result{"All nodes sorted by cumulative\n Cumulative       Edge       Seq id\n"};
    for (const auto* node : nodes)
        result.append(fmt::format("{:.10f}  {:.10f}  {}\n", node->cumulative_edge_length.as_number(), node->edge_length.as_number(), node->seq_id));
    return result;

} // acmacs::tal::v3::Tree::report_cumulative

// ----------------------------------------------------------------------

// enum class CumulativeReport { clusters, all };
// std::string x_report_cumulative(acmacs::verbose verb, CumulativeReport report)
// {
//     struct CumulativeEntry
//     {
//         acmacs::tal::SeqId seq_id;
//         acmacs::tal::EdgeLength edge_length;
//         acmacs::tal::EdgeLength cumulative_edge_length;
//         acmacs::tal::EdgeLength gap_to_next{acmacs::tal::EdgeLengthNotSet};

//         CumulativeEntry(const acmacs::tal::SeqId& a_seq_id, acmacs::tal::EdgeLength a_edge, acmacs::tal::EdgeLength a_cumulative)
//             : seq_id{a_seq_id}, edge_length{a_edge}, cumulative_edge_length{a_cumulative}
//         {
//         }

//         void set_gap(const CumulativeEntry& next, bool cumulative)
//         {
//             if (cumulative)
//                 gap_to_next = cumulative_edge_length - next.cumulative_edge_length;
//             else
//                 gap_to_next = edge_length - next.edge_length;
//         }

//         static std::string header(bool cumulative_first)
//         {
//             if (cumulative_first)
//                 return " cumulative       edge      gap to next   seq id\n";
//             else
//                 return "    edge       cumulative   gap to next   seq id\n";
//         }

//         std::string format(bool cumulative_first) const
//         {
//             if (cumulative_first)
//                 return fmt::format("{:.10f}  {:.10f}  {:.10f}  {}\n", cumulative_edge_length.as_number(), edge_length.as_number(), gap_to_next.as_number(), seq_id);
//             else
//                 return fmt::format("{:.10f}  {:.10f}  {:.10f}  {}\n", edge_length.as_number(), cumulative_edge_length.as_number(), gap_to_next.as_number(), seq_id);
//         }
//     };

//     cumulative_calculate();

//     std::vector<CumulativeEntry> nodes;
//     tree::iterate_leaf(*this, [&nodes](const Node& node) { nodes.emplace_back(node.seq_id, node.edge_length, node.cumulative_edge_length); });
//     std::sort(std::begin(nodes), std::end(nodes), [](const auto& en1, const auto& en2) { return en1.cumulative_edge_length > en2.cumulative_edge_length; });
//     for (auto it = std::begin(nodes); it != std::prev(std::end(nodes)); ++it)
//         it->set_gap(*std::next(it), true);
//     // const auto gap_average = std::accumulate(std::begin(nodes), std::prev(std::end(nodes)), EdgeLength{0.0}, [](EdgeLength sum, const auto& en) { return sum + en.gap_to_next; }) / (nodes.size() -
//     // 1);

//     std::sort(std::begin(nodes), std::end(nodes), [](const auto& e1, const auto& e2) { return e1.gap_to_next > e2.gap_to_next; });

//     EdgeLength sum_gap_top{0.0};
//     const auto top_size = std::min(20L, static_cast<ssize_t>(nodes.size()));
//     for (auto it = std::begin(nodes); it != std::next(std::begin(nodes), top_size); ++it)
//         sum_gap_top += it->gap_to_next;
//     const EdgeLength ave_gap_top = sum_gap_top / top_size;
//     const EdgeLength gap_cut = (nodes.front().gap_to_next - ave_gap_top) * 0.1 + ave_gap_top;
//     auto to_cut = nodes.begin();
//     for (auto it = std::next(std::begin(nodes)); it != std::next(std::begin(nodes), top_size); ++it) {
//         if (it->gap_to_next > gap_cut && it->edge_length < to_cut->edge_length)
//             to_cut = it;
//     }

//     std::string result{"Cumulative edges and cut suggestion\n\n"};
//     // result.append(fmt::format("Ave Gap: {}\n", gap_average.as_number()));
//     // result.append(fmt::format("Ave Top Gap: {}\n", ave_gap_top.as_number()));
//     result.append(CumulativeEntry::header(true));
//     result.append(to_cut->format(true));
//     if (to_cut != nodes.begin()) {
//         const auto after_cut = std::prev(to_cut);
//         result.append(after_cut->format(true));
//     }
//     result.append(1, '\n');
//     // if (verb == verbose::yes) {
//     //     fmt::print(result);
//     //     fmt::print("{{\"N\": \"nodes\", \"select\": {{\"cumulative >=\": {:.10f}, \"report\": true}}, \"apply\": \"hide\"}}\n", to_cut->cumulative_edge_length.as_number());
//     // }
//     result.append("Top nodes sorted by gap to next\n");
//     result.append(CumulativeEntry::header(true));
//     for (auto it = std::begin(nodes); it != std::next(std::begin(nodes), top_size); ++it)
//         result.append(it->format(true));

//     if (report == CumulativeReport::all) {
//         std::sort(std::begin(nodes), std::end(nodes), [](const auto& en1, const auto& en2) { return en1.cumulative_edge_length > en2.cumulative_edge_length; });
//         result.append("\nAll nodes sorted by cumulative\n");
//         result.append(CumulativeEntry::header(true));
//         for (const auto& entry : nodes)
//             result.append(entry.format(true));
//     }
//     return result;

// } // x_report_cumulative

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::branches_by_edge() const
{
    const auto sort_by_edge = [](auto& nods) { std::sort(std::begin(nods), std::end(nods), [](const Node* n1, const Node* n2) { return n1->edge_length > n2->edge_length; }); };
    // const auto sort_by_cumulative = [](auto& nods) { std::sort(std::begin(nods), std::end(nods), [](const Node* n1, const Node* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length; }); };
    const auto edge = [](const Node* node) { return node->edge_length.as_number(); };

    cumulative_calculate();
    number_leaves_in_subtree();

    fmt::print("max cumulative: {}\n", max_cumulative_shown().as_number());

    std::vector<const Node*> nodes;

    const auto collect = [&nodes](const Node& node) { nodes.push_back(&node); };
    tree::iterate_leaf_pre(*this, collect, collect);
    // sort_by_cumulative(nodes);
    sort_by_edge(nodes);

    fmt::print("mean edge {}\n", acmacs::statistics::mean(std::begin(nodes), std::end(nodes), edge));
    fmt::print("mean edge (top 20%  ) {}\n", acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), nodes.size() / 5), edge));
    fmt::print("mean edge (top 10%  ) {}\n", acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), nodes.size() / 10), edge));
    fmt::print("mean edge (top  5%  ) {}\n", acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), nodes.size() / 20), edge));
    fmt::print("mean edge (top  1%  ) {}\n", acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), nodes.size() / 100), edge));
    fmt::print("mean edge (top  0.5%) {}\n", acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), nodes.size() / 200), edge));
    fmt::print("HINT: hide by edge, if edge > mean of top 1%\n\n");

    sort_by_edge(nodes);
    fmt::print("    Edge       Cumulative     Seq Id\n");
    for (auto [no, node] : acmacs::enumerate(nodes)) {
        fmt::print("{:.10f}  {:.10f}   {} [{}]\n", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->seq_id, node->number_leaves_in_subtree_);
        if (no > nodes.size() / 50)
            break;
    }
    fmt::print("\n");

} // acmacs::tal::v3::Tree::branches_by_edge

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
    select_update(nodes, update, descent, *this, [cumulative_min=EdgeLength{cumulative_min}](Node& node) { return !node.hidden && node.cumulative_edge_length >= cumulative_min; });

} // acmacs::tal::v3::Tree::select_cumulative

void acmacs::tal::v3::Tree::select_all(NodeSet& nodes, Select update)
{
    select_update(nodes, update, Descent::yes, *this, [](const Node& node) { return node.is_leaf() && !node.hidden; });
}

void acmacs::tal::v3::Tree::select_by_date(NodeSet& nodes, Select update, std::string_view start, std::string_view end)
{
    select_update(nodes, update, Descent::yes, *this, [start,end](const Node& node) { return node.is_leaf() && !node.hidden && (start.empty() || node.date >= start) && (end.empty() || node.date < end); });

} // acmacs::tal::v3::Tree::select_by_date

void acmacs::tal::v3::Tree::select_by_seq_id(NodeSet& nodes, Select update, std::string_view regexp)
{
    std::regex re{std::begin(regexp), std::end(regexp), std::regex_constants::ECMAScript|std::regex_constants::icase|std::regex_constants::optimize};
    select_update(nodes, update, Descent::yes, *this, [&re](const Node& node) { return node.is_leaf() && !node.hidden && std::regex_search(node.seq_id->begin(), node.seq_id->end(), re); });
}

void acmacs::tal::v3::Tree::select_by_aa(NodeSet& nodes, Select update, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos1)
{
    select_update(nodes, update, Descent::yes, *this, [&aa_at_pos1](const Node& node) { return node.is_leaf() && !node.hidden && acmacs::seqdb::matches(node.aa_sequence, aa_at_pos1); });
}

void acmacs::tal::v3::Tree::select_matches_chart_antigens(NodeSet& nodes, Select update)
{
    select_update(nodes, update, Descent::yes, *this, [](const Node& node) { return node.is_leaf() && node.antigen_index_in_chart_.has_value(); });
}

void acmacs::tal::v3::Tree::select_matches_chart_sera(NodeSet& nodes, Select update, serum_match_t match_type)
{
    const auto serum_matches = [match_type](const Node& node) -> bool {
        if (!node.is_leaf())
            return false;
        for (const auto& [serum_index, reassortant_matches, passage_type_matches] : node.serum_index_in_chart_) {
            switch (match_type) {
              case serum_match_t::name:
                  return true;
              case serum_match_t::reassortant:
                  if (reassortant_matches)
                      return true;
                  break;
              case serum_match_t::passage_type:
                  if (reassortant_matches && passage_type_matches)
                      return true;
                  break;
            }
        }
        return false;
    };
    select_update(nodes, update, Descent::yes, *this, serum_matches);
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::hide(const NodeSet& nodes)
{
    for (Node* node : nodes)
        node->hidden = true;
    row_no_set_ = false;
    clades_reset();

} // acmacs::tal::v3::Tree::hide

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
            else if (lineage_ != ref.entry->lineage && !ref.entry->lineage.empty())
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
        if (node1.number_leaves_in_subtree_ == node2.number_leaves_in_subtree_)
            return reorder_by_max_edge_length(node1, node2);
        else
            return node1.number_leaves_in_subtree_ < node2.number_leaves_in_subtree_;
    };

    switch (method) {
      case Ladderize::MaxEdgeLength:
          fmt::print(stderr, "INFO: ladderizing by MaxEdgeLength\n");
          tree::iterate_post(*this, [reorder_by_max_edge_length](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_max_edge_length); });
          break;
      case Ladderize::NumberOfLeaves:
          fmt::print(stderr, "INFO: ladderizing by NumberOfLeaves\n");
          number_leaves_in_subtree();
          tree::iterate_post(*this, [reorder_by_number_of_leaves](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_number_of_leaves); });
          break;
      case Ladderize::None:
          fmt::print(stderr, "WARNING: no ladderizing\n");
          break;
    }

    row_no_set_ = false;
    clades_reset();

} // acmacs::tal::v3::Tree::ladderize

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::number_leaves_in_subtree() const
{
    if (number_leaves_in_subtree_ <= subtree.size()) {
        const auto set_number_strains = [](const Node& node) {
            node.number_leaves_in_subtree_ = std::accumulate(std::begin(node.subtree), std::end(node.subtree), 0UL, [](size_t sum, const auto& subnode) {
                if (!subnode.hidden)
                    return sum + subnode.number_leaves_in_subtree_;
                else
                    return sum;
            });
        };
        tree::iterate_post(*this, set_number_strains);
    }

} // acmacs::tal::v3::Tree::number_leaves_in_subtree

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

    row_no_set_ = false;
    clades_reset();

} // acmacs::tal::v3::Tree::re_root

// ----------------------------------------------------------------------

acmacs::Counter<std::string_view> acmacs::tal::v3::Tree::stat_by_month() const
{
    acmacs::Counter<std::string_view> by_month;
    tree::iterate_leaf(*this, [&by_month](const Node& node) {
        if (node.date.size() >= 7)
            by_month.count(node.date.substr(0, 7));
    });
    return by_month;

} // acmacs::tal::v3::Tree::stat_by_month

// ----------------------------------------------------------------------

std::pair<date::year_month_day, date::year_month_day> acmacs::tal::v3::Tree::suggest_time_series_start_end(const Counter<std::string_view>& stat) const
{
    std::vector<std::pair<date::year_month_day, date::year_month_day>> chunks;
    auto it = std::begin(stat.counter());
    date::year_month_day prev;
    for (; it != std::end(stat.counter()); ++it) {
        try {
            prev = date::from_string(std::begin(stat.counter())->first, date::allow_incomplete::yes, date::throw_on_error::yes);
            break;
        }
        catch (date::date_parse_error&) {
        }
    }
    auto cur = prev, start = prev;
    for (++it; it != std::end(stat.counter()); ++it) {
        try {
            cur = date::from_string(it->first, date::allow_incomplete::yes, date::throw_on_error::yes);
            if (date::months_between_dates(prev, cur) > 2) {
                chunks.emplace_back(start, prev);
                start = cur;
            }
            prev = cur;
        }
        catch (date::date_parse_error&) {
        }
    }
    if (start != prev)
        chunks.emplace_back(start, prev);
    return *std::max_element(std::begin(chunks), std::end(chunks),
                             [](const auto& e1, const auto& e2) { return date::months_between_dates(e1.first, e1.second) < date::months_between_dates(e2.first, e2.second); });

} // acmacs::tal::v3::Tree::suggest_time_series_start_end

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::Tree::report_time_series() const
{
    const auto by_month = stat_by_month();
    fmt::memory_buffer out;
    for (auto [month, count] : by_month.counter())
        fmt::format_to(out, "  {} {}\n", month, count);
    const auto [first, last] = suggest_time_series_start_end(by_month);
    return fmt::format("INFO: Months total:{} Suggested:{} {} .. {}\n{}", by_month.size(), date::months_between_dates(first, last) + 1,
                       date::display(first, date::allow_incomplete::yes), date::display(last, date::allow_incomplete::yes), fmt::to_string(out));

} // acmacs::tal::v3::Tree::report_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::update_common_aa() const
{
    tree::iterate_post(*this, [](const Node& node) {
        for (const auto& child : node.subtree) {
            if (!child.hidden) {
                if (child.is_leaf())
                    node.common_aa_.update(child.aa_sequence);
                else
                    node.common_aa_.update(child.common_aa_);
            }
        }
    });

} // acmacs::tal::v3::Tree::update_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::report_common_aa() const
{
    number_leaves_in_subtree();
    tree::iterate_pre_parent(*this, [](const Node& node, const Node& parent) {
        if (node.number_leaves_in_subtree_ >= 100) {
            if (const auto rep = node.common_aa_.report(parent.common_aa_); !rep.empty())
                fmt::print("(children:{} leaves:{}) {}\n", node.subtree.size(), node.number_leaves_in_subtree_, rep);
        }
    });

} // acmacs::tal::v3::Tree::report_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::update_aa_transitions() const
{
    cumulative_calculate();

    const auto aa_at = [](const Node& node, seqdb::pos0_t pos) {
        if (node.is_leaf())
            return node.aa_sequence.at(pos);
        else
            return node.common_aa_.at(pos);
    };

    tree::iterate_post(*this, [aa_at](const Node& node) {
        for (seqdb::pos0_t pos{0}; *pos < node.common_aa_.size(); ++pos) {
            if (node.common_aa_.is_no_common(pos)) {
                CounterChar counter;
                for (const auto& child : node.subtree) {
                    if (const auto aa = aa_at(child, pos); CommonAA::is_common(aa)) {
                        child.aa_transitions_.add(pos, aa);
                        counter.count(aa);
                    }
                    else if (const auto found = child.aa_transitions_.find(pos); found) {
                        counter.count(found->right);
                    }
                }
                if ((node.number_leaves_in_subtree_ == 36 || node.number_leaves_in_subtree_ == (2124 - 36) || node.number_leaves_in_subtree_ == (2124 - 37) || node.number_leaves_in_subtree_ == 2124 || node.number_leaves_in_subtree_ == 2125) && pos == seqdb::pos1_t{91})
                    fmt::print(stderr, "DEBUG: leaves:{} pos:{} counter: {}\n", node.number_leaves_in_subtree_, pos, counter);
                if (const auto [max_aa, max_count] = counter.max(); max_count > 1) {
                    node.remove_aa_transition(pos, max_aa);
                    node.aa_transitions_.add(pos, max_aa);
                }
            }
        }
    });

    std::vector<const Node*> sorted_leaf_nodes;
    tree::iterate_leaf(*this, [&sorted_leaf_nodes](const Node& node) { sorted_leaf_nodes.push_back(&node); });
    std::sort(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), [](const auto* n1, const auto* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length; });

    // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&sorted_leaf_nodes](const Node& node) {
        if (!node.aa_transitions_.empty()) {
            const auto node_left_edge = node.cumulative_edge_length - node.edge_length;

            if (const auto node_for_left =
                    std::find_if(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), [node_left_edge](const auto* nd) { return nd->cumulative_edge_length < node_left_edge; });
                node_for_left != std::end(sorted_leaf_nodes)) {
                node.aa_transitions_.set_left((*node_for_left)->aa_sequence);
                node.node_for_left_aa_transitions_ = *node_for_left;
            }
        }

        node.aa_transitions_.remove_left_right_same();
    };
    tree::iterate_leaf_pre(*this, add_left_part, add_left_part);

} // acmacs::tal::v3::Tree::update_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::report_aa_transitions() const
{
    number_leaves_in_subtree();
    tree::iterate_pre(*this, [](const Node& node) {
        if (node.number_leaves_in_subtree_ >= 20) {
            if (const auto rep = node.aa_transitions_.display(); !rep.empty())
                fmt::print("(children:{} leaves:{}) {}\n", node.subtree.size(), node.number_leaves_in_subtree_, rep);
        }
    });

} // acmacs::tal::v3::Tree::report_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clades_reset()
{
    tree::iterate_leaf(*this, [](Node& node) { node.clades.clear(); });
    clades_.clear();

} // acmacs::tal::v3::Tree::clades_reset

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_set(std::string_view name, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& substitutions, std::string_view display_name, size_t inclusion_tolerance,
                                      size_t exclusion_tolerance)
{
    set_row_no();

    size_t num = 0;
    const std::string name_s{name};
    auto& clade_sections = find_clade(name, display_name).sections;
    tree::iterate_leaf(*this, [&substitutions, name_s, &num, &clade_sections, inclusion_tolerance](Node& node) {
        if (!node.hidden && acmacs::seqdb::matches(node.aa_sequence, substitutions)) {
            node.clades.add(name_s);
            ++num;
            if (clade_sections.empty() || (node.row_no_ - clade_sections.back().last->row_no_) > inclusion_tolerance)
                clade_sections.emplace_back(&node);
            else
                clade_sections.back().last = &node;
        }
    });

    // remove small sections
    clade_sections.erase(
        std::remove_if(std::begin(clade_sections), std::end(clade_sections), [exclusion_tolerance](const auto& section) { return (section.last->row_no_ - section.first->row_no_) < exclusion_tolerance; }),
        std::end(clade_sections));

    // fmt::print(stderr, "DEBUG: clade \"{}\": {} leaves, {} sections\n", name, num, clade_sections.size());

} // acmacs::tal::v3::Tree::clade_set

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_report(std::string_view name) const
{
    const auto report = [](std::string_view clade_name, const auto& sections) {
        fmt::print("Clade \"{}\" ({})\n", clade_name, sections.size());
        for (auto it = std::begin(sections); it != std::end(sections); ++it) {
            if (it != std::begin(sections))
                fmt::print("    gap {}\n", it->first->row_no_ - std::prev(it)->last->row_no_ - 1);
            fmt::print(" ({}) {} {} .. {} {}\n", it->last->row_no_ - it->first->row_no_ + 1, it->first->row_no_, it->first->seq_id, it->last->row_no_, it->last->seq_id);
        }
        fmt::print("\n");
    };

    if (name.empty()) {
        for (const auto& clade : clades_)
            report(clade.name, clade.sections);
    }
    else if (const auto* found = find_clade(name); found)
        report(name, found->sections);
    else
        fmt::print(stderr, "WARNING: no clade \"{}\" defined\n", name);

} // acmacs::tal::v3::Tree::clade_report

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::set_row_no() const
{
    if (!row_no_set_) {
        size_t row_no = 0;
        tree::iterate_leaf(*this, [&row_no](const Node& node) {
            if (!node.hidden)
                node.row_no_ = row_no++;
        });
        row_no_set_ = true;
    }

} // acmacs::tal::v3::Tree::set_row_no

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::match(const acmacs::chart::Chart& chart) const
{
    if (!chart_matched_) {
        auto antigens = chart.antigens();
        auto sera = chart.sera();
        std::map<std::string, size_t, std::less<>> antigen_names;
        std::map<acmacs::virus::name_t, std::vector<size_t>, std::less<>> serum_names;
        for (size_t ag_no = 0; ag_no < antigens->size(); ++ag_no)
            antigen_names[antigens->at(ag_no)->full_name()] = ag_no;
        for (size_t sr_no = 0; sr_no < sera->size(); ++sr_no)
            serum_names[sera->at(sr_no)->name()].push_back(sr_no);
        tree::iterate_leaf(*this, [&antigen_names,&serum_names,&sera](const Node& node) {
            for (const auto& hi_name : node.hi_names) {
                if (const auto found = antigen_names.find(hi_name); found != std::end(antigen_names)) {
                    node.antigen_index_in_chart_ = found->second;
                    break;
                }
            }
            const auto parsed_seq_id = acmacs::virus::parse_name(node.seq_id);
            if (const auto found = serum_names.find(parsed_seq_id.name); found != std::end(serum_names)) {
                for (auto serum_index : found->second)
                    node.serum_index_in_chart_.emplace_back(serum_index, sera->at(serum_index)->reassortant() == parsed_seq_id.reassortant, sera->at(serum_index)->passage().is_egg() == parsed_seq_id.passage.is_egg());
            }
        });
        chart_matched_ = true;
    }

} // acmacs::tal::v3::Tree::match

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
