#pragma once

#include "acmacs-base/time-series.hh"
#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class TimeSeries : public LayoutElementWithColoring
    {
      public:
        TimeSeries(Tal& tal) : LayoutElementWithColoring(tal, 0.0) {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        void add_horizontal_line_above(const Node* node, const line_t& line);

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
            SlotSeparatorParameters separator;
            SlotLabelParameters label;
        };

        struct Parameters
        {
            acmacs::time_series::parameters time_series;
            SlotParameters slot;
            DashParameters dash;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        struct horizontal_line_t : public line_t
        {
            const Node* node;

            constexpr horizontal_line_t(const Node* a_node, const line_t& a_line) : line_t(a_line), node(a_node) {}
        };

        Parameters parameters_;
        acmacs::time_series::series series_;
        std::vector<horizontal_line_t> horizontal_lines_;

        void draw_labels(acmacs::surface::Surface& surface) const;
        std::pair<std::string, std::string> labels(const acmacs::time_series::slot& slot) const;
        void draw_horizontal_lines(acmacs::surface::Surface& surface, const DrawTree* draw_tree) const;

    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
