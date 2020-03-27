#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        width_to_height_ratio() = 0.0;
        update_from_parameters();
        sort();
        set_prefix();
        set_aa_transitions();
        add_gaps_to_tree();
        add_separators_to_time_series();
        // fmt::print(stderr, "DEBUG hz-sections prepare width_to_height_ratio: {}\n", width_to_height_ratio());
        // auto& layout = tal().draw().layout();
        if (parameters().report)
            report();
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::HzSections::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::update_from_parameters()
{
    const auto& tree = tal().tree();
    for (const auto& section_data : parameters().sections) {
        auto& section = find_add_section(sections_, section_data.id);

        if (!section_data.first.empty())
            section.first = tree.find_node_by_seq_id(section_data.first);
        else if (!section.first)
            section.first = tree.first_prev_leaf;
        while (section.first->leaf_pos != Tree::leaf_position::first && section.first->first_prev_leaf)
            section.first = section.first->first_prev_leaf;

        if (!section_data.last.empty())
            section.last = tree.find_node_by_seq_id(section_data.last);
        else if (!section.last)
            section.last = tree.last_next_leaf;
        if (section.last->leaf_pos == Tree::leaf_position::first && section.last->first_prev_leaf)
            section.last = section.last->first_prev_leaf;
        while (section.last->leaf_pos != Tree::leaf_position::last && section.last->last_next_leaf)
            section.last = section.last->last_next_leaf;

        section.shown = section_data.shown;
        if (!section_data.label.empty())
            section.label = section_data.label;
    }

} // acmacs::tal::v3::HzSections::update_from_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::set_aa_transitions()
{
    auto pre = [this](const Node& node) {
        if (!node.aa_transitions_.empty()) {
            for (auto& section : sections_) {
                if (section.first && section.last && section.first->node_id.vertical >= node.first_prev_leaf->node_id.vertical && section.last->node_id.vertical <= node.last_next_leaf->node_id.vertical)
                    section.aa_transitions.add_or_replace(node.aa_transitions_);
            }
        }
    };
    tree::iterate_pre(tal().tree(), pre);

} // acmacs::tal::v3::HzSections::set_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::sort()
{
    std::sort(std::begin(sections_), std::end(sections_), [](const auto& s1, const auto& s2) { return s1.first->node_id.vertical < s2.first->node_id.vertical; });

} // acmacs::tal::v3::HzSections::sort

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::set_prefix()
{
    for (auto [no, section] : acmacs::enumerate(sections_))
        section.prefix.assign(1, 'A' + static_cast<char>(no));

} // acmacs::tal::v3::HzSections::set_prefix

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::add_gaps_to_tree()
{
    for (const auto& section : sections_) {
        if (section.shown) {
            Tree::set_top_gap(*section.first, parameters().tree_top_gap);
            Tree::set_bottom_gap(*section.last, parameters().tree_bottom_gap);
        }
    }

} // acmacs::tal::v3::HzSections::add_gaps_to_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::add_separators_to_time_series()
{
    if (auto* time_series = tal().draw().layout().find<TimeSeries>(); time_series) {
        for (const auto& section : sections_) {
            if (section.shown) {
                const auto warn_if_present = section.id.size() < 3 || section.id[section.id.size() - 2] != '-';
                time_series->add_horizontal_line_above(section.first, parameters().line, warn_if_present);
                if (section.last->last_next_leaf) {
                    // fmt::print(stderr, "DEBUG add_horizontal_line_above: {} --> {}\n", section.last->seq_id, section.last->last_next_leaf->seq_id);
                    time_series->add_horizontal_line_above(section.last->last_next_leaf, parameters().line, warn_if_present);
                }
            }
        }
    }

} // acmacs::tal::v3::HzSections::add_separators_to_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::draw(acmacs::surface::Surface& /*surface*/) const
{
    // fmt::print(stderr, "DEBUG HzSections width_to_height_ratio: {}\n", width_to_height_ratio());


} // acmacs::tal::v3::HzSections::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::report() const
{
    size_t longest_id{0}, longest_first{0}, longest_last{0};
    for (const auto& section : sections_) {
        longest_id = std::max(longest_id, section.id.size());
        if (section.first)
            longest_first = std::max(longest_first, section.first->seq_id.size());
        if (section.last)
            longest_last = std::max(longest_last, section.last->seq_id.size());
    }

    fmt::print(stderr, "HZ sections ({})\n", sections_.size());

    bool hidden_present{false};
    for (const auto& section : sections_) {
        if (section.shown)
            fmt::print(stderr, "    {:1s}. {:{}s} {:{}s} {:{}s} {} label:\"{}\"\n", section.prefix, fmt::format("\"{}\"", section.id), longest_id + 2, section.first ? section.first->seq_id : seq_id_t{}, longest_first, section.last ? section.last->seq_id : seq_id_t{}, longest_last, section.aa_transitions, section.label);
        else
            hidden_present = true;
    }
    if (hidden_present) {
        fmt::print(stderr, "DEBUG hidden hz_sections\n");
        for (const auto& section : sections_) {
            if (!section.shown)
                fmt::print(stderr, "    {:1s}. {:{}s} {:{}s} {:{}s} {} label:\"{}\"\n", section.prefix, fmt::format("\"{}\"", section.id), longest_id + 2, section.first ? section.first->seq_id : seq_id_t{}, longest_first, section.last ? section.last->seq_id : seq_id_t{}, longest_last, section.aa_transitions, section.label);
        }
    }

} // acmacs::tal::v3::HzSections::report

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSectionMarker::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        tal().draw().layout().prepare_element<HzSections>(stage);
        if (const auto* hz_sections = tal().draw().layout().find<HzSections>(); !hz_sections || hz_sections->sections().empty())
            width_to_height_ratio() = 0.0;
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::HzSectionMarker::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSectionMarker::draw(acmacs::surface::Surface& surface) const
{
    if (const auto* hz_sections = tal().draw().layout().find<HzSections>(); hz_sections && !hz_sections->sections().empty()) {
        if (const auto* draw_tree = tal().draw().layout().find_draw_tree(); draw_tree) {
            const auto& viewport = surface.viewport();
            const auto* time_series = tal().draw().layout().find<TimeSeries>();
            for (const auto& section : hz_sections->sections()) {
                const auto pos_y_top = pos_y_above(*section.first, draw_tree->vertical_step());
                const auto pos_y_bottom = pos_y_below(*section.last, draw_tree->vertical_step());
                const std::vector<PointCoordinates> path{{viewport.left(), pos_y_top}, {viewport.right(), pos_y_top}, {viewport.right(), pos_y_bottom}, {viewport.left(), pos_y_bottom}};
                surface.path_outline(std::begin(path), std::end(path), parameters().line.color, parameters().line.line_width);
                if (time_series) {
                    tal().draw().layout().draw_horizontal_line_between(time_series, this, pos_y_top, parameters().line.color, parameters().line.line_width);
                    tal().draw().layout().draw_horizontal_line_between(time_series, this, pos_y_bottom, parameters().line.color, parameters().line.line_width);
                }
            }
        }
    }

} // acmacs::tal::v3::HzSectionMarker::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
