#pragma once

#include "acmacs-draw/surface-cairo.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    enum class Position { normal, absolute };

    class LayoutElement
    {
      public:
        LayoutElement(double width_to_height_ratio) : width_to_height_ratio_{width_to_height_ratio} {}
        virtual ~LayoutElement() = default;

        constexpr double width_to_height_ratio() const { return width_to_height_ratio_; }
        constexpr void width_to_height_ratio(double whr) { width_to_height_ratio_ = whr; }
        virtual Position position() const { return Position::normal; }
        virtual void draw(acmacs::surface::Surface& surface) const = 0;

      private:
        double width_to_height_ratio_;
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
