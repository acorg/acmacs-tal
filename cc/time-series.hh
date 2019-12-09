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
        TimeSeries(Tal& tal) : LayoutElementWithColoring(0.0), tal_{tal} {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct DashParameters
        {
            double width{0.5}; // relative to parameters_.slot.width
            Pixels line_width{0.5};
        };

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
        // constexpr void first(date::year_month_day&& date) { first_ = std::move(date); }
        // constexpr void last(date::year_month_day&& date) { last_ = std::move(date); }

      private:
        Tal& tal_;
        Parameters parameters_;
        acmacs::time_series::series series_;

        void draw_labels(acmacs::surface::Surface& surface) const;
        std::pair<std::string, std::string> labels(const acmacs::time_series::slot& slot) const;

    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
