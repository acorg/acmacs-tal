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

        void prepare(verbose verb) override;
        void draw(acmacs::surface::Surface& surface, verbose verb) const override;

        void add_horizontal_line_above(const Node* node, const LineParameters& line);

        // ----------------------------------------------------------------------

        struct SlotSeparatorParameters
        {
            Pixels width{0.5};
            Color color{BLACK};
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

        struct Parameters
        {
            acmacs::time_series::parameters time_series;
            SlotParameters slot;
            DashParameters dash;
            flat_map_t<SeqId, PerNodeParameters> per_nodes;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        struct horizontal_line_t : public LineParameters
        {
            const Node* node;

            constexpr horizontal_line_t(const Node* a_node, const LineParameters& a_line) : LineParameters(a_line), node(a_node) {}
        };

        Parameters parameters_;
        acmacs::time_series::series series_;
        std::vector<horizontal_line_t> horizontal_lines_;

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
