#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::update_aa_transitions_eu_20210503(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    // looks similar to update_aa_transitions_eu_20200514

    AD_DEBUG(parameters.debug, "update_aa_transitions_eu_20210503");
    tree.cumulative_calculate();

    // for each intermediate node find closest leaf node of its subtree, i.e. a leaf in the subtree that has minimum cumulative length

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
