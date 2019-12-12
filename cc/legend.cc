#include "acmacs-tal/legend.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Legend::prepare()
{

} // acmacs::tal::v3::Legend::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::LegendWorldMap::draw(acmacs::surface::Surface& surface) const
{
    surface.text(parameters().offset, "LegendWorldMap", RED, parameters().size, TextStyle{}, RotationDegrees(30));

} // acmacs::tal::v3::LegendWorldMap::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
