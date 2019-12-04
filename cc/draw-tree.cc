#include "acmacs-tal/draw-tree.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawTree::draw(acmacs::surface::Surface& surface) const
{
    surface.circle({0.5 * width_to_height_ratio(), 0.5}, Scaled{0.3}, RED, Pixels{1.0});

} // acmacs::tal::v3::DrawTree::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
