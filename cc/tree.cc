#include <algorithm>
#include <regex>
#include <set>

#include "acmacs-base/statistics.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

const acmacs::tal::v3::Node& acmacs::tal::v3::Node::find_first_leaf() const
{
    if (is_leaf())
        return *this;
    else
        return subtree.front().find_first_leaf();

} // acmacs::tal::v3::Node::find_first_leaf

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

std::vector<const acmacs::tal::v3::Node*> acmacs::tal::v3::Node::shown_children() const
{
    std::vector<const Node*> children;
    for (const auto& child : subtree) {
        if (!child.hidden)
            children.push_back(&child);
    }
    return children;

} // acmacs::tal::v3::Node::shown_children

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::erase()
{
    *this = Tree();
    // data_buffer_.erase();

} // acmacs::tal::v3::Tree::erase

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
// std::string x_report_cumulative(CumulativeReport report)
// {
//     struct CumulativeEntry
//     {
//         acmacs::tal::seq_id_t seq_id;
//         acmacs::tal::EdgeLength edge_length;
//         acmacs::tal::EdgeLength cumulative_edge_length;
//         acmacs::tal::EdgeLength gap_to_next{acmacs::tal::EdgeLengthNotSet};

//         CumulativeEntry(const acmacs::tal::seq_id_t& a_seq_id, acmacs::tal::EdgeLength a_edge, acmacs::tal::EdgeLength a_cumulative)
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

std::vector<const acmacs::tal::v3::Node*> acmacs::tal::v3::Tree::sorted_by_edge() const
{
    std::vector<const Node*> sorted;
    const auto collect = [&sorted](const Node& node) { sorted.push_back(&node); };
    tree::iterate_leaf_pre(*this, collect, collect);
    std::sort(std::begin(sorted), std::end(sorted), [](const Node* n1, const Node* n2) { return n1->edge_length > n2->edge_length; });
    return sorted;

} // acmacs::tal::v3::Tree::sorted_by_edge

// ----------------------------------------------------------------------

