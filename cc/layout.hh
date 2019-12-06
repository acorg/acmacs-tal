#pragma once

#include "acmacs-draw/surface.hh"
#include "acmacs-tal/coloring.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    enum class Position { normal, absolute };

    // ----------------------------------------------------------------------

    class LayoutElement;
    class DrawTree;

    class Layout
    {
      public:
        LayoutElement& add(std::unique_ptr<LayoutElement> element);

        double width_relative_to_height() const;
        void prepare();
        void draw(acmacs::surface::Surface& surface) const;

        DrawTree& draw_tree();

      private:
        std::vector<std::unique_ptr<LayoutElement>> elements_;

    };

    // ======================================================================

    struct DrawOutline
    {
        bool outline{false};
        Color outline_color{PINK};
        Pixels outline_width{2};

        void draw(acmacs::surface::Surface& surface) const;
    };

    // ----------------------------------------------------------------------

    class LayoutElement
    {
      public:
        LayoutElement(double width_to_height_ratio) : width_to_height_ratio_{width_to_height_ratio} {}
        virtual ~LayoutElement() = default;

        constexpr double width_to_height_ratio() const { return width_to_height_ratio_; }
        constexpr double& width_to_height_ratio() { return width_to_height_ratio_; }
        // constexpr void width_to_height_ratio(double whr) { width_to_height_ratio_ = whr; }
        constexpr DrawOutline& outline() { return outline_; }
        constexpr const DrawOutline& outline() const { return outline_; }

        virtual Position position() const { return Position::normal; }
        virtual void prepare() {}
        virtual void draw(acmacs::surface::Surface& surface) const = 0;

        virtual DrawTree* draw_tree() { return nullptr; }

      private:
        double width_to_height_ratio_;
        DrawOutline outline_;
    };

    // ----------------------------------------------------------------------

    class Gap : public LayoutElement
    {
      public:
        Gap() : LayoutElement(0.05) {}
        void draw(acmacs::surface::Surface& /*surface*/) const override {}
    };

    // ----------------------------------------------------------------------

    class LayoutElementWithColoring : public LayoutElement
    {
      public:
        LayoutElementWithColoring(double width_to_height_ratio) : LayoutElement(width_to_height_ratio), coloring_{std::make_unique<ColoringUniform>(CYAN)} {}

        void coloring(std::unique_ptr<Coloring> coloring) { coloring_ = std::move(coloring); }

      protected:
        Color color(const Node& node) const { return coloring_->color(node); }

      private:
        std::unique_ptr<Coloring> coloring_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
