#include "acmacs-base/flat-map.hh"
#include "acmacs-base/color-distinct.hh"
#include "acmacs-tal/dash-bar.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DashBarBase::draw_label(acmacs::surface::Surface& surface, std::string_view text, Color color, const parameters::Label& label_param) const
{
    const auto& viewport = surface.viewport();
    const Scaled label_size{viewport.size.height * label_param.scale};
    const auto text_size = surface.text_size(text, label_size, label_param.text_style);
    double pos_y{0};
    switch (label_param.vpos) {
        case parameters::vertical_position::top:
            pos_y = viewport.top() + label_param.offset[1] + text_size.height;
            break;
        case parameters::vertical_position::middle:
            pos_y = viewport.center().y() + label_param.offset[1] + text_size.height / 2.0;
            break;
        case parameters::vertical_position::bottom:
            pos_y = viewport.bottom() + label_param.offset[1];
            break;
    }
    double pos_x{0};
    switch (label_param.hpos) {
        case parameters::horizontal_position::left:
            pos_x = viewport.left() + label_param.offset[0] - text_size.width;
            break;
        case parameters::horizontal_position::middle:
            pos_x = viewport.center().x() + label_param.offset[0] - text_size.width / 2.0;
            break;
        case parameters::horizontal_position::right:
            pos_x = viewport.right() + label_param.offset[0];
            break;
    }
    AD_DEBUG("draw_label \"{}\" {} {} {}", text, PointCoordinates{pos_x, pos_y}, color, label_size);
    surface.text({pos_x, pos_y}, text, color, label_size, label_param.text_style, label_param.rotation);

} // acmacs::tal::v3::DashBar::draw_label

// ----------------------------------------------------------------------

void acmacs::tal::v3::DashBar::draw(acmacs::surface::Surface& surface) const
{
    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto& viewport = surface.viewport();
    const auto vertical_step = draw_tree->vertical_step();
    const auto dash_width = parameters().dash.width;
    const auto dash_line_width = parameters().dash.line_width;
    const auto dash_pos_x = viewport.left() + viewport.size.width * (1.0 - dash_width) * 0.5;

    for (const auto& for_nodes : parameters().for_nodes) {
        for (const auto* node : for_nodes.nodes) {
            if (!node->hidden) {
                const auto vpos = vertical_step * node->cumulative_vertical_offset_;
                surface.line({dash_pos_x, vpos}, {dash_pos_x + viewport.size.width * dash_width, vpos}, for_nodes.color, dash_line_width, surface::LineCap::Round);
            }
        }
    }

    for (const auto& label : parameters().labels) {
        draw_label(surface, label.text, label.color, label);
        // const Scaled label_size{viewport.size.height * label.scale};
        // const auto text_size = surface.text_size(label.text, label_size, label.text_style);
        // double pos_y{0};
        // switch (label.vpos) {
        //     case parameters::vertical_position::top:
        //         pos_y = viewport.top() + label.offset[1] + text_size.height;
        //         break;
        //     case parameters::vertical_position::middle:
        //         pos_y = viewport.center().y() + label.offset[1] + text_size.height / 2.0;
        //         break;
        //     case parameters::vertical_position::bottom:
        //         pos_y = viewport.bottom() + label.offset[1];
        //         break;
        // }
        // double pos_x{0};
        // switch (label.hpos) {
        //     case parameters::horizontal_position::left:
        //         pos_x = viewport.left() + label.offset[0] - text_size.width;
        //         break;
        //     case parameters::horizontal_position::middle:
        //         pos_x = viewport.center().x() + label.offset[0] - text_size.width / 2.0;
        //         break;
        //     case parameters::horizontal_position::right:
        //         pos_x = viewport.right() + label.offset[0];
        //         break;
        // }
        // surface.text({pos_x, pos_y}, label.text, label.color, label_size, label.text_style, label.rotation);
    }

} // acmacs::tal::v3::DashBar::draw

// ======================================================================

