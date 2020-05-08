#pragma once

#include "acmacs-base/time-series.hh"
#include "acmacs-base/flat-map.hh"
#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class TimeSeries : public LayoutElementWithColoring
    {
      public:
        TimeSeries(Tal& tal) : LayoutElementWithColoring(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        void add_horizontal_line_above(const Node* node, const LineParameters& line, bool warn_if_present);

        // ----------------------------------------------------------------------

        struct SlotSeparatorParameters : LineParameters
        {
            SlotSeparatorParameters() : LineParameters{BLACK, Pixels{0.5}, surface::Dash::NoDash} {}
        };

        struct SlotLabelParameters
        {
            Rotation rotation{Rotation90DegreesAnticlockwise}; // Rotation90DegreesAnticlockwise, Rotation90DegreesClockwise
            Color color{BLACK};
            double scale{0.7};                         // relative to parameters_.slot.width
            double offset{0.002}; // relative to the time series area height
        };

        struct SlotParameters
        {
            double width{0.01}; // relative to the time series area height
            std::array<SlotSeparatorParameters, 12> separator; // left separator in each month, only the first element used for non-monthly ts
            std::array<Color, 12> background{TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT};
            SlotLabelParameters label;
        };

        struct PerNodeParameters
        {
            std::optional<Color> color;
            std::optional<double> width; // fraction of slot width
            std::optional<Pixels> line_width;
        };

        enum class color_scale_type { bezier_gradient };
        struct ColorScaleParameters
        {
            bool show{true};
            color_scale_type type{color_scale_type::bezier_gradient};
            std::array<Color, 3> colors{Color{0x440154}, Color{0x40ffff}, Color{0xfde725}};
            double offset{0.008};
            double height{0.01};
        };

        struct Parameters
        {
            acmacs::time_series::parameters time_series;
            SlotParameters slot;
            DashParameters dash;
            small_map_with_unique_keys_t<seq_id_t, PerNodeParameters> per_nodes;
            ColorScaleParameters color_scale;
            std::string report;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

        Color color_for(date::year_month_day date) const;

      private:
        struct horizontal_line_t : public LineParameters
        {
            const Node* node;

            constexpr horizontal_line_t(const Node* a_node, const LineParameters& a_line) : LineParameters(a_line), node(a_node) {}
        };

        Parameters parameters_;
        acmacs::time_series::series series_;
        std::vector<horizontal_line_t> horizontal_lines_;
        std::vector<Color> color_scale_;

        void make_color_scale();
        void draw_color_scale(acmacs::surface::Surface& surface) const;

        void draw_background_separators(acmacs::surface::Surface& surface) const;
        void draw_labels(acmacs::surface::Surface& surface) const;
        std::pair<std::string, std::string> labels(const acmacs::time_series::slot& slot) const;
        size_t slot_month(const acmacs::time_series::slot& slot) const;
        void draw_horizontal_lines(acmacs::surface::Surface& surface, const DrawTree* draw_tree) const;

    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
