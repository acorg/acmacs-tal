#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

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
            last_ = date::next_month(last);
    }
    const auto number_of_months = date::months_between_dates(first_, last_);
    if (width_to_height_ratio() <= 0.0)
        width_to_height_ratio() = number_of_months * parameters_.slot.width;
    fmt::print(stderr, "DEBUG: months in time series: {} [{} .. {})\n", number_of_months, first_, last_);

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw(acmacs::surface::Surface& surface) const
{
    draw_labels(surface);

    const auto& draw_tree = tal_.draw().layout().draw_tree();
    const auto& viewport = surface.viewport();
    const auto dash_pos_x = viewport.origin.x() + parameters_.slot.width * (1.0 - parameters_.dash.width) * 0.5;

    tree::iterate_leaf(tal_.tree(), [&, this, dash_pos_x](const Node& leaf) {
        if (!leaf.date.empty()) {
            if (const auto leaf_date = date::from_string(leaf.date); leaf_date >= first_ && leaf_date < last_) {
                const auto month_no = date::calendar_months_between_dates(first_, leaf_date);
                const auto dash_offset_x = dash_pos_x + month_no * parameters_.slot.width;
                if (!leaf.hidden) {
                    surface.line({dash_offset_x, draw_tree.vertical_step() * leaf.cumulative_vertical_offset_},
                                 {dash_offset_x + parameters_.slot.width * parameters_.dash.width, draw_tree.vertical_step() * leaf.cumulative_vertical_offset_}, color(leaf), parameters_.dash.line_width, surface::LineCap::Round);
                }
            }
        }
    });

} // acmacs::tal::v3::TimeSeries::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_labels(acmacs::surface::Surface& surface) const
{
    const auto& viewport = surface.viewport();

    const Scaled month_label_size{parameters_.slot.width * parameters_.slot.label.scale};
    const TextStyle text_style{};
    const auto longest_month_label_size = surface.text_size("May ", month_label_size, text_style);
    const auto longest_year_label_size = surface.text_size("00", month_label_size, text_style);
    const auto year_label_offset_y = longest_year_label_size.width + parameters_.slot.label.offset;
    const auto month_label_offset_y = year_label_offset_y + longest_month_label_size.width;

    const auto origin_y = viewport.origin.y();
    double month_top, month_bottom, year_top, year_bottom, month_label_offset_x;
    if (parameters_.slot.label.rotation == Rotation90DegreesClockwise) {
        month_label_offset_x = (parameters_.slot.width - std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - month_label_offset_y;
        month_bottom = origin_y + viewport.size.height + parameters_.slot.label.offset;
        year_top = origin_y - year_label_offset_y;
        year_bottom = origin_y + viewport.size.height + parameters_.slot.label.offset + longest_month_label_size.width;
    }
    else {
        month_label_offset_x = (parameters_.slot.width + std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - parameters_.slot.label.offset;
        month_bottom = origin_y + viewport.size.height + month_label_offset_y;
        year_top = origin_y - parameters_.slot.label.offset - longest_month_label_size.width;
        year_bottom = origin_y + viewport.size.height + year_label_offset_y;
    }

    double line_offset_x = viewport.origin.x();
    for (auto month = first_; month < last_; month = date::next_month(month)) {
        surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters_.slot.separator.color, parameters_.slot.separator.width);

        const auto month_label = date::month_3(month);
        const auto year_label = date::year_2(month);
        const auto label_offset_x = line_offset_x + month_label_offset_x;
        surface.text({label_offset_x, month_top}, month_label, parameters_.slot.label.color, month_label_size, text_style, parameters_.slot.label.rotation);
        surface.text({label_offset_x, year_top}, year_label, parameters_.slot.label.color, month_label_size, text_style, parameters_.slot.label.rotation);
        surface.text({label_offset_x, month_bottom}, month_label, parameters_.slot.label.color, month_label_size, text_style, parameters_.slot.label.rotation);
        surface.text({label_offset_x, year_bottom}, year_label, parameters_.slot.label.color, month_label_size, text_style, parameters_.slot.label.rotation);
        line_offset_x += parameters_.slot.width;
    }
    surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters_.slot.separator.color, parameters_.slot.separator.width);

} // acmacs::tal::v3::TimeSeries::draw_labels

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
