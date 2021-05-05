#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3::detail
{
    static void set_aa_transitions_eu_20210205(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters);

    // static void update_aa_transitions_eu_20200909(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
    // static void set_aa_transitions_eu_20200915(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters);

    // ----------------------------------------------------------------------

    template <bool dbg = false> inline std::pair<size_t, std::string> number_of_children_with_the_same_common_aa(const Node& node, char aa, seqdb::pos0_t pos, double tolerance)
    {
        fmt::memory_buffer common_children;
        size_t num{0};
        for (const auto& child : node.subtree) {
            if (child.is_leaf()) {
                if (child.aa_sequence.at(pos) == aa)
                    ++num;
            }
            else {
                if (child.common_aa_->at<dbg>(pos, tolerance) == aa) {
                    if constexpr (dbg)
                        fmt::format_to(common_children, " {}", child.node_id);
                    ++num;
                }
            }
        }
        return std::make_pair(num, fmt::to_string(common_children));
    }

    template <bool dbg = false> inline std::pair<bool, std::string> is_common_with_tolerance(const Node& node, seqdb::pos0_t pos, double tolerance)
    {
        fmt::memory_buffer msg;
        const auto aa = node.common_aa_->at(pos, tolerance);
        if (aa == NoCommon) {
            if constexpr (dbg)
                fmt::format_to(msg, "common:no");
            return {false, fmt::to_string(msg)};
        }
        // tolerance problem: aa is common with tolerance but in
        // reality just 1 or 2 child nodes have this aa and other
        // children (with much fewer leaves) have different
        // aa's. In that case consider that aa to be not
        // common. See H3 and M346L labelling in the 3a clade.
        const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa<dbg>(node, aa, pos, tolerance);
        const auto not_common = // num_common_aa_children > 0 && number_of_children_with_the_same_common_aa <= 1 &&
            node.subtree.size() > static_cast<size_t>(num_common_aa_children);
        if constexpr (dbg) {
            fmt::format_to(msg, "common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !not_common, pos, node.node_id, aa, tolerance,
                           num_common_aa_children, common_children, node.subtree.size());
        }
        return {!not_common, fmt::to_string(msg)};
    }

    template <bool dbg = false> inline std::pair<bool, std::string> is_common_with_tolerance_for_child(const Node& node, seqdb::pos0_t pos, double tolerance)
    {
        fmt::memory_buffer msg;
        const auto aa = node.common_aa_->at(pos, tolerance);
        if (aa == NoCommon) {
            if constexpr (dbg)
                fmt::format_to(msg, "common:no");
            return {false, fmt::to_string(msg)};
        }
        else {
            const auto [num_common_aa_children, common_children] = number_of_children_with_the_same_common_aa<dbg>(node, aa, pos, tolerance);
            const auto not_common = num_common_aa_children <= 1; // && node.subtree.size() > static_cast<size_t>(num_common_aa_children);
            if constexpr (dbg) {
                fmt::format_to(msg, "common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !not_common, pos, node.node_id, aa, tolerance,
                               num_common_aa_children, common_children, node.subtree.size());
            }
            return {!not_common, fmt::to_string(msg)};
        }
    }

    template <bool dbg> inline void set_aa_transitions_for_pos_eu_20210205(Node& node, seqdb::pos0_t pos, double non_common_tolerance)
    {
        if constexpr (dbg)
            AD_DEBUG("eu-20200915 @{} node:{:5.3s} leaves:{:4d}", pos, node.node_id, node.number_leaves_in_subtree());
        if (const auto [common, msg] = is_common_with_tolerance<dbg>(node, pos, non_common_tolerance); !common) {
            if constexpr (dbg) {
                AD_DEBUG("            <not-common> {}", node.common_aa_->report_sorted_max_first(pos, " {value}:{counter_percent:.1f}%({counter})"));
                AD_DEBUG("                [is_common_with_tolerance]: {}", msg);
            }
            for (auto& child : node.subtree) {
                if (!child.is_leaf()) {
                    if (const auto [common_child, msg_child] = is_common_with_tolerance_for_child<dbg>(child, pos, non_common_tolerance); common_child) {
                        const auto aa = child.common_aa_->at(pos, non_common_tolerance);
                        child.aa_transitions_.add(pos, aa);
                        if constexpr (dbg) {
                            AD_DEBUG("                <child> add {}{} node:{:5.3s} leaves:{:4d}", pos, aa, child.node_id,
                                     child.number_leaves_in_subtree()); //, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG("                    [is_common_with_tolerance_for_child]: {}", msg_child);
                        }
                    }
                    else if constexpr (dbg) {
                        AD_DEBUG("                <child> not-common [is_common_with_tolerance_for_child]: {}", msg_child);
                    }
                }
            }
        }
        else {
            if constexpr (dbg) {
                // const auto node_aa = node.common_aa_->at(pos, non_common_tolerance);
                AD_DEBUG("            <common> {}", node.common_aa_->report_sorted_max_first(pos, " {value}:{counter_percent:.1f}%({counter})"));
                AD_DEBUG("                [is_common_with_tolerance]: {}", msg);
            }
            for (auto& child : node.subtree) {
                if (child.is_leaf()) {
                    if constexpr (dbg)
                        AD_DEBUG("eu-20200915    {:5.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
                }
                else if (!child.common_aa_->empty(pos)) {
                    const auto child_aa = child.common_aa_->at(pos, non_common_tolerance);
                    if constexpr (dbg) {
                        AD_DEBUG("                {:5.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
                                 child.common_aa_->report_sorted_max_first(pos, " {value}:{counter_percent:.1f}%({counter})"));
                    }
                    if (const auto [common_child, msg_child] = is_common_with_tolerance_for_child(child, pos, non_common_tolerance /*, dbg*/); /* child_aa != node_aa && */ common_child) {
                        child.replace_aa_transition(pos, child_aa);
                        if constexpr (dbg) {
                            AD_DEBUG("                <child> <replaced> {:5.3s} {:3d}{}", child.node_id, pos,
                                     child_aa); //, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
                            AD_DEBUG("                    [is_common_with_tolerance_for_child]: {}", msg);
                        }
                    }
                }
            }
        }
    }

}

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::update_aa_transitions_eu_20200915(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    const auto leaves_ratio_threshold = 0.005;

    const auto longest_sequence = tree.longest_aa_sequence();
    update_common_aa(tree, longest_sequence);

    AD_DEBUG(parameters.debug, "eu-20200915 set aa transitions =============================================================");
    set_aa_transitions_eu_20210205(tree, longest_sequence, parameters);
    AD_DEBUG(parameters.debug, "eu-20200915 update aa transitions ================================================================================");
    const auto& root_sequence = tree.find_first_leaf().aa_sequence;
    // const auto total_leaves = tree.number_leaves_in_subtree();

    // AD_DEBUG("update aa transitions");
    // const Timeit ti{"update aa transitions"};
    for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
        const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;

        // add to top children of the tree differences between them and root, even if that subtree has common aa (with tolerance)
        // const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
        // for (auto& top_child : tree.subtree) {
        //     if (const auto aa = top_child.common_aa_->at(pos, non_common_tolerance); aa != NoCommon && aa != root_sequence.at(pos))
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
                    // AD_DEBUG(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** pre   {:5.3} {} [{}]", node.node_id,
                    // node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
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
                            AD_DEBUG(dbg, "eu-20200915 {}{:3d}{} {:5.3} leaves:{:5d} transition {}", this_transition->left, pos, this_transition->right, node.node_id, node.number_leaves_in_subtree(),
                                     *this_transition);
                        }
                        // else {
                        //     AD_DEBUG(dbg, "eu-20200915 same-left-right {}{:3d}{} {:5.3} leaves:{:5d} transition {}", this_transition->left, pos, this_transition->right, node.node_id,
                        //                 node.number_leaves_in_subtree(), *this_transition);
                        // }
                    }
                    transitions_stack.emplace_back(node.aa_transitions_);
                },
                // post
                [&transitions_stack, &repeat, pos, leaves_ratio_threshold, dbg](Node& node) {
                    // AD_DEBUG(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** post1 {:5.3} {} [{}]", node.node_id,
                    // node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
                    if (const auto& fl = transitions_stack.back(); fl.flips) {
                        const auto& min_flip_distance = fl.flip_distances.min_value();
                        const auto node_leaves = node.number_leaves_in_subtree();
                        const auto leaves_ratio = static_cast<double>(fl.leaves) / static_cast<double>(node_leaves);
                        if (min_flip_distance.first < 3 && leaves_ratio > leaves_ratio_threshold) {
                            AD_DEBUG(dbg, "eu-20200915 remove flips_in_children {:3d} {:5.3} leaves:{:5d} children:{:3d}    flips:{:3d}  leaves:{:5d} ({:4.1f}%)  min_flip_distance:{} num:{}", pos,
                                     node.node_id, node_leaves, node.subtree.size(), fl.flips, fl.leaves, leaves_ratio * 100.0, min_flip_distance.first, min_flip_distance.second);
                            node.aa_transitions_.remove(pos);
                            repeat = true;
                        }
                        else {
                            AD_DEBUG(dbg, "eu-20200915 keep flips_in_children {:3d} {:5.3} leaves:{:5d} children:{:3d}    flips:{:3d}  leaves:{:5d} ({:4.1f}%)  min_flip_distance:{} num:{}", pos,
                                     node.node_id, node_leaves, node.subtree.size(), fl.flips, fl.leaves, leaves_ratio * 100.0, min_flip_distance.first, min_flip_distance.second);
                        }
                    }
                    // AD_DEBUG(node.node_id.vertical == 4756 && pos >= seqdb::pos1_t{159} && pos <= seqdb::pos1_t{161}, "** post2 {:5.3} {} [{}]", node.node_id,
                    // node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes), pos);
                    transitions_stack.pop_back();
                });
        }
    }

    tree::iterate_pre(tree, [&parameters](Node& node) { node.aa_transitions_.remove_left_right_same(parameters, node); });

    if (!tree.aa_transitions_.empty())
        AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

} // acmacs::tal::v3::detail::update_aa_transitions_eu_20200915

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::set_aa_transitions_eu_20210205(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters)
{
    size_t nodes_processed{0};
    auto start = acmacs::timestamp();
    tree::iterate_post(tree, [longest_sequence, &parameters, &nodes_processed, &start](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
            if (dbg)
                set_aa_transitions_for_pos_eu_20210205<true>(node, pos, non_common_tolerance);
            else
                set_aa_transitions_for_pos_eu_20210205<false>(node, pos, non_common_tolerance);
        }
        ++nodes_processed;
        if ((nodes_processed % 10000) == 0) {
            AD_DEBUG("nodes_processed: {}      last chunk: {}", nodes_processed, acmacs::format_duration(acmacs::elapsed(start)));
            start = acmacs::timestamp();
        }
    });

    if (parameters.debug && parameters.report_pos) {
        // AD_DEBUG("eu-20200915 added aa transitions =============================================================");
        size_t offset = 0;
        tree::iterate_pre_post(
            tree,
            [&parameters, &offset](const Node& node) mutable {
                ++offset;
                if (const auto* aatr = node.aa_transitions_.find(*parameters.report_pos); aatr) {
                    AD_DEBUG("  {:{}s}{:5.3} {}", "", offset * 2, node.node_id, aatr->display());
                }
            },
            [&offset](const Node& /*node*/) { --offset; });
    }

} // acmacs::tal::v3::detail::set_aa_transitions_eu_20210205

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::update_aa_transitions_eu_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG(parameters.debug, "update_aa_transitions_eu_20200514");

    set_closest_leaf_for_intermediate(tree);

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
        if (!branch.closest_leaves.empty()) {
            for (auto& child : branch.subtree) {
                if (!child.closest_leaves.empty() && branch.closest_leaves[0] != child.closest_leaves[0]) {
                    for (seqdb::pos0_t pos{0}; pos < std::min(branch.closest_leaves[0]->aa_sequence.size(), child.closest_leaves[0]->aa_sequence.size()); ++pos) {
                        // transitions to/from X ignored
                        // perhaps we need to look for a second closest leaf if found closest leaf has X at the pos
                        if (const auto left_aa = branch.closest_leaves[0]->aa_sequence.at(pos), right_aa = child.closest_leaves[0]->aa_sequence.at(pos);
                            left_aa != right_aa && left_aa != 'X' && right_aa != 'X') {
                            child.aa_transitions_.add(pos, left_aa, right_aa);
                            AD_DEBUG(parameters.debug && parameters.report_pos && pos == *parameters.report_pos,
                                        "update_aa_transitions_eu_20200514 node:{:4.3s} {}{}{} leaves:{:5d} closest-cumul:{} closest:{}", child.node_id,
                                        left_aa, pos, right_aa, child.number_leaves_in_subtree(), child.closest_leaves[0]->cumulative_edge_length, child.closest_leaves[0]->seq_id);
                        }
                    }
                }
            }
        }
        else
            AD_WARNING("update_aa_transitions_eu_20200514: closest leaf not found for the branch node {}", branch.node_id);
    });

} // acmacs::tal::v3::detail::update_aa_transitions_eu_20200514

