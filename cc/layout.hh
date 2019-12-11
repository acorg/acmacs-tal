#pragma once

#include "acmacs-draw/surface.hh"
#include "acmacs-tal/coloring.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    enum class Position { normal, absolute };

    // ----------------------------------------------------------------------

    class Tal;
    class LayoutElement;
    class DrawTree;
    class TimeSeries;
    class Clades;

    class Layout
    {
      public:
        LayoutElement& add(std::unique_ptr<LayoutElement> element);

        double width_relative_to_height() const;
        void prepare();
        void draw(acmacs::surface::Surface& surface) const;

        template <typename Element> const Element* find() const;
        template <typename Element> void prepare_element();
        size_t index_of(const LayoutElement* look_for) const;

      private:
        std::vector<std::unique_ptr<LayoutElement>> elements_;
    };

    extern template const DrawTree* Layout::find<DrawTree>() const;
    extern template const TimeSeries* Layout::find<TimeSeries>() const;
    extern template const Clades* Layout::find<Clades>() const;

    extern template void Layout::prepare_element<DrawTree>();
    extern template void Layout::prepare_element<TimeSeries>();
    extern template void Layout::prepare_element<Clades>();

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
        LayoutElement(Tal& tal, double width_to_height_ratio) : tal_{tal}, width_to_height_ratio_{width_to_height_ratio} {}
        virtual ~LayoutElement() = default;

        constexpr double width_to_height_ratio() const { return width_to_height_ratio_; }
        constexpr double& width_to_height_ratio() { return width_to_height_ratio_; }
        // constexpr void width_to_height_ratio(double whr) { width_to_height_ratio_ = whr; }
        constexpr DrawOutline& outline() { return outline_; }
        constexpr const DrawOutline& outline() const { return outline_; }

        virtual Position position() const { return Position::normal; }
        virtual void prepare() { prepared_ = true; }
        virtual void draw(acmacs::surface::Surface& surface) const = 0;

        double pos_y_above(const Node& node, double vertical_step) const;
        double pos_y_below(const Node& node, double vertical_step) const;

      protected:
        bool prepared_{false};
        constexpr const Tal& tal() const { return tal_; }
        constexpr Tal& tal() { return tal_; }

      private:
        Tal& tal_;
        double width_to_height_ratio_;
        DrawOutline outline_;
    };

    // ----------------------------------------------------------------------

    class Gap : public LayoutElement
    {
      public:
        Gap(Tal& tal) : LayoutElement(tal, 0.05) {}

        void draw(acmacs::surface::Surface& /*surface*/) const override {}
    };

    // ----------------------------------------------------------------------

    class LayoutElementWithColoring : public LayoutElement
    {
      public:
        LayoutElementWithColoring(Tal& tal, double width_to_height_ratio) : LayoutElement(tal, width_to_height_ratio), coloring_{std::make_unique<ColoringUniform>(CYAN)} {}

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