#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

acmacs::tal::v3::DrawTree::DrawTree(Tal& tal)
    : LayoutElement(0.7), tal_{tal}, coloring_{std::make_unique<ColoringUniform>(CYAN)}
{
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::prepare()
{
    const auto tree_height = tal_.tree().compute_cumulative_vertical_offsets();
    tal_.tree().number_leaves_in_subtree();

    // color leaves

    vertical_step_ = height_ / tree_height;
    horizontal_step_ = width_to_height_ratio() * height_ / tal_.tree().max_cumulative_shown().as_number();

} // acmacs::tal::v3::DrawTree::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::draw(acmacs::surface::Surface& surface) const
{
    const Scaled line_width{vertical_step_ * 0.5};
    const Scaled text_size{vertical_step_ * 0.8};

    tree::iterate_leaf_pre(
        tal_.tree(),
        // leaf
        [=, this, &surface](const Node& leaf) {
            surface.line({horizontal_step_ * (leaf.cumulative_edge_length - leaf.edge_length).as_number(), vertical_step_ * leaf.cumulative_vertical_offset_},
                         {horizontal_step_ * leaf.cumulative_edge_length.as_number(), vertical_step_ * leaf.cumulative_vertical_offset_}, BLACK, line_width);
            surface.text({horizontal_step_ * leaf.cumulative_edge_length.as_number() + text_size.value() * 0.5, vertical_step_ * leaf.cumulative_vertical_offset_ + text_size.value() * 0.3},
                         std::string{leaf.seq_id}, coloring_->color(leaf), text_size);
        },
        // pre
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
