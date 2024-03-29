#include "acmacs-base/read-file.hh"
#include "acmacs-base/color-gradient.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/settings.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare(preparation_stage_t stage)
{
    using namespace std::string_view_literals;

    if (stage == 1 && prepared_ < stage) {
        tal().draw().layout().prepare_element<DrawTree>(stage);

        const auto ts_stat = acmacs::time_series::stat(parameters().time_series, tal().tree().all_dates());
        const auto [first, after_last] = acmacs::time_series::suggest_start_end(parameters().time_series, ts_stat);
        if (parameters().time_series.first == date::invalid_date())
            parameters().time_series.first = first;
        if (parameters().time_series.after_last == date::invalid_date())
            parameters().time_series.after_last = after_last;

        series_ = acmacs::time_series::make(parameters().time_series);
        make_color_scale();
        set_width_to_height_ratio();

        const auto long_report = [&ts_stat]() -> std::string { return fmt::format("time series report:\n{}", ts_stat.report("    {value}  {counter:6d}\n")); };
        if (parameters().report == "-"sv)
            AD_INFO("{}", long_report());
        else if (!parameters().report.empty())
            acmacs::file::write(parameters().report, long_report());
        if (!ts_stat.counter().empty())
            AD_INFO("time series {} full range {} .. {}", id(), ts_stat.counter().begin()->first, ts_stat.counter().rbegin()->first);
        AD_INFO("time series {} suggested  {} .. {}", id(), first, after_last);
        AD_INFO("time series {} used       {} .. {}", id(), parameters().time_series.first, parameters().time_series.after_last);
        tal().settings().setenv("time-series-range"sv, acmacs::time_series::range_name(parameters().time_series, series_, "-"));
    }
    else if (stage == 3 && prepared_ < stage) {
        tal().draw().layout().prepare_element<DrawTree>(stage);
        prepare_dashes();
    }
    LayoutElementWithColoring::prepare(stage);

} // acmacs::tal::v3::TimeSeries::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::set_width_to_height_ratio()
{
    if (width_to_height_ratio() <= 0.0)
        width_to_height_ratio() = static_cast<double>(series_.size()) * parameters().slot.width;

} // acmacs::tal::v3::TimeSeries::set_width_to_height_ratio

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare_dashes()
{
    prepare_coloring();

    AD_LOG(acmacs::log::time_series, "slots ({})", series_.size());
    for (size_t slot_no = 0; slot_no < series_.size(); ++slot_no)
        AD_LOG(acmacs::log::time_series, "    {:2d} {} - {}", slot_no, series_[slot_no].first, series_[slot_no].after_last);

    dashes_.clear();
    tree::iterate_leaf(tal().tree(), [this](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty()) {
            try {
                auto leaf_date = date::from_string(leaf.date, date::allow_incomplete::yes, date::throw_on_error::yes);
                if (date::get_month(leaf_date) == 0)
                    date::increment_month(leaf_date, 6);
                if (date::get_day(leaf_date) == 0)
                    date::increment_day(leaf_date, 15);
                if (const auto slot_no = acmacs::time_series::find(series_, leaf_date); slot_no < series_.size()) {
                    AD_LOG(acmacs::log::time_series, "[{}] -> [{}] slot:{} {}", leaf.date, leaf_date, slot_no, leaf.seq_id);
                    prepare_dash(slot_no, leaf);
                    // auto dash_color = coloring().color(leaf);
                    // auto dash_line_width = parameters().dash.line_width;
                    // auto dash_width = parameters().dash.width;
                    // parameters().per_nodes.find_then(leaf.seq_id, [&](const auto& per_node) {
                    //     if (per_node.color.has_value())
                    //         dash_color = *per_node.color;
                    //     if (per_node.line_width.has_value())
                    //         dash_line_width = *per_node.line_width;
                    //     if (per_node.width.has_value())
                    //         dash_width = *per_node.width;
                    // });
                    // dashes_.push_back(dash_t{.color = dash_color, .line_width = dash_line_width, .width = dash_width, .slot = slot_no, .y = leaf.cumulative_vertical_offset_});
                }
                else
                    AD_LOG(acmacs::log::time_series, "[{}] -> [{}] no slot {}", leaf.date, leaf_date, leaf.seq_id);
            }
            catch (date::date_parse_error& err) {
                AD_WARNING("{}", err);
            }
        }
    });

} // acmacs::tal::v3::TimeSeries::prepare_colors

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare_dash(size_t slot_no, const Node& leaf)
{
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

} // acmacs::tal::v3::TimeSeries::prepare_dash

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeries::prepare_coloring()
{
    tree::iterate_leaf(tal().tree(), [this](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty())
            coloring().prepare(leaf);
    });
    coloring().prepare();

} // acmacs::tal::v3::TimeSeries::prepare_coloring

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

