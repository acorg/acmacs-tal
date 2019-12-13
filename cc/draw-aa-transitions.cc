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
            const auto node_id = fmt::format("{}", node.node_id_);
            transitions_.emplace_back(&node, parameters_for_node(node_id).label);
        }
    });
    std::sort(std::begin(transitions_), std::end(transitions_), [](const auto& e1, const auto& e2) { return e1.node->node_id_ < e2.node->node_id_; });

    fmt::print("INFO: AA transitions ({})\n", transitions_.size());
    size_t max_id{0}, max_name{0};
    for (const auto& transition : transitions_) {
        max_id = std::max(max_id, fmt::format("{}", transition.node->node_id_).size());
        max_name = std::max(max_name, transition.node->aa_transitions_.display().size());
    }
    for (const auto& transition : transitions_) {
        const auto node_id = fmt::format("\"{}\"", transition.node->node_id_);
        const auto name = fmt::format("\"{}\",", transition.node->aa_transitions_.display());
        fmt::print("  {{\"node_id\": {:>{}s}, \"name\": {:<{}s} \"label\": {{\"offset\": [{}, {}]}}}}\n", node_id, max_id + 2, name, max_name + 3, transition.label.offset[0], transition.label.offset[1]);
    }

    LayoutElement::prepare();

} // acmacs::tal::v3::Legend::prepare

// ----------------------------------------------------------------------

const acmacs::tal::v3::DrawAATransitions::TransitionParameters& acmacs::tal::v3::DrawAATransitions::parameters_for_node(std::string_view node_id) const
{
    if (auto found = std::find_if(std::begin(parameters_.per_node), std::end(parameters_.per_node), [node_id](const auto& for_node) { return for_node.node_id == node_id; }); found != std::end(parameters_.per_node))
        return *found;
    else
        return parameters_.all_nodes;

} // acmacs::tal::v3::DrawAATransitions::parameters_for_node

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
    const auto text_line_interleave = parameters().text_line_interleave;

    for (const auto& transition : transitions_) {
        const auto names = transition.node->aa_transitions_.names();

        const Scaled text_size{transition.label.scale};
        std::vector<Size> name_sizes(names.size());
        std::transform(std::begin(names), std::end(names), std::begin(name_sizes), [&surface, &transition, text_size](const auto& name) { return surface.text_size(name, text_size, transition.label.text_style); });
        Size box_size = std::accumulate(std::begin(name_sizes), std::end(name_sizes), Size{}, [](const Size& result, const Size& name_size) {
            return Size{std::max(result.width, name_size.width), result.height + name_size.height};
        });
        box_size.height += (names.size() - 1) * text_line_interleave * name_sizes.front().height;

        const PointCoordinates at_edge_line{horizontal_step * (transition.node->cumulative_edge_length.as_number() - transition.node->edge_length.as_number() / 2.0), vertical_step * transition.node->cumulative_vertical_offset_};
        const PointCoordinates box_top_left{at_edge_line.x() + transition.label.offset[0], at_edge_line.y() + transition.label.offset[1]};
        // surface.rectangle(box_top_left, box_size, GREY, Pixels{1});

        if (transition.label.tether.show) {
            const PointCoordinates box_bottom_right{box_top_left.x() + box_size.width, box_top_left.y() + box_size.height};
            const auto box_center = middle(box_top_left, box_bottom_right);
            const auto vspace = name_sizes.front().height * text_line_interleave;
            PointCoordinates at_box{box_center.x(), box_top_left.y() - vspace};
            if (box_top_left.y() <= at_edge_line.y() && box_bottom_right.y() >= at_edge_line.y()) {
                if (box_top_left.x() > at_edge_line.x())
                    at_box.x(box_top_left.x());
                else
                    at_box.x(box_bottom_right.x());
                at_box.y(box_center.y());
            }
            else if (box_bottom_right.y() < at_edge_line.y())
                at_box.y(box_bottom_right.y() + vspace);
            surface.line(at_edge_line, at_box, transition.label.tether.line.color, transition.label.tether.line.line_width);
        }

        auto pos_x = box_top_left.x();
        auto pos_y = box_top_left.y();
        for (auto [index, name] : acmacs::enumerate(names)) {
            if (index)
                pos_y += name_sizes[index].height * (1.0 + text_line_interleave);
            else
                pos_y += name_sizes[index].height;
            surface.text({pos_x + (box_size.width - name_sizes[index].width) / 2.0, pos_y}, name, transition.label.color, text_size, transition.label.text_style);
        }
    }

} // acmacs::tal::v3::DrawAATransitions::draw_transitions

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
