#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/legend.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/draw-aa-transitions.hh"

// ----------------------------------------------------------------------

acmacs::tal::v3::DrawTree::DrawTree(Tal& tal)
    : LayoutElementWithColoring(tal, 0.7)
{
} // acmacs::tal::v3::DrawTree::DrawTree

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::legend(std::unique_ptr<Legend>&& a_legend)
{
    legend_ = std::move(a_legend);

} // acmacs::tal::v3::DrawTree::legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::prepare(preparation_stage_t stage)
{
    auto& tree = tal().tree();
    if (stage == 1 && prepared_ < stage) {
        // AD_DEBUG("DrawTree::prepare");
        tree.set_first_last_next_node_id();
        if (parameters().aa_transitions.calculate)
            update_aa_transitions(tree, parameters().aa_transitions);

        tree::iterate_leaf(tree, [this](const Node& leaf) {
            if (!leaf.hidden)
                coloring().prepare(leaf);
        });
        coloring().prepare();
        AD_LOG(acmacs::log::coloring, "tree {}", coloring().report());

        if (parameters().aa_transitions.report)
            report_aa_transitions(tree, parameters().aa_transitions);
    }
    else if (stage == 3 && prepared_ < stage) {
        const auto tree_height = tal().tree().compute_cumulative_vertical_offsets();
        const auto leaves_to_show = tree.number_leaves_in_subtree();
        AD_INFO("tree [id: {}] Shown leaves: {}", id(), leaves_to_show);
        vertical_step_ = height_ / tree_height;
        const auto reserve = leaves_to_show < 3000 ? (1.0 - double(leaves_to_show) / 3000.0) * 0.1 : 0.0; // if too few leaves, names are seen and occupy much space
        horizontal_step_ = width_to_height_ratio() * height_ * (1.0 - reserve) / tree.max_cumulative_shown().as_number();
        AD_LOG(acmacs::log::tree, "tree vertical step: {} (tree height: {}) horizontal step: {}", vertical_step_, tree_height, horizontal_step_);
    }
    LayoutElementWithColoring::prepare(stage);

} // acmacs::tal::v3::DrawTree::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::draw(acmacs::surface::Surface& surface) const
{
    const Scaled line_width{vertical_step() * 0.5};
    const Scaled text_size{vertical_step() * 0.8};
    const Scaled node_id_text_size{3e-4};

    tree::iterate_leaf_pre(
        tal().tree(),
        // leaf
        [=, this, &surface](const Node& leaf) {
            if (!leaf.hidden) {
                surface.line({horizontal_step_ * (leaf.cumulative_edge_length - leaf.edge_length).as_number(), vertical_step() * leaf.cumulative_vertical_offset_},
                             {horizontal_step_ * leaf.cumulative_edge_length.as_number(), vertical_step() * leaf.cumulative_vertical_offset_}, leaf.color_edge_line,
                             line_width * leaf.edge_line_width_scale);
                const auto label_size = text_size * leaf.label_scale;
                const auto label_color = leaf.label_color.has_value() ? *leaf.label_color : coloring().color(leaf);
                surface.text({horizontal_step_ * leaf.cumulative_edge_length.as_number() + *label_size * 0.5, vertical_step() * leaf.cumulative_vertical_offset_ + *label_size * 0.3},
                             std::string{leaf.seq_id}, label_color, label_size);
            }
        },
        // pre
        [&surface, this, line_width, vertical_additon = line_width.value() / 2.0, node_id_text_size](const Node& node) {
            if (!node.hidden) {
                if (const auto shown_children = node.shown_children(); !shown_children.empty()) {
                    surface.line({horizontal_step_ * (node.cumulative_edge_length - node.edge_length).as_number(), vertical_step() * node.cumulative_vertical_offset_},
                                 {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * node.cumulative_vertical_offset_}, node.color_edge_line,
                                 line_width * node.edge_line_width_scale);

                    surface.line({horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * shown_children.front()->cumulative_vertical_offset_ - vertical_additon},
                                 {horizontal_step_ * node.cumulative_edge_length.as_number(), vertical_step() * shown_children.back()->cumulative_vertical_offset_ + vertical_additon}, BLACK,
                                 line_width);

                    surface.text(
                        {horizontal_step_ * (node.cumulative_edge_length - node.edge_length).as_number() + *node_id_text_size, vertical_step() * node.cumulative_vertical_offset_ - *line_width},
                        fmt::format("{}", node.node_id), BLACK, node_id_text_size);
                }
                else
                    AD_WARNING("node is not hidden but all ist children are hidden {} total children: {}", node.node_id, node.subtree.size());
            }
        });

    if (const auto* draw_aa_transitions = tal().draw().layout().find<DrawAATransitions>(); draw_aa_transitions)
        draw_aa_transitions->draw_transitions(surface, *this);
    if (const auto* draw_on_tree = tal().draw().layout().find<DrawOnTree>(); draw_on_tree)
        draw_on_tree->draw_on_tree(surface, *this);
    if (legend_)
        legend_->draw(surface, coloring());

} // acmacs::tal::v3::DrawTree::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawOnTree::draw(acmacs::surface::Surface& /*surface*/) const
{
    // do nothing
    // draw_on_tree() called by DrawTree::draw is used for drawing

} // acmacs::tal::v3::DrawOnTree::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawOnTree::draw_on_tree(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const
{
    const TextStyle text_style{};

    for (const auto& per_node : parameters().per_node) {
        if (const auto* leaf = tal().tree().find_node_by_seq_id(per_node.seq_id); leaf) {
            if (!per_node.text.text.empty() && per_node.text.color != TRANSPARENT) {
                const PointCoordinates origin{(per_node.text.absolute_x.has_value() ? *per_node.text.absolute_x : draw_tree.horizontal_step() * leaf->cumulative_edge_length.as_number()) +
                                                  per_node.text.offset.x(),
                                              draw_tree.vertical_step() * leaf->cumulative_vertical_offset_ + per_node.text.offset.y() + per_node.text.size.value() * 0.3};
                surface.text(origin, per_node.text.text, per_node.text.color, per_node.text.size);
            }
            if (per_node.line.color != TRANSPARENT && per_node.line.line_width.value() > 0.0 && per_node.line.offset[0] != per_node.line.offset[1]) {
                const PointCoordinates origin{(per_node.line.absolute_x.has_value() ? *per_node.line.absolute_x : draw_tree.horizontal_step() * leaf->cumulative_edge_length.as_number()),
                                              draw_tree.vertical_step() * leaf->cumulative_vertical_offset_};
                surface.line(origin + PointCoordinates{per_node.line.offset[0]}, origin + PointCoordinates{per_node.line.offset[1]}, per_node.line.color, per_node.line.line_width);
            }
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
