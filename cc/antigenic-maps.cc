#include "acmacs-base/enumerate.hh"
#include "acmacs-tal/antigenic-maps.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/hz-sections.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        tal().draw().layout().prepare_element<HzSections>(stage);
        columns_rows();
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::AntigenicMaps::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::columns_rows()
{
    if (const auto* hz_sections = tal().draw().layout().find<HzSections>(); hz_sections && !hz_sections->sections().empty()) {
        const auto num_sections = hz_sections->sections().size();
        if (parameters().columns == 0)
            columns_ = num_sections / 3 + (num_sections % 3 ? 1 : 0);
        else
            columns_ = parameters().columns;
        // if (columns_ < 2)
        //     rows_ = num_sections;
        // else
        rows_ = 3;
        width_to_height_ratio() = static_cast<double>(columns_) / 3.0;
    }
    else {
        columns_ = 0;
        rows_ = 0;
        width_to_height_ratio() = 0.0;
    }

} // acmacs::tal::v3::AntigenicMaps::columns_rows

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::draw(acmacs::surface::Surface& surface) const
{
    if (columns_ && rows_) {
        const auto& viewport = surface.viewport();
        const auto gap = surface.convert(Pixels{parameters().gap_between_maps}).value();
        const auto map_size = viewport.size.height / static_cast<double>(rows_) - gap * static_cast<double>(rows_ - 1) / static_cast<double>(rows_);
        const auto* hz_sections = tal().draw().layout().find<HzSections>();
        for (auto [section_no, section] : acmacs::enumerate(hz_sections->sections())) {
            const auto left = static_cast<double>(section_no % columns_) * (map_size + gap);
            const auto top = static_cast<double>(section_no / columns_) * (map_size + gap);
            draw_map(surface.subsurface({left, top}, Scaled{map_size}, Size{1.0, 1.0}, false), section);
        }
    }

} // acmacs::tal::v3::AntigenicMaps::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::draw_map(acmacs::surface::Surface& surface, const HzSection& section) const
{
    surface.rectangle(surface.viewport().origin, surface.viewport().size, BLUE, Pixels{1});

} // acmacs::tal::v3::AntigenicMaps::draw_map

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
