#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/draw-aa-transitions.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::prepare(verbose verb)
{
    if (!prepared_) {
        tal().draw().layout().prepare_element<Clades>(verb);

        const auto tree_height = tal().tree().compute_cumulative_vertical_offsets();
        tal().tree().number_leaves_in_subtree();

        vertical_step_ = height_ / tree_height;
        horizontal_step_ = width_to_height_ratio() * height_ / tal().tree().max_cumulative_shown().as_number();
    }
    LayoutElementWithColoring::prepare(verb);

} // acmacs::tal::v3::DrawTree::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::draw(acmacs::surface::Surface& surface, verbose verb) const
{
    const Scaled line_width{vertical_step() * 0.5};
    const Scaled text_size{vertical_step() * 0.8};

    tree::iterate_leaf_pre(
        tal().tree(),
        // leaf
        [=, this, &surface](const Node& leaf) {
            if (!leaf.hidden) {
                surface.line({horizontal_step_ * (leaf.cumulative_edge_length - leaf.edge_length).as_number(), vertical_step() * leaf.cumulative_vertical_offset_},
                             {horizontal_step_ * leaf.cumulative_edge_length.as_number(), vertical_step() * leaf.cumulative_vertical_offset_}, leaf.color_edge_line,
                             line_width * leaf.edge_line_width_scale);
                const auto label_size = text_size.value() * leaf.label_scale;
                const auto label_color = leaf.label_color.has_value() ? *leaf.label_color : color(leaf);
                surface.text({horizontal_step_ * leaf.cumulative_edge_length.as_number() + label_size * 0.5, vertical_step() * leaf.cumulative_vertical_offset_ + label_size * 0.3},
                             std::string{leaf.seq_id}, label_color, Scaled{label_size});
            }
        },
        // pre
        [&surface, this, line_width, vertical_additon = line_width.value() / 2.0](const Node& node) {
            if (!node.hidden) {
                if (const auto shown_children = node.shown_children(); !shown_children.empty()) {
                    surface.line({horizontal_step_ * (node.cumulative_edge_length - node.edge_length).as_number(), vertical_step() * node.cumulative_vertical_offset_},
                                 {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * node.cumulative_vertical_offset_}, node.color_edge_line,
                                 line_width * node.edge_line_width_scale);

                    surface.line({horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * shown_children.front()->cumulative_vertical_offset_ - vertical_additon},
                                 {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * shown_children.back()->cumulative_vertical_offset_ + vertical_additon}, BLACK,
                                 line_width);
                }
                else
                    fmt::print(stderr, "WARNING: node is not hidden but all ist children are hidden {} total children: {}\n", node.node_id_, node.subtree.size());
            }
        });

    if (const auto* draw_aa_transitions = tal().draw().layout().find<DrawAATransitions>(); draw_aa_transitions)
        draw_aa_transitions->draw_transitions(surface, *this, verb);
    if (const auto* draw_on_tree = tal().draw().layout().find<DrawOnTree>(); draw_on_tree)
        draw_on_tree->draw_on_tree(surface, *this, verb);

} // acmacs::tal::v3::DrawTree::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawOnTree::draw(acmacs::surface::Surface& /*surface*/, verbose /*verb*/) const
{
    // do nothing
    // draw_on_tree() called by DrawTree::draw is used for drawing

} // acmacs::tal::v3::DrawOnTree::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawOnTree::draw_on_tree(acmacs::surface::Surface& surface, const DrawTree& draw_tree, verbose /*verb*/) const
{
    const TextStyle text_style{};

    for (const auto& per_node : parameters().per_node) {
        if (!per_node.text.text.empty()) {
        }
    }

    for (const auto& text : parameters().texts) {
        surface.text(text.offset, text.text, text.color, text.size, text_style, NoRotation);
    }

} // acmacs::tal::v3::DrawOnTree::draw_on_tree

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
