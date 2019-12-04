#pragma once

#include "acmacs-draw/surface-cairo.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class LayoutElement
    {
      public:
        virtual ~LayoutElement() = default;
    };

    // ----------------------------------------------------------------------

    class Layout
    {
      public:
        void add(std::unique_ptr<LayoutElement> element);

        double width_relative_to_height() const;
        void draw(acmacs::surface::Surface& surface) const;

      private:
        std::vector<std::unique_ptr<LayoutElement>> elements_;

    };
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
