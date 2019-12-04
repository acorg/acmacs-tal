#pragma once

#include "acmacs-draw/surface-cairo.hh"
#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    struct Margins
    {
        double left{0.025}, right{0.025}, top{0.025}, bottom{0.025}; // relative to height_
        // for debugging
        bool outline{false};
        Color outline_color{PINK};
        Pixels outline_width{2};
    };

    class Draw
    {
      public:
        void export_pdf(std::string_view filename) const;

        Margins& margins() { return margins_; }
        Layout& layout() { return layout_; }
        void prepare();

      private:
        double height_{1000.0};
        double width_to_height_ratio_{1.0};
        Margins margins_;
        Layout layout_;

        void set_width_to_height_ratio();
        void draw_outline(acmacs::surface::Surface& surface) const;
    };
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
