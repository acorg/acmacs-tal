#include "acmacs-tal/hz-sections.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        width_to_height_ratio() = 0.0;
        // fmt::print(stderr, "DEBUG: hz-sections prepare width_to_height_ratio: {}\n", width_to_height_ratio());
        // auto& layout = tal().draw().layout();
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::HzSections::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::draw(acmacs::surface::Surface& /*surface*/) const
{
    // fmt::print(stderr, "DEBUG: HzSections width_to_height_ratio: {}\n", width_to_height_ratio());

    for (const auto& section : sections_) {
        fmt::print(stderr, "DEBUG: hz-section \"{}\" {} {} \"{}\"\n", section.id, section.first ? section.first->seq_id : seq_id_t{}, section.last ? section.last->seq_id : seq_id_t{}, section.label);
    }

} // acmacs::tal::v3::HzSections::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
