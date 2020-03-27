#include "acmacs-base/enumerate.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/clades.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/hz-sections.hh"

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Clades::clade_t::intersects(const clade_t& rhs) const
{
    for (const auto& sec_lhs : sections) {
        for (const auto& sec_rhs : rhs.sections) {
            const auto& [upper, lower] = sec_lhs.first->node_id.vertical < sec_rhs.first->node_id.vertical ? std::pair{sec_lhs, sec_rhs} : std::pair{sec_rhs, sec_lhs};
            if (upper.last->node_id.vertical > lower.first->node_id.vertical)
                return true;
        }
    }
    return false;

} // acmacs::tal::v3::Clades::clade_t::intersects

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::prepare(preparation_stage_t stage)
{
    if (!prepared_) {
        auto& layout = tal().draw().layout();
        time_series_to_the_left_ = layout.index_of(layout.find<TimeSeries>()) < layout.index_of(this);

        make_clades();
        if (width_to_height_ratio() <= 0.0 && number_of_slots())
            width_to_height_ratio() = (number_of_slots() + 1) * parameters_.slot.width;

    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::Clades::prepare

// ----------------------------------------------------------------------

const acmacs::tal::v3::Clades::CladeParameters& acmacs::tal::v3::Clades::parameters_for_clade(std::string_view name) const
{
    if (auto found = std::find_if(std::begin(parameters_.per_clade), std::end(parameters_.per_clade), [name](const auto& for_clade) { return for_clade.name == name; }); found != std::end(parameters_.per_clade))
        return *found;
    else
        return parameters_.all_clades;

} // acmacs::tal::v3::Clades::parameters_for_clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::make_clades()
{
    // fmt::print(stderr, "DEBUG Clades::make_clades\n");
    make_sections();
    set_slots();
    add_gaps_to_tree();
    add_separators_to_time_series();
    report_clades();

    if (auto hz_sections = tal().draw().layout().find<HzSections>(); hz_sections) {
        for (const auto& clade : clades_) {
            for (auto [section_no, clade_section] : acmacs::enumerate(clade.sections)) {
                hz_sections->add_section(HzSections::Section{HzSections::section_id_t{fmt::format("{}-{}", clade.name, section_no)}, clade_section.first, clade_section.last,
                                                             clade_section.display_name});
            }
        }
    }

} // acmacs::tal::v3::Clades::make_clades

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::make_sections()
{
    const auto& tree_clades = tal().tree().clades();
    for (const auto& tree_clade : tree_clades) {
        // AD_LOG(acmacs::log::clades, "tree clade {}", tree_clade.name);
        const auto& clade_param = parameters_for_clade(tree_clade.name);
        // fmt::print(stderr, "DEBUG parameters_for_clade \"{}\": slot:{} display_name:{} hidden:{}\n", tree_clade.name, clade_param.slot_no, clade_param.display_name, clade_param.hidden);
        if (!clade_param.hidden) {
            auto& clade = clades_.emplace_back(tree_clade.name);
            for (const auto& tree_section : tree_clade.sections) {
                auto& section = clade.sections.emplace_back(tree_section.first, tree_section.last, tree_clade.display_name);
                section.slot_no = clade_param.slot_no;
                section.label = clade_param.label;
                section.arrow = clade_param.arrow;
                section.horizontal_line = clade_param.horizontal_line;
                if (!clade_param.display_name.empty())
                    section.display_name = clade_param.display_name;
            }

            // merge sections
            for (auto section = std::begin(clade.sections), merge_to = section; section != std::end(clade.sections) && std::next(section) != std::end(clade.sections); ++section) {
                const auto next_section = std::next(section);
                if ((next_section->first->node_id.vertical - section->last->node_id.vertical) <= clade_param.section_inclusion_tolerance) {
                    merge_to->last = next_section->last;
                    next_section->first = nullptr;
                }
                else
                    merge_to = next_section;
            }
            clade.sections.erase(std::remove_if(std::begin(clade.sections), std::end(clade.sections), [](const auto& sec) { return sec.first == nullptr; }), std::end(clade.sections));

            // remove small sections
            const auto is_section_small = [tol = clade_param.section_exclusion_tolerance](const auto& sec) { return sec.size() <= tol; };
            if (const size_t num_small_sections = static_cast<size_t>(std::count_if(std::begin(clade.sections), std::end(clade.sections), is_section_small)); num_small_sections < clade.sections.size())
                clade.sections.erase(std::remove_if(std::begin(clade.sections), std::end(clade.sections), is_section_small), std::end(clade.sections));

            if (clade.sections.empty())
                clades_.erase(std::prev(clades_.end()));
        }
    }

} // acmacs::tal::v3::Clades::make_sections

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::set_slots()
{
    if (!clades_.empty()) {
        std::vector<clade_t*> clade_refs(clades_.size());
        std::transform(std::begin(clades_), std::end(clades_), std::begin(clade_refs), [](auto& clad) { return &clad; });
        // smallest clade first (by its longest section)
        std::sort(std::begin(clade_refs), std::end(clade_refs), [](const clade_t* c1, const clade_t* c2) {
            if (c1->sections.empty() || c2->sections.empty())
                return true;

            const auto cmp = [](const auto& s1, const auto& s2) { return s1.size() < s2.size(); };
            const auto longest1 = std::max_element(std::begin(c1->sections), std::end(c1->sections), cmp)->size();
            const auto longest2 = std::max_element(std::begin(c2->sections), std::end(c2->sections), cmp)->size();
            if (longest1 == longest2) {
                const auto sum = [](size_t acc, const auto& sec) { return acc + sec.size(); };
                const auto total1 = std::accumulate(std::begin(c1->sections), std::end(c1->sections), 0UL, sum);
                const auto total2 = std::accumulate(std::begin(c2->sections), std::end(c2->sections), 0UL, sum);
                return total1 < total2;
            }
            else
                return longest1 < longest2;
        });

        slot_no_t slot_no{0};
        for (auto clade = std::begin(clade_refs); clade != std::end(clade_refs); ++clade) {
            if ((*clade)->sections.front().slot_no == NoSlot) {
                for (auto prev_clade = std::begin(clade_refs); prev_clade != clade; ++prev_clade) {
                    if ((*clade)->intersects(**prev_clade) && (*prev_clade)->sections.front().slot_no == slot_no) {
                        ++slot_no;
                        break;
                    }
                }
                for (auto& section : (*clade)->sections)
                    section.slot_no = slot_no;
            }
        }
    }

} // acmacs::tal::v3::Clades::set_slots

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::add_gaps_to_tree()
{
    for (const auto& clade : clades_) {
        const auto& clade_param = parameters_for_clade(clade.name);
        for (const auto& section : clade.sections) {
            Tree::set_top_gap(*section.first, clade_param.tree_top_gap);
            Tree::set_bottom_gap(*section.last, clade_param.tree_bottom_gap);
        }
    }

} // acmacs::tal::v3::Clades::add_gaps_to_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::add_separators_to_time_series()
{
    if (auto* time_series = tal().draw().layout().find<TimeSeries>(); time_series) {
        for (const auto& clade : clades_) {
            const auto& clade_param = parameters_for_clade(clade.name);
            for (const auto& section : clade.sections) {
                AD_LOG(acmacs::log::clades, "\"{}\" \"{}\": {} .. {}", clade.name, section.display_name, section.first->seq_id, section.last ? section.last->seq_id : seq_id_t{});
                if (clade_param.time_series_top_separator)
                    time_series->add_horizontal_line_above(section.first, clade_param.horizontal_line, true);
                if (clade_param.time_series_bottom_separator) {
                    if (section.last->last_next_leaf)
                        time_series->add_horizontal_line_above(section.last->last_next_leaf, clade_param.horizontal_line, true);
                }
            }
        }
    }

} // acmacs::tal::v3::Clades::add_separators_to_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::report_clades()
{
    if (parameters_.report) {
        fmt::print(">>> ==================== Clades ({}) ==================================================\n", clades_.size());
        for (const auto& clade : clades_) {
            const auto& clade_param = parameters_for_clade(clade.name);
            fmt::print("Clade {} ({})    {{\"name\": \"{}\", \"display_name\": \"{}\", \"section_inclusion_tolerance\": {}, \"section_exclusion_tolerance\": {}, \"show\": {}}}\n", clade.name,
                       clade.sections.size(), clade.name, clade_param.display_name, clade_param.section_inclusion_tolerance, clade_param.section_exclusion_tolerance, !clade_param.hidden);
            for (size_t section_no = 0; section_no < clade.sections.size(); ++section_no) {
                const auto& section = clade.sections[section_no];
                fmt::print("  {} [{}] slot:{} {} {} .. {} {}\n", section.display_name, section.size(), section.slot_no, section.first->node_id, section.first->seq_id, section.last->node_id,
                           section.last->seq_id);
                if (section_no < (clade.sections.size() - 1))
                    fmt::print("   gap {}\n", clade.sections[section_no + 1].first->node_id.vertical - section.last->node_id.vertical - 1);
            }
        }
        fmt::print(">>> ===================================================================================\n", clades_.size());
    }

} // acmacs::tal::v3::Clades::report_clades

// ----------------------------------------------------------------------

size_t acmacs::tal::v3::Clades::number_of_slots() const
{
    if (clades_.empty())
        return 0;

    slot_no_t max_slot_no{0};
    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections)
            max_slot_no = std::max(max_slot_no, section.slot_no);
    }
    return *max_slot_no + 1;

} // acmacs::tal::v3::Clades::number_of_slots

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::draw(acmacs::surface::Surface& surface) const
{
    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto vertical_step = draw_tree->vertical_step();
    const auto& viewport = surface.viewport();

    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections) {
            const auto pos_x = time_series_to_the_left_ ?
                    (viewport.left() + parameters_.slot.width * (*section.slot_no + 1)) :
                    (viewport.right() - parameters_.slot.width * (*section.slot_no + 1));
            const auto pos_y_top = pos_y_above(*section.first, vertical_step);
            const auto pos_y_bottom = pos_y_below(*section.last, vertical_step);

            // arrow
            surface.double_arrow({pos_x, pos_y_top}, {pos_x, pos_y_bottom}, section.arrow.color, section.arrow.line_width, section.arrow.arrow_width);

            // horizontal lines
            const auto left = time_series_to_the_left_ ? viewport.left() : pos_x;
            const auto right = time_series_to_the_left_ ? pos_x : viewport.right();
            surface.line({left, pos_y_top}, {right, pos_y_top}, section.horizontal_line.color, section.horizontal_line.line_width, section.horizontal_line.dash);
            surface.line({left, pos_y_bottom}, {right, pos_y_bottom}, section.horizontal_line.color, section.horizontal_line.line_width, section.horizontal_line.dash);

            // label
            const Scaled label_size{parameters_.slot.width * section.label.scale};
            const auto text_size = surface.text_size(section.display_name, label_size, section.label.text_style);
            double vertical_pos;
            switch (section.label.vpos) {
                case vertical_position::top:
                    vertical_pos = vertical_step * section.first->cumulative_vertical_offset_ + section.label.offset[1] + text_size.height;
                    break;
                case vertical_position::middle:
                    vertical_pos = vertical_step * (section.first->cumulative_vertical_offset_ + section.last->cumulative_vertical_offset_) / 2.0 + section.label.offset[1] + text_size.height / 2.0;
                    break;
                case vertical_position::bottom:
                    vertical_pos = vertical_step * section.last->cumulative_vertical_offset_ + section.label.offset[1];
                    break;
            }
            const auto text_pos_x = time_series_to_the_left_ ? (pos_x + section.label.offset[0]) : (pos_x - text_size.width - section.label.offset[0]);
            surface.text({text_pos_x, vertical_pos}, section.display_name, section.label.color, label_size, section.label.text_style, section.label.rotation);

        }
    }

} // acmacs::tal::v3::Clades::draw

// ----------------------------------------------------------------------

acmacs::tal::v3::Clades::CladeParameters& acmacs::tal::v3::Clades::Parameters::find_or_add_pre_clade(std::string_view name)
{
    if (auto found = std::find_if(std::begin(per_clade), std::end(per_clade), [name](const auto& param) { return param.name == name; }); found != std::end(per_clade)) {
        return *found;
    }
    else {
        per_clade.push_back(all_clades);
        per_clade.back().name = name;
        return per_clade.back();
    }

} // acmacs::tal::v3::Clades::Parameters::find_or_add_pre_clade

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
