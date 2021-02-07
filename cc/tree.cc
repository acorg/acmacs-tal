#include <algorithm>
#include <regex>
#include <set>
#include <stack>

#include "acmacs-base/statistics.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-virus/virus-name-v1.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/draw-tree.hh"

// ----------------------------------------------------------------------

const acmacs::tal::v3::Node& acmacs::tal::v3::Node::find_first_leaf() const
{
    if (is_leaf())
        return *this;
    else
        return subtree.front().find_first_leaf();

} // acmacs::tal::v3::Node::find_first_leaf

// ----------------------------------------------------------------------

void acmacs::tal::v3::Node::replace_aa_transition(seqdb::pos0_t pos, char right)
{
    // AD_DEBUG_IF(pos == seqdb::pos1_t{160}, "replace_aa_transition {:5.3} {}{} (was: {})", node_id, pos, right, aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
    remove_aa_transition(pos, right);
    aa_transitions_.add(pos, right);
    // AD_DEBUG_IF(pos == seqdb::pos1_t{160}, "    --> {}", aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));

} // acmacs::tal::v3::Node::replace_aa_transition

// ----------------------------------------------------------------------

void acmacs::tal::v3::Node::remove_aa_transition(seqdb::pos0_t pos, char right)
{
    auto remove = [pos,right](Node& node) -> bool {
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
        Timeit time1(">>>> cumulative_calculate: ", report_time::no);
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

std::string acmacs::tal::v3::Tree::report_cumulative(size_t max) const
{
    cumulative_calculate();

    std::vector<const Node*> nodes;
    tree::iterate_leaf(*this, [&nodes](const Node& node) { nodes.push_back(&node); });
    std::sort(std::begin(nodes), std::end(nodes), [](const auto& en1, const auto& en2) { return en1->cumulative_edge_length > en2->cumulative_edge_length; });

    auto result{fmt::format("Nodes {} sorted by cumulative\n Cumulative       Edge       Seq id\n", max > 0 ? fmt::format("(max {})", max) : std::string{"(all)"})};
    for (const auto [no, node] : acmacs::enumerate(nodes)) {
        result.append(fmt::format("{:.10f}  {:.10f}  {}\n", node->cumulative_edge_length.as_number(), node->edge_length.as_number(), node->seq_id));
        if (max > 0 && no >= max)
            break;
    }
    return result;

} // acmacs::tal::v3::Tree::report_cumulative

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

std::vector<const acmacs::tal::v3::Node*> acmacs::tal::v3::Tree::sorted_by_cumulative_edge(leaves_only lo) const
{
    cumulative_calculate();
    std::vector<const Node*> sorted;
    const auto collect = [&sorted](const Node& node) { sorted.push_back(&node); };
    if (lo == leaves_only::yes)
        tree::iterate_leaf(*this, collect);
    else
        tree::iterate_leaf_pre(*this, collect, collect);
    std::sort(std::begin(sorted), std::end(sorted), [](const Node* n1, const Node* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length; });
    return sorted;

} // acmacs::tal::v3::Tree::sorted_by_cumulative_edge

// ----------------------------------------------------------------------

double acmacs::tal::v3::Tree::mean_edge_of(double fraction_or_number, const std::vector<const Node*>& sorted) const
{
    const auto edge = [](const Node* node) { return node->edge_length.as_number(); };
    if (fraction_or_number <= 1.0)
        return acmacs::statistics::mean(std::begin(sorted), std::next(std::begin(sorted), static_cast<ssize_t>(static_cast<double>(sorted.size()) * fraction_or_number)), edge);
    else
        return acmacs::statistics::mean(std::begin(sorted), std::next(std::begin(sorted), static_cast<ssize_t>(fraction_or_number)), edge);

} // acmacs::tal::v3::Tree::mean_edge_of

// ----------------------------------------------------------------------

double acmacs::tal::v3::Tree::mean_cumulative_edge_of(double fraction_or_number, const std::vector<const Node*>& sorted) const
{
    const auto cumulative_edge = [](const Node* node) { return node->cumulative_edge_length.as_number(); };
    if (fraction_or_number <= 1.0)
        return acmacs::statistics::mean(std::begin(sorted), std::next(std::begin(sorted), static_cast<ssize_t>(static_cast<double>(sorted.size()) * fraction_or_number)), cumulative_edge);
    else
        return acmacs::statistics::mean(std::begin(sorted), std::next(std::begin(sorted), static_cast<ssize_t>(fraction_or_number)), cumulative_edge);

} // acmacs::tal::v3::Tree::mean_cumulative_edge_of

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
    // const auto sort_by_cumulative = [](auto& nods) { std::sort(std::begin(nods), std::end(nods), [](const Node* n1, const Node* n2) { return n1->cumulative_edge_length > n2->cumulative_edge_length;
    // }); }; const auto edge = [](const Node* node) { return node->edge_length.as_number(); }; const auto mean_edge = [&nodes, edge](double fraction) {
    //     return acmacs::statistics::mean(std::begin(nodes), std::next(std::begin(nodes), static_cast<ssize_t>(nodes.size() * fraction)), edge);
    // };

    // sort_by_cumulative(nodes);
    // sort_by_edge(nodes);

    const std::vector<const Node*> nodes = sorted_by_edge();

    fmt::print("  mean edge (all      : {:4d}) {}\n", nodes.size(), mean_edge_of(1.0, nodes));
    fmt::print("  mean edge (top 20%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.2), mean_edge_of(0.2, nodes));
    fmt::print("  mean edge (top 10%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.1), mean_edge_of(0.1, nodes));
    fmt::print("  mean edge (top  5%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.05), mean_edge_of(0.05, nodes));
    fmt::print("  mean edge (top  1%  : {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.01), mean_edge_of(0.01, nodes));
    fmt::print("  mean edge (top  0.5%: {:4d}) {}\n", static_cast<size_t>(static_cast<double>(nodes.size()) * 0.005), mean_edge_of(0.005, nodes));
    fmt::print("  HINT: hide by edge, if edge > mean of top 1%\n\n");

    fmt::print("        Edge     Cumulative     Seq Id\n");
    for (auto [no, node] : acmacs::enumerate(nodes)) {
        if (node->number_leaves_in_subtree() > 1) {
            fmt::print("    {:.8f}  {:.8f}    [{}]", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->number_leaves_in_subtree());
            size_t no2{0};
            tree::iterate_leaf_stop(*node, [&no2](const auto& subnode) {
                fmt::print("  {}", subnode.seq_id);
                ++no2;
                return no2 > 2;
            });
            fmt::print("\n");
        }
        else
            fmt::print("    {:.8f}  {:.8f}   {}\n", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->seq_id);
        if (no > 20) // nodes.size() / 100)
            break;
    }
    fmt::print("\n");

    NodeSet selected;
    const auto mean_edge = mean_edge_of(0.01, nodes);
    select_if_edge_more_than(selected, Select::init, mean_edge);
    fmt::print("  selected with edge > {}: {}\n", mean_edge, selected.size());
    if (!selected.empty()) {
        fmt::print("       edge   node-id   Seq Id\n");
        for (Node* node : selected) {
            fmt::print("    {} {:4.4} {}\n", node->edge_length, node->node_id, node->seq_id);
            node->color_edge_line = RED;
        }
    }

} // acmacs::tal::v3::Tree::branches_by_edge

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::branches_by_cumulative_edge()
{
    number_leaves_in_subtree();

    AD_INFO("cumulative edge report");
    const std::vector<const Node*> nodes = sorted_by_cumulative_edge(leaves_only::no);

    std::vector<double> cumulative_gaps(nodes.size() - 1);
    const Node* prev{nullptr};
    std::vector<double>::iterator cur_gap{cumulative_gaps.begin()};
    for (const auto* node : nodes) {
        if (prev) {
            *cur_gap = prev->cumulative_edge_length.as_number() - node->cumulative_edge_length.as_number();
            ++cur_gap;
        }
        prev = node;
    }
    std::sort(std::begin(cumulative_gaps), std::end(cumulative_gaps), [](double g1, double g2) { return g1 > g2; });

    const size_t num_top = cumulative_gaps.size() / 200 + 1; // +1 to avoid div by zero
    const auto mean_top = std::accumulate(std::begin(cumulative_gaps), std::next(std::begin(cumulative_gaps), static_cast<ssize_t>(num_top)), 0.0) / static_cast<double>(num_top);

    fmt::print("  cumulative_gaps   mean {} top: {:.8f}\n", num_top, mean_top);
    for (auto gp = std::begin(cumulative_gaps); gp != std::end(cumulative_gaps); ++gp) {
        fmt::print("    {:.8f}\n", *gp);
        if ((gp - std::begin(cumulative_gaps)) > 20)
            break;
    }

    fmt::print("        Edge     Cumulative     Seq Id\n");
    for (auto [no, node] : acmacs::enumerate(nodes)) {
        if (node->number_leaves_in_subtree() > 1) {
            fmt::print("    {:.8f}  {:.8f}    [{}]", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->number_leaves_in_subtree());
            size_t no2{0};
            tree::iterate_leaf_stop(*node, [&no2](const auto& subnode) {
                fmt::print("  {}", subnode.seq_id);
                ++no2;
                return no2 > 2;
            });
            fmt::print("\n");
        }
        else
            fmt::print("    {:.8f}  {:.8f}   {}\n", node->edge_length.as_number(), node->cumulative_edge_length.as_number(), node->seq_id);
        if (no > 20) // nodes.size() / 100)
            break;
    }
    fmt::print("\n");

} // acmacs::tal::v3::Tree::branches_by_cumulative_edge

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
    const std::vector<const Node*> sorted = sorted_by_edge();
    select_update(nodes, update, Descent::yes, *this, [edge_min=EdgeLength{mean_edge_of(fraction_or_number, sorted)}](Node& node) { return !node.hidden && node.edge_length >= edge_min; });

} // acmacs::tal::v3::Tree::select_if_edge_more_than_mean_edge_of

void acmacs::tal::v3::Tree::select_all(NodeSet& nodes, Select update)
{
    select_update(nodes, update, Descent::yes, *this, [](const Node& node) { return node.is_leaf() && !node.hidden; });
}

void acmacs::tal::v3::Tree::select_all_and_intermediate(NodeSet& nodes, Select update)
{
    select_update(nodes, update, Descent::yes, *this, [](const Node& node) { return !node.hidden; });
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

acmacs::chart::PointIndexList acmacs::tal::v3::Tree::chart_antigens_in_tree() const
{
    acmacs::chart::PointIndexList indexes;
    tree::iterate_leaf(*this, [&indexes](const Node& node) {
        if (node.antigen_index_in_chart_.has_value())
            indexes.insert(*node.antigen_index_in_chart_);
    });
    return indexes;

} // acmacs::tal::v3::Tree::chart_antigens_in_tree

acmacs::chart::PointIndexList acmacs::tal::v3::Tree::chart_antigens_in_section(const Node* first, const Node* last) const
{
    acmacs::chart::PointIndexList indexes;
    bool use{first == nullptr};
    tree::iterate_leaf(*this, [&indexes, &use, first, last](const Node& node) {
        if (!use && &node == first)
            use = true;
        if (use && node.antigen_index_in_chart_.has_value())
            indexes.insert(*node.antigen_index_in_chart_);
        if (use && &node == last)
            use = false;
    });
    return indexes;

} // acmacs::tal::v3::Tree::chart_antigens_in_section

acmacs::chart::PointIndexList acmacs::tal::v3::Tree::chart_sera_in_tree(serum_match_t match_type) const
{
    acmacs::chart::PointIndexList indexes;
    tree::iterate_leaf(*this, [&indexes, match_type](const Node& node) {
        for (const auto& [serum_index, reassortant_matches, passage_type_matches] : node.serum_index_in_chart_) {
            switch (match_type) {
              case serum_match_t::name:
                  indexes.insert(serum_index);
                  break;
              case serum_match_t::reassortant:
                  if (reassortant_matches)
                      indexes.insert(serum_index);
                  break;
              case serum_match_t::passage_type:
                  if (reassortant_matches && passage_type_matches)
                      indexes.insert(serum_index);
                  break;
            }
        }
    });
    return indexes;

} // acmacs::tal::v3::Tree::chart_sera_in_tree

acmacs::chart::PointIndexList acmacs::tal::v3::Tree::chart_sera_in_section(const Node* first, const Node* last) const
{
    acmacs::chart::PointIndexList indexes;
    bool use{first == nullptr};
    tree::iterate_leaf(*this, [&indexes, &use, first, last](const Node& node) {
        if (!use && &node == first)
            use = true;
        if (use) {
            for (const auto& en : node.serum_index_in_chart_)
                indexes.insert(std::get<size_t>(en));
        }
        if (use && &node == last)
            use = false;
    });
    return indexes;

} // acmacs::tal::v3::Tree::chart_sera_in_section

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::select_by_top_cumulative_gap(NodeSet& nodes, Select update, double top_gap_rel)
{
    if (top_gap_rel <= 1.0) {
        AD_WARNING("invalid value for \"top-cumulative-gap\": {}, must be >1.0, node selection criterium ignored", top_gap_rel);
        return;
    }

    if (const std::vector<const Node*> sorted = sorted_by_cumulative_edge(leaves_only::no); sorted.size() > 10) {
        std::vector<double> cumulative_gaps(sorted.size() - 1);
        for (auto ps = std::next(sorted.begin()); ps != sorted.end(); ++ps)
            cumulative_gaps[static_cast<size_t>(ps - sorted.begin() - 1)] = (*std::prev(ps))->cumulative_edge_length.as_number() - (*ps)->cumulative_edge_length.as_number();
        std::sort(std::begin(cumulative_gaps), std::end(cumulative_gaps), [](double g1, double g2) { return g1 > g2; });
        if ((cumulative_gaps[0] / cumulative_gaps[1]) > top_gap_rel) {
            double cut_off_cumulative_edge{0.0};
            for (auto ps = std::next(sorted.begin()); ps != sorted.end(); ++ps) {
                if (const auto gap = (*std::prev(ps))->cumulative_edge_length.as_number() - (*ps)->cumulative_edge_length.as_number(); float_equal(gap, cumulative_gaps[0])) {
                    cut_off_cumulative_edge = (*std::prev(ps))->cumulative_edge_length.as_number();
                    break;
                }
            }
            select_if_cumulative_more_than(nodes, update, cut_off_cumulative_edge, Descent::no);
            AD_INFO("\"top-cumulative-gap\": {}: cut_off_gaps: {:.8f}  cut_off_cumulative_edge: {:.8f}", top_gap_rel, cumulative_gaps[0], cut_off_cumulative_edge);
        }
        else
            AD_INFO("\"top-cumulative-gap\" not applied, ratio of top cumul gaps: {} <= {}", cumulative_gaps[0] / cumulative_gaps[1], top_gap_rel);
    }

} // acmacs::tal::v3::Tree::select_by_top_cumulative_gap

// ----------------------------------------------------------------------

void acmacs::tal::v3::Node::hide()
{
    hidden = true;
    for (auto& child : subtree)
        child.hide();

} // acmacs::tal::v3::Node::hide

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::hide(const NodeSet& nodes, hide_if_too_many_leaves force)
{
    size_t leaves_to_hide{0};
    for (const auto& node : nodes)
        leaves_to_hide += node->number_leaves_in_subtree();
    const auto total_leaves{number_leaves_in_subtree()};
    const auto percent_to_hide{double(leaves_to_hide) / double(total_leaves) * 100.0};

    if ((leaves_to_hide * 2) > total_leaves && force == hide_if_too_many_leaves::no) {
        AD_WARNING("requested to hide too many leaves (ignored): {} nodes with {} leaves ({:.1f}% of all leaves)", nodes.size(), leaves_to_hide, percent_to_hide);
        return;
    }

    AD_INFO("hiding {} nodes with {} leaves ({:.1f}% of all leaves)", nodes.size(), leaves_to_hide, percent_to_hide);

    for (Node* node : nodes)
        node->hide();

    // if all children are hidden, hide parent too
    tree::iterate_post(*this, [](Node& node) {
        if (const auto shown_children = node.shown_children(); shown_children.empty())
            node.hidden = true;
    });

    structure_modified("hiding node");

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
    gisaid = ref.seq().gisaid;

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
    std::stack<Node*> parents;
    const auto pre_populate = [&parents](Node& node) { parents.push(&node); };
    const auto post_populate = [&parents](Node&) { parents.pop(); };
    const auto leaf_populate = [&parents, &seqdb, already_in_tree](const Node& node) {
        if (!node.ref.empty()) {
            // AD_DEBUG("[populate_with_nuc_duplicates] {}", node.ref.seq_id());
            for (const auto& slave : node.ref.seq().slaves()) {
                if (const auto seq_id = slave.seq_id(); !already_in_tree(seq_id)) {
                    // AD_DEBUG("[populate_with_nuc_duplicates]     {}", seq_id);
                    parents.top()->to_populate.emplace_back(seq_id, node.edge_length).populate(slave, seqdb);
                }
            }
        }
    };
    tree::iterate_leaf_pre_post(*this, leaf_populate, pre_populate, post_populate);

    size_t added_leaves{0};
    const auto post_add_nuc_duplicate = [&added_leaves](Node& node) {
        if (!node.to_populate.empty()) {
            added_leaves += node.to_populate.size();
            std::move(std::begin(node.to_populate), std::end(node.to_populate), std::back_inserter(node.subtree));
            node.to_populate.clear();
        }
    };
    tree::iterate_post(*this, post_add_nuc_duplicate);

    AD_INFO("populate_with_nuc_duplicates:\n  initial: {:5d}\n  added:   {:5d}\n  total:   {:5d}", all_seq_ids.size(), added_leaves, all_seq_ids.size() + added_leaves);

} // acmacs::tal::v3::Tree::populate_with_nuc_duplicates

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::set_first_last_next_node_id()
{
    if (structure_modified_) {
        // AD_DEBUG("set_first_last_next_node_id");
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
            parents.pop_back();
        };

        tree::iterate_leaf_pre_post(*this, leaf, pre, post);
        structure_modified_ = false;
        // AD_DEBUG("structure_modified_ <- false");
    }

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
    // Timeit time_ladderize(">>>> [time] ladderize: ");

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

    // ----------------------------------------------------------------------

    set_first_last_next_node_id();

    // set max_edge_length field for every node
    tree::iterate_leaf_post(*this, set_leaf, set_parent);

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

    // AD_DEBUG("ladderized");
    structure_modified("ladderizing");

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
    AD_INFO("re-rooting");
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

    // AD_DEBUG("re-rooted");
    structure_modified("re_root");

} // acmacs::tal::v3::Tree::re_root

// ----------------------------------------------------------------------

std::vector<std::string_view> acmacs::tal::v3::Tree::all_dates() const
{
    std::vector<std::string_view> dates;
    tree::iterate_leaf(*this, [&dates](const Node& node) { dates.push_back(node.date); });
    std::sort(std::begin(dates), std::end(dates));
    dates.erase(std::unique(std::begin(dates), std::end(dates)), std::end(dates));
    return dates;

} // acmacs::tal::v3::Tree::all_dates

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::Tree::report_aa_at(const std::vector<acmacs::seqdb::pos1_t>& pos, bool /*names*/) const
{
    std::vector<acmacs::CounterChar> counter(pos.size());
    tree::iterate_leaf(*this, [&pos, &counter](const Node& leaf) {
        for (auto it = pos.begin(); it != pos.end(); ++it)
            counter[static_cast<size_t>(it - pos.begin())].count(acmacs::seqdb::at_pos(leaf.aa_sequence, *it));
    });
    std::string result;
    for (auto it = pos.begin(); it != pos.end(); ++it) {
        result += fmt::format("  {}\n{}\n", *it, counter[static_cast<size_t>(it - pos.begin())].report_sorted_max_first("    {first} {second:5d}\n"));
    }
    return result;

} // acmacs::tal::v3::Tree::report_aa_at

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::aa_at_pos_report(size_t tolerance) const
{
    struct chunk_t
    {
        // constexpr chunk_t() = default;
        constexpr chunk_t(char aaa, node_id_t::value_type vert) : aa{aaa}, first{vert}, last{vert} {}
        constexpr size_t size() const { return last - first + 1ul; }
        char aa;
        node_id_t::value_type first;
        node_id_t::value_type last;
    };

    std::vector<std::vector<chunk_t>> aa_at_pos;

    tree::iterate_leaf(*this, [&aa_at_pos, tolerance](const Node& node) {
        if (!node.hidden) {
            if (aa_at_pos.size() < *node.aa_sequence.size())
                aa_at_pos.resize(*node.aa_sequence.size());
            for (seqdb::pos0_t pos{0}; pos < node.aa_sequence.size(); ++pos) {
                const auto aa = node.aa_sequence.at(pos);
                if (aa_at_pos[*pos].size() > 1 && aa_at_pos[*pos].back().aa != aa && aa_at_pos[*pos].back().size() < tolerance)
                    aa_at_pos[*pos].pop_back();
                if (!aa_at_pos[*pos].empty() && aa_at_pos[*pos].back().aa == aa)
                    aa_at_pos[*pos].back().last = node.node_id.vertical;
                else
                    aa_at_pos[*pos].emplace_back(aa, node.node_id.vertical);
            }
        }
    });

    AD_INFO("aa-at-pos-report");
    for (const auto [pos, chunks] : acmacs::enumerate(aa_at_pos)) {
        if (chunks.size() > 1) {
            fmt::print("{:3d}  ({:3d})\n", pos + 1, chunks.size());
            for (const auto& chunk : chunks) {
                fmt::print("   {} {:5d}  {:5d} .. {:5d}\n", chunk.aa, chunk.size(), chunk.first, chunk.last);
            }
        }
    }

} // acmacs::tal::v3::Tree::aa_at_pos_report

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::aa_at_pos_counter_report(double tolerance) const
{
    using CounterAA = acmacs::CounterCharSome<' ', '`'>;
    std::vector<CounterAA> counter_aa_at_pos;

    tree::iterate_leaf(*this, [&counter_aa_at_pos](const Node& node) {
        if (!node.hidden) {
            if (counter_aa_at_pos.size() < *node.aa_sequence.size())
                counter_aa_at_pos.resize(*node.aa_sequence.size());
            for (seqdb::pos0_t pos{0}; pos < node.aa_sequence.size(); ++pos)
                counter_aa_at_pos[*pos].count(node.aa_sequence.at(pos));
        }
    });

    AD_INFO("aa-at-pos-counter-report");
    const auto& root_seq = find_first_leaf().aa_sequence;
    for (const auto [pos, counter] : acmacs::enumerate(counter_aa_at_pos)) {
        const auto total = static_cast<double>(counter.total());
        std::vector<std::pair<char, double>> aa_precent;
        for (const auto& [aa, count] : counter.pairs(CounterAA::sorted::yes)) {
            if (const auto percent = static_cast<double>(count) / total; percent >= tolerance)
                aa_precent.emplace_back(aa, percent);
        }
        const auto root_aa = root_seq.at(seqdb::pos0_t{pos});
        if (aa_precent.size() > 1 || root_aa != aa_precent.front().first) {
            fmt::print("{:3d}", pos + 1);
            for (auto it = aa_precent.begin(); it != aa_precent.end(); ++it)
                fmt::print("  {}:{:.0f}%", it->first, it->second * 100.0);
            fmt::print("  root:{}\n", root_aa);
        }
    }

} // acmacs::tal::v3::Tree::aa_at_pos_counter_report

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

acmacs::tal::v3::Tree::clade_t* acmacs::tal::v3::Tree::find_clade(std::string_view clade_name)
{
    if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [clade_name](const auto& cl) { return cl.name == clade_name; }); found != std::end(clades_))
        return &*found;
    else
        return nullptr;

} // acmacs::tal::v3::Tree::find_clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::add_clade(std::string_view clade_name, std::string_view display_name)
{
    if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [clade_name](const auto& cl) { return cl.name == clade_name; }); found == std::end(clades_))
        clades_.emplace_back(clade_name, display_name);

} // acmacs::tal::v3::Tree::add_clade

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Tree::has_sequences() const
{
    if (!first_prev_leaf)
        return !find_first_leaf().aa_sequence.empty();
    else
        return !first_prev_leaf->aa_sequence.empty();

} // acmacs::tal::v3::Tree::has_sequences

