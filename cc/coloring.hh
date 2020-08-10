#pragma once

#include "acmacs-base/color-continent.hh"
#include "acmacs-base/flat-map.hh"
#include "seqdb-3/sequence.hh"

// ----------------------------------------------------------------------

namespace acmacs::surface
{
    class Surface;
}

namespace acmacs::tal::inline v3
{
    class Node;

    class coloring_error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    class Coloring
    {
      public:
        virtual ~Coloring() = default;

        virtual void prepare(const Node& /*node*/) {}
        virtual void prepare() {} // to be called after coloring prepare(Node) for each node
        virtual Color color(const Node& node) const = 0;
        virtual std::string report() const = 0;
        virtual std::string_view legend_type() const { return "none"; }
    };

    // ----------------------------------------------------------------------

    class ColoringUniform : public Coloring
    {
      public:
        ColoringUniform(Color color) : color_{color} {}

        Color color(const Node& node) const override;
        std::string report() const override;

        void set(Color color) { color_ = color; }

      private:
        Color color_;
    };

    // ----------------------------------------------------------------------

    class ColoringByContinent : public Coloring
    {
      public:
        ColoringByContinent() : colors_{continent_colors()} {}
        Color color(const Node& node) const override;
        std::string report() const override;
        std::string_view legend_type() const override { return "world-map"; }

        void set(std::string_view continent, Color color) { colors_.emplace_or_replace(continent, color); }

      private:
        continent_colors_t colors_;
    };

    // ----------------------------------------------------------------------

    class ColoringByPosBase : public Coloring
    {
      public:
        struct color_count_t
        {
            Color color{PINK};
            size_t count{0};
        };

        ColoringByPosBase(acmacs::seqdb::pos1_t pos) : pos_{pos} {}
        Color color(const Node& node) const override;
        std::string report() const override;
        constexpr acmacs::seqdb::pos1_t pos() const { return pos_; }
        constexpr const auto& colors() const { return colors_; }
        size_t total_count() const;

        enum class legend_layout { horizontal, vertical };
        void draw_legend(acmacs::surface::Surface& surface, const PointCoordinates& origin, legend_layout layout, Color title_color, Scaled text_size, double interleave,
                                                     bool show_count, double count_scale, Color count_color) const;

      protected:
        constexpr auto& colors() { return colors_; }
        void sort_colors_by_frequency();

      private:
        acmacs::seqdb::pos1_t pos_;
        acmacs::small_map_with_unique_keys_t<char, color_count_t> colors_;

    }; // class ColoringByPosBase

    // ----------------------------------------------------------------------

    class ColoringByPosAAColors : public ColoringByPosBase
    {
      public:
        using ColoringByPosBase::ColoringByPosBase;
        std::string_view legend_type() const override { return "color-by-pos-aa-colors"; }
        void prepare(const Node& node) override;
        void prepare() override; // to be called after coloring prepare(Node) for each node

    }; // class ColoringByPosAAColors

    // ----------------------------------------------------------------------

    class ColoringByPosAAFrequency : public ColoringByPosBase
    {
      public:
        using ColoringByPosBase::ColoringByPosBase;
        std::string_view legend_type() const override { return "color-by-pos-aa-frequency"; }
        void prepare(const Node& node) override;
        void prepare() override; // to be called after coloring prepare(Node) for each node
        void add_color(Color color) { color_order_.push_back(color); }

      private:
        std::vector<Color> color_order_;

    }; // class ColoringByPosAAFrequency

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