// ======================================================================
// derek's method 2016

void acmacs::tal::v3::detail::update_aa_transitions_derek_2016(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG(parameters.debug, "update_aa_transitions_derek_2016");

    tree.cumulative_calculate();
    const auto longest_sequence = tree.longest_aa_sequence();
    detail::update_common_aa(tree, longest_sequence);

    const auto aa_at = [](const Node& node, seqdb::pos0_t pos) {
        if (node.is_leaf())
            return node.aa_sequence.at(pos);
        else
            return node.common_aa_->at(pos);
    };

    tree::iterate_post(tree, [aa_at, longest_sequence, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
            const auto common_aa_at = node.common_aa_->at(pos);
            AD_DEBUG(dbg, "update_aa_transitions_derek_2016 counting {} node:{:4.3s} leaves:{:4d} common-aa:{} is_no_common:{}", pos, node.node_id, node.number_leaves_in_subtree(), common_aa_at,
                        common_aa_at == NoCommon);
            if (common_aa_at == NoCommon) {
                CounterChar counter;
                for (auto& child : node.subtree) {
                    if (const auto aa = aa_at(child, pos); is_common(aa)) {
                        child.aa_transitions_.add(pos, aa);
                        counter.count(aa);
                    }
                    else if (const auto found = child.aa_transitions_.find(pos); found) {
                        counter.count(found->right);
                    }
                }
                AD_DEBUG(dbg, "  update_aa_transitions_derek_2016 counting {} node:{:4.3s} leaves:{:4d} counter: {}", pos, node.node_id, node.number_leaves_in_subtree(), counter);
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

    AD_DEBUG(parameters.debug, "update_aa_transitions_derek_2016 adding left part");
    // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&sorted_leaf_nodes, &parameters](Node& node) {
        const auto dbg = parameters.debug && parameters.report_pos && node.aa_transitions_.has(*parameters.report_pos);
        AD_DEBUG(dbg, "update_aa_transitions_derek_2016 (add left) {:4.3s} aa-transitions: {}  empty:{}", node.node_id,
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
                AD_DEBUG(dbg, "update_aa_transitions_derek_2016 (add left) {:4.3s} {} node-for-left: {} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes), node.node_for_left_aa_transitions_->node_id,
                            node.node_for_left_aa_transitions_->seq_id);
            }
            else {
                AD_DEBUG(dbg, "update_aa_transitions_derek_2016 (add left) no node for left {:4.3s} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes));
            }
        }

        node.aa_transitions_.remove_left_right_same(parameters, node);
        node.aa_transitions_.remove_empty_right();
    };
    tree::iterate_leaf_pre(tree, add_left_part, add_left_part);

} // acmacs::tal::v3::detail::update_aa_transitions_derek_2016

// ======================================================================
// Eu 2020-09-09

// void acmacs::tal::v3::update_aa_transitions_eu_20200909(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
// {
//     AD_DEBUG(parameters.debug, "eu-20200909");

//     const auto longest_sequence = tree.longest_aa_sequence();
//     update_common_aa(tree, longest_sequence);
//     const auto& root_sequence = tree.find_first_leaf().aa_sequence;

//     const auto is_not_common_with_tolerance = [](const Node& node, seqdb::pos0_t pos, double tolerance) -> bool {
//         const auto aa = node.common_aa_->at(pos, tolerance);
//         if (aa == NoCommon)
//             return true;
//         // tolerance problem: aa is common with tolerance but in
//         // reality just 1 or 2 child nodes have this aa and other
//         // children (with much fewer leaves) have different
//         // aa's. In that case consider that aa to be not
//         // common. See H3 and M346L labelling in the 3a clade.
//         const auto number_of_children_with_the_same_common_aa =
//             std::count_if(std::begin(node.subtree), std::end(node.subtree), [aa, pos, tolerance](const Node& child) { return child.common_aa_->at(pos, tolerance) == aa; });
//         return number_of_children_with_the_same_common_aa > 0 && number_of_children_with_the_same_common_aa <= 1 &&
//                node.subtree.size() > static_cast<size_t>(number_of_children_with_the_same_common_aa);
//     };

//     tree::iterate_post(tree, [longest_sequence, is_not_common_with_tolerance, &parameters](Node& node) {
//         for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
//             const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
//             const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
//             if (is_not_common_with_tolerance(node, pos, non_common_tolerance)) {
//                 AD_DEBUG(dbg, "eu-20200909 {} node:{:4.3s} leaves:{:4d} {}", pos, node.node_id, node.number_leaves_in_subtree(),
//                             node.common_aa_->size() > *pos ? node.common_aa_->counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})") : std::string{});
//                 for (auto& child : node.subtree) {
//                     if (!child.is_leaf()) {
//                         if (!is_not_common_with_tolerance(child, pos, non_common_tolerance)) {
//                             child.replace_aa_transition(pos, child.common_aa_->at(pos, non_common_tolerance));
//                             // AD_DEBUG("replace_aa_transition {} {} {} --> {}", child.node_id, pos, aa, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
//                         }
//                     }
//                 }
//             }
//             else {
//                 if (node.number_leaves_in_subtree() > 1000 && dbg) {
//                     AD_DEBUG(dbg, "eu-20200909 common {} node:{:4.3s} leaves:{:4d} {}", pos, node.node_id, node.number_leaves_in_subtree(),
//                                 node.common_aa_->counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
//                     for (const auto& child : node.subtree) {
//                         if (child.is_leaf()) {
//                             AD_DEBUG(dbg, "    {:4.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
//                         }
//                         else {
//                             AD_DEBUG(dbg, "    {:4.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
//                                         child.common_aa_->counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
//                         }
//                     }
//                 }
//             }
//         }
//     });

//     // add to top children of the tree differences between them and root, even if that subtree has common aa (with tolerance)
//     for (auto& top_child : tree.subtree) {
//         for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
//             if (const auto aa = top_child.common_aa_->at(pos, parameters.non_common_tolerance_for(pos)); aa != NoCommon && aa != root_sequence.at(pos))
//                 top_child.replace_aa_transition(pos, aa);
//         }
//     }

//     tree::iterate_post(tree, [&root_sequence, &parameters](Node& node) {
//         node.aa_transitions_.set_left(root_sequence);
//         // if (!node.aa_transitions_.empty())
//         //     AD_DEBUG("set_left {} {}", node.node_id, node.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
//         node.aa_transitions_.remove_left_right_same(parameters, node);
//     });

//     if (!tree.aa_transitions_.empty())
//         AD_WARNING("Root AA transions: {} (hide some roots to show this transion(s) in the first branch)", tree.aa_transitions_);

// } // acmacs::tal::v3::update_aa_transitions_eu_20200909

// ======================================================================
//  Eu 2020-09-15

// [[maybe_unused]] void acmacs::tal::v3::set_aa_transitions_eu_20200915(Tree& tree, seqdb::pos0_t longest_sequence, const draw_tree::AATransitionsParameters& parameters)
// {
//     // const auto total_leaves = tree.number_leaves_in_subtree();

//     const auto is_not_common_with_tolerance = [](const Node& node, seqdb::pos0_t pos, double tolerance, auto dbg) -> std::pair<bool, std::string> {
//         const auto aa = node.common_aa_->at(pos, tolerance);
//         std::string msg;
//         if (aa == NoCommon) {
//             if (dbg)
//                 msg = "common:no";
//             return {true, msg};
//         }
//         // tolerance problem: aa is common with tolerance but in
//         // reality just 1 or 2 child nodes have this aa and other
//         // children (with much fewer leaves) have different
//         // aa's. In that case consider that aa to be not
//         // common. See H3 and M346L labelling in the 3a clade.
//         std::string common_children;
//         const auto number_of_children_with_the_same_common_aa = std::count_if(std::begin(node.subtree), std::end(node.subtree), [aa, pos, tolerance, &common_children, dbg](const Node& child) {
//             if (child.is_leaf()) {
//                 return child.aa_sequence.at(pos) == aa;
//             }
//             else {
//                 if (child.common_aa_->at(pos, tolerance, dbg) == aa) {
//                     common_children += fmt::format(" {}", child.node_id);
//                     return true;
//                 }
//                 else
//                     return false;
//             }
//         });
//         const auto result = number_of_children_with_the_same_common_aa > 0 && number_of_children_with_the_same_common_aa <= 1 &&
//                node.subtree.size() > static_cast<size_t>(number_of_children_with_the_same_common_aa);
//         if (dbg)
//             msg = fmt::format("common:{} <-- {} {:5.3} aa:{} tolerance:{} number_of_children_with_the_same_common_aa:{} ({}) subtree-size:{}", !result, pos, node.node_id, aa, tolerance,
//                               number_of_children_with_the_same_common_aa, common_children, node.subtree.size());
//         return {result, msg};
//     };

//     tree::iterate_post(tree, [longest_sequence, is_not_common_with_tolerance, &parameters](Node& node) {
//         for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
//             const auto dbg = parameters.debug && parameters.report_pos && pos == *parameters.report_pos;
//             const auto non_common_tolerance = parameters.non_common_tolerance_for(pos);
//             AD_DEBUG(dbg, "eu-20200915 @{} node:{:5.3s} leaves:{:4d}", pos, node.node_id, node.number_leaves_in_subtree());
//             if (const auto [not_common, msg] = is_not_common_with_tolerance(node, pos, non_common_tolerance, dbg); not_common) {
//                 AD_DEBUG(dbg, "            <not-common> {}", node.common_aa_->size() > *pos ? node.common_aa_->counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})") : std::string{});
//                 AD_DEBUG(dbg, "                [is_not_common_with_tolerance]: {}", msg);
//                 for (auto& child : node.subtree) {
//                     if (!child.is_leaf()) {
//                         if (const auto [not_common_child, msg_child] = is_not_common_with_tolerance(child, pos, non_common_tolerance, dbg); !not_common_child) {
//                             const auto aa = child.common_aa_->at(pos, non_common_tolerance);
//                             child.aa_transitions_.add(pos, aa);
//                             AD_DEBUG(dbg, "                <child> add {}{} node:{:5.3s} leaves:{:4d} --> {}", pos, aa, child.node_id, child.number_leaves_in_subtree(), child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
//                             AD_DEBUG(dbg, "                    [is_not_common_with_tolerance]: {}", msg_child);
//                         }
//                         else {
//                             AD_DEBUG(dbg, "                <child> not-common [is_not_common_with_tolerance]: {}", msg_child);
//                         }
//                     }
//                 }
//             }
//             else {
//                 const auto node_aa = node.common_aa_->at(pos, non_common_tolerance);
//                 AD_DEBUG(dbg, "            <common> {}", node.common_aa_->counter(pos).report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
//                 AD_DEBUG(dbg, "                [is_not_common_with_tolerance]: {}", msg);
//                 for (auto& child : node.subtree) {
//                     if (child.is_leaf()) {
//                         // AD_DEBUG(dbg, "eu-20200915    {:5.3s} {} {}{}", child.node_id, child.seq_id, pos, child.aa_sequence.at(pos));
//                     }
//                     else if (!child.common_aa_->empty()) {
//                         const auto child_aa = child.common_aa_->at(pos, non_common_tolerance);
//                         const auto counter = child.common_aa_->counter(pos);
//                         AD_DEBUG(dbg, "                {:5.3s} leaves:{:4d} {}", child.node_id, child.number_leaves_in_subtree(),
//                                     counter.report_sorted_max_first(" {value}:{counter_percent:.1f}%({counter})"));
//                         if (const auto [not_common_child, msg_child] = is_not_common_with_tolerance(child, pos, non_common_tolerance, dbg); child_aa != node_aa && !not_common_child) {
//                             child.replace_aa_transition(pos, child_aa);
//                             AD_DEBUG(dbg, "                <child> <replaced> {:5.3s} {:3d}{} --> {}", child.node_id, pos, child_aa, child.aa_transitions_.display(std::nullopt, AA_Transitions::show_empty_left::yes));
//                             AD_DEBUG(dbg, "                    [is_not_common_with_tolerance]: {}", msg);
//                         }
//                     }
//                 }
//             }
//         }
//     });

// } // acmacs::tal::v3::set_aa_transitions_eu_20200915

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
