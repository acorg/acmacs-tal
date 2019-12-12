#include "acmacs-draw/continent-map.hh"
#include "acmacs-tal/legend.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Legend::prepare()
{

} // acmacs::tal::v3::Legend::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::LegendContinentMap::draw(acmacs::surface::Surface& surface) const
{
    continent_map_draw(surface.subsurface(parameters().offset, parameters().size, continent_map_size(), false));

} // acmacs::tal::v3::LegendContinentMap::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