double acmacs::tal::v3::Tree::mean_edge_of(double fraction_or_number) const
{
    const auto edge = [](const Node* node) { return node->edge_length.as_number(); };
    std::vector<const Node*> nodes = sorted_by_edge();
    if (fraction_or_number <= 1.0)
        return acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), static_cast<ssize_t>(static_cast<double>(nodes.size()) * fraction_or_number)), edge);
    else
        return acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), static_cast<ssize_t>(fraction_or_number)), edge);

} // acmacs::tal::v3::Tree::mean_edge_of

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::branches_by_edge()
{
    cumulative_calculate();
    number_leaves_in_subtree();

    AD_INFO("edge report");
    fmt::print("  max cumulative: {}\n", max_cumulative_shown().as_number());


    // const auto collect = [&nodes](const Node& node) { nodes.push_back(&node); };
    // tree::iterate_leaf_pre(*this, collect, collect);

    // const auto sort_by_edge = [](auto& nods) { std::sort(std::begin(nods), std::end(nods), [](const Node* n1, const Node* n2) { return n1->edge_length > n2->edge_length; }); };
    // const auto sort_by_cumulative = [](auto& nods) { std::sort(std::begin(nods), std::end(nods), [](const Node* n1, const Node* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length; }); };
    // const auto edge = [](const Node* node) { return node->edge_length.as_number(); };
    // const auto mean_edge = [&nodes, edge](double fraction) {
    //     return acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), static_cast<ssize_t>(nodes.size() * fraction)), edge);
    // };

    // sort_by_cumulative(nodes);
    // sort_by_edge(nodes);

    std::vector<const Node*> nodes = sorted_by_edge();

    fmt::print("  mean edge (all      : {:4d}) {}\n", nodes.size(), mean_edge_of(1.0));
    fmt::print("  mean edge (top 20%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.2), mean_edge_of(0.2));
    fmt::print("  mean edge (top 10%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.1), mean_edge_of(0.1));
    fmt::print("  mean edge (top  5%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.05), mean_edge_of(0.05));
    fmt::print("  mean edge (top  1%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.01), mean_edge_of(0.01));
    fmt::print("  mean edge (top  0.5%: {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.005), mean_edge_of(0.005));
    fmt::print("  HINT: hide by edge, if edge > mean of top 1%\n\n");

    fmt::print("        Edge       Cumulative     Seq Id\n");
    for (auto [no, node] : acmacs::enumerate(nodes)) {
        fmt::print("    {:.10f}  {:.10f}   {} [{}]\n", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->seq_id, node->number_leaves_in_subtree());
        if (no > nodes.size() / 100)
            break;
    }
    fmt::print("\n");

    NodeSet selected;
    select_if_edge_more_than(selected, Select::init, mean_edge_of(0.01));
    for (Node* node : selected) {
        fmt::print("  long edge {} {} {}", node->edge_length, node->node_id, node->seq_id);
        node->color_edge_line = RED;
    }

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

void acmacs::tal::v3::Tree::select_if_edge_more_than(NodeSet& nodes, Select update, double edge_min)
{
    select_update(nodes, update, Descent::yes, *this, [edge_min=EdgeLength{edge_min}](Node& node) { return !node.hidden && node.edge_length >= edge_min; });

} // acmacs::tal::v3::Tree::select_if_edge_more_than

void acmacs::tal::v3::Tree::select_if_edge_more_than_mean_edge_of(NodeSet& nodes, Select update, double fraction_or_number)
{
    select_update(nodes, update, Descent::yes, *this, [edge_min=EdgeLength{mean_edge_of(fraction_or_number)}](Node& node) { return !node.hidden && node.edge_length >= edge_min; });

} // acmacs::tal::v3::Tree::select_if_edge_more_than_mean_edge_of

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

void acmacs::tal::v3::Tree::select_by_country(NodeSet& nodes, Select update, std::string_view country_to_select)
{
    select_update(nodes, update, Descent::yes, *this, [country_to_select](const Node& node) { return node.is_leaf() && !node.hidden && node.country == country_to_select; });

} // acmacs::tal::v3::Tree::select_by_country

void acmacs::tal::v3::Tree::select_by_continent(NodeSet& nodes, Select update, std::string_view continent_to_select)
{
    select_update(nodes, update, Descent::yes, *this, [continent_to_select](const Node& node) { return node.is_leaf() && !node.hidden && node.continent == continent_to_select; });

} // acmacs::tal::v3::Tree::select_by_continent

void acmacs::tal::v3::Tree::select_by_location(NodeSet& nodes, Select update, std::string_view location)
{
    select_update(nodes, update, Descent::yes, *this, [location](const Node& node) {
        return node.is_leaf() && !node.hidden && !node.strain_name.empty() && ::virus_name::location(acmacs::virus::v2::name_t{node.strain_name}) == location;
    });

} // acmacs::tal::v3::Tree::select_by_location

void acmacs::tal::v3::Tree::select_by_aa(NodeSet& nodes, Select update, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos1)
{
    select_update(nodes, update, Descent::yes, *this, [&aa_at_pos1](const Node& node) { return node.is_leaf() && !node.hidden && acmacs::seqdb::matches(node.aa_sequence, aa_at_pos1); });
}

void acmacs::tal::v3::Tree::select_by_nuc(NodeSet& nodes, Select update, const acmacs::seqdb::nucleotide_at_pos1_eq_list_t& nuc_at_pos1)
{
    select_update(nodes, update, Descent::yes, *this, [&nuc_at_pos1](const Node& node) { return node.is_leaf() && !node.hidden && acmacs::seqdb::matches(node.nuc_sequence, nuc_at_pos1); });
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

void acmacs::tal::v3::Node::hide()
{
    hidden = true;
    for (auto& child : subtree)
        child.hide();

} // acmacs::tal::v3::Node::hide

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::hide(const NodeSet& nodes)
{
    for (Node* node : nodes)
        node->hide();

    // if all children are hidden, hide parent too
    tree::iterate_post(*this, [](Node& node) {
        if (const auto shown_children = node.shown_children(); shown_children.empty())
            node.hidden = true;
    });

    nodes_hidden_ = true;

} // acmacs::tal::v3::Tree::hide

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::match_seqdb(std::string_view seqdb_filename)
{
    acmacs::seqdb::setup(seqdb_filename);
    const auto& seqdb = acmacs::seqdb::get();

    tree::iterate_leaf(*this, [this,&seqdb](Node& node) {
        if (const auto subset = seqdb.select_by_seq_id(node.seq_id); !subset.empty()) {
            node.populate(subset.front(), seqdb);
            if (virus_type_.empty())
                virus_type_ = node.ref.entry->virus_type;
            else if (virus_type_ != node.ref.entry->virus_type)
                AD_WARNING("multiple virus_types from seqdb for \"{}\": {} and {}", node.seq_id, virus_type_, node.ref.entry->virus_type);
            if (lineage_.empty())
                lineage_ = node.ref.entry->lineage;
            else if (lineage_ != node.ref.entry->lineage && !node.ref.entry->lineage.empty())
                AD_WARNING("multiple lineages from seqdb for \"{}\": {} and {}", node.seq_id, lineage_, node.ref.entry->lineage);
        }
        else {
            AD_WARNING("seq_id \"{}\" not found in seqdb", node.seq_id);
        }
    });

} // acmacs::tal::v3::Tree::match_seqdb

// ----------------------------------------------------------------------

void acmacs::tal::v3::Node::populate(const acmacs::seqdb::ref& a_ref, const acmacs::seqdb::Seqdb& seqdb)
{
    ref = a_ref;
    aa_sequence = ref.aa_aligned(seqdb);
    nuc_sequence = ref.nuc_aligned(seqdb);
    date = ref.entry->date();
    strain_name = ref.entry->name;
    continent = ref.entry->continent;
    country = ref.entry->country;
    hi_names = ref.seq().hi_names;

} // acmacs::tal::v3::Node::populate

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::populate_with_nuc_duplicates()
{
    std::vector<seq_id_t> all_seq_ids;
    tree::iterate_leaf(*this, [&all_seq_ids](const Node& node) { all_seq_ids.push_back(node.seq_id); });
    std::sort(std::begin(all_seq_ids), std::end(all_seq_ids));
    const auto already_in_tree = [&all_seq_ids](const seq_id_t& look_for) {
        const auto found = std::lower_bound(std::begin(all_seq_ids), std::end(all_seq_ids), look_for);
        return found != std::end(all_seq_ids) && *found == look_for;
    };

    const auto& seqdb = acmacs::seqdb::get();
    seqdb.find_slaves();
    Node* parent{nullptr};
    const auto pre = [&parent](Node& node) { parent = &node; };
    const auto leaf = [&parent, &seqdb, already_in_tree](const Node& node) {
        if (!node.ref.empty()) {
            for (const auto& slave : node.ref.seq().slaves())
                if (const auto seq_id = slave.seq_id(); !already_in_tree(seq_id)) {
                    parent->to_populate.emplace_back(seq_id, node.edge_length).populate(slave, seqdb);
                }
        }
    };
    tree::iterate_leaf_pre(*this, leaf, pre);

    size_t added_leaves{0};
    const auto post = [&added_leaves](Node& node) {
        if (!node.to_populate.empty()) {
            added_leaves += node.to_populate.size();
            std::move(std::begin(node.to_populate), std::end(node.to_populate), std::back_inserter(node.subtree));
            node.to_populate.clear();
        }
    };
    tree::iterate_post(*this, post);

    set_first_last_next_node_id();

    AD_INFO("populate_with_nuc_duplicates:\n  initial: {:5d}\n  added:   {:5d}\n  total:   {:5d}", all_seq_ids.size(), added_leaves, all_seq_ids.size() + added_leaves);

} // acmacs::tal::v3::Tree::populate_with_nuc_duplicates

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::just_imported()
{
    set_first_last_next_node_id();

} // acmacs::tal::v3::Tree::just_imported

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::set_first_last_next_node_id()
{
    // Timeit time_set_first_last_next_node_id(">>>> [time] set_first_last_next_node_id: ");

    node_id_t::value_type vertical{0};
    Node* prev_leaf{nullptr};
    std::vector<Node*> parents;

    const auto leaf = [&vertical, &prev_leaf, &parents](Node& node) {
        if (!node.hidden) {
            node.node_id.vertical = vertical;
            node.node_id.horizontal = 0;
            if (prev_leaf) {
                prev_leaf->last_next_leaf = &node;
                node.first_prev_leaf = prev_leaf;
            }
            else
                node.first_prev_leaf = nullptr;
            node.last_next_leaf = nullptr;
            prev_leaf = &node;
            for (Node* parent : parents) {
                if (!parent->first_prev_leaf)
                    parent->first_prev_leaf = &node;
                parent->last_next_leaf = &node;
            }
            ++vertical;
        }
    };

    const auto pre = [&parents](Node& node) {
        node.first_prev_leaf = nullptr; // reset
        parents.push_back(&node);
        if (node.subtree.size() > 1) {
            node.subtree.front().leaf_pos = leaf_position::first;
            node.subtree.back().leaf_pos = leaf_position::last;
            for (auto child = std::next(std::begin(node.subtree)); child != std::prev(std::end(node.subtree)); ++child)
                child->leaf_pos = leaf_position::middle;
        }
        else
            node.subtree.front().leaf_pos = leaf_position::single;
    };

    const auto post = [&parents](Node& node) {
        node.node_id.vertical = (node.subtree.front().node_id.vertical + node.subtree.back().node_id.vertical) / 2;
        node.node_id.horizontal = std::max(node.subtree.front().node_id.horizontal, node.subtree.back().node_id.horizontal) + 1;
        // if (parents.back() != &node)
        //     throw std::runtime_error("Tree::find_first_last_leaves::post: internal");
        parents.pop_back();
    };

    tree::iterate_leaf_pre_post(*this, leaf, pre, post);

} // acmacs::tal::v3::Tree::set_first_last_next_node_id

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::report_first_last_leaves(size_t min_number_of_leaves) const
{
    size_t level{0};

    const auto pre = [min_number_of_leaves, &level](const Node& node) {
        if (!node.first_prev_leaf || !node.last_next_leaf)
            throw std::runtime_error("Tree::report_first_last_leaves::pre: internal");
        if (const auto num_leaves = node.number_leaves_in_subtree(); num_leaves >= min_number_of_leaves) {
            const auto aa_transitions = node.aa_transitions_.display();
            fmt::print("{:{}s}[{}]  {}  --  {}{}\n", "", level * 4, num_leaves, node.first_prev_leaf->seq_id, node.last_next_leaf->seq_id, aa_transitions.empty() ? std::string{} : "  --  " + aa_transitions);
        }
        ++level;
    };

    const auto post = [&level](const Node&) { --level; };

    tree::iterate_pre_post(*this, pre, post);

} // acmacs::tal::v3::Tree::report_first_last_leaves

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::ladderize(Ladderize method)
{
    Timeit time_ladderize(">>>> [time] ladderize: ");

    const auto set_leaf = [](Node& node) { node.ladderize_helper_ = ladderize_helper_t{node.edge_length, node.date, node.seq_id}; };

    const auto set_parent = [](Node& node) {
        const auto max_subtree_edge_node =
            std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return node1.ladderize_helper_.edge < node2.ladderize_helper_.edge; });
        const auto max_subtree_date_node =
            std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return node1.ladderize_helper_.date < node2.ladderize_helper_.date; });
        const auto max_subtree_name_node =
            std::max_element(node.subtree.begin(), node.subtree.end(), [](const auto& node1, const auto& node2) { return node1.ladderize_helper_.seq_id < node2.ladderize_helper_.seq_id; });
        node.ladderize_helper_.edge = node.edge_length + max_subtree_edge_node->ladderize_helper_.edge;
        node.ladderize_helper_.date = max_subtree_date_node->ladderize_helper_.date;
        node.ladderize_helper_.seq_id = max_subtree_name_node->ladderize_helper_.seq_id;
    };

    // set max_edge_length field for every node
    tree::iterate_leaf_post(*this, set_leaf, set_parent);

    const auto reorder_by_max_edge_length = [](const Node& node1, const Node& node2) -> bool {
        if (node1.ladderize_helper_.edge == node2.ladderize_helper_.edge) {
            if (node1.ladderize_helper_.date == node2.ladderize_helper_.date)
                return node1.ladderize_helper_.seq_id < node2.ladderize_helper_.seq_id;
            else
                return node1.ladderize_helper_.date < node2.ladderize_helper_.date;
        }
        else
            return node1.ladderize_helper_.edge < node2.ladderize_helper_.edge;
    };

    const auto reorder_by_number_of_leaves = [reorder_by_max_edge_length](const Node& node1, const Node& node2) -> bool {
        if (node1.number_leaves_in_subtree() == node2.number_leaves_in_subtree())
            return reorder_by_max_edge_length(node1, node2);
        else
            return node1.number_leaves_in_subtree() < node2.number_leaves_in_subtree();
    };

    switch (method) {
        case Ladderize::MaxEdgeLength:
            AD_INFO("ladderizing by MaxEdgeLength");
            tree::iterate_post(*this, [reorder_by_max_edge_length](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_max_edge_length); });
            break;
        case Ladderize::NumberOfLeaves:
            AD_INFO("ladderizing by NumberOfLeaves");
            number_leaves_in_subtree();
            tree::iterate_post(*this, [reorder_by_number_of_leaves](Node& node) { std::sort(node.subtree.begin(), node.subtree.end(), reorder_by_number_of_leaves); });
            break;
        case Ladderize::None:
            AD_WARNING("no ladderizing");
            break;
    }

    clades_reset();
    set_first_last_next_node_id();

} // acmacs::tal::v3::Tree::ladderize

// ----------------------------------------------------------------------

const acmacs::tal::v3::Node* acmacs::tal::v3::Tree::find_node_by_seq_id(const seq_id_t& look_for) const
{
    const Node* found{nullptr};

    const auto leaf = [&found, look_for](const Node& node) -> bool {
        if (node.seq_id == look_for) {
            found = &node;
            return true;
        }
        else
            return false;
    };

    tree::iterate_leaf_stop(*this, leaf);
    return found;

} // acmacs::tal::v3::Tree::find_node_by_seq_id

// ----------------------------------------------------------------------

acmacs::tal::v3::NodePath acmacs::tal::v3::Tree::find_path_by_seq_id(const seq_id_t& look_for) const
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

} // acmacs::tal::v3::Tree::find_path_by_seq_id

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::re_root(const seq_id_t& new_root)
{
    auto path = find_path_by_seq_id(new_root);
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

    clades_reset();
    set_first_last_next_node_id();

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
    if (it == std::end(stat.counter()))
        return {date::invalid_date(), date::invalid_date()};

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

std::string acmacs::tal::v3::Tree::report_time_series(report_size rs) const
{
    if (const auto by_month = stat_by_month(); !by_month.empty()) {
        const auto [first, last] = suggest_time_series_start_end(by_month);
        const auto brief = fmt::format("Months total:{} Suggested:{} {} .. {}", by_month.size(), date::months_between_dates(first, last) + 1, date::display(first, date::allow_incomplete::yes),
                                       date::display(last, date::allow_incomplete::yes));
        if (rs == report_size::brief)
            return brief;

        fmt::memory_buffer out;
        for (auto [month, count] : by_month.counter())
            fmt::format_to(out, "  {} {}\n", month, count);
        return fmt::format("{}\n{}", brief, fmt::to_string(out));
    }
    else
        return "No dates for sequences available";

} // acmacs::tal::v3::Tree::report_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::update_common_aa() const
{
    tree::iterate_post(*this, [this](const Node& node) {
        for (const auto& child : node.subtree) {
            if (!child.hidden) {
                if (child.is_leaf()) {
                    node.common_aa_.update(child.aa_sequence);
                    longest_sequence_ = std::max(longest_sequence_, seqdb::pos0_t{child.aa_sequence.size()});
                }
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
        if (node.number_leaves_in_subtree() >= 100) {
            if (const auto rep = node.common_aa_.report(parent.common_aa_); !rep.empty())
                fmt::print("node:{:4.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::Tree::report_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::update_aa_transitions() const
{
    Timeit time1(">>>> update_aa_transitions cumulative_calculate: ", report_time::no);
    cumulative_calculate();
    time1.report();

    const auto aa_at = [](const Node& node, seqdb::pos0_t pos) {
        if (node.is_leaf())
            return node.aa_sequence.at(pos);
        else
            return node.common_aa_.at(pos);
    };

    Timeit time2(">>>> update_aa_transitions counting: ", report_time::no);
    tree::iterate_post(*this, [aa_at, this](const Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence_; ++pos) {
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
                // if (pos == seqdb::pos1_t{484})
                //     AD_DEBUG("node:{:4.3s} leaves:{:4d} pos:{:3d} counter: {}", node.node_id, node.number_leaves_in_subtree(), pos, counter);
                if (const auto [max_aa, max_count] = counter.max(); max_count > 1) {
                    node.remove_aa_transition(pos, max_aa);
                    node.aa_transitions_.add(pos, max_aa);
                }
            }
        }
    });
    time2.report();

    Timeit time3(">>>> update_aa_transitions sorted_leaf_nodes: ", report_time::no);
    std::vector<const Node*> sorted_leaf_nodes;
    tree::iterate_leaf(*this, [&sorted_leaf_nodes](const Node& node) { sorted_leaf_nodes.push_back(&node); });
    const auto sorted_leaf_nodes_cmp = [](const auto* n1, const auto* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length; };
    std::sort(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), sorted_leaf_nodes_cmp); // bigger cumul length first
    time3.report();

    Timeit time4(">>>> update_aa_transitions left part: ", report_time::no);
    // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&sorted_leaf_nodes](const Node& node) {
        if (!node.aa_transitions_.empty()) {
            const auto node_left_edge = node.cumulative_edge_length - node.edge_length;

            const auto node_for_left = std::upper_bound(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), node_left_edge,
                                                        [](const auto& a_node_left_edge, const auto* nd) { return a_node_left_edge > nd->cumulative_edge_length; });
            // const auto node_for_left = std::find_if(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), [node_left_edge](const auto* nd) { return nd->cumulative_edge_length < node_left_edge; });
            if (node_for_left != std::end(sorted_leaf_nodes)) {
                node.aa_transitions_.set_left((*node_for_left)->aa_sequence);
                node.node_for_left_aa_transitions_ = *node_for_left;
            }
        }

        node.aa_transitions_.remove_left_right_same();
    };
    tree::iterate_leaf_pre(*this, add_left_part, add_left_part);
    time4.report();

} // acmacs::tal::v3::Tree::update_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::report_aa_transitions() const
{
    number_leaves_in_subtree();
    tree::iterate_pre(*this, [](const Node& node) {
        if (node.number_leaves_in_subtree() >= 20) {
            if (const auto rep = node.aa_transitions_.display(); !rep.empty())
                fmt::print("{} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::Tree::report_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clades_reset()
{
    AD_LOG(acmacs::log::clades, "reset");
    tree::iterate_leaf(*this, [](Node& node) { node.clades.clear(); });
    clades_.clear();

} // acmacs::tal::v3::Tree::clades_reset

// ----------------------------------------------------------------------

const acmacs::tal::v3::Tree::clade_t* acmacs::tal::v3::Tree::find_clade(std::string_view clade_name) const
{
    if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [clade_name](const auto& cl) { return cl.name == clade_name; }); found != std::end(clades_))
        return &*found;
    else
        return nullptr;

} // acmacs::tal::v3::Tree::find_clade

// ----------------------------------------------------------------------

acmacs::tal::v3::Tree::clade_t& acmacs::tal::v3::Tree::find_or_add_clade(std::string_view clade_name, std::string_view display_name)
{
    if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [clade_name](const auto& cl) { return cl.name == clade_name; }); found != std::end(clades_))
        return *found;
    else
        return clades_.emplace_back(clade_name, display_name);

} // acmacs::tal::v3::Tree::find_or_add_clade

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Tree::has_sequences() const
{
    if (!first_prev_leaf)
        return !find_first_leaf().aa_sequence.empty();
    else
        return !first_prev_leaf->aa_sequence.empty();

} // acmacs::tal::v3::Tree::has_sequences

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_set(std::string_view clade_name, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos, std::string_view display_name)
{
    if (nodes_hidden_) {
        clades_reset();
        set_first_last_next_node_id();
        nodes_hidden_ = false;
    }

    size_t num = 0;
    const std::string clade_name_s{clade_name};
    auto& clade_sections = find_or_add_clade(clade_name, display_name).sections;
    tree::iterate_leaf(*this, [&aa_at_pos, clade_name_s, &num, &clade_sections](Node& node) {
        if (!node.hidden && acmacs::seqdb::matches(node.aa_sequence, aa_at_pos)) {
            node.clades.add(clade_name_s);
            ++num;
            if (clade_sections.empty() || (node.node_id.vertical - clade_sections.back().last->node_id.vertical) > 1)
                clade_sections.emplace_back(&node);
            else
                clade_sections.back().last = &node;
        }
    });

    // // remove small sections
    // clade_sections.erase(
    //     std::remove_if(std::begin(clade_sections), std::end(clade_sections), [exclusion_tolerance](const auto& section) { return (section.last->node_id.vertical - section.first->node_id.vertical) < exclusion_tolerance; }),
    //     std::end(clade_sections));

    AD_LOG(acmacs::log::clades, "\"{}\": {} leaves, {} sections", clade_name, num, clade_sections.size());
    // AD_DEBUG("clade \"{}\": {} leaves, {} sections", clade_name, num, clade_sections.size());
} // acmacs::tal::v3::Tree::clade_set

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_set(std::string_view clade_name, const acmacs::seqdb::nucleotide_at_pos1_eq_list_t& nuc_at_pos, std::string_view display_name)
{
    size_t num = 0;
    const std::string clade_name_s{clade_name};
    auto& clade_sections = find_or_add_clade(clade_name, display_name).sections;
    tree::iterate_leaf(*this, [&nuc_at_pos, clade_name_s, &num, &clade_sections](Node& node) {
        if (!node.hidden && acmacs::seqdb::matches(node.nuc_sequence, nuc_at_pos)) {
            node.clades.add(clade_name_s);
            ++num;
            if (clade_sections.empty() || (node.node_id.vertical - clade_sections.back().last->node_id.vertical) > 1)
                clade_sections.emplace_back(&node);
            else
                clade_sections.back().last = &node;
        }
    });

} // acmacs::tal::v3::Tree::clade_set

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_report(std::string_view clade_name_to_report) const
{
    const auto report = [](std::string_view clade_name, const auto& sections) {
        fmt::print("Clade \"{}\" ({})\n", clade_name, sections.size());
        for (auto it = std::begin(sections); it != std::end(sections); ++it) {
            if (it != std::begin(sections))
                fmt::print("    gap {}\n", it->first->node_id.vertical - std::prev(it)->last->node_id.vertical - 1);
            fmt::print(" ({}) {} {} .. {} {}\n", it->last->node_id.vertical - it->first->node_id.vertical + 1, it->first->node_id, it->first->seq_id, it->last->node_id, it->last->seq_id);
        }
        fmt::print("\n");
    };

    if (clade_name_to_report.empty()) {
        for (const auto& clade : clades_)
            report(clade.name, clade.sections);
    }
    else if (const auto* found = find_clade(clade_name_to_report); found)
        report(clade_name_to_report, found->sections);
    else
        AD_WARNING("no clade \"{}\" defined", clade_name_to_report);

} // acmacs::tal::v3::Tree::clade_report

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

double acmacs::tal::v3::Tree::compute_cumulative_vertical_offsets()
{
    double height{0.0};
    tree::iterate_leaf_post(*this,
                            [&height](Node& leaf) {
                                if (!leaf.hidden) {
                                    height += leaf.vertical_offset_;
                                    leaf.cumulative_vertical_offset_ = height;
                                }
                            },
                            [](Node& node) {
                                if (const auto& shown_children = node.shown_children(); !shown_children.empty())
                                    node.cumulative_vertical_offset_ = (shown_children.front()->cumulative_vertical_offset_ + shown_children.back()->cumulative_vertical_offset_) / 2.0;
                            });
    return height;

} // acmacs::tal::v3::Tree::compute_cumulative_vertical_offsets

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
