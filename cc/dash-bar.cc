#include "acmacs-tal/dash-bar.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DashBar::prepare()
{

} // acmacs::tal::v3::DashBar::prepare

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
        const Scaled label_size{viewport.size.height * label.scale};
        const auto text_size = surface.text_size(label.text, label_size, label.text_style);
        double pos_y;
        switch (label.vpos) {
            case vertical_position::top:
                pos_y = viewport.top() + label.offset[1] + text_size.height;
                break;
            case vertical_position::middle:
                pos_y = viewport.center().y() + label.offset[1] + text_size.height / 2.0;
                break;
            case vertical_position::bottom:
                pos_y = viewport.bottom() + label.offset[1];
                break;
        }
        double pos_x;
        switch (label.hpos) {
            case horizontal_position::left:
                pos_x = viewport.left() + label.offset[0] - text_size.width;
                break;
            case horizontal_position::middle:
                pos_x = viewport.center().x() + label.offset[0] - text_size.width / 2.0;
                break;
            case horizontal_position::right:
                pos_x = viewport.right() + label.offset[0];
                break;
        }
        surface.text({pos_x, pos_y}, label.text, label.color, label_size, label.text_style, label.rotation);
    }

} // acmacs::tal::v3::DashBar::draw

// ======================================================================

void acmacs::tal::v3::DashBarClades::prepare()
{

} // acmacs::tal::v3::DashBarClades::prepare

// ----------------------------------------------------------------------

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
            double pos_y;
            switch (clade.label.vpos) {
                case vertical_position::top:
                    pos_y = first_line_pos_y + clade.label.offset[1] + text_size.height;
                    break;
                case vertical_position::middle:
                    pos_y = sum_line_pos_y / num_lines + clade.label.offset[1] + text_size.height / 2.0;
                    break;
                case vertical_position::bottom:
                    pos_y = last_line_pos_y + clade.label.offset[1];
                    break;
            }
            double pos_x;
            switch (clade.label.hpos) {
                case horizontal_position::left:
                    pos_x = viewport.left() + clade.label.offset[0] - text_size.width;
                    break;
                case horizontal_position::middle:
                    pos_x = viewport.center().x() + clade.label.offset[0] - text_size.width / 2.0;
                    break;
                case horizontal_position::right:
                    pos_x = viewport.right() + clade.label.offset[0];
                    break;
            }
            surface.text({pos_x, pos_y}, clade.label.text, clade.label.color, label_size, clade.label.text_style, clade.label.rotation);
        }
    }

} // acmacs::tal::v3::DashBarClades::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
