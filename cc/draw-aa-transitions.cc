#include "acmacs-base/enumerate.hh"
#include "acmacs-tal/draw-aa-transitions.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::prepare()
{
    tree::iterate_pre(tal().tree(), [this](const Node& node) {
        if (!node.hidden && node.number_leaves_in_subtree_ >= parameters().minimum_number_leaves_in_subtree && node.aa_transitions_) {
            transitions_.emplace_back(&node, LabelParameters{BLACK, 0.01, vertical_position::top, horizontal_position::middle, {0.0, 0.0}, {}, NoRotation, LabelTetherParameters{}});
        }
    });

    LayoutElement::prepare();

} // acmacs::tal::v3::Legend::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::draw(acmacs::surface::Surface& /*surface*/) const
{
    // do nothing
    // draw_transitions() called by DrawTree::draw is used for drawing transitions

} // acmacs::tal::v3::LegendContinentMap::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::draw_transitions(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const
{
    const auto vertical_step = draw_tree.vertical_step();
    const auto horizontal_step = draw_tree.horizontal_step();
    const TextStyle text_style{"monospace"};

    for (const auto& transition : transitions_) {
        const auto names = transition.node->aa_transitions_.names();

        const Scaled text_size{transition.label.scale};
        std::vector<Size> name_sizes(names.size());
        std::transform(std::begin(names), std::end(names), std::begin(name_sizes), [&surface, text_size, &text_style](const auto& name) { return surface.text_size(name, text_size, text_style); });
        Size box_size = std::accumulate(std::begin(name_sizes), std::end(name_sizes), Size{}, [](const Size& result, const Size& name_size) {
            return Size{std::max(result.width, name_size.width), result.height + name_size.height};
        });
        box_size.height += (names.size() - 1) * parameters().text_line_interleave * name_sizes.front().height;

        auto pos_x = horizontal_step * (transition.node->cumulative_edge_length.as_number() - transition.node->edge_length.as_number() / 2.0);
        auto pos_y = vertical_step * transition.node->cumulative_vertical_offset_;
        for (auto [index, name] : acmacs::enumerate(names)) {
            pos_y += name_sizes[index].height * (1.0 + parameters().text_line_interleave);
            surface.text({pos_x + (box_size.width - name_sizes[index].width) / 2.0, pos_y}, name, transition.label.color, text_size, text_style);
        }
    }

} // acmacs::tal::v3::DrawAATransitions::draw_transitions

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
