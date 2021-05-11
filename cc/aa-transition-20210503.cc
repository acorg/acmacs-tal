#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3::detail
{
    inline bool process_children(const Node& parent, const AA_Transition& parent_transition, double number_of_leaves_ratio_threshold, bool dbg, const char* child_name)
    {
        // returns if parent needs to be updated, i.e. its transition removed and transitions of its children updated
        for (auto& child : parent.subtree) {
            if (auto* child_transition = child.aa_transitions_.find(parent_transition.pos); child_transition) {
                if (const auto parent_leaves = static_cast<double>(parent.number_leaves_in_subtree()), child_leaves = static_cast<double>(child.number_leaves_in_subtree()),
                    ratio = child_leaves / parent_leaves;
                    ratio > number_of_leaves_ratio_threshold) {
                        AD_DEBUG(dbg, "update_aa_transitions_eu_20210503: flipping aa transtions for {} in {} {} (parent, {} leaves) and {} {} ({}, {:.0f} leaves), ratio: {:.2f}", parent_transition.pos,
                                 parent.node_id, parent_transition.display(), parent_leaves, child.node_id, child.aa_transitions_.display(), child_name, child_leaves, ratio);
                    return true;
                }
                else
                    AD_DEBUG(dbg, "update_aa_transitions_eu_20210503: IGNORED flipping aa transtions for {} in {} {} (parent, {} leaves) and {} {} ({}, {:.0f} leaves), ratio: {:.2f}",
                             parent_transition.pos, parent.node_id, parent_transition.display(), parent_leaves, child.node_id, child.aa_transitions_.display(), child_name, child_leaves, ratio);
            }
        }
        return false;
    }

} // namespace acmacs::tal::inline v3::detail

// ----------------------------------------------------------------------

// Algorithm:
//
// 1. For each intermediate node find the closest leaf in its subtree,
// i.e. a child leaf with the minimal cumulative edge length. We have
// to keep references to the multiple closest leaves because signle
// leaf may have X at some position or its sequence can be too short
// and then algorithm may fail to set aa substitution label
// correctly. See warning "no closest leaf" below.
//
// 2. For each intermediate child node of a parent node, if the
// closest leaf of the parent node and the closest leaf of the child
// node have different aa's at a position and none of those aa's is X,
// set aa transition label for that position for the child node to
// transition from parent aa to child aa. If an aa is X, look for the
// second (third, etc.) closest leaf of the corresponding node (parent
// of child). If no non-X in the collected closest leaves found, do not
// set aa transition label, as if parent and child aa's are the same.
//
// 3. Descent the tree from the root node. For each intermediate node
// (parent) that has a transition label for a position, if its child
// intermediate node also has transition label for that position, mark
// that (parent) node for updating. If no children have transition
// labels for that position, look in the grand-children intermediate
// nodes of the parent and mark parent for updating if a grand-child
// has label for that position. If parent node is marked, do 4. before
// descending further.
//
// 4. If a node is marked for updating, set right (to) part of its the
// transition label for that position to be the same as the left
// (from) part. For each child node update its transition labels
// assuming closest leaf of its parent has just set aa (in the left
// and right part). I.e. if the closest leaf of the child has
// different aa than found in the parent transition label, then set
// the label accordingly, if aa's are the same, remove the label if it
// is present. Then continue descending the tree in the step 3 and
// re-process children as parent nodes in the step 3.
//
// 5. Remove labels in the tree nodes that have the same left and
// right aa's.
//
// In the result there should be no transition labels for the same
// position in the nodes that are close to each other.

