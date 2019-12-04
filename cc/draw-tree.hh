#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class DrawTree : public LayoutElement
    {
      public:
        DrawTree() : LayoutElement(0.7) {}

        void draw(acmacs::surface::Surface& surface) const override;

      private:
    };

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