void acmacs::tal::v3::DashBarClades::draw(acmacs::surface::Surface& surface) const
{
    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto& viewport = surface.viewport();
    const auto dash_width = parameters().dash.width;
    const auto dash_pos_x = viewport.left() + viewport.size.width * (1.0 - dash_width) * 0.5;

    for (const auto& clade : parameters().clades) {
        double first_line_pos_y{1e20}, last_line_pos_y{-1.0}, sum_line_pos_y{0.0};
        size_t num_lines{0};
        tree::iterate_leaf(tal().tree(), [&surface, &clade, &first_line_pos_y, &last_line_pos_y, &sum_line_pos_y, &num_lines, dash_pos_x, dash_width, viewport_width = viewport.size.width,
                                          vertical_step = draw_tree->vertical_step(), dash_line_width = parameters().dash.line_width](const Node& leaf) {
            if (!leaf.hidden) {
                if (leaf.clades.exists(clade.name)) {
                    const double vpos = vertical_step * leaf.cumulative_vertical_offset_;
                    first_line_pos_y = std::min(first_line_pos_y, vpos);
                    last_line_pos_y = std::max(last_line_pos_y, vpos);
                    sum_line_pos_y += vpos;
                    ++num_lines;
                    surface.line({dash_pos_x, vpos}, {dash_pos_x + viewport_width * dash_width, vpos}, clade.color, dash_line_width, surface::LineCap::Round);
                }
            }
        });
        if (!clade.label.text.empty()) {
            const Scaled label_size{viewport.size.height * clade.label.scale};
            const auto text_size = surface.text_size(clade.label.text, label_size, clade.label.text_style);
            double pos_y{0};
            switch (clade.label.vpos) {
                case parameters::vertical_position::top:
                    pos_y = first_line_pos_y + clade.label.offset[1] + text_size.height;
                    break;
                case parameters::vertical_position::middle:
                    pos_y = sum_line_pos_y / static_cast<double>(num_lines) + clade.label.offset[1] + text_size.height / 2.0;
                    break;
                case parameters::vertical_position::bottom:
                    pos_y = last_line_pos_y + clade.label.offset[1];
                    break;
            }
            double pos_x{0};
            switch (clade.label.hpos) {
                case parameters::horizontal_position::left:
                    pos_x = viewport.left() + clade.label.offset[0] - text_size.width;
                    break;
                case parameters::horizontal_position::middle:
                    pos_x = viewport.center().x() + clade.label.offset[0] - text_size.width / 2.0;
                    break;
                case parameters::horizontal_position::right:
                    pos_x = viewport.right() + clade.label.offset[0];
                    break;
            }
            surface.text({pos_x, pos_y}, clade.label.text, clade.label.color, label_size, clade.label.text_style, clade.label.rotation);
        }
    }

} // acmacs::tal::v3::DashBarClades::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DashBarAAAt::draw(acmacs::surface::Surface& surface) const
{
    const auto* draw_tree = tal().draw().layout().find_draw_tree();
    const auto& viewport = surface.viewport();
    const auto vertical_step = draw_tree->vertical_step();
    const auto dash_width = parameters().dash.width;
    const auto dash_line_width = parameters().dash.line_width;
    const auto dash_pos_x = viewport.left() + viewport.size.width * (1.0 - dash_width) * 0.5;

    acmacs::CounterCharSome<'A', 'Z'> counter_aa;
    tree::iterate_leaf(tal().tree(), [&counter_aa, pos = parameters().pos](const Node& leaf) {
        if (!leaf.hidden)
            counter_aa.count(leaf.aa_sequence.at(pos));
    });

    acmacs::small_map_with_unique_keys_t<char, Color> colors;
    const auto& aa_by_fequency = counter_aa.sorted();
    for (const auto [ind, aa] : acmacs::enumerate(aa_by_fequency))
        colors.emplace_not_replace(aa, acmacs::color::distinct(ind));
    colors.emplace_or_replace('X', GREY);
    colors.emplace_or_replace(' ', TRANSPARENT);

    tree::iterate_leaf(tal().tree(), [&surface, &colors, pos = parameters().pos, dash_pos_x, dash_width, viewport_width = viewport.size.width, vertical_step, dash_line_width](const Node& leaf) {
        if (!leaf.hidden) {
            const double vpos = vertical_step * leaf.cumulative_vertical_offset_;
            // AD_DEBUG("at pos '{}'", leaf.aa_sequence.at(pos));
            surface.line({dash_pos_x, vpos}, {dash_pos_x + viewport_width * dash_width, vpos}, colors.get(leaf.aa_sequence.at(pos)), dash_line_width, surface::LineCap::Round);
        }
    });

    if (!parameters().labels_by_frequency.empty()) {
        for (const auto [ind, aa] : acmacs::enumerate(aa_by_fequency)) {
            if (ind < parameters().labels_by_frequency.size()) {
                draw_label(surface, fmt::format("{}{}", parameters().pos, aa), colors.get(aa), parameters().labels_by_frequency[ind]);
            }
            else {
                auto label_param = parameters().labels_by_frequency.back();
                label_param.offset[1] += label_param.scale * static_cast<double>(ind - parameters().labels_by_frequency.size() + 1);
                draw_label(surface, fmt::format("{}{}", parameters().pos, aa), colors.get(aa), label_param);
            }
        }
    }
    else {
        for (const auto [ind, aa] : acmacs::enumerate(aa_by_fequency)) {
            parameters::Label label_param{.scale = 0.008, .offset = {0.0, 0.0}};
            label_param.offset[1] = label_param.scale * static_cast<double>(ind);
            draw_label(surface, fmt::format("{}{}", parameters().pos, aa), colors.get(aa), label_param);
        }
    }

} // acmacs::tal::v3::DashBarAAAt::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
