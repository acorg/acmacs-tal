#pragma once

#include "acmacs-base/debug.hh"
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
    class DrawOnTree;
    class TimeSeries;
    class Clades;
    class DrawAATransitions;
    class Title;
    class LegendContinentMap;
    class Gap;
    class DashBar;
    class DashBarClades;
    class HzSections;
    class HzSectionMarker;

    using LayoutElementId = named_string_t<struct LayoutElementId_tag>;

    using preparation_stage_t = size_t;

    class Layout
    {
      public:
        LayoutElement& add(std::unique_ptr<LayoutElement> element);

        double width_relative_to_height() const;
        void prepare();
        void draw(acmacs::surface::Surface& surface) const;

        template <typename Element> const Element* find(const LayoutElementId& id = {}) const;
        template <typename Element> Element* find(const LayoutElementId& id = {});

        // counts elements of the same type
        size_t count(LayoutElement& element) const;

        template <typename Element> void prepare_element(preparation_stage_t stage);

        size_t index_of(const LayoutElement* look_for) const;

        const DrawTree* find_draw_tree(bool throw_error = true) const; // throws error or prints warning if not found

        void draw_horizontal_line_between(const LayoutElement* elt1, const LayoutElement* elt2, double y_pos, Color line_color, Pixels line_width) const;

      private:
        std::vector<std::unique_ptr<LayoutElement>> elements_;
        mutable surface::Surface* surface_{nullptr};
    };

    extern template const DrawTree* Layout::find<DrawTree>(const LayoutElementId& id) const;
    extern template const DrawOnTree* Layout::find<DrawOnTree>(const LayoutElementId& id) const;
    extern template const TimeSeries* Layout::find<TimeSeries>(const LayoutElementId& id) const;
    extern template const Clades* Layout::find<Clades>(const LayoutElementId& id) const;
    extern template const DrawAATransitions* Layout::find<DrawAATransitions>(const LayoutElementId& id) const;
    extern template const Title* Layout::find<Title>(const LayoutElementId& id) const;
    extern template const LegendContinentMap* Layout::find<LegendContinentMap>(const LayoutElementId& id) const;
    extern template const Gap* Layout::find<Gap>(const LayoutElementId& id) const;
    extern template const DashBar* Layout::find<DashBar>(const LayoutElementId& id) const;
    extern template const DashBarClades* Layout::find<DashBarClades>(const LayoutElementId& id) const;
    extern template const HzSections* Layout::find<HzSections>(const LayoutElementId& id) const;
    extern template const HzSectionMarker* Layout::find<HzSectionMarker>(const LayoutElementId& id) const;

    extern template DrawTree* Layout::find<DrawTree>(const LayoutElementId& id);
    extern template DrawOnTree* Layout::find<DrawOnTree>(const LayoutElementId& id);
    extern template TimeSeries* Layout::find<TimeSeries>(const LayoutElementId& id);
    extern template Clades* Layout::find<Clades>(const LayoutElementId& id);
    extern template DrawAATransitions* Layout::find<DrawAATransitions>(const LayoutElementId& id);
    extern template Title* Layout::find<Title>(const LayoutElementId& id);
    extern template LegendContinentMap* Layout::find<LegendContinentMap>(const LayoutElementId& id);
    extern template Gap* Layout::find<Gap>(const LayoutElementId& id);
    extern template DashBar* Layout::find<DashBar>(const LayoutElementId& id);
    extern template DashBarClades* Layout::find<DashBarClades>(const LayoutElementId& id);
    extern template HzSections* Layout::find<HzSections>(const LayoutElementId& id);
    extern template HzSectionMarker* Layout::find<HzSectionMarker>(const LayoutElementId& id);

    extern template void Layout::prepare_element<DrawTree>(preparation_stage_t stage);
    extern template void Layout::prepare_element<TimeSeries>(preparation_stage_t stage);
    extern template void Layout::prepare_element<Clades>(preparation_stage_t stage);
    extern template void Layout::prepare_element<HzSections>(preparation_stage_t stage);

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
        virtual void prepare(preparation_stage_t stage) { prepared_ = stage; }
        virtual void draw(acmacs::surface::Surface& surface) const = 0;

        double pos_y_above(const Node& node, double vertical_step) const;
        double pos_y_below(const Node& node, double vertical_step) const;

        constexpr const LayoutElementId& id() const { return id_; }
        void id(const LayoutElementId& a_id) { id_ = a_id; }

        // ----------------------------------------------------------------------

        struct LineParameters
        {
            Color color{BLACK};
            Pixels line_width{1.0};
            surface::Dash dash{surface::Dash::NoDash};
        };

        struct LineWithOffsetParameters : public LineParameters
        {
            std::array<PointCoordinates, 2> offset{PointCoordinates{0.0, 0.0}, PointCoordinates{0.0, 0.0}}; // relative to node or {absolute_x, node-y}
            std::optional<double> absolute_x;
        };

        struct WorldMapDotParameters
        {
            PointCoordinates coordinates{0.0, 0.0}; // {lat, long}: {0,0} - middle Africa, {-33.87,151.21} - Sydney, {49.25, -123.1} - Vancouver
            Color outline{WHITE};
            Color fill{BLACK};
            Pixels outline_width{1.0};
            Pixels size{3.0};
        };

        enum class vertical_position { top, middle, bottom };
        enum class horizontal_position { left, middle, right };

        struct LabelTetherParameters
        {
            bool show{false};
            LineParameters line;
        };

        struct LabelParameters
        {
            Color color{BLACK};
            double scale{0.7}; // relative to parameters_.slot.width
            vertical_position vpos{vertical_position::middle};
            horizontal_position hpos{horizontal_position::left};
            std::array<double, 2> offset{0.004, 0.0}; // relative to the area height
            std::string text;
            Rotation rotation{NoRotation};
            LabelTetherParameters tether;
            TextStyle text_style;
        };

        struct DashParameters
        {
            double width{0.5}; // fraction of slot width
            Pixels line_width{0.5};
        };

        struct TextParameters
        {
            std::string text;
            PointCoordinates offset{0.0, 0.0}; // relative to node or {absolute_x, node-y}
            std::optional<double> absolute_x;
            Color color{BLACK};
            Scaled size{0.007};
        };

      protected:
        preparation_stage_t prepared_{0};
        constexpr const Tal& tal() const { return tal_; }
        constexpr Tal& tal() { return tal_; }

      private:
        Tal& tal_;
        double width_to_height_ratio_;
        DrawOutline outline_;
        LayoutElementId id_;
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