void acmacs::tal::v3::detail::update_aa_transitions_eu_20210503(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    // parameters
    const double child_number_of_leaves_ratio_threshold = 0.0;
    const double enkel_number_of_leaves_ratio_threshold = 0.3;

    AD_DEBUG(parameters.debug, "update_aa_transitions_eu_20210503");

    set_closest_leaf_for_intermediate(tree);

    const auto find_closest_with_aa_at = [number_of_leaves_in_tree=tree.number_leaves_in_subtree()](seqdb::pos0_t pos, const Node& node) -> std::pair<char, const Node*> {
        auto aa{'X'};
        for (const auto* bcl : node.closest_leaves) {
            aa = bcl->aa_sequence.at(pos);
            if (aa != 'X' && aa != ' ')
                return {aa, bcl};
        }
        if (number_of_leaves_in_tree < 10000 && node.number_leaves_in_subtree() > node.closest_leaves.size() && *pos < static_cast<size_t>(static_cast<double>(*node.closest_leaves[0]->aa_sequence.size()) * 0.9)) // ignore last 10% of positions anyway
            AD_WARNING("update_aa_transitions_eu_20210503: no closest leaf sequence with certain amino acid at {} found for node:{} (num-leaves:{}, closest-leaves:{}), increase "
                       "max_number_of_closest_leaves_to_set in aa-transitions.cc:219",
                       pos, node.node_id, node.number_leaves_in_subtree(), node.closest_leaves.size());
        return {aa, nullptr};
    };

    tree::iterate_pre(tree, [&parameters, &find_closest_with_aa_at](Node& branch) {
        if (!branch.closest_leaves.empty()) {
            for (auto& child : branch.subtree) {
                if (!child.closest_leaves.empty() && branch.closest_leaves[0] != child.closest_leaves[0]) {
                    for (seqdb::pos0_t pos{0}; pos < std::min(branch.closest_leaves[0]->aa_sequence.size(), child.closest_leaves[0]->aa_sequence.size()); ++pos) {
                        const auto [left_aa, left_aa_node] = find_closest_with_aa_at(pos, branch);
                        const auto [right_aa, right_aa_node] = find_closest_with_aa_at(pos, child);
                        // transitions to/from X ignored, space in aa means sequence is too short
                        if (left_aa != right_aa && left_aa != 'X' && left_aa != ' ' && right_aa != 'X' && right_aa != ' ') {
                            child.aa_transitions_.add(pos, left_aa, right_aa);
                            AD_DEBUG(parameters.debug && parameters.report_pos && pos == *parameters.report_pos,
                                     "update_aa_transitions_eu_20210503 node:{:5.3s} {}{}{} leaves:{:5d} closest-cumul:{} closest:{}", child.node_id, left_aa, pos, right_aa,
                                     child.number_leaves_in_subtree(), right_aa_node->cumulative_edge_length, right_aa_node->seq_id);
                        }
                    }
                }
            }
        }
    });

    // for each intermediate node check if a child has an aa transition label for the same position

    const auto update_children = [child_number_of_leaves_ratio_threshold](const Node& parent, const AA_Transition& parent_transition, bool dbg) {
        return detail::process_children(parent, parent_transition, child_number_of_leaves_ratio_threshold, dbg, "child");
    };

    const auto update_enkels = [enkel_number_of_leaves_ratio_threshold](const Node& parent, const AA_Transition& parent_transition, bool dbg) {
        for (auto& child : parent.subtree) {
            if (detail::process_children(child, parent_transition, enkel_number_of_leaves_ratio_threshold, dbg, "enkel"))
                return true;
        }
        return false;
    };

    tree::iterate_pre(tree, [&parameters, update_children, update_enkels, &find_closest_with_aa_at](Node& node) {
        for (auto& aa_transition : node.aa_transitions_) {
            const bool dbg = parameters.debug && parameters.report_pos && aa_transition.pos == *parameters.report_pos;
            // AD_DEBUG(dbg, "update_aa_transitions_eu_20210503 node:{:6.3} {}", node.node_id, aa_transition.display());
            if (update_children(node, aa_transition, dbg) || update_enkels(node, aa_transition, dbg)) {
                // remove transition from node, pretend node has the
                // same left and right (equals left) for this pos,
                // update transitions for all children
                aa_transition.right = aa_transition.left; // mark for removal, cannot remove now due to iteration over transitions
                for (auto& child : node.subtree) {
                    if (!child.is_leaf()) {
                        if (auto* child_transition = child.aa_transitions_.find(aa_transition.pos); child_transition) {
                            if (child_transition->right == aa_transition.right)
                                child.aa_transitions_.remove(aa_transition.pos);
                            else
                                child_transition->left = aa_transition.right;
                        }
                        else if (const auto [aa_right, child_right] = find_closest_with_aa_at(aa_transition.pos, child); aa_right != 'X' && aa_right != ' ' && aa_right != aa_transition.right)
                            child.aa_transitions_.add(aa_transition.pos, aa_transition.right, aa_right);
                        AD_DEBUG(dbg, "update_aa_transitions_eu_20210503:      child ({}) updated: {}", child.node_id, child.aa_transitions_.display());
                    }
                }
            }
        }
        node.aa_transitions_.remove_left_right_same(parameters, node);
    });
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
