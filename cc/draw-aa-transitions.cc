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

void acmacs::tal::v3::DrawAATransitions::calculate_boxes(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const
{
    const auto text_line_interleave = parameters().text_line_interleave;
    const auto vertical_step = draw_tree.vertical_step();
    const auto horizontal_step = draw_tree.horizontal_step();
    auto interleave{0.0};

    for (auto& transition : transitions_) {
        const auto names = transition.node->aa_transitions_.names();

        const Scaled text_size{transition.label.scale};
        transition.name_sizes.resize(names.size());
        std::transform(std::begin(names), std::end(names), std::begin(transition.name_sizes),
                       [&surface, &transition, text_size](const auto& name) { return surface.text_size(name, text_size, transition.label.text_style); });
        transition.box.size = std::accumulate(std::begin(transition.name_sizes), std::end(transition.name_sizes), Size{}, [](const Size& result, const Size& name_size) {
            return Size{std::max(result.width, name_size.width), result.height + name_size.height};
        });
        interleave = text_line_interleave * transition.name_sizes.front().height;
        transition.box.size.height += (names.size() - 1) * interleave;

        transition.at_edge_line.x(horizontal_step * (transition.node->cumulative_edge_length.as_number() - transition.node->edge_length.as_number() / 2.0));
        transition.at_edge_line.y(vertical_step * transition.node->cumulative_vertical_offset_);
        transition.box.origin.x(transition.at_edge_line.x() + transition.label.offset[0]);
        transition.box.origin.y(transition.at_edge_line.y() + transition.label.offset[1]);
    }

    // overlapping detection

    using transition_iterator = decltype(std::begin(transitions_));
    const auto find_overlapping = [](transition_iterator primary, transition_iterator end) {
        std::vector<transition_iterator> overlapping;
        for (transition_iterator secondary = std::next(primary); secondary != end; ++secondary) {
            if (primary->box.is_overlap(secondary->box))
                overlapping.push_back(secondary);
        }
        return overlapping;
    };

    // const auto report_overlapping = [](transition_iterator primary, const auto& overlapping) {
    //     if (!overlapping.empty()) {
    //         fmt::print(stderr, "    ({}) {} \"{}\" {:.6f}\n", overlapping.size(), primary->node->node_id_, primary->node->aa_transitions_.display(), primary->box);
    //         for (const auto& over : overlapping)
    //             fmt::print(stderr, "        {} \"{}\" {:.6f}\n", over->node->node_id_, over->node->aa_transitions_.display(), over->box);
    //     }
    // };

    // move upper box up and lower box down
    // boxes are not moved horizontally
    const auto make_separate = [interleave](auto& upper, auto& lower) {
        const auto dist_y = (upper.box.bottom() - lower.box.top() + interleave * 4) / 2.0;
        upper.box.origin.y(upper.box.top() - dist_y);
        upper.label.offset[1] -= dist_y;
        lower.box.origin.y(lower.box.top() + dist_y);
        lower.label.offset[1] += dist_y;
    };

    for (size_t overlapping_no = 1; overlapping_no > 0;) {
        bool overlapping_present = false;
        for (auto trn = std::begin(transitions_); trn != std::end(transitions_); ++trn) {
            const auto overlapping = find_overlapping(trn, std::end(transitions_));
            // report_overlapping(trn, overlapping);

            if (!overlapping.empty()) {
                overlapping_present = true;
                const auto& over = overlapping.front();
                if (trn->box.top() < over->box.top())
                    make_separate(*trn, *over);
                else
                    make_separate(*over, *trn);
            }
        }
        if (overlapping_present)
            ++overlapping_no;
        else
            overlapping_no = 0;
    }

} // acmacs::tal::v3::DrawAATransitions::calculate_transion_boxes

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::report() const
{
    fmt::print("INFO: ==================== AA transitions ({}) ==================================================\n", transitions_.size());
    size_t max_id{0}, max_name{0};
    for (const auto& transition : transitions_) {
        max_id = std::max(max_id, fmt::format("{}", transition.node->node_id_).size());
        max_name = std::max(max_name, transition.node->aa_transitions_.display().size());
    }

    fmt::print("[\n");
    bool comma = false;
    for (const auto& transition : transitions_) {
        const auto node_id = fmt::format("\"{}\"", transition.node->node_id_);
        const auto name = fmt::format("\"{}\",", transition.node->aa_transitions_.display());
        if (comma)
            fmt::print(",\n");
        fmt::print("    {{\"node_id\": {:>{}s}, \"name\": {:<{}s} \"label\": {{\"offset\": [{:9.6f}, {:9.6f}], \"?box_size\": {:.6f}}}, \"?first-leaf\": \"{}\"}}", node_id, max_id + 2, name, max_name + 3,
                   transition.label.offset[0], transition.label.offset[1], transition.box.size, transition.node->first_leaf().seq_id);
        comma = true;
    }
    fmt::print("\n]\n");
    fmt::print("INFO: ============================================================================================\n", transitions_.size());

} // acmacs::tal::v3::DrawAATransitions::report

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::draw_transitions(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const
{
    calculate_boxes(surface, draw_tree);
    report();

    const auto text_line_interleave = parameters().text_line_interleave;

    for (const auto& transition : transitions_) {
        // if (!transition.node->aa_transitions_.find(seqdb::pos1_t{484}))
        //     continue;

        const auto names = transition.node->aa_transitions_.names();
        const Scaled text_size{transition.label.scale};

        //const PointCoordinates box_top_left{transition.at_edge_line.x() + transition.label.offset[0], transition.at_edge_line.y() + transition.label.offset[1]};
        // surface.rectangle(transition.box.origin, transition.box.size, GREY, Pixels{1});

        if (transition.label.tether.show) {
            const auto vspace = transition.name_sizes.front().height * text_line_interleave;
            PointCoordinates at_box{transition.box.center().x(), transition.box.top() - vspace};
            if (transition.box.top() <= transition.at_edge_line.y() && transition.box.bottom() >= transition.at_edge_line.y()) {
                if (transition.box.left() >= transition.at_edge_line.x())
                    at_box.x(transition.box.left());
                else
                    at_box.x(transition.box.right());
                at_box.y(transition.box.center().y());
            }
            else if (transition.box.bottom() < transition.at_edge_line.y())
                at_box.y(transition.box.bottom() + vspace);
            if (distance(transition.at_edge_line, at_box) > distance(transition.box.origin, transition.box.bottom_right()) * 0.2)
                surface.line(transition.at_edge_line, at_box, transition.label.tether.line.color, transition.label.tether.line.line_width);
        }

        auto pos_x = transition.box.left();
        auto pos_y = transition.box.top();
        for (auto [index, name] : acmacs::enumerate(names)) {
            if (index)
                pos_y += transition.name_sizes[index].height * (1.0 + text_line_interleave);
            else
                pos_y += transition.name_sizes[index].height;
            surface.text({pos_x + (transition.box.size.width - transition.name_sizes[index].width) / 2.0, pos_y}, name, transition.label.color, text_size, transition.label.text_style);
        }
    }

} // acmacs::tal::v3::DrawAATransitions::draw_transitions

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
