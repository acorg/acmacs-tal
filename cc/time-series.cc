#include "acmacs-base/string-split.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare()
{
    if (!prepared_) {
        tal().draw().layout().prepare_element<DrawTree>();

        if (parameters().time_series.first == date::invalid_date() || parameters().time_series.after_last == date::invalid_date()) {
            const auto month_stat = tal().tree().stat_by_month();
            const auto [first, last] = tal().tree().suggest_time_series_start_end(month_stat);
            fmt::print(stderr, "DEBUG: suggested: {} {}\n", first, last);
            if (parameters().time_series.first == date::invalid_date())
                parameters().time_series.first = first;
            if (parameters().time_series.after_last == date::invalid_date())
                parameters().time_series.after_last = date::next_month(last);
        }
        series_ = acmacs::time_series::make(parameters().time_series);
        if (width_to_height_ratio() <= 0.0)
            width_to_height_ratio() = series_.size() * parameters().slot.width;
        // fmt::print(stderr, "DEBUG: time series: {} {}\n", series_.size(), series_);
    }
    LayoutElementWithColoring::prepare();

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::add_horizontal_line_above(const Node* node, const line_t& line)
{
    if (const auto found = std::find_if(std::begin(horizontal_lines_), std::end(horizontal_lines_), [node](const auto& hl) { return hl.node == node; }); found != std::end(horizontal_lines_)) {
        if (found->color != line.color || found->line_width != line.line_width)
            fmt::print(stderr, "WARNING: time series horizontal line above {} {} already added with different parameters\n", node->node_id_, node->seq_id);
    }
    else
        horizontal_lines_.emplace_back(node, line);

} // acmacs::tal::v3::TimeSeries::add_horizontal_line_above

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw(acmacs::surface::Surface& surface) const
{
    draw_labels(surface);

    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto vertical_step = draw_tree->vertical_step();
    const auto& viewport = surface.viewport();
    const auto dash_pos_x = viewport.left() + parameters().slot.width * (1.0 - parameters().dash.width) * 0.5;

    tree::iterate_leaf(tal().tree(), [&, this, dash_pos_x, dash_width=parameters().dash.width, slot_width=parameters().slot.width](const Node& leaf) {
        if (!leaf.date.empty()) {
            const auto leaf_date = date::from_string(leaf.date, date::allow_incomplete::yes, date::throw_on_error::yes);
            if (const auto slot_no = acmacs::time_series::find(series_, leaf_date); slot_no < series_.size()) {
                const auto dash_offset_x = dash_pos_x + slot_no * slot_width;
                if (!leaf.hidden) {
                    surface.line({dash_offset_x, vertical_step * leaf.cumulative_vertical_offset_},
                                 {dash_offset_x + slot_width * dash_width, vertical_step * leaf.cumulative_vertical_offset_},
                                 color(leaf), parameters().dash.line_width, surface::LineCap::Round);
                }
            }
        }
    });

    draw_horizontal_lines(surface, draw_tree);

} // acmacs::tal::v3::TimeSeries::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_labels(acmacs::surface::Surface& surface) const
{
    const auto& viewport = surface.viewport();

    const Scaled month_label_size{parameters().slot.width * parameters().slot.label.scale};
    const TextStyle text_style{};
    const auto longest_month_label_size = surface.text_size("May ", month_label_size, text_style);
    const auto longest_year_label_size = surface.text_size("00", month_label_size, text_style);
    const auto year_label_offset_y = longest_year_label_size.width + parameters().slot.label.offset;
    const auto month_label_offset_y = year_label_offset_y + longest_month_label_size.width;

    const auto origin_y = viewport.origin.y();
    double month_top, month_bottom, year_top, year_bottom, month_label_offset_x;
    if (parameters().slot.label.rotation == Rotation90DegreesClockwise) {
        month_label_offset_x = (parameters().slot.width - std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - month_label_offset_y;
        month_bottom = origin_y + viewport.size.height + parameters().slot.label.offset;
        year_top = origin_y - year_label_offset_y;
        year_bottom = origin_y + viewport.size.height + parameters().slot.label.offset + longest_month_label_size.width;
    }
    else {
        month_label_offset_x = (parameters().slot.width + std::max(longest_month_label_size.height, longest_year_label_size.height)) * 0.5;
        month_top = origin_y - parameters().slot.label.offset;
        month_bottom = origin_y + viewport.size.height + month_label_offset_y;
        year_top = origin_y - parameters().slot.label.offset - longest_month_label_size.width;
        year_bottom = origin_y + viewport.size.height + year_label_offset_y;
    }

    double line_offset_x = viewport.origin.x();
    for (const auto& slot : series_) {
        surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters().slot.separator.color, parameters().slot.separator.width);

        const auto [year_label, month_label] = labels(slot);
        const auto label_offset_x = line_offset_x + month_label_offset_x;
        surface.text({label_offset_x, month_top}, month_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, year_top}, year_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, month_bottom}, month_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, year_bottom}, year_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        line_offset_x += parameters().slot.width;
    }
    surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters().slot.separator.color, parameters().slot.separator.width);

} // acmacs::tal::v3::TimeSeries::draw_labels

// ----------------------------------------------------------------------

std::pair<std::string, std::string> acmacs::tal::v3::TimeSeries::labels(const acmacs::time_series::slot& slot) const
{
    switch (parameters().time_series.intervl) {
        case acmacs::time_series::v2::interval::year:
            return {date::year_2(slot.first), {}};
        case acmacs::time_series::v2::interval::month:
            return {date::year_2(slot.first), date::month_3(slot.first)};
        case acmacs::time_series::v2::interval::week:
            return {{}, date::year4_month2_day2(slot.first)};
        case acmacs::time_series::v2::interval::day:
            return {{}, date::year4_month2_day2(slot.first)};
    }
    return {date::year_2(slot.first), date::month_3(slot.first)}; // g++9 wants this

} // acmacs::tal::v3::TimeSeries::labels

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_horizontal_lines(acmacs::surface::Surface& surface, const DrawTree* draw_tree) const
{
    tree::iterate_leaf(tal().tree(), [this, &surface, &draw_tree](const Node& leaf) {
        if (auto hl_found = std::find_if(std::begin(horizontal_lines_), std::end(horizontal_lines_), [leafp=&leaf](const auto& hl) { return hl.node == leafp; }); hl_found != std::end(horizontal_lines_)) {
            const auto pos_y_top = pos_y_above(*hl_found->node, draw_tree->vertical_step());
            surface.line({surface.viewport().left(), pos_y_top}, {surface.viewport().right(), pos_y_top}, hl_found->color, hl_found->line_width);
        }
    });

} // acmacs::tal::v3::TimeSeries::draw_horizontal_lines

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
