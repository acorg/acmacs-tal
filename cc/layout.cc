#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawOutline::draw(acmacs::surface::Surface& surface) const
{
    if (outline) {
        const auto& viewport = surface.viewport();
        surface.rectangle(viewport.origin, viewport.size, outline_color, outline_width);
    }

} // acmacs::tal::v3::DrawOutline::draw

// ----------------------------------------------------------------------

acmacs::tal::v3::LayoutElement& acmacs::tal::v3::Layout::add(std::unique_ptr<LayoutElement> element)
{
    elements_.push_back(std::move(element));
    return *elements_.back();

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

acmacs::tal::v3::DrawTree& acmacs::tal::v3::Layout::draw_tree()
{
    for (auto& element : elements_) {
        if (auto* dt = element->draw_tree(); dt)
            return *dt;
    }
    throw std::runtime_error("No DrawTree element in layout");

} // acmacs::tal::v3::Layout::draw_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::prepare()
{
    for (auto& element : elements_)
        element->prepare();

} // acmacs::tal::v3::Layout::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::draw(acmacs::surface::Surface& surface) const
{
    double normal_left = 0.0;
    for (const auto& element : elements_) {
        switch (element->position()) {
            case Position::normal: {
                auto& drawing_area = surface.subsurface({normal_left, 0.0}, Scaled{element->width_to_height_ratio()}, Size{element->width_to_height_ratio(), 1.0}, false);
                element->outline().draw(drawing_area);
                element->draw(drawing_area);
                normal_left += element->width_to_height_ratio();
            } break;
            case Position::absolute:
                fmt::print(stderr, "WARNING: Drawing elements with Position::absolute not yet implemented\n");
                break;
        }
    }

} // acmacs::tal::v3::Layout::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
