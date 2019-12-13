#include "acmacs-tal/dash-bar.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

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

    tree::iterate_leaf(tal().tree(), [this, &surface, dash_pos_x, dash_width, viewport_width = viewport.size.width, vertical_step = draw_tree->vertical_step(),
                                      dash_line_width = parameters().dash.line_width](const Node& leaf) {
        if (!leaf.hidden) {
            for (const auto& clade : parameters().clades) {
                if (leaf.clades.exists(clade.name))
                    surface.line({dash_pos_x, vertical_step * leaf.cumulative_vertical_offset_}, {dash_pos_x + viewport_width * dash_width, vertical_step * leaf.cumulative_vertical_offset_},
                                 clade.color, dash_line_width, surface::LineCap::Round);
            }
        }
    });

} // acmacs::tal::v3::DashBarClades::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