// ----------------------------------------------------------------------

template <typename AA_AT> static inline void clade_set_by(std::string_view clade_name, AA_AT&& aa_at_pos, std::string_view display_name, acmacs::tal::Tree& tree)
{
    tree.add_clade(clade_name, display_name);
    size_t num = 0;
    acmacs::tal::tree::iterate_leaf(tree, [&aa_at_pos, clade_name, &num](acmacs::tal::Node& node) {
        const acmacs::seqdb::sequence_aligned_ref_t* sequence{nullptr};
        if constexpr (std::is_same_v<std::decay_t<AA_AT>, acmacs::seqdb::amino_acid_at_pos1_eq_list_t>)
            sequence = &node.aa_sequence;
        else
            sequence = &node.nuc_sequence;
        if (acmacs::seqdb::matches(*sequence, std::forward<AA_AT>(aa_at_pos))) {
            node.clades.add(clade_name);
            ++num;
        }
    });
    AD_LOG(acmacs::log::clades, "\"{}\": {} leaves", clade_name, num);
}

void acmacs::tal::v3::Tree::clade_set_by_aa_at_pos(std::string_view clade_name, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos, std::string_view display_name)
{
    ::clade_set_by(clade_name, aa_at_pos, display_name, *this);

} // acmacs::tal::v3::Tree::clade_set_by_aa_at_pos

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::clade_set_by_nuc_at_pos(std::string_view clade_name, const acmacs::seqdb::nucleotide_at_pos1_eq_list_t& nuc_at_pos, std::string_view display_name)
{
    ::clade_set_by(clade_name, nuc_at_pos, display_name, *this);

} // acmacs::tal::v3::Tree::clade_set_by_nuc_at_pos

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::make_clade_sections()
{
    AD_LOG(acmacs::log::clades, "make_clade_sections");

    set_first_last_next_node_id();

    bool clade_data_found = false;
    tree::iterate_leaf(*this, [this, &clade_data_found](Node& node) {
        if (!node.hidden) {
            for (const auto& clade_name : node.clades) {
                if (clade_t* clade = find_clade(clade_name); clade) {
                    if (clade->sections.empty() || (node.node_id.vertical - clade->sections.back().last->node_id.vertical) > 1)
                        clade->sections.emplace_back(&node);
                    else
                        clade->sections.back().last = &node;
                    clade_data_found = true;
                }
            }
        }
    });
    if (!clade_data_found)
        AD_WARNING("no clade names found in tree nodes, forgot to add \"clades-{{virus-type/lineage}}\" or \"clades-whocc\" in settings?");

} // acmacs::tal::v3::Tree::make_clade_sections

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
            const auto parsed_seq_id = acmacs::virus::name::parse(node.seq_id);
            if (const auto found = serum_names.find(parsed_seq_id.name()); found != std::end(serum_names)) {
                for (auto serum_index : found->second)
                    node.serum_index_in_chart_.emplace_back(serum_index, sera->at(serum_index)->reassortant() == parsed_seq_id.reassortant, sera->at(serum_index)->passage().is_egg() == parsed_seq_id.passage.is_egg());
            }
        });
        chart_matched_ = true;
    }

} // acmacs::tal::v3::Tree::match

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::set_top_gap(const Node& node, double gap) const
{
    // gap is a fraction of tree height (number of shown leaves)
    const auto gap_to_use{gap * static_cast<double>(number_leaves_in_subtree())};
    if (node.vertical_offset_ < gap_to_use)
        node.vertical_offset_ = gap_to_use;

} // acmacs::tal::v3::Tree::set_top_gap

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::set_bottom_gap(const Node& node, double gap) const
{
    if (node.last_next_leaf)
        set_top_gap(*node.last_next_leaf, gap);

} // acmacs::tal::v3::Tree::set_bottom_gap

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
