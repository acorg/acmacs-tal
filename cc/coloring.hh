#pragma once

#include "acmacs-base/color.hh"
#include "acmacs-base/flat-map.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Node;

    class Coloring
    {
      public:
        virtual ~Coloring() = default;

        virtual Color color(const Node& node) const = 0;
    };

    // ----------------------------------------------------------------------

    class ColoringUniform : public Coloring
    {
      public:
        ColoringUniform(Color color) : color_{color} {}

        Color color(const Node& node) const override;

        void set(Color color) { color_ = color; }

      private:
        Color color_;
    };

    // ----------------------------------------------------------------------

    class ColoringByContinent : public Coloring
    {
      public:
        ColoringByContinent() : colors_{continent_colors_dark()} {}
        Color color(const Node& node) const override;

        void set(std::string_view continent, Color color) { colors_.emplace_or_replace(std::string{continent}, color); }

      private:
        acmacs::flat_map_t<std::string, Color> colors_;
    };

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
