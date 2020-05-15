#include "acmacs-base/read-file.hh"
#include "acmacs-base/color-gradient.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare(preparation_stage_t stage)
{
    using namespace std::string_view_literals;

    if (!prepared_) {
        tal().draw().layout().prepare_element<DrawTree>(stage);

        const auto ts_stat = acmacs::time_series::stat(parameters().time_series, tal().tree().all_dates());
        const auto [first, after_last] = acmacs::time_series::suggest_start_end(parameters().time_series, ts_stat);
        if (parameters().time_series.first == date::invalid_date())
            parameters().time_series.first = first;
        if (parameters().time_series.after_last == date::invalid_date())
            parameters().time_series.after_last = after_last;

        series_ = acmacs::time_series::make(parameters().time_series);
        make_color_scale();
        if (width_to_height_ratio() <= 0.0)
            width_to_height_ratio() = static_cast<double>(series_.size()) * parameters().slot.width;

        const auto long_report = [&ts_stat]() -> std::string { return fmt::format("time series report:\n{}", ts_stat.report("    {value}  {counter:6d}\n")); };
        if (parameters().report == "-"sv)
            AD_INFO("{}", long_report());
        else if (!parameters().report.empty())
            acmacs::file::write(parameters().report, long_report());
        if (!ts_stat.counter().empty())
            AD_INFO("time series full range {} .. {}", ts_stat.counter().begin()->first, ts_stat.counter().rbegin()->first);
        AD_INFO("time series suggested  {} .. {}", first, after_last);
        AD_INFO("time series used       {} .. {}", parameters().time_series.first, parameters().time_series.after_last);
    }
    else if (stage == 3 && prepared_ < stage) {
        tal().draw().layout().prepare_element<DrawTree>(stage);
        prepare_dashes();
    }
    LayoutElementWithColoring::prepare(stage);

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare_dashes()
{
    tree::iterate_leaf(tal().tree(), [this](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty())
            coloring().prepare(leaf);
    });
    coloring().prepare();

    dashes_.clear();
    tree::iterate_leaf(tal().tree(), [this](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty()) {
            try {
                const auto leaf_date = date::from_string(leaf.date, date::allow_incomplete::yes, date::throw_on_error::yes);
                if (const auto slot_no = acmacs::time_series::find(series_, leaf_date); slot_no < series_.size()) {
                    auto dash_color = coloring().color(leaf);
                    auto dash_line_width = parameters().dash.line_width;
                    auto dash_width = parameters().dash.width;
                    parameters().per_nodes.find_then(leaf.seq_id, [&](const auto& per_node) {
                        if (per_node.color.has_value())
                            dash_color = *per_node.color;
                        if (per_node.line_width.has_value())
                            dash_line_width = *per_node.line_width;
                        if (per_node.width.has_value())
                            dash_width = *per_node.width;
                    });
                    dashes_.push_back(dash_t{.color = dash_color, .line_width = dash_line_width, .width = dash_width, .slot = slot_no, .y = leaf.cumulative_vertical_offset_});
                }
            }
            catch (date::date_parse_error& err) {
                AD_WARNING("{}", err);
            }
        }
    });

} // acmacs::tal::v3::TimeSeries::prepare_colors

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::add_horizontal_line_above(const Node* node, const parameters::Line& line, bool warn_if_present)
{
    // AD_DEBUG("TimeSeries::add_horizontal_line_above: {}", node->seq_id);
    if (const auto found = std::find_if(std::begin(horizontal_lines_), std::end(horizontal_lines_), [node](const auto& hl) { return hl.node == node; }); found != std::end(horizontal_lines_)) {
        if ((found->color != line.color || found->line_width != line.line_width) && warn_if_present)
            AD_WARNING("time series horizontal line above {} {} already added with different parameters", node->node_id, node->seq_id);
    }
    else
        horizontal_lines_.emplace_back(node, line);

} // acmacs::tal::v3::TimeSeries::add_horizontal_line_above

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw(acmacs::surface::Surface& surface) const
{
    draw_color_scale(surface);
    draw_background_separators(surface);
    draw_labels(surface);
    draw_legend(surface);

    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto vertical_step = draw_tree->vertical_step();
    const auto& viewport = surface.viewport();

    for (const auto& dash : dashes_) {
        const auto dash_offset_x = viewport.left() + static_cast<double>(dash.slot) * parameters().slot.width + parameters().slot.width * (1.0 - dash.width) * 0.5;
        surface.line({dash_offset_x, vertical_step * dash.y}, {dash_offset_x + parameters().slot.width * dash.width, vertical_step * dash.y},
                     dash.color, dash.line_width, surface::LineCap::Round);
    }

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
    if (parameters().time_series.intervl == acmacs::time_series::interval::year)
        year_top = month_top;

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

void acmacs::tal::v3::TimeSeries::make_color_scale()
{
    if (!series_.empty() && parameters().color_scale.show) {
        switch (parameters().color_scale.type) {
            case color_scale_type::bezier_gradient:
                color_scale_ = acmacs::color::bezier_gradient(parameters().color_scale.colors[0], parameters().color_scale.colors[1], parameters().color_scale.colors[2], series_.size());
                break;
        }
    }

} // acmacs::tal::v3::TimeSeries::make_color_scale

// ----------------------------------------------------------------------

Color acmacs::tal::v3::TimeSeries::color_for(date::year_month_day date) const
{
    if (!series_.empty() && !color_scale_.empty()) {
        if (const auto series_slot = std::find_if(std::begin(series_), std::end(series_), [&date](const auto& slot) { return slot.within(date); }); series_slot != std::end(series_))
            return color_scale_[static_cast<size_t>(series_slot - std::begin(series_))];
    }
    return GREEN;

} // acmacs::tal::v3::TimeSeries::color_for

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_color_scale(acmacs::surface::Surface& surface) const
{
    if (!series_.empty() && parameters().color_scale.show) {
        const auto& viewport = surface.viewport();
        auto rect_x = viewport.left();
        const auto rect_top = viewport.bottom() + parameters().color_scale.offset;
        switch (parameters().color_scale.type) {
            case color_scale_type::bezier_gradient:
                for (const auto& color : color_scale_) {
                    surface.rectangle_filled({rect_x, rect_top}, {parameters().slot.width, parameters().color_scale.height}, PINK, Pixels{0}, color);
                    rect_x += parameters().slot.width;
                }
                break;
        }
    }

} // acmacs::tal::v3::TimeSeries::draw_color_scale

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::draw_legend(acmacs::surface::Surface& surface) const
{
    // AD_INFO("Time series {}", coloring().report());

    if (parameters().legend.show) {
        if (const auto* col = dynamic_cast<const ColoringByPosBase*>(&coloring()); col) {
            const Scaled text_size{parameters().legend.scale};
            const auto count_text_size{text_size * parameters().legend.count_scale};
            const TextStyle text_style{};
            const auto gap = *text_size * parameters().legend.gap_scale;
            const auto& viewport = surface.viewport();
            const auto total_percent = static_cast<double>(col->total_count()) / 100.0;

            const auto text_y = viewport.origin.y() + viewport.size.height + parameters().legend.offset;
            auto text_x = viewport.origin.x();
            const auto pos_text = fmt::format("{}", col->pos());
            surface.text({text_x, text_y}, pos_text, parameters().legend.pos_color, text_size, text_style, NoRotation);
            text_x += surface.text_size(pos_text, text_size, text_style).width + gap;
            for (const auto& [aa, color_count] : col->colors()) {
                const std::string aa_t(1, aa);
                surface.text({text_x, text_y}, aa_t, color_count.color, text_size, text_style, NoRotation);
                if (parameters().legend.show_count) {
                    const auto [aa_height, aa_width] = surface.text_size(aa_t, text_size, text_style);
                    text_x += surface.text_size(std::string(1, aa), text_size, text_style).width;
                    surface.text({text_x, text_y - aa_height + *count_text_size * 0.5}, fmt::format("{:.1f}%", static_cast<double>(color_count.count) / total_percent), parameters().legend.count_color,
                                 count_text_size, text_style, NoRotation);
                    surface.text({text_x, text_y}, fmt::format("{}", color_count.count), parameters().legend.count_color, count_text_size, text_style, NoRotation);
                }
                text_x += gap;
            }
        }
    }

} // acmacs::tal::v3::TimeSeries::draw_legend

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
