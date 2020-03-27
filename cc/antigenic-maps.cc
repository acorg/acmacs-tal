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
        width_to_height_ratio() = columns_ / 3.0;
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
        const auto map_size = viewport.size.height / rows_;
        const auto* hz_sections = tal().draw().layout().find<HzSections>();
        for (auto [section_no, section] : acmacs::enumerate(hz_sections->sections())) {
            const auto left = (section_no % columns_) * map_size;
            const auto top = (section_no / columns_) * map_size;
            surface.rectangle({left, top}, {map_size, map_size}, BLUE, Pixels{1});
        }
    }

} // acmacs::tal::v3::AntigenicMaps::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: