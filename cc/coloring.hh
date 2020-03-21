#pragma once

#include "acmacs-base/color.hh"
#include "acmacs-base/flat-map.hh"
#include "seqdb-3/sequence.hh"

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

        void set(std::string_view continent, Color color) { colors_.emplace_or_replace(continent, color); }

      private:
        continent_colors_t colors_;
    };

    // ----------------------------------------------------------------------

    class ColoringByPos : public Coloring
    {
      public:
        ColoringByPos(acmacs::seqdb::pos1_t pos) : pos_{pos} {}
        Color color(const Node& node) const override;

      private:
        acmacs::seqdb::pos1_t pos_;
        mutable acmacs::small_map_with_unique_keys_t<char, Color> colors_;
    };

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
