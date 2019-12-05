#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class DrawTree : public LayoutElement
    {
      public:
        DrawTree(Tal& tal) : LayoutElement(0.7), tal_{tal} {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

      private:
        Tal& tal_;
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
