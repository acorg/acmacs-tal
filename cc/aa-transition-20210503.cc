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

    tree::iterate_post(tree, [&parameters](Node& branch) { // _post is necessary!
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

                            // check if a child of child has a flipping transition for this pos
                            if (parameters.debug && parameters.report_pos && pos == *parameters.report_pos) {
                                for (auto& enkel : child.subtree) {
                                    if (auto* enkel_transition = enkel.aa_transitions_.find(pos); enkel_transition)
                                        AD_WARNING("update_aa_transitions_eu_20210503:       sub-child {} of {} has transition {}", child.node_id, enkel.node_id, enkel_transition->display());
                                }
                            }

                            // // check if child node (set earlier)
                            // if (parameters.debug && parameters.report_pos && pos == *parameters.report_pos) {
                            // }
                        }
                    }
                }
            }
        }
        else
            AD_WARNING("update_aa_transitions_eu_20210503: closest leaf not found for the branch node {}", branch.node_id);
    });
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
