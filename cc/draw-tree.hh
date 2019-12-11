#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class DrawTree : public LayoutElementWithColoring
    {
      public:
        DrawTree(Tal& tal) : LayoutElementWithColoring(tal, 0.7) {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        constexpr double vertical_step() const { return vertical_step_; }

      private:
        const double height_{1.0};
        double vertical_step_{0};
        double horizontal_step_{0};
    };

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
