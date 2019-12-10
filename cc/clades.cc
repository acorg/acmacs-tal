#include "acmacs-tal/clades.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::prepare()
{
    auto& draw_tree = tal_.draw().layout().draw_tree();
    draw_tree.prepare();

    make_clades();

    if (width_to_height_ratio() <= 0.0)
        width_to_height_ratio() = (number_of_slots() + 1) * parameters_.slot.width;

} // acmacs::tal::v3::Clades::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::make_clades()
{
    const auto& tree_clades = tal_.tree().clades();
    for (const auto& tree_clade : tree_clades) {
        auto& clade = clades_.emplace_back(tree_clade.name);
        for (const auto& tree_section : tree_clade.sections) {
            auto& section = clade.sections.emplace_back(tree_section.first, tree_section.last, tree_clade.display_name);
        }
        // merge sections
        // remove small sections
    }
    // hide clades according to settings

    // set slots
    size_t slot_no{0};
    for (auto& clade : clades_) {
        for (auto& section : clade.sections) {
            section.slot_no = slot_no_t{slot_no};
            ++slot_no;
        }
    }

} // acmacs::tal::v3::Clades::make_clades

// ----------------------------------------------------------------------

size_t acmacs::tal::v3::Clades::number_of_slots() const
{
    slot_no_t max_slot_no{0};
    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections)
            max_slot_no = std::max(max_slot_no, section.slot_no);
    }
    return *max_slot_no + 1;

} // acmacs::tal::v3::Clades::number_of_slots

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::draw(acmacs::surface::Surface& surface) const
{
    const auto& draw_tree = tal_.draw().layout().draw_tree();
    const auto vertical_step = draw_tree.vertical_step();
    const auto& viewport = surface.viewport();

    const TextStyle text_style{};

    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections) {
            const auto pos_x = viewport.origin.x() + parameters_.slot.width * (*section.slot_no + 1);
            surface.line({pos_x, vertical_step * section.first->cumulative_vertical_offset_},
                         {pos_x, vertical_step * section.last->cumulative_vertical_offset_},
                         section.arrow.color, section.arrow.line_width);
            surface.text({pos_x, vertical_step * section.first->cumulative_vertical_offset_},
                         section.display_name, section.label.color, Scaled{parameters_.slot.width * section.label.scale},
                         text_style, section.label.rotation);
        }
    }

} // acmacs::tal::v3::Clades::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
