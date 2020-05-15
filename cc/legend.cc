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
            auto origin = parameters().offset + Size{0.0, *parameters().text_size};
            surface.text(origin, fmt::format("{}", coloring_by_pos->pos()), parameters().title_color, parameters().text_size);
            const auto total_percent = static_cast<double>(coloring_by_pos->total_count()) / 100.0;
            const auto count_text_size{parameters().text_size * parameters().count_scale};
            for (const auto& [aa, color_count] : coloring_by_pos->colors()) {
                origin.y(origin.y() + *parameters().text_size * (1.0 + parameters().interleave));
                const auto aa_t{fmt::format("{}", aa)};
                surface.text(origin, aa_t, color_count.color, parameters().text_size);
                if (parameters().show_count) {
                    const auto [aa_height, aa_width] = surface.text_size(std::string(1, 'W') /*aa_t*/, parameters().text_size);
                    surface.text({origin.x() + aa_width * 1.1, origin.y() - aa_height + *count_text_size * 1.45}, fmt::format("{:.1f}%", static_cast<double>(color_count.count) / total_percent), parameters().count_color, count_text_size);
                    surface.text({origin.x() + aa_width * 1.1, origin.y()}, fmt::format("{}", color_count.count), parameters().count_color, count_text_size);
                }
            }
        }
        else
            AD_WARNING("legend with coloring by pos requested but tree is not colored by pos, cannot obtain data for the legend");
    }

} // acmacs::tal::v3::LegendColoredByPos::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
