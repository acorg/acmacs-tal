#include "acmacs-draw/continent-map.hh"
#include "acmacs-tal/legend.hh"
#include "acmacs-tal/coloring.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::LegendContinentMap::draw(acmacs::surface::Surface& surface, const Coloring&) const
{
    if (parameters().show) {
        auto& map_surface = surface.subsurface(parameters().offset, parameters().size, continent_map_size(), false);
        continent_map_draw(map_surface);
        // map_surface.rectangle(map_surface.viewport().origin, map_surface.viewport().size, PINK, Pixels{2});

        const auto latitude = [height = map_surface.viewport().size.height](double lat) { return (90.0 - lat) / 180.0 * height; };
        const auto longitude = [width = map_surface.viewport().size.width](double longi) { return (180.0 + longi) / 360.0 * width; };

        if (parameters().equator.color != TRANSPARENT && parameters().equator.line_width.value() > 0.0)
            map_surface.line(map_surface.viewport().left_center(), map_surface.viewport().right_center(), parameters().equator.color, parameters().equator.line_width, parameters().equator.dash);
        if (parameters().tropics.color != TRANSPARENT && parameters().tropics.line_width.value() > 0.0) {
            const auto tropic_of_cancer_y = latitude(23.43668);
            map_surface.line({map_surface.viewport().left(), tropic_of_cancer_y}, {map_surface.viewport().right(), tropic_of_cancer_y}, parameters().tropics.color, parameters().tropics.line_width,
                             parameters().tropics.dash);
            const auto tropic_of_capricorn_y = latitude(-23.43668);
            map_surface.line({map_surface.viewport().left(), tropic_of_capricorn_y}, {map_surface.viewport().right(), tropic_of_capricorn_y}, parameters().tropics.color,
                             parameters().tropics.line_width, parameters().tropics.dash);
        }

        for (const auto& dot : parameters().dots) {
            map_surface.circle_filled({longitude(dot.coordinates.y()), latitude(dot.coordinates.x())}, dot.size, AspectNormal, NoRotation, dot.outline, dot.outline_width,
                                      acmacs::surface::Dash::NoDash, dot.fill);
        }
    }

} // acmacs::tal::v3::LegendContinentMap::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::LegendColoredByPos::draw(acmacs::surface::Surface& surface, const Coloring& coloring) const
{
    if (parameters().show) {
        if (const auto* coloring_by_pos = dynamic_cast<const ColoringByPosBase*>(&coloring); coloring_by_pos) {
            coloring_by_pos->draw_legend(surface, parameters().offset + Size{0.0, *parameters().text_size}, ColoringByPosBase::legend_layout::vertical, parameters().title_color,
                                         parameters().text_size, parameters().interleave, parameters().show_count ? ColoringByPosBase::legend_show_count::yes : ColoringByPosBase::legend_show_count::no,
                                         parameters().show_pos ? ColoringByPosBase::legend_show_pos::yes : ColoringByPosBase::legend_show_pos::no, parameters().count_scale, parameters().count_color);
        }
        else
            AD_WARNING("legend with coloring by pos requested but tree is not colored by pos, cannot obtain data for the legend");
    }

} // acmacs::tal::v3::LegendColoredByPos::draw

// ----------------------------------------------------------------------
