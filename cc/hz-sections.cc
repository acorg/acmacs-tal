#include <numeric>
#include <algorithm>

#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::HzSection::aa_transitions_format() const
{
    if (label_aa_transitions)
        return *label_aa_transitions;
    else
        return fmt::format("{}", aa_transitions);

} // acmacs::tal::v3::HzSection::aa_transitions_format

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
        // AD_DEBUG("hz-sections prepare width_to_height_ratio: {}", width_to_height_ratio());
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
        if (section_data.label)
            section.label = *section_data.label;
        section.label_aa_transitions = section_data.label_aa_transitions;
    }

} // acmacs::tal::v3::HzSections::update_from_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::set_aa_transitions()
{
    auto pre = [this](const Node& node) {
        if (!node.hidden && !node.aa_transitions_.empty()) {
            for (auto& section : sections_) {
                if (!node.first_prev_leaf || !node.last_next_leaf)
                    AD_WARNING("hz section {}  node:{} {} node.first_prev_leaf:{} node.last_next_leaf:{}", section.id, node.node_id, node.seq_id, fmt::ptr(node.first_prev_leaf), fmt::ptr(node.last_next_leaf));
                if ((section.first ? section.first->node_id.vertical >= node.first_prev_leaf->node_id.vertical : true) && (section.last ? section.last->node_id.vertical <= node.last_next_leaf->node_id.vertical : true))
                    section.aa_transitions.add_or_replace(node.aa_transitions_);
            }
        }
    };

    tal().tree().set_first_last_next_node_id();
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
    char no{0};
    for (auto& section : sections_) {
        if (section.shown) {
            section.prefix.assign(1, 'A' + no);
            ++no;
        }
        else
            section.prefix.clear();
    }

} // acmacs::tal::v3::HzSections::set_prefix

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::add_gaps_to_tree()
{
    for (const auto& section : sections_) {
        if (section.shown) {
            // AD_DEBUG("hz section {} {} gaps top:{} bottom:{}", section.prefix, section.label, parameters().tree_top_gap, parameters().tree_bottom_gap);
            tal().tree().set_top_gap(*section.first, parameters().tree_top_gap);
            tal().tree().set_bottom_gap(*section.last, parameters().tree_bottom_gap);
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
                    // AD_DEBUG("add_horizontal_line_above: \"{}\" {} --> {}", section.label, section.last->seq_id, section.last->last_next_leaf->seq_id);
                    time_series->add_horizontal_line_above(section.last->last_next_leaf, parameters().line, warn_if_present);
                }
            }
        }
    }

} // acmacs::tal::v3::HzSections::add_separators_to_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::draw(acmacs::surface::Surface& /*surface*/) const
{
    // AD_DEBUG("HzSections width_to_height_ratio: {}", width_to_height_ratio());


} // acmacs::tal::v3::HzSections::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::report() const
{
    size_t longest_id{0}, longest_first{0}, longest_last{0}, longest_label{0}, longest_label_aa{0}, longest_substs{0};
    for (const auto& section : sections_) {
        longest_id = std::max(longest_id, section.id.size());
        if (section.first)
            longest_first = std::max(longest_first, section.first->seq_id.size());
        if (section.last)
            longest_last = std::max(longest_last, section.last->seq_id.size());
        longest_label = std::max(longest_label, section.label.size());
        longest_label_aa = std::max(longest_label_aa, section.label_aa_transitions ? section.label_aa_transitions->size() : section.aa_transitions.display_most_important(0).size());
        longest_substs = std::max(longest_substs, section.aa_transitions.display_most_important(0).size());
    }

    const auto print_section_entry = [longest_id, longest_first, longest_last, longest_label, longest_label_aa, longest_substs](const auto& section) {
        fmt::print(stderr, "    {{\"L\": \"{:1s}\", \"id\": {:{}s} \"F\": {:5d}, \"first\": {:{}s} \"L\": {:5d}, \"last\": {:{}s} \"show\": {:6s} \"label\": {:{}s} \"aa_transitions\": {:{}s} \"All transitions\": {:{}s}}},\n",
                   section.prefix,
                   fmt::format("\"{}\",", section.id), longest_id + 3, section.first ? section.first->node_id.vertical : node_id_t::value_type{0},
                   fmt::format("\"{}\",", section.first ? section.first->seq_id : seq_id_t{}), longest_first + 3, section.last ? section.last->node_id.vertical : node_id_t::value_type{0},
                   fmt::format("\"{}\",", section.last ? section.last->seq_id : seq_id_t{}), longest_last + 3,
                   fmt::format("{},", section.shown),
                   fmt::format("\"{}\",", section.label), longest_label + 3,
                   fmt::format("\"{}\",", section.aa_transitions_format()), longest_label_aa + 3,
                   fmt::format("\"{}\"", section.aa_transitions), longest_substs + 2
                   );
    };

    AD_INFO("HZ sections");
    fmt::print(stderr, "[\n");
    std::for_each(std::begin(sections_), std::end(sections_), print_section_entry);
    fmt::print(stderr, "]\n\n");

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
                if (section.shown) {
                    const auto pos_y_top = pos_y_above(*section.first, draw_tree->vertical_step());
                    const auto pos_y_bottom = pos_y_below(*section.last, draw_tree->vertical_step());
                    const std::vector<PointCoordinates> path{{viewport.left(), pos_y_top}, {viewport.right(), pos_y_top}, {viewport.right(), pos_y_bottom}, {viewport.left(), pos_y_bottom}};
                    surface.path_outline(std::begin(path), std::end(path), parameters().line.color, parameters().line.line_width);
                    if (time_series) {
                        tal().draw().layout().draw_horizontal_line_between(time_series, this, pos_y_top, parameters().line.color, parameters().line.line_width);
                        tal().draw().layout().draw_horizontal_line_between(time_series, this, pos_y_bottom, parameters().line.color, parameters().line.line_width);
                    }
                    const auto prefix_size = surface.text_size(section.prefix, parameters().label_size);
                    surface.rectangle_filled({viewport.right() - prefix_size.width * 0.7, pos_y_top + prefix_size.height * 0.5}, {prefix_size.width * 1.4, prefix_size.height * 2.0},
                                             parameters().line.color, Pixels{0}, WHITE);
                    surface.text({viewport.right() - prefix_size.width * 0.5, pos_y_top + prefix_size.height * 2.0}, section.prefix, parameters().label_color, parameters().label_size);
                }
            }
        }
    }

} // acmacs::tal::v3::HzSectionMarker::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
