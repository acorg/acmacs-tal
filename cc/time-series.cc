#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare()
{
    auto& draw_tree = tal_.draw().layout().draw_tree();
    draw_tree.prepare();

    if (first_ == date::invalid_date() || last_ == date::invalid_date()) {
        const auto month_stat = tal_.tree().stat_by_month();
        const auto [first, last] = tal_.tree().suggest_time_series_start_end(month_stat);
        fmt::print(stderr, "DEBUG: suggested: {} {}\n", first, last);
        if (first_ == date::invalid_date())
            first_ = first;
        if (last_ == date::invalid_date())
            last_ = last;
    }
    number_of_months_ = date::months_between_dates(first_, last_) + 1;
    if (width_to_height_ratio() <= 0.0)
        width_to_height_ratio() = number_of_months_ * month_width_;
    fmt::print(stderr, "DEBUG: months in time series: {} {}..{}\n", number_of_months_, first_, last_);

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw(acmacs::surface::Surface& surface) const
{
    const auto& viewport = surface.viewport();

    const Scaled month_label_size{month_width_ * month_label_scale_};
    const TextStyle text_style{};
    const auto longest_month_label_size = surface.text_size("May ", month_label_size, text_style);
    const auto longest_year_label_size = surface.text_size("00", month_label_size, text_style);
    const auto year_label_offset_y = longest_year_label_size.width + month_label_offset_from_time_series_area;
    const auto month_label_offset_y = year_label_offset_y + longest_month_label_size.width;

    const auto origin_y = viewport.origin.y();
    double month_top, month_bottom, year_top, year_bottom, month_label_offset_x;
    if (month_label_rotation_ == Rotation90DegreesClockwise) {
        month_label_offset_x = (month_width_ - std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - month_label_offset_y;
        month_bottom = origin_y + viewport.size.height + month_label_offset_from_time_series_area;
        year_top = origin_y - year_label_offset_y;
        year_bottom = origin_y + viewport.size.height + month_label_offset_from_time_series_area + longest_month_label_size.width;
    }
    else {
        month_label_offset_x = (month_width_ + std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - month_label_offset_from_time_series_area;
        month_bottom = origin_y + viewport.size.height + month_label_offset_y;
        year_top = origin_y - month_label_offset_from_time_series_area - longest_month_label_size.width;
        year_bottom = origin_y + viewport.size.height + year_label_offset_y;
    }

    double line_offset_x = viewport.origin.x();
    for (auto month = first_; month <= last_; month = date::next_month(month)) {
        surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, month_separator_line_color_, month_separator_line_width_);

        const auto month_label = date::month_3(month);
        const auto year_label = date::year_2(month);
        const auto label_offset_x = line_offset_x + month_label_offset_x;
        surface.text({label_offset_x, month_top}, month_label, month_label_color_, month_label_size, text_style, month_label_rotation_);
        surface.text({label_offset_x, year_top}, year_label, month_label_color_, month_label_size, text_style, month_label_rotation_);
        surface.text({label_offset_x, month_bottom}, month_label, month_label_color_, month_label_size, text_style, month_label_rotation_);
        surface.text({label_offset_x, year_bottom}, year_label, month_label_color_, month_label_size, text_style, month_label_rotation_);
        line_offset_x += month_width_;
    }
    surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, month_separator_line_color_, month_separator_line_width_);

} // acmacs::tal::v3::TimeSeries::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
