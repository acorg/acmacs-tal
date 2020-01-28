#include "acmacs-tal/title.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Title::draw(acmacs::surface::Surface& surface, verbose /*verb*/) const
{
    const TextStyle text_style{};
    surface.text(parameters().offset, parameters().text, parameters().color, parameters().size, text_style, NoRotation);

} // acmacs::tal::v3::Title::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
