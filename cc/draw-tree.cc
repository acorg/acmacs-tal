#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::prepare()
{
    const auto tree_height = tal_.tree().compute_cumulative_vertical_offsets();
    tal_.tree().number_leaves_in_subtree();

    vertical_step_ = height_ / tree_height;
    horizontal_step_ = width_to_height_ratio() * height_ / tal_.tree().max_cumulative_shown().as_number();

} // acmacs::tal::v3::DrawTree::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::draw(acmacs::surface::Surface& surface) const
{
    const Scaled line_width{vertical_step_ / 2.0};

    tree::iterate_leaf_pre(
        tal_.tree(),
        [&surface, this, line_width](const Node& leaf) {
            surface.line({horizontal_step_ * (leaf.cumulative_edge_length - leaf.edge_length).as_number(), vertical_step_ * leaf.cumulative_vertical_offset_},
                         {horizontal_step_ * leaf.cumulative_edge_length.as_number(), vertical_step_ * leaf.cumulative_vertical_offset_}, BLACK, line_width);
        },
        [&surface, this, line_width, vertical_additon = line_width.value() / 2.0](const Node& node) {
            surface.line({horizontal_step_ * (node.cumulative_edge_length - node.edge_length).as_number(), vertical_step_ * node.cumulative_vertical_offset_},
                         {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step_ * node.cumulative_vertical_offset_}, BLACK, line_width);
            const auto shown_children = node.shown_children();
            surface.line({horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step_ * shown_children.front()->cumulative_vertical_offset_ - vertical_additon},
                         {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step_ * shown_children.back()->cumulative_vertical_offset_ + vertical_additon}, BLACK, line_width);
        });

} // acmacs::tal::v3::DrawTree::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
