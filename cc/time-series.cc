#include "acmacs-base/string-split.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare(preparation_stage_t stage)
{
    if (!prepared_) {
        tal().draw().layout().prepare_element<DrawTree>(stage);

        if (parameters().time_series.first == date::invalid_date() || parameters().time_series.after_last == date::invalid_date()) {
            const auto month_stat = tal().tree().stat_by_month();
            const auto [first, last] = tal().tree().suggest_time_series_start_end(month_stat);
            fmt::print("INFO: Time Series range suggested: {} {}\n", first, last);
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
    LayoutElementWithColoring::prepare(stage);

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::add_horizontal_line_above(const Node* node, const LineParameters& line)
{
    // fmt::print(stderr, "DEBUG: TimeSeries::add_horizontal_line_above: {}\n", node->seq_id);
    if (const auto found = std::find_if(std::begin(horizontal_lines_), std::end(horizontal_lines_), [node](const auto& hl) { return hl.node == node; }); found != std::end(horizontal_lines_)) {
        if (found->color != line.color || found->line_width != line.line_width)
            fmt::print(stderr, "WARNING: time series horizontal line above {} {} already added with different parameters\n", node->node_id, node->seq_id);
    }
    else
        horizontal_lines_.emplace_back(node, line);

} // acmacs::tal::v3::TimeSeries::add_horizontal_line_above

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw(acmacs::surface::Surface& surface) const
{
    draw_background_separators(surface);
    draw_labels(surface);

    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto vertical_step = draw_tree->vertical_step();
    const auto& viewport = surface.viewport();

    tree::iterate_leaf(tal().tree(), [&, this, slot_width = parameters().slot.width](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty()) {
            const auto leaf_date = date::from_string(leaf.date, date::allow_incomplete::yes, date::throw_on_error::yes);
            if (const auto slot_no = acmacs::time_series::find(series_, leaf_date); slot_no < series_.size()) {
                const auto specific = parameters().per_nodes.at_ptr(leaf.seq_id);
                const auto dash_color = specific && specific->color.has_value() ? *specific->color : color(leaf);
                const auto dash_line_width = specific && specific->line_width.has_value() ? *specific->line_width : parameters().dash.line_width;
                const auto dash_width = specific && specific->width.has_value() ? *specific->width : parameters().dash.width;
                const auto dash_offset_x = viewport.left() + slot_no * slot_width + parameters().slot.width * (1.0 - dash_width) * 0.5;
                surface.line({dash_offset_x, vertical_step * leaf.cumulative_vertical_offset_}, {dash_offset_x + slot_width * dash_width, vertical_step * leaf.cumulative_vertical_offset_},
                             dash_color, dash_line_width, surface::LineCap::Round);
            }
        }
    });

    draw_horizontal_lines(surface, draw_tree);

} // acmacs::tal::v3::TimeSeries::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_background_separators(acmacs::surface::Surface& surface) const
{
    if (!series_.empty()) {
        const auto& viewport = surface.viewport();
        double line_offset_x = viewport.left();
        for (const auto& slot : series_) {
            const auto month_no = slot_month(slot);
            if (parameters().slot.background[month_no] != TRANSPARENT)
                surface.rectangle_filled({line_offset_x, viewport.top()}, {parameters().slot.width, viewport.size.height}, parameters().slot.background[month_no], Pixels{0},
                                         parameters().slot.background[month_no]);
            const auto& sep_param = parameters().slot.separator[month_no];
            surface.line({line_offset_x, viewport.top()}, {line_offset_x, viewport.bottom()}, sep_param.color, sep_param.line_width, sep_param.dash);
            line_offset_x += parameters().slot.width;
        }
        auto next_month_no = slot_month(series_.back()) + 1;
        if (next_month_no > 11)
            next_month_no = 0;
        const auto& last_sep_param = parameters().slot.separator[next_month_no];
        surface.line({line_offset_x, viewport.top()}, {line_offset_x, viewport.bottom()}, last_sep_param.color, last_sep_param.line_width, last_sep_param.dash);
    }

} // acmacs::tal::v3::TimeSeries::draw_background_separators

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

    double line_offset_x = viewport.left();
    for (const auto& slot : series_) {
        // surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters().slot.separator.color, parameters().slot.separator.width);

        const auto [year_label, month_label] = labels(slot);
        const auto label_offset_x = line_offset_x + month_label_offset_x;
        surface.text({label_offset_x, month_top}, month_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, year_top}, year_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, month_bottom}, month_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        surface.text({label_offset_x, year_bottom}, year_label, parameters().slot.label.color, month_label_size, text_style, parameters().slot.label.rotation);
        line_offset_x += parameters().slot.width;
    }
    // surface.line({line_offset_x, viewport.origin.y()}, {line_offset_x, viewport.origin.y() + viewport.size.height}, parameters().slot.separator.color, parameters().slot.separator.width);

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

size_t acmacs::tal::v3::TimeSeries::slot_month(const acmacs::time_series::slot& slot) const
{
    switch (parameters().time_series.intervl) {
        case acmacs::time_series::v2::interval::month:
            return static_cast<unsigned>(slot.first.month()) - 1;
        case acmacs::time_series::v2::interval::day:
        case acmacs::time_series::v2::interval::week:
        case acmacs::time_series::v2::interval::year:
            return 0;
    }
    return 0;                   // g++-9

} // acmacs::tal::v3::TimeSeries::slot_month

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