acmacs::color::Modifier acmacs::tal::v3::TimeSeries::color_for(date::year_month_day date) const
{
    if (!series_.empty() && !color_scale_.empty()) {
        if (const auto series_slot = std::find_if(std::begin(series_), std::end(series_), [&date](const auto& slot) { return slot.within(date); }); series_slot != std::end(series_))
            return acmacs::color::Modifier{color_scale_[static_cast<size_t>(series_slot - std::begin(series_))]};
    }
    return {};

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
        if (const auto* coloring_by_pos = dynamic_cast<const ColoringByPosBase*>(&coloring()); coloring_by_pos) {
            const auto& viewport = surface.viewport();
            coloring_by_pos->draw_legend(surface, PointCoordinates{viewport.origin.x(), viewport.origin.y() + viewport.size.height + parameters().legend.offset},
                                         ColoringByPosBase::legend_layout::horizontal, parameters().legend.pos_color, Scaled{parameters().legend.scale}, parameters().legend.gap_scale,
                                         parameters().legend.show_count, parameters().legend.show_pos, parameters().legend.count_scale, parameters().legend.count_color);
        }
    }

} // acmacs::tal::v3::TimeSeries::draw_legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::set_width_to_height_ratio()
{
    if (width_to_height_ratio() <= 0.0)
        width_to_height_ratio() = static_cast<double>(number_of_slots()) * parameters().slot.width;

} // acmacs::tal::v3::TimeSeriesWithShift::set_width_to_height_ratio

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::prepare_coloring()
{
    tree::iterate_leaf(tal().tree(), [this](const Node& leaf) {
        if (!leaf.hidden && !leaf.date.empty()) {
            for (auto& coloring : coloring_)
                coloring->prepare(leaf);
        }
    });

    // unify colors in all colorings
    auto& master = coloring_[0]->key_color_count();
    for (auto it = std::next(std::begin(coloring_)); it != std::end(coloring_); ++it) {
        for (const auto& [key, color_count] : (*it)->key_color_count()) {
            master.find_then_else(
                key, //
                [count = color_count.count](auto& master_color_count) { master_color_count.count += count; },
                [key=key, color=color_count.color, count=color_count.count](auto& mstr) { mstr.emplace_or_replace(key, color, count); });
        }
    }
    // set unified colors
    for (auto it = std::next(std::begin(coloring_)); it != std::end(coloring_); ++it)
        (*it)->key_color_count() = master;

    // for (const auto& [key, color_count] : master)
    //     AD_DEBUG("unify {} {} {:4d}", key, color_count.color, color_count.count);

    for (auto& coloring : coloring_)
        coloring->prepare();

} // acmacs::tal::v3::TimeSeriesWithShift::prepare_coloring

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::prepare_dash(size_t slot_no, const Node& leaf)
{
    const Color no_color{0xFFDEAD00};

    auto dash_line_width = parameters().dash.line_width;
    auto dash_width = parameters().dash.width;
    auto forced_dash_color = no_color;
    parameters().per_nodes.find_then(leaf.seq_id, [&](const auto& per_node) {
        if (per_node.color.has_value())
            forced_dash_color = *per_node.color;
        if (per_node.line_width.has_value())
            dash_line_width = *per_node.line_width;
        if (per_node.width.has_value())
            dash_width = *per_node.width;
    });

    size_t slot_offset = 0;
    for (auto& coloring : coloring_) {
        auto dash_color = forced_dash_color;
        if (dash_color == no_color)
            dash_color = coloring->color(leaf);
        dashes().push_back(dash_t{.color = dash_color, .line_width = dash_line_width, .width = dash_width, .slot = slot_no + slot_offset, .y = leaf.cumulative_vertical_offset_});
        slot_offset += parameters().shift;
    }

} // acmacs::tal::v3::TimeSeriesWithShift::prepare_dash

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::draw_background_separators(acmacs::surface::Surface& surface) const
{
    if (!series().empty()) {
        const auto& viewport = surface.viewport();
        double line_offset_x = viewport.left();
        for ([[maybe_unused]] const auto slot_no : range_from_0_to(number_of_slots() + 1)) {
            const auto& sep_param = parameters().slot.separator[0];
            surface.line({line_offset_x, viewport.top()}, {line_offset_x, viewport.bottom()}, sep_param.color, sep_param.line_width, sep_param.dash);
            line_offset_x += parameters().slot.width;
        }
    }

} // acmacs::tal::v3::TimeSeriesWithShift::draw_background_separators

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::draw_labels(acmacs::surface::Surface& surface) const
{
    const auto& viewport = surface.viewport();
    const TextStyle text_style{};
    const auto shift = static_cast<double>(parameters().shift);
    const Scaled label_size{parameters().slot.width * parameters().slot.label.scale};
    const auto label_pos_y = viewport.origin.y() - *label_size * 0.3;

    double label_offset_x{parameters().slot.width * shift * 0.5};
    for (auto& coloring : coloring_) {
        const auto label = fmt::format("{}", dynamic_cast<const ColoringByPosBase*>(coloring.get())->pos());
        const auto label_drawn_size = surface.text_size(label, label_size, text_style);
        const auto label_pos_x = label_offset_x - label_drawn_size.width * 0.5 + parameters().slot.label.offset;
        surface.text({label_pos_x, label_pos_y}, label, parameters().slot.label.color, label_size, text_style, parameters().slot.label.rotation);
        label_offset_x += parameters().slot.width * shift;
    }

} // acmacs::tal::v3::TimeSeriesWithShift::draw_labels

// ----------------------------------------------------------------------

void acmacs::tal::v3::TimeSeriesWithShift::draw_legend(acmacs::surface::Surface& surface) const
{
    // AD_INFO("Time series {}", coloring().report());

    if (parameters().legend.show) {
        if (const auto* coloring_by_pos = dynamic_cast<const ColoringByPosBase*>(coloring_[0].get()); coloring_by_pos) {
            const auto& viewport = surface.viewport();
            coloring_by_pos->draw_legend(surface, PointCoordinates{viewport.origin.x(), viewport.origin.y() + viewport.size.height + parameters().legend.offset},
                                         ColoringByPosBase::legend_layout::horizontal, parameters().legend.pos_color, Scaled{parameters().legend.scale}, parameters().legend.gap_scale,
                                         ColoringByPosBase::legend_show_count::no, ColoringByPosBase::legend_show_pos::no, parameters().legend.count_scale, parameters().legend.count_color);
        }
    }

} // acmacs::tal::v3::TimeSeriesWithShift::draw_legend

// ----------------------------------------------------------------------
