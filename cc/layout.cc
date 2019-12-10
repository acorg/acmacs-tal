#include "acmacs-tal/layout.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/clades.hh"

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

void acmacs::tal::v3::Layout::prepare()
{
    for (auto& element : elements_)
        element->prepare();

} // acmacs::tal::v3::Layout::prepare

// ----------------------------------------------------------------------

template <typename Element> inline Element* acmacs::tal::v3::Layout::find()
{
    for (auto& element : elements_) {
        if (auto* found = dynamic_cast<Element*>(element.get()); found)
            return found;
    }
    return nullptr;
}

template acmacs::tal::v3::DrawTree* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawTree>();
template acmacs::tal::v3::TimeSeries* acmacs::tal::v3::Layout::find<acmacs::tal::v3::TimeSeries>();
template acmacs::tal::v3::Clades* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Clades>();

// ----------------------------------------------------------------------

template <typename Element> void acmacs::tal::v3::Layout::prepare_element()
{
    for (auto& element : elements_) {
        if (auto* found = dynamic_cast<Element*>(element.get()); found)
            found->prepare();
    }

} // acmacs::tal::v3::Layout::prepare_tree

template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::DrawTree>();
template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::TimeSeries>();
template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::Clades>();

// ----------------------------------------------------------------------

size_t acmacs::tal::v3::Layout::index_of(const LayoutElement* look_for) const
{
    for (size_t index = 0; index < elements_.size(); ++index) {
        if (elements_[index].get() == look_for)
            return index;
    }
    return static_cast<size_t>(-1);

} // acmacs::tal::v3::Layout::index_of

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
