#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_calculate()
{
    EdgeLength cumulative{0.0};
    const auto leaf = [&cumulative](Node& node) { node.cumulative_edge_length = cumulative + node.edge_length; };
    const auto pre = [&cumulative](Node& node) { node.cumulative_edge_length = cumulative + node.edge_length; cumulative = node.cumulative_edge_length; };
    const auto post = [&cumulative](Node& node) { cumulative -= node.edge_length; };

    tree::iterate_leaf_pre_post(*this, leaf, pre, post);

} // acmacs::tal::v3::Tree::cumulative_calculate

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tree::cumulative_reset()
{
    tree::iterate_leaf(*this, [](Node& node) { node.cumulative_edge_length = EdgeLength{-1.0}; });

} // acmacs::tal::v3::Tree::cumulative_reset

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
