#pragma once

#include "acmacs-base/log.hh"
#include "acmacs-draw/surface.hh"
#include "acmacs-tal/coloring.hh"
#include "acmacs-tal/parameters.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    enum class Position { normal, absolute };
    enum class throw_error { no, yes };

    // ----------------------------------------------------------------------

    class Tal;
    class LayoutElement;
    class DrawTree;
    class DrawOnTree;
    class TimeSeries;
    class TimeSeriesWithShift;
    class Clades;
    class DrawAATransitions;
    class Title;
    // class LegendContinentMap;
    class Gap;
    class DashBar;
    class DashBarAAAt;
    class DashBarClades;
    class HzSections;
    class HzSectionMarker;
    class AntigenicMaps;

    using LayoutElementId = named_string_t<struct LayoutElementId_tag>;

    using preparation_stage_t = size_t;

    class Layout
    {
      public:
        LayoutElement& add(std::unique_ptr<LayoutElement> element);

        double width_relative_to_height() const;
        void reset();
        void prepare();
        void draw(acmacs::surface::Surface& surface) const;

        template <typename Element> const Element* find(const LayoutElementId& id = {}) const;
        template <typename Element> Element* find(const LayoutElementId& id = {});

        // counts elements of the same type
        size_t count(LayoutElement& element) const;

        template <typename Element> void prepare_element(preparation_stage_t stage);

        size_t index_of(const LayoutElement* look_for) const;

        const DrawTree* find_draw_tree(throw_error te = throw_error::yes) const; // throws error or prints warning if not found
        DrawTree* find_draw_tree(throw_error te = throw_error::yes); // throws error or prints warning if not found

        void draw_horizontal_line_between(const LayoutElement* elt1, const LayoutElement* elt2, double y_pos, Color line_color, Pixels line_width) const;

      private:
        std::vector<std::unique_ptr<LayoutElement>> elements_;
        mutable surface::Surface* surface_{nullptr};
    };

    extern template const DrawTree* Layout::find<DrawTree>(const LayoutElementId& id) const;
    extern template const DrawOnTree* Layout::find<DrawOnTree>(const LayoutElementId& id) const;
    extern template const TimeSeries* Layout::find<TimeSeries>(const LayoutElementId& id) const;
    extern template const TimeSeriesWithShift* Layout::find<TimeSeriesWithShift>(const LayoutElementId& id) const;
    extern template const Clades* Layout::find<Clades>(const LayoutElementId& id) const;
    extern template const DrawAATransitions* Layout::find<DrawAATransitions>(const LayoutElementId& id) const;
    extern template const Title* Layout::find<Title>(const LayoutElementId& id) const;
    // extern template const LegendContinentMap* Layout::find<LegendContinentMap>(const LayoutElementId& id) const;
    extern template const Gap* Layout::find<Gap>(const LayoutElementId& id) const;
    extern template const DashBar* Layout::find<DashBar>(const LayoutElementId& id) const;
    extern template const DashBarAAAt* Layout::find<DashBarAAAt>(const LayoutElementId& id) const;
    extern template const DashBarClades* Layout::find<DashBarClades>(const LayoutElementId& id) const;
    extern template const HzSections* Layout::find<HzSections>(const LayoutElementId& id) const;
    extern template const HzSectionMarker* Layout::find<HzSectionMarker>(const LayoutElementId& id) const;
    extern template const AntigenicMaps* Layout::find<AntigenicMaps>(const LayoutElementId& id) const;

    extern template DrawTree* Layout::find<DrawTree>(const LayoutElementId& id);
    extern template DrawOnTree* Layout::find<DrawOnTree>(const LayoutElementId& id);
    extern template TimeSeries* Layout::find<TimeSeries>(const LayoutElementId& id);
    extern template TimeSeriesWithShift* Layout::find<TimeSeriesWithShift>(const LayoutElementId& id);
    extern template Clades* Layout::find<Clades>(const LayoutElementId& id);
    extern template DrawAATransitions* Layout::find<DrawAATransitions>(const LayoutElementId& id);
    extern template Title* Layout::find<Title>(const LayoutElementId& id);
    // extern template LegendContinentMap* Layout::find<LegendContinentMap>(const LayoutElementId& id);
    extern template Gap* Layout::find<Gap>(const LayoutElementId& id);
    extern template DashBar* Layout::find<DashBar>(const LayoutElementId& id);
    extern template DashBarAAAt* Layout::find<DashBarAAAt>(const LayoutElementId& id);
    extern template DashBarClades* Layout::find<DashBarClades>(const LayoutElementId& id);
    extern template HzSections* Layout::find<HzSections>(const LayoutElementId& id);
    extern template HzSectionMarker* Layout::find<HzSectionMarker>(const LayoutElementId& id);
    extern template AntigenicMaps* Layout::find<AntigenicMaps>(const LayoutElementId& id);

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

        constexpr const Tal& tal() const { return tal_; }
        constexpr Tal& tal() { return tal_; }

      protected:
        preparation_stage_t prepared_{0};

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

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& /*surface*/) const override {}

        struct Parameters
        {
            std::optional<double> pixels;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    };

    // ----------------------------------------------------------------------

    class LayoutElementWithColoring : public LayoutElement
    {
      public:
        LayoutElementWithColoring(Tal& tal, double width_to_height_ratio) : LayoutElement(tal, width_to_height_ratio), coloring_{std::make_unique<ColoringUniform>(CYAN)} {}

        void coloring(std::unique_ptr<Coloring> coloring) { coloring_ = std::move(coloring); }
        std::string_view legend_type() const { return coloring_ ? coloring_->legend_type() : std::string_view{"none"}; }

      protected:
        // Color color(const Node& node) const { return coloring_->color(node); }
        // std::string coloring_report() const { return coloring_->report(); }
        Coloring& coloring() { return *coloring_; }
        const Coloring& coloring() const { return *coloring_; }

      private:
        std::unique_ptr<Coloring> coloring_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
