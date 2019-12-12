#include "acmacs-tal/title.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Title::prepare()
{

} // acmacs::tal::v3::Title::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Title::draw(acmacs::surface::Surface& surface) const
{
    const TextStyle text_style{};
    surface.text(parameters().offset, parameters().display_name, parameters().color, parameters().size, text_style, NoRotation);

} // acmacs::tal::v3::Title::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
