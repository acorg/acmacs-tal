#include "acmacs-tal/title.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/settings.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Title::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        // AD_DEBUG("Title::prepare \"{}\"", parameters().text);
        parameters().text = tal().settings().substitute_to_string(parameters().text);
        // AD_DEBUG("     --> \"{}\"", parameters().text);
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::Title::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Title::draw(acmacs::surface::Surface& surface) const
{
    const TextStyle text_style{};
    surface.text(parameters().offset, parameters().text, parameters().color, parameters().size, text_style, NoRotation);

} // acmacs::tal::v3::Title::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
