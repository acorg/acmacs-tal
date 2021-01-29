#pragma once

#include "acmacs-base/color.hh"
#include "acmacs-base/size-scale.hh"
#include "acmacs-draw/surface-line.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3::parameters
{
    struct Line
    {
        Color color{BLACK};
        Pixels line_width{1.0};
        surface::Dash dash{surface::Dash::NoDash};
    };

    struct LineWithOffset : public Line
    {
        std::array<PointCoordinates, 2> offset{PointCoordinates{0.0, 0.0}, PointCoordinates{0.0, 0.0}}; // relative to node or {absolute_x, node-y}
        std::optional<double> absolute_x;
    };

    enum class vertical_position { top, middle, bottom };
    enum class horizontal_position { left, middle, right };

    struct LabelTether
    {
        bool show{false};
        parameters::Line line{};
    };

    struct Label
    {
        Color color{BLACK};
        double scale{0.7}; // relative to parameters_.slot.width
        vertical_position vpos{vertical_position::middle};
        horizontal_position hpos{horizontal_position::left};
        std::array<double, 2> offset{0.004, 0.0}; // relative to the area height
        std::string text{};
        Rotation rotation{NoRotation};
        LabelTether tether{};
        TextStyle text_style{};
        bool show{true};
    };

    struct Dash
    {
        double width{0.5}; // fraction of slot width
        Pixels line_width{0.5};
    };

    struct Text
    {
        std::string text;
        PointCoordinates offset{0.0, 0.0}; // relative to node or {absolute_x, node-y}
        std::optional<double> absolute_x;
        Color color{BLACK};
        Scaled size{0.007};
    };

} // namespace acmacs::tal::inline v3::parameters

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
