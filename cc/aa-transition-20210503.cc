#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::update_aa_transitions_eu_20210503(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    // looks similar to update_aa_transitions_eu_20200514

    AD_DEBUG(parameters.debug, "update_aa_transitions_eu_20210503");

    set_closest_leaf_for_intermediate(tree);

    // for each intermediate node find closest leaf node of its subtree, i.e. a leaf in the subtree that has minimum cumulative length
    tree::iterate_pre(tree, [&parameters](Node& branch) {
        if (branch.closest_leaf) {
            for (auto& child : branch.subtree) {
                if (child.closest_leaf && branch.closest_leaf != child.closest_leaf) {
                    for (seqdb::pos0_t pos{0}; pos < std::min(branch.closest_leaf->aa_sequence.size(), child.closest_leaf->aa_sequence.size()); ++pos) {
                        // transitions to/from X ignored
                        // perhaps we need to look for a second closest leaf if found closest leaf has X at the pos
                        if (const auto left_aa = branch.closest_leaf->aa_sequence.at(pos), right_aa = child.closest_leaf->aa_sequence.at(pos);
                            left_aa != right_aa && left_aa != 'X' && right_aa != 'X') {
                            child.aa_transitions_.add(pos, left_aa, right_aa);

                            AD_DEBUG(parameters.debug && parameters.report_pos && pos == *parameters.report_pos,
                                     "update_aa_transitions_eu_20210503 node:{:4.3s} {}{}{} leaves:{:5d} closest-cumul:{} closest:{}", child.node_id, left_aa, pos, right_aa,
                                     child.number_leaves_in_subtree(), child.closest_leaf->cumulative_edge_length, child.closest_leaf->seq_id);
                        }
                    }
                }
            }
        }
    });

    // for each intermediate node check if a child has an aa transition label for the same position
    tree::iterate_pre(tree, [&parameters](Node& node) {
        for (auto& aa_transition : node.aa_transitions_) {
            bool update = false;
            for (auto& child : node.subtree) {
                if (auto* child_transition = child.aa_transitions_.find(aa_transition.pos); child_transition) {
                    AD_DEBUG(parameters.debug && parameters.report_pos && aa_transition.pos == *parameters.report_pos,
                             "update_aa_transitions_eu_20210503: flipping aa transtions for {} in {} {} (parent) and {} {} (child)", aa_transition.pos, node.node_id, aa_transition.display(),
                             child.node_id, child.aa_transitions_.display());
                    update = true;
                    break;
                }
            }
            if (update) {
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
                        else if (const auto child_right = child.closest_leaf->aa_sequence.at(aa_transition.pos); child_right != 'X' && child_right != aa_transition.right)
                            child.aa_transitions_.add(aa_transition.pos, aa_transition.right, child_right);
                        AD_DEBUG(parameters.debug && parameters.report_pos && aa_transition.pos == *parameters.report_pos,
                                 "update_aa_transitions_eu_20210503:      child ({}) updated: {}", child.node_id, child.aa_transitions_.display());
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
