#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    struct Margins
    {
        double left{0.025}, right{0.0}, top{0.025}, bottom{0.025}; // relative to height_
        // for debugging
    };

    class Draw
    {
      public:
        void reset();
        void prepare();
        void export_pdf(std::string_view filename) const;

        constexpr Margins& margins() { return margins_; }
        constexpr Layout& layout() { return layout_; }
        constexpr const Layout& layout() const { return layout_; }
        constexpr DrawOutline& outline() { return outline_; }
        constexpr const DrawOutline& outline() const { return outline_; }

        constexpr auto canvas_height() const { return height_; }
        void canvas_height(double height);

      private:
        double height_{1000.0};
        double width_to_height_ratio_{1.0};
        Margins margins_;
        Layout layout_;
        DrawOutline outline_;

        void set_width_to_height_ratio();
    };
}

// ----------------------------------------------------------------------
