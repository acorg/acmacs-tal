#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-tal/aa-transition.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    // ----------------------------------------------------------------------
    // AA subst calculation eu-20200514
    static void update_aa_transitions_eu_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters);

    // ----------------------------------------------------------------------
    // AA subst calculation derek-2016
    // returns length of the longest sequence found under root
    static seqdb::pos0_t update_common_aa(Node& root);
    static void report_common_aa(const Node& root, std::optional<seqdb::pos1_t> pos_to_report, size_t number_leaves_threshold);
    static void update_aa_transitions_derek_2016(Tree& tree, const draw_tree::AATransitionsParameters& parameters);

    // ----------------------------------------------------------------------
    // AA subst calculation eu-20200909
    static void update_aa_transitions_eu_20200909(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
    static void set_aa_transitions_eu_20200915(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters);
    static void set_aa_transitions_eu_20210205(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters);
    static void update_aa_transitions_eu_20200915(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::reset_aa_transitions(Tree& tree)
{
    const auto reset_node = [](Node& node) { node.aa_transitions_.clear(); };

    tree::iterate_leaf_pre(tree, reset_node, reset_node);

} // acmacs::tal::v3::reset_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::update_aa_transitions(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    switch (parameters.method) {
      case draw_tree::AATransitionsParameters::method::eu_20200514:
          update_aa_transitions_eu_20200514(tree, parameters);
          break;
      case draw_tree::AATransitionsParameters::method::eu_20200909:
          update_aa_transitions_eu_20200909(tree, parameters);
          break;
      case draw_tree::AATransitionsParameters::method::eu_20200915:
          update_aa_transitions_eu_20200915(tree, parameters);
          break;
      case draw_tree::AATransitionsParameters::method::derek_2016:
          update_aa_transitions_derek_2016(tree, parameters);
          break;
    }

} // acmacs::tal::v3::update_aa_transitions

// ======================================================================

void acmacs::tal::v3::update_aa_transitions_eu_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG_IF(parameters.debug, "update_aa_transitions_eu_20200514");
    tree.cumulative_calculate();

    // see https://notebooks.antigenic-cartography.org/eu/results/2020-0513-tree-aa-subst-left/
    // for testing, methods description and known issues

    // 1. for each branch node find closest child leaf node
    // --
    // 2. for each pos, for each branch node
    // AA at pos at branch node is AA at pos for its closest_leaf
    // if AA at pos for node differs from AA at the same pos for its
    // parent node, add aa transition with left part being AA at
    // parent node, right part being AA at this node

    tree::iterate_post(tree, [&parameters](Node& branch) {
        branch.closest_leaf = nullptr;
        for (const auto& child : branch.subtree) {
            if (const auto* leaf = child.is_leaf() ? &child : child.closest_leaf; leaf) {
                if (branch.closest_leaf == nullptr || branch.closest_leaf->cumulative_edge_length > leaf->cumulative_edge_length)
                    branch.closest_leaf = leaf;
            }
        }

        if (branch.closest_leaf) {
            for (auto& child : branch.subtree) {
                if (child.closest_leaf && branch.closest_leaf != child.closest_leaf) {
                    for (seqdb::pos0_t pos{0}; pos < std::min(branch.closest_leaf->aa_sequence.size(), child.closest_leaf->aa_sequence.size()); ++pos) {
                        // transitions to/from X ignored
                        // perhaps we need to look for a second closest leaf if found closest leaf has X at the pos
                        if (const auto left_aa = branch.closest_leaf->aa_sequence.at(pos), right_aa = child.closest_leaf->aa_sequence.at(pos);
                            left_aa != right_aa && left_aa != 'X' && right_aa != 'X') {
                            child.aa_transitions_.add(pos, left_aa, right_aa);
                            AD_DEBUG_IF(parameters.debug && parameters.report_pos && pos == *parameters.report_pos,
                                        "update_aa_transitions_eu_20200514 node:{:4.3s} {}{}{} leaves:{:5d} closest-cumul:{} closest:{}", child.node_id,
                                        left_aa, pos, right_aa, child.number_leaves_in_subtree(), child.closest_leaf->cumulative_edge_length, child.closest_leaf->seq_id);
                        }
                    }
                }
            }
        }
        else
            AD_WARNING("update_aa_transitions_eu_20200514: closest leaf not found for the branch node {}", branch.node_id);
    });

} // acmacs::tal::v3::update_aa_transitions_eu_20200514

// ----------------------------------------------------------------------

