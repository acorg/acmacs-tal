#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::add(std::unique_ptr<LayoutElement> element)
{
    elements_.push_back(std::move(element));

} // acmacs::tal::v3::Layout::add

// ----------------------------------------------------------------------

double acmacs::tal::v3::Layout::width_relative_to_height() const
{
    double width = 0.0;
    for (const auto& element : elements_) {
        if (element->position() == Position::normal)
            width += element->width_to_height_ratio();
    }
    return width;

} // acmacs::tal::v3::Layout::width_relative_to_height

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::draw(acmacs::surface::Surface& surface) const
{
    for (const auto& element : elements_) {
    }

} // acmacs::tal::v3::Layout::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
