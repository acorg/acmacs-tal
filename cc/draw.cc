#include "acmacs-draw/surface-cairo.hh"
#include "acmacs-tal/draw.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Draw::prepare()
{
    set_width_to_height_ratio();
    layout_.prepare();

} // acmacs::tal::v3::Draw::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Draw::set_width_to_height_ratio()
{
    width_to_height_ratio_ = (layout().width_relative_to_height() + margins().left + margins().right) / (1.0 + margins().top + margins().bottom);
    fmt::print("INFO: pdf width_to_height_ratio: {:.4f}\n", width_to_height_ratio_);

} // acmacs::tal::v3::Draw::set_width_to_height_ratio

// ----------------------------------------------------------------------

void acmacs::tal::v3::Draw::export_pdf(std::string_view filename) const
{
    acmacs::surface::PdfCairo pdf{filename, height_ * width_to_height_ratio_, height_, width_to_height_ratio_};
    const auto& viewport = pdf.viewport();
    pdf.rectangle_filled(viewport.origin, viewport.size, WHITE, Pixels{0}, WHITE);
    auto& drawing_area = pdf.subsurface({margins_.left, margins_.top}, Scaled{width_to_height_ratio_ - margins_.left - margins_.right},
                                        Size{(width_to_height_ratio_ - margins_.left - margins_.right) / (1.0 - margins_.top - margins_.bottom), 1.0}, false);
    outline().draw(drawing_area);
    layout_.draw(drawing_area);

} // acmacs::tal::v3::Draw::export_pdf

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