acmacs::seqdb::pos0_t acmacs::tal::v3::update_common_aa(Node& root)
{
    seqdb::pos0_t longest_sequence{0};
    tree::iterate_post(root, [&longest_sequence](Node& node) {
        for (auto& child : node.subtree) {
            if (!child.hidden) {
                if (child.is_leaf()) {
                    node.common_aa_.update(child.aa_sequence);
                    longest_sequence = std::max(longest_sequence, seqdb::pos0_t{child.aa_sequence.size()});
                }
                else
                    node.common_aa_.update(child.common_aa_);
            }
        }
    });
    return longest_sequence;

} // acmacs::tal::v3::update_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::report_common_aa(const Node& root, std::optional<seqdb::pos1_t> pos_to_report, size_t number_leaves_threshold)
{
    AD_INFO("common AA");
    root.number_leaves_in_subtree();
    tree::iterate_pre_parent(root, [&pos_to_report, number_leaves_threshold](const Node& node, const Node& parent) {
        if (node.number_leaves_in_subtree() >= number_leaves_threshold) {
            if (const auto rep = node.common_aa_.report(parent.common_aa_, pos_to_report); !rep.empty())
                fmt::print(stderr, "    node:{:4.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::report_common_aa

// ======================================================================
// derek's method 2016

void acmacs::tal::v3::update_aa_transitions_derek_2016(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG_IF(parameters.debug, "update_aa_transitions_derek_2016");

    tree.cumulative_calculate();
    const auto longest_sequence = update_common_aa(tree);

    const auto aa_at = [](const Node& node, seqdb::pos0_t pos) {
        if (node.is_leaf())
            return node.aa_sequence.at(pos);
        else
            return node.common_aa_.at(pos);
    };

    tree::iterate_post(tree, [aa_at, longest_sequence, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto common_aa_at = node.common_aa_.at(pos);
            AD_DEBUG_IF(dbg, "update_aa_transitions_derek_2016 counting {} node:{:4.3s} leaves:{:4d} common-aa:{} is_no_common:{}", pos, node.node_id, node.number_leaves_in_subtree(), common_aa_at,
                        common_aa_at == CommonAA::NoCommon);
            if (common_aa_at == CommonAA::NoCommon) {
                CounterChar counter;
                for (auto& child : node.subtree) {
                    if (const auto aa = aa_at(child, pos); CommonAA::is_common(aa)) {
                        child.aa_transitions_.add(pos, aa);
                        counter.count(aa);
                    }
                    else if (const auto found = child.aa_transitions_.find(pos); found) {
                        counter.count(found->right);
                    }
                }
                AD_DEBUG_IF(dbg, "  update_aa_transitions_derek_2016 counting {} node:{:4.3s} leaves:{:4d} counter: {}", pos, node.node_id, node.number_leaves_in_subtree(), counter);
                if (const auto [max_aa, max_count] = counter.max(); max_count > 1)
                    node.replace_aa_transition(pos, max_aa);
            }
            // else {
            //     if (node.aa_transitions_.has(pos))
            //         AD_WARNING("update_aa_transitions_before_20200513 (the only common) has for pos {}", pos);
            // }
        }
    });

    const std::vector<const Node*> sorted_leaf_nodes = tree.sorted_by_cumulative_edge(Tree::leaves_only::yes); // bigger cumul length first

    AD_DEBUG_IF(parameters.debug, "update_aa_transitions_derek_2016 adding left part");
    // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&sorted_leaf_nodes, &parameters](Node& node) {
        const auto dbg = parameters.debug && parameters.report_pos && node.aa_transitions_.has(*parameters.report_pos);
        AD_DEBUG_IF(dbg, "update_aa_transitions_derek_2016 (add left) {:4.3s} aa-transitions: {}  empty:{}", node.node_id,
                    node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes), node.aa_transitions_.empty());
        if (!node.aa_transitions_.empty()) {
            const auto node_left_edge = node.cumulative_edge_length - node.edge_length;

            const auto node_for_left = std::upper_bound(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), node_left_edge,
                                                        [](const auto& a_node_left_edge, const auto* nd) { return a_node_left_edge > nd->cumulative_edge_length; });
            // const auto node_for_left = std::find_if(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), [node_left_edge](const auto* nd) { return nd->cumulative_edge_length <
            // node_left_edge; });
            if (node_for_left != std::end(sorted_leaf_nodes)) {
                node.aa_transitions_.set_left((*node_for_left)->aa_sequence);
                node.node_for_left_aa_transitions_ = *node_for_left;
                AD_DEBUG_IF(dbg, "update_aa_transitions_derek_2016 (add left) {:4.3s} {} node-for-left: {} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes), node.node_for_left_aa_transitions_->node_id,
                            node.node_for_left_aa_transitions_->seq_id);
            }
            else {
                AD_DEBUG_IF(dbg, "update_aa_transitions_derek_2016 (add left) no node for left {:4.3s} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes));
            }
        }

        node.aa_transitions_.remove_left_right_same(parameters, node);
        node.aa_transitions_.remove_empty_right();
    };
    tree::iterate_leaf_pre(tree, add_left_part, add_left_part);

} // acmacs::tal::v3::update_aa_transitions_derek_2016

// ======================================================================
// Eu 2020-09-09

void acmacs::tal::v3::update_aa_transitions_eu_20200909(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG_IF(parameters.debug, "eu-20200909");

    const auto longest_sequence = update_common_aa(tree);
    const auto& root_sequence = tree.find_first_leaf().aa_sequence;

    const auto is_not_common_with_tolerance = [](const Node& node, seqdb::pos0_t pos, double tolerance) -> bool {
        const auto aa = node.common_aa_.at(pos, tolerance);
        if (aa == CommonAA::NoCommon)
            return true;
        // tolerance problem: aa is common with tolerance but in
        // reality just 1 or 2 child nodes have this aa and other
        // children (with much fewer leaves) have different
        // aa's. In that case consider that aa to be not
        // common. See H3 and M346L labelling in the 3a clade.
        const auto number_of_children_with_the_same_common_aa =
            std::count_if(std::begin(node.subtree), std::end(node.subtree), [aa, pos, tolerance](const Node& child) { return child.common_aa_.at(pos, tolerance) == aa; });
        return number_of_children_with_the_same_common_aa > 0 && number_of_children_with_the_same_common_aa <= 1 &&
               node.subtree.size() > static_cast<size_t>(number_of_children_with_the_same_common_aa);
    };

    tree::iterate_post(tree, [longest_sequence, is_not_common_with_tolerance, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
            if (is_not_common_with_tolerance(node, pos, non_common_tolerance)) {
                AD_DEBUG_IF(dbg, "eu-20200909 {} node:{:4.3s} leaves:{:4d} {}", pos, node.node_id, node.number_leaves_in_subtree(),
                            node.common_aa_.size() > *pos ? node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})") : std::string{});
                for (auto& child : node.subtree) {
                    if (!child.is_leaf()) {
                        if (!is_not_common_with_tolerance(child, pos, non_common_tolerance)) {
                            child.replace_aa_transition(pos, child.common_aa_.at(pos, non_common_tolerance));
                            // AD_DEBUG("replace_aa_transition {} {} {} --> {}", child.node_id, pos, aa, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                        }
                    }
                }
            }
            else {
                if (node.number_leaves_in_subtree() > 1000 && dbg) {
                    AD_DEBUG_IF(dbg, "eu-20200909 common {} node:{:4.3s} leaves:{:4d} {}", pos, node.node_id, node.number_leaves_in_subtree(),
                                node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                    for (const auto& child : node.subtree) {
                        if (child.is_leaf()) {
                            AD_DEBUG_IF(dbg, "    {:4.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
                        }
                        else {
                            AD_DEBUG_IF(dbg, "    {:4.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
                                        child.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                        }
                    }
                }
            }
        }
    });

    // add to top children of the tree differences between them and root, even if that subtree has common aa (with tolerance)
    for (auto& top_child : tree.subtree) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            if (const auto aa = top_child.common_aa_.at(pos, parameters.non_common_tolerance_for(pos)); aa != CommonAA::NoCommon && aa != root_sequence.at(pos))
                top_child.replace_aa_transition(pos, aa);
        }
    }

    tree::iterate_post(tree, [&root_sequence, &parameters](Node& node) {
        node.aa_transitions_.set_left(root_sequence);
        // if (!node.aa_transitions_.empty())
        //     AD_DEBUG("set_left {} {}", node.node_id, node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
        node.aa_transitions_.remove_left_right_same(parameters, node);
    });

    if (!tree.aa_transitions_.empty())
        AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

} // acmacs::tal::v3::update_aa_transitions_eu_20200909

// ======================================================================
//  Eu 2020-09-15

void acmacs::tal::v3::set_aa_transitions_eu_20200915(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters)
{
    // const auto total_leaves = tree.number_leaves_in_subtree();

    const auto is_not_common_with_tolerance = [](const Node& node, seqdb::pos0_t pos, double tolerance, auto dbg) -> std::pair<bool, std::string> {
        const auto aa = node.common_aa_.at(pos, tolerance);
        std::string msg;
        if (aa == CommonAA::NoCommon) {
            if (dbg)
                msg = "common:no";
            return {true, msg};
        }
        // tolerance problem: aa is common with tolerance but in
        // reality just 1 or 2 child nodes have this aa and other
        // children (with much fewer leaves) have different
        // aa's. In that case consider that aa to be not
        // common. See H3 and M346L labelling in the 3a clade.
        std::string common_children;
        const auto number_of_children_with_the_same_common_aa = std::count_if(std::begin(node.subtree), std::end(node.subtree), [aa, pos, tolerance, &common_children, dbg](const Node& child) {
            if (child.is_leaf()) {
                return child.aa_sequence.at(pos) == aa;
            }
            else {
                if (child.common_aa_.at(pos, tolerance, dbg) == aa) {
                    common_children += fmt::format(" {}", child.node_id);
                    return true;
                }
                else
                    return false;
            }
        });
        const auto result = number_of_children_with_the_same_common_aa > 0 && number_of_children_with_the_same_common_aa <= 1 &&
               node.subtree.size() > static_cast<size_t>(number_of_children_with_the_same_common_aa);
        if (dbg)
            msg = fmt::format("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !result, pos, node.node_id, aa, tolerance,
                              number_of_children_with_the_same_common_aa, common_children, node.subtree.size());
        return {result, msg};
    };

    tree::iterate_post(tree, [longest_sequence, is_not_common_with_tolerance, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
            AD_DEBUG_IF(dbg, "eu-20200915 @{} node:{:5.3s} leaves:{:4d}", pos, node.node_id, node.number_leaves_in_subtree());
            if (const auto [not_common, msg] = is_not_common_with_tolerance(node, pos, non_common_tolerance, dbg); not_common) {
                AD_DEBUG_IF(dbg, "            <not-common> {}", node.common_aa_.size() > *pos ? node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})") : std::string{});
                AD_DEBUG_IF(dbg, "                [is_not_common_with_tolerance]: {}", msg);
                for (auto& child : node.subtree) {
                    if (!child.is_leaf()) {
                        if (const auto [not_common_child, msg_child] = is_not_common_with_tolerance(child, pos, non_common_tolerance, dbg); !not_common_child) {
                            const auto aa = child.common_aa_.at(pos, non_common_tolerance);
                            child.aa_transitions_.add(pos, aa);
                            AD_DEBUG_IF(dbg, "                <child> add {}{} node:{:5.3s} leaves:{:4d} --> {}", pos, aa, child.node_id, child.number_leaves_in_subtree(), child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG_IF(dbg, "                    [is_not_common_with_tolerance]: {}", msg_child);
                        }
                        else {
                            AD_DEBUG_IF(dbg, "                <child> not-common [is_not_common_with_tolerance]: {}", msg_child);
                        }
                    }
                }
            }
            else {
                const auto node_aa = node.common_aa_.at(pos, non_common_tolerance);
                AD_DEBUG_IF(dbg, "            <common> {}", node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                AD_DEBUG_IF(dbg, "                [is_not_common_with_tolerance]: {}", msg);
                for (auto& child : node.subtree) {
                    if (child.is_leaf()) {
                        // AD_DEBUG_IF(dbg, "eu-20200915    {:5.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
                    }
                    else if (!child.common_aa_.empty()) {
                        const auto child_aa = child.common_aa_.at(pos, non_common_tolerance);
                        const auto counter = child.common_aa_.counter(pos);
                        AD_DEBUG_IF(dbg, "                {:5.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
                                    counter.report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                        if (const auto [not_common_child, msg_child] = is_not_common_with_tolerance(child, pos, non_common_tolerance, dbg); child_aa != node_aa && !not_common_child) {
                            child.replace_aa_transition(pos, child_aa);
                            AD_DEBUG_IF(dbg, "                <child> <replaced> {:5.3s} {:3d}{} --> {}", child.node_id, pos, child_aa, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG_IF(dbg, "                    [is_not_common_with_tolerance]: {}", msg);
                        }
                    }
                }
            }
        }
    });

} // acmacs::tal::v3::set_aa_transitions_eu_20200915

// ======================================================================
//  Eu 2021-02-05

void acmacs::tal::v3::set_aa_transitions_eu_20210205(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters)
{
    const auto number_of_children_with_the_same_common_aa = [](const Node& node, char aa, seqdb::pos0_t pos, double tolerance, bool dbg) {
        std::string common_children;
        size_t num{0};
        for (const auto& child : node.subtree) {
            if (child.is_leaf()) {
                if (child.aa_sequence.at(pos) == aa)
                    ++num;
            }
            else {
                if (child.common_aa_.at(pos, tolerance, dbg) == aa) {
                    if (dbg)
                        common_children += fmt::format(" {}", child.node_id);
                    ++num;
                }
            }
        }
        return std::make_pair(num, common_children);
    };

    const auto is_common_with_tolerance = [number_of_children_with_the_same_common_aa](const Node& node, seqdb::pos0_t pos, double tolerance, auto dbg) -> std::pair<bool, std::string> {
        std::string msg;
        const auto aa = node.common_aa_.at(pos, tolerance);
        if (aa == CommonAA::NoCommon) {
            if (dbg)
                msg = "common:no";
            return {false, msg};
        }
        // tolerance problem: aa is common with tolerance but in
        // reality just 1 or 2 child nodes have this aa and other
        // children (with much fewer leaves) have different
        // aa's. In that case consider that aa to be not
        // common. See H3 and M346L labelling in the 3a clade.
        const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa(node, aa, pos, tolerance, dbg);
        const auto not_common = // num_common_aa_children > 0 && number_of_children_with_the_same_common_aa <= 1 &&
            node.subtree.size() > static_cast<size_t>(num_common_aa_children);
        if (dbg)
            msg = fmt::format("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !not_common, pos, node.node_id, aa, tolerance,
                              num_common_aa_children, common_children, node.subtree.size());
        return {!not_common, msg};
    };

    const auto is_common_with_tolerance_for_child = [number_of_children_with_the_same_common_aa](const Node& node, seqdb::pos0_t pos, double tolerance, auto dbg) -> std::pair<bool, std::string> {
        std::string msg;
        const auto aa = node.common_aa_.at(pos, tolerance);
        if (aa == CommonAA::NoCommon) {
            if (dbg)
                msg = "common:no";
            return {false, msg};
        }
        else {
            const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa(node, aa, pos, tolerance, dbg);
            const auto not_common = num_common_aa_children <= 1; // && node.subtree.size() > static_cast<size_t>(num_common_aa_children);
            if (dbg)
                msg = fmt::format("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !not_common, pos, node.node_id, aa, tolerance,
                                  num_common_aa_children, common_children, node.subtree.size());
            return {!not_common, msg};
        }
    };

    tree::iterate_post(tree, [longest_sequence, is_common_with_tolerance, is_common_with_tolerance_for_child, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
            AD_DEBUG_IF(dbg, "eu-20200915 @{} node:{:5.3s} leaves:{:4d}", pos, node.node_id, node.number_leaves_in_subtree());
            if (const auto [common, msg] = is_common_with_tolerance(node, pos, non_common_tolerance, dbg); !common) {
                AD_DEBUG_IF(dbg, "            <not-common> {}",
                            node.common_aa_.size() > *pos ? node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})") : std::string{});
                AD_DEBUG_IF(dbg, "                [is_common_with_tolerance]: {}", msg);
                for (auto& child : node.subtree) {
                    if (!child.is_leaf()) {
                        if (const auto [common_child, msg_child] = is_common_with_tolerance_for_child(child, pos, non_common_tolerance, dbg); common_child) {
                            const auto aa = child.common_aa_.at(pos, non_common_tolerance);
                            child.aa_transitions_.add(pos, aa);
                            AD_DEBUG_IF(dbg, "                <child> add {}{} node:{:5.3s} leaves:{:4d}", pos, aa, child.node_id,
                                        child.number_leaves_in_subtree()); //, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG_IF(dbg, "                    [is_common_with_tolerance_for_child]: {}", msg_child);
                        }
                        else {
                            AD_DEBUG_IF(dbg, "                <child> not-common [is_common_with_tolerance_for_child]: {}", msg_child);
                        }
                    }
                }
            }
            else {
                const auto node_aa = node.common_aa_.at(pos, non_common_tolerance);
                AD_DEBUG_IF(dbg, "            <common> {}", node.common_aa_.counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                AD_DEBUG_IF(dbg, "                [is_common_with_tolerance]: {}", msg);
                for (auto& child : node.subtree) {
                    if (child.is_leaf()) {
                        // AD_DEBUG_IF(dbg, "eu-20200915    {:5.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
                    }
                    else if (!child.common_aa_.empty()) {
                        const auto child_aa = child.common_aa_.at(pos, non_common_tolerance);
                        const auto counter = child.common_aa_.counter(pos);
                        AD_DEBUG_IF(dbg, "                {:5.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
                                    counter.report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
                        if (const auto [common_child, msg_child] = is_common_with_tolerance_for_child(child, pos, non_common_tolerance, dbg); /* child_aa != node_aa && */ common_child) {
                            child.replace_aa_transition(pos, child_aa);
                            AD_DEBUG_IF(dbg, "                <child> <replaced> {:5.3s} {:3d}{}", child.node_id, pos,
                                        child_aa); //, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG_IF(dbg, "                    [is_common_with_tolerance_for_child]: {}", msg);
                        }
                    }
                }
            }
        }
    });

    if (parameters.debug && parameters.report_pos) {
        AD_DEBUG("eu-20200915 added aa transitions =============================================================");
        size_t offset = 0;
        tree::iterate_pre_post(
            tree,
            [&parameters, &offset](const Node& node) mutable {
                ++offset;
                if (const auto* aatr = node.aa_transitions_.find(*parameters.report_pos); aatr) {
                    AD_DEBUG("  {:{}s}{:5.3} {}", "", offset * 2, node.node_id, aatr->display());
                }
            },
            [&offset](const Node& node) { --offset; });
    }

} // acmacs::tal::v3::set_aa_transitions_eu_20210205

// ----------------------------------------------------------------------

void acmacs::tal::v3::update_aa_transitions_eu_20200915(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    const auto leaves_ratio_threshold = 0.005;

    const auto longest_sequence = update_common_aa(tree);
    AD_DEBUG_IF(parameters.debug, "eu-20200915 set aa transitions =============================================================");
    set_aa_transitions_eu_20210205(tree, longest_sequence, parameters);
    // set_aa_transitions_eu_20200915(tree, longest_sequence, parameters);
    AD_DEBUG_IF(parameters.debug, "eu-20200915 update aa transitions ================================================================================");
    const auto& root_sequence = tree.find_first_leaf().aa_sequence;
    // const auto total_leaves = tree.number_leaves_in_subtree();

    for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
        const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;

        // add to top children of the tree differences between them and root, even if that subtree has common aa (with tolerance)
        // const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
        // for (auto& top_child : tree.subtree) {
        //     if (const auto aa = top_child.common_aa_.at(pos, non_common_tolerance); aa != CommonAA::NoCommon && aa != root_sequence.at(pos))
        //         top_child.replace_aa_transition(pos, aa);
        // }

        struct flips_leaves_t
        {
            const AA_Transitions& transitions;
            size_t flips{0};
            size_t leaves{0};
            acmacs::Counter<ssize_t> flip_distances;

            flips_leaves_t(const AA_Transitions& tr) : transitions{tr} {}

            void add(size_t number_of_leaves, ssize_t distance)
            {
                ++flips;
                leaves += number_of_leaves;
                flip_distances.count(distance);
            }
        };

        for (bool repeat{true}; repeat;) {
            repeat = false;
            std::vector<flips_leaves_t> transitions_stack;
            tree::iterate_pre_post(
                tree,
                // pre
                [&root_sequence, &transitions_stack, pos, dbg](Node& node) {
                    // AD_DEBUG_IF(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** pre   {:5.3} {} [{}]", node.node_id, node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
                    if (AA_Transition* this_transition = node.aa_transitions_.find(pos); this_transition) {
                        const auto prev = std::find_if(transitions_stack.rbegin(), transitions_stack.rend(), [pos](const auto& en) { return en.transitions.find(pos) != nullptr; });
                        if (prev != transitions_stack.rend()) {
                            const auto& prev_trans = *prev->transitions.find(pos);
                            this_transition->left = prev_trans.right;
                            if (this_transition->left != this_transition->right && this_transition->right == prev_trans.left) {
                                prev->add(node.number_leaves_in_subtree(), prev - transitions_stack.rbegin());
                            }
                        }
                        else
                            this_transition->left = root_sequence.at(pos);
                        if (this_transition->left != this_transition->right) {
                            AD_DEBUG_IF(dbg, "eu-20200915 {}{:3d}{} {:5.3} leaves:{:5d} transition {}", this_transition->left, pos, this_transition->right, node.node_id,
                                        node.number_leaves_in_subtree(), *this_transition);
                        }
                        // else {
                        //     AD_DEBUG_IF(dbg, "eu-20200915 same-left-right {}{:3d}{} {:5.3} leaves:{:5d} transition {}", this_transition->left, pos, this_transition->right, node.node_id,
                        //                 node.number_leaves_in_subtree(), *this_transition);
                        // }
                    }
                    transitions_stack.emplace_back(node.aa_transitions_);
                },
                // post
                [&transitions_stack, &repeat, pos, leaves_ratio_threshold, dbg](Node& node) {
                    // AD_DEBUG_IF(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** post1 {:5.3} {} [{}]", node.node_id, node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
                    if (const auto& fl = transitions_stack.back(); fl.flips) {
                        const auto& min_flip_distance = fl.flip_distances.min_value();
                        const auto node_leaves = node.number_leaves_in_subtree();
                        const auto leaves_ratio = static_cast<double>(fl.leaves) / static_cast<double>(node_leaves);
                        if (min_flip_distance.first < 3 && leaves_ratio > leaves_ratio_threshold) {
                            AD_DEBUG_IF(dbg, "eu-20200915 remove flips_in_children {:3d} {:5.3} leaves:{:5d} children:{:3d}    flips:{:3d}  leaves:{:5d} ({:4.1f}%)  min_flip_distance:{} num:{}", pos,
                                        node.node_id, node_leaves, node.subtree.size(), fl.flips, fl.leaves, leaves_ratio * 100.0, min_flip_distance.first, min_flip_distance.second);
                            node.aa_transitions_.remove(pos);
                            repeat = true;
                        }
                        else {
                            AD_DEBUG_IF(dbg, "eu-20200915 keep flips_in_children {:3d} {:5.3} leaves:{:5d} children:{:3d}    flips:{:3d}  leaves:{:5d} ({:4.1f}%)  min_flip_distance:{} num:{}", pos,
                                        node.node_id, node_leaves, node.subtree.size(), fl.flips, fl.leaves, leaves_ratio * 100.0, min_flip_distance.first, min_flip_distance.second);
                        }
                    }
                    // AD_DEBUG_IF(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** post2 {:5.3} {} [{}]", node.node_id, node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
                    transitions_stack.pop_back();
                });
        }
    }

    tree::iterate_pre(tree, [&parameters](Node& node) { node.aa_transitions_.remove_left_right_same(parameters, node); });

    if (!tree.aa_transitions_.empty())
        AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

} // acmacs::tal::v3::update_aa_transitions_eu_20200915

// ----------------------------------------------------------------------

void acmacs::tal::v3::report_aa_transitions(const Node& root, const draw_tree::AATransitionsParameters& parameters)
{
    report_common_aa(root, parameters.report_pos, parameters.report_number_leaves_threshold);
    AD_INFO("AA transitions");
    root.number_leaves_in_subtree();
    tree::iterate_pre(root, [&parameters](const Node& node) {
        if (node.number_leaves_in_subtree() >= parameters.report_number_leaves_threshold) {
            if (const auto rep = node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes); !rep.empty())
                fmt::print(stderr, "   {:5.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::report_aa_transitions

// ======================================================================
// CommonAA
// ======================================================================

void acmacs::tal::v3::CommonAA::update(acmacs::seqdb::sequence_aligned_ref_t seq)
{
    resize(*seq.size());
    for (seqdb::pos0_t pos{0}; pos < seq.size(); ++pos) {
        if (const auto aa = seq.at(pos); aa != Any)
            at_pos_[*pos].count(aa);
    }

} // acmacs::tal::v3::CommonAA::update

// ----------------------------------------------------------------------

void acmacs::tal::v3::CommonAA::update(const CommonAA& subtree)
{
    if (at_pos_.empty()) {
        at_pos_ = subtree.at_pos_;
    }
    else {
        resize(subtree.at_pos_.size());
        // AD_DEBUG("CommonAA::update size:{} subtree.at_pos_.size:{}", size(), subtree.at_pos_.size());
        for (size_t pos{0}; pos < std::min(at_pos_.size(), subtree.at_pos_.size()); ++pos)
            at_pos_[pos].update(subtree.at_pos_[pos]);
    }

} // acmacs::tal::v3::CommonAA::update

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::CommonAA::report() const
{
    throw std::runtime_error{"CommonAA::report() not implemented"};

} // acmacs::tal::v3::CommonAA::report

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::CommonAA::report(const CommonAA& /*parent*/, std::optional<seqdb::pos1_t> /*pos_to_report*/) const
{
    throw std::runtime_error{"CommonAA::report(parent) not implemented"};

} // acmacs::tal::v3::CommonAA::report

// ======================================================================
// CommonAA
// ======================================================================

// void acmacs::tal::v3::CommonAA::update(seqdb::pos0_t pos, char aa)
// {
//     if (aa != Any) {
//         if (at(pos) == Any)
//             set(pos, aa);
//         else if (at(pos) != aa)
//             set_to_no_common(pos);
//     }

// } // acmacs::tal::v3::CommonAA::update

// // ----------------------------------------------------------------------

// void acmacs::tal::v3::CommonAA::update(acmacs::seqdb::sequence_aligned_ref_t seq)
// {
//     if (empty()) {
//         get().assign(*seq);
//     }
//     else {
//         if (seq.size() < size())
//             resize(seq.size());
//         for (seqdb::pos0_t pos{0}; pos < size(); ++pos)
//             update(pos, seq.at(pos));
//     }

// } // acmacs::tal::v3::CommonAA::update

// // ----------------------------------------------------------------------

// void acmacs::tal::v3::CommonAA::update(const CommonAA& subtree)
// {
//     if (empty()) {
//         *this = subtree;
//     }
//     else {
//         if (subtree.size() < size())
//             resize(subtree.size());
//         for (seqdb::pos0_t pos{0}; pos < size(); ++pos)
//             update(pos, subtree.at(pos));
//     }

// } // acmacs::tal::v3::CommonAA::update

// // ----------------------------------------------------------------------

// std::string acmacs::tal::v3::CommonAA::report() const
// {
//     fmt::memory_buffer out;
//     for (seqdb::pos0_t pos{0}; pos < size(); ++pos) {
//         if (is_common(pos))
//             fmt::format_to(out, " {}{}", pos, at(pos));
//     }
//     return fmt::format("common:{} {}", num_common(), fmt::to_string(out));

// } // acmacs::tal::v3::CommonAA::report

// // ----------------------------------------------------------------------

// std::string acmacs::tal::v3::CommonAA::report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report) const
// {
//     fmt::memory_buffer out;
//     size_t num_common = 0;
//     for (seqdb::pos0_t pos{0}; pos < size(); ++pos) {
//         if (is_common(pos) && !parent.is_common(pos) && (!pos_to_report || pos_to_report == pos)) {
//             fmt::format_to(out, " {}{}", pos, at(pos));
//             ++num_common;
//         }
//     }
//     if (num_common == 0)
//         return std::string{};
//     else
//         return fmt::format("common:{} {}", num_common, fmt::to_string(out));

// } // acmacs::tal::v3::CommonAA::report

// ======================================================================

void acmacs::tal::v3::AA_Transitions::remove_left_right_same(const draw_tree::AATransitionsParameters& parameters, const Node& node)
{
    // AD_DEBUG_IF(!data_.empty(), "remove_left_right_same {}", display());
    remove_if([&parameters /*, &node */](const auto& en) {
        // const auto dbg = en.left_right_same() && parameters.debug && parameters.report_pos && en.pos == *parameters.report_pos;
        // AD_DEBUG_IF(dbg, "remove_left_right_same {:5.3} {}", node.node_id, en.display());
        return en.left_right_same() && (!parameters.show_same_left_right_for_pos || en.pos != *parameters.show_same_left_right_for_pos);
    });
    // AD_DEBUG_IF(!data_.empty(), "  --> {}", display());

} // acmacs::tal::v3::AA_Transitions::remove_left_right_same

// ----------------------------------------------------------------------

void acmacs::tal::v3::AA_Transitions::add_or_replace(const AA_Transition& to_add)
{
    data_.erase(std::remove_if(std::begin(data_), std::end(data_), [&to_add](const auto& en) { return en.pos == to_add.pos; }), std::end(data_));
    data_.push_back(to_add);

} // acmacs::tal::v3::AA_Transitions::add_or_replace

// ----------------------------------------------------------------------

void acmacs::tal::v3::AA_Transitions::add_or_replace(const AA_Transitions& transitions)
{
    for (const auto& transition : transitions.data_)
        add_or_replace(transition);

} // acmacs::tal::v3::AA_Transitions::add_or_replace

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::AA_Transitions::display(std::optional<seqdb::pos1_t> pos1, show_empty_left sel) const
{
    if (data_.empty())
        return {};
    std::vector<const AA_Transition*> res;
    for (const auto& en : data_) {
        if ((sel == show_empty_left::yes || !en.empty_left()) && !en.empty_right() && (!pos1 || *pos1 == en.pos))
            res.push_back(&en);
    }
    return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat->display(); });

} // acmacs::tal::v3::AA_Transitions::display

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::AA_Transitions::display_most_important(size_t num) const
{
    if (data_.empty())
        return {};
    std::vector<const AA_Transition*> res;
    for (const auto& en : data_) {
        if (!en.empty_left() && !en.empty_right())
            res.push_back(&en);
    }
    if (num == 0 || res.size() < num)
        return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat->display(); });

    // HA recepter binding domain is 63 - 286 https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3020035/
    auto to_remove{res.size() - num};
    for (auto resp = res.begin(); resp != res.end() && to_remove > 0; ++resp) { // removing entries that are out of recepter binding domain
        if ((**resp).pos < seqdb::pos1_t{63} || (**resp).pos > seqdb::pos1_t{286}) {
            *resp = nullptr;
            --to_remove;
        }
    }
    for (auto resp = res.begin(); resp != res.end() && to_remove > 0; ++resp) { // removing initial (and not removed before) entries
        if (*resp) {
            *resp = nullptr;
            --to_remove;
        }
    }
    return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat ? aat->display() : std::string{}; });

} // acmacs::tal::v3::AA_Transitions::display_most_important

// ----------------------------------------------------------------------

bool acmacs::tal::v3::AA_Transitions::has(seqdb::pos1_t pos) const
{
    for (const auto& en : data_) {
        if (en.pos == pos && !en.empty_right())
            return true;
    }
    return false;

} // acmacs::tal::v3::AA_Transitions::has

// ----------------------------------------------------------------------

std::vector<std::string> acmacs::tal::v3::AA_Transitions::names(const std::vector<acmacs::seqdb::pos1_t>& selected_pos, std::string_view overwrite) const
{
    std::vector<std::string> result;
    if (overwrite.empty()) {
        for (const auto& en : data_) {
            if (!en.empty_left() && (selected_pos.empty() || en.pos_is_in(selected_pos)))
                result.push_back(en.display());
        }
    }
    else {
        const auto res = acmacs::string::split(overwrite, " ", acmacs::string::Split::StripRemoveEmpty);
        result.resize(res.size());
        std::transform(std::begin(res), std::end(res), std::begin(result), [](std::string_view src) { return std::string{src}; });
    }
    return result;

} // acmacs::tal::v3::AA_Transitions::names

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
