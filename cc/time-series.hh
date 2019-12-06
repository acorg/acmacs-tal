#pragma once

#include "acmacs-base/date.hh"
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

      private:
        Tal& tal_;
        date::year_month_day first_{date::invalid_date()}, last_{date::invalid_date()}; // inclusive
        date::period_diff_t number_of_months_{0};
        double month_width_{0.01}; // relative to time series area height

        double dash_width_{0.5}; // relative to month_width_
        Pixels dash_line_width_{0.5};

        Pixels month_separator_line_width_{0.5};
        Color month_separator_line_color_{BLACK};
        Rotation month_label_rotation_{Rotation90DegreesAnticlockwise}; // Rotation90DegreesAnticlockwise, Rotation90DegreesClockwise
        Color month_label_color_{BLACK};
        double month_label_scale_{0.7}; // relative to month_width_
        double month_label_offset_from_time_series_area{0.002}; // relative to time series area height

        void draw_labels(acmacs::surface::Surface& surface) const;
    };

    // ----------------------------------------------------------------------

}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
