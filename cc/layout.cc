#include "acmacs-base/range-v3.hh"
#include "acmacs-tal/layout.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/clades.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-aa-transitions.hh"
#include "acmacs-tal/title.hh"
#include "acmacs-tal/legend.hh"
#include "acmacs-tal/dash-bar.hh"
#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/antigenic-maps.hh"

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

size_t acmacs::tal::v3::Layout::count(LayoutElement& element) const
{
    const auto& type = typeid(element);
    const auto if_same_type = [&type](const auto& elt) { return typeid(*elt) == type; };
    return static_cast<size_t>(ranges::count_if(elements_, if_same_type));

} // acmacs::tal::v3::Layout::count

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

void acmacs::tal::v3::Layout::reset()
{
    elements_.clear();

} // acmacs::tal::v3::Layout::reset

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::prepare()
{
    for (preparation_stage_t stage = 1; stage <= 3; ++stage) {
        for (auto& element : elements_)
            element->prepare(stage);
    }

} // acmacs::tal::v3::Layout::prepare

// ----------------------------------------------------------------------

template <typename Element> inline const Element* acmacs::tal::v3::Layout::find(const LayoutElementId& id) const
{
    for (const auto& element : elements_) {
        if (const auto* found = dynamic_cast<const Element*>(element.get()); found && (id.empty() || found->id() == id))
            return found;
    }
    return nullptr;
}

template const acmacs::tal::v3::DrawTree* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawTree>(const LayoutElementId& id) const;
template const acmacs::tal::v3::DrawOnTree* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawOnTree>(const LayoutElementId& id) const;
template const acmacs::tal::v3::TimeSeries* acmacs::tal::v3::Layout::find<acmacs::tal::v3::TimeSeries>(const LayoutElementId& id) const;
template const acmacs::tal::v3::TimeSeriesWithShift* acmacs::tal::v3::Layout::find<acmacs::tal::v3::TimeSeriesWithShift>(const LayoutElementId& id) const;
template const acmacs::tal::v3::Clades* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Clades>(const LayoutElementId& id) const;
template const acmacs::tal::v3::DrawAATransitions* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawAATransitions>(const LayoutElementId& id) const;
template const acmacs::tal::v3::Title* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Title>(const LayoutElementId& id) const;
// template const acmacs::tal::v3::LegendContinentMap* acmacs::tal::v3::Layout::find<acmacs::tal::v3::LegendContinentMap>(const LayoutElementId& id) const;
template const acmacs::tal::v3::Gap* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Gap>(const LayoutElementId& id) const;
template const acmacs::tal::v3::DashBar* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBar>(const LayoutElementId& id) const;
template const acmacs::tal::v3::DashBarAAAt* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBarAAAt>(const LayoutElementId& id) const;
template const acmacs::tal::v3::DashBarClades* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBarClades>(const LayoutElementId& id) const;
template const acmacs::tal::v3::HzSections* acmacs::tal::v3::Layout::find<acmacs::tal::v3::HzSections>(const LayoutElementId& id) const;
template const acmacs::tal::v3::HzSectionMarker* acmacs::tal::v3::Layout::find<acmacs::tal::v3::HzSectionMarker>(const LayoutElementId& id) const;
template const acmacs::tal::v3::AntigenicMaps* acmacs::tal::v3::Layout::find<acmacs::tal::v3::AntigenicMaps>(const LayoutElementId& id) const;

template <typename Element> inline Element* acmacs::tal::v3::Layout::find(const LayoutElementId& id)
{
    for (auto& element : elements_) {
        if (auto* found = dynamic_cast<Element*>(element.get()); found && (id.empty() || found->id() == id))
            return found;
    }
    return nullptr;
}

template acmacs::tal::v3::DrawTree* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawTree>(const LayoutElementId& id);
template acmacs::tal::v3::DrawOnTree* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawOnTree>(const LayoutElementId& id);
template acmacs::tal::v3::TimeSeries* acmacs::tal::v3::Layout::find<acmacs::tal::v3::TimeSeries>(const LayoutElementId& id);
template acmacs::tal::v3::TimeSeriesWithShift* acmacs::tal::v3::Layout::find<acmacs::tal::v3::TimeSeriesWithShift>(const LayoutElementId& id);
template acmacs::tal::v3::Clades* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Clades>(const LayoutElementId& id);
template acmacs::tal::v3::DrawAATransitions* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DrawAATransitions>(const LayoutElementId& id);
template acmacs::tal::v3::Title* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Title>(const LayoutElementId& id);
// template acmacs::tal::v3::LegendContinentMap* acmacs::tal::v3::Layout::find<acmacs::tal::v3::LegendContinentMap>(const LayoutElementId& id);
template acmacs::tal::v3::Gap* acmacs::tal::v3::Layout::find<acmacs::tal::v3::Gap>(const LayoutElementId& id);
template acmacs::tal::v3::DashBar* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBar>(const LayoutElementId& id);
template acmacs::tal::v3::DashBarAAAt* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBarAAAt>(const LayoutElementId& id);
template acmacs::tal::v3::DashBarClades* acmacs::tal::v3::Layout::find<acmacs::tal::v3::DashBarClades>(const LayoutElementId& id);
template acmacs::tal::v3::HzSections* acmacs::tal::v3::Layout::find<acmacs::tal::v3::HzSections>(const LayoutElementId& id);
template acmacs::tal::v3::HzSectionMarker* acmacs::tal::v3::Layout::find<acmacs::tal::v3::HzSectionMarker>(const LayoutElementId& id);
template acmacs::tal::v3::AntigenicMaps* acmacs::tal::v3::Layout::find<acmacs::tal::v3::AntigenicMaps>(const LayoutElementId& id);

// ----------------------------------------------------------------------

template <typename Element> void acmacs::tal::v3::Layout::prepare_element(preparation_stage_t stage)
{
    for (auto& element : elements_) {
        if (auto* found = dynamic_cast<Element*>(element.get()); found)
            found->prepare(stage);
    }

} // acmacs::tal::v3::Layout::prepare_tree

template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::DrawTree>(preparation_stage_t stage);
template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::TimeSeries>(preparation_stage_t stage);
template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::Clades>(preparation_stage_t stage);
template void acmacs::tal::v3::Layout::prepare_element<acmacs::tal::v3::HzSections>(preparation_stage_t stage);

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

const acmacs::tal::v3::DrawTree* acmacs::tal::v3::Layout::find_draw_tree(throw_error te) const
{
    if (const auto* draw_tree = find<DrawTree>(); draw_tree)
        return draw_tree;
    else if (te == throw_error::yes)
        throw error{"No tree section in the layout"};
    else {
        AD_WARNING("No tree section in the layout");
        return nullptr;
    }

} // acmacs::tal::v3::Layout::find_draw_tree

// ----------------------------------------------------------------------

acmacs::tal::v3::DrawTree* acmacs::tal::v3::Layout::find_draw_tree(throw_error te)
{
    if (auto* draw_tree = find<DrawTree>(); draw_tree)
        return draw_tree;
    else if (te == throw_error::yes)
        throw error{"No tree section in the layout"};
    else {
        AD_WARNING("No tree section in the layout");
        return nullptr;
    }

} // acmacs::tal::v3::Layout::find_draw_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::draw(acmacs::surface::Surface& surface) const
{
    surface_ = &surface;

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
                element->draw(surface);
                break;
        }
    }

    surface_ = nullptr;

} // acmacs::tal::v3::Layout::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::Layout::draw_horizontal_line_between(const LayoutElement* elt1, const LayoutElement* elt2, double y_pos, Color line_color, Pixels line_width) const
{
    if (elt1 && elt2 /* && line_width > Pixels{0} */ && line_color != TRANSPARENT) {
        double x1left{-1.0}, x1right{-1.0}, x2left{-1.0}, x2right{-1.0}, normal_left{0.0};
        for (const auto& element : elements_) {
            switch (element->position()) {
                case Position::normal:
                    if (elt1 == element.get())
                        x1left = normal_left;
                    else if (elt2 == element.get())
                        x2left = normal_left;
                    normal_left += element->width_to_height_ratio();
                    if (elt1 == element.get())
                        x1right = normal_left;
                    else if (elt2 == element.get())
                        x2right = normal_left;
                    break;
                case Position::absolute:
                    if (elt1 == element.get()) {
                        x1left = surface_->viewport().left();
                        x1right = surface_->viewport().right();
                    }
                    else if (elt2 == element.get()) {
                        x2left = surface_->viewport().left();
                        x2right = surface_->viewport().right();
                    }
                    break;
            }
        }

        if (x1right < x2left)
            surface_->line({x1right, y_pos}, {x2left, y_pos}, line_color, line_width);
        else
            surface_->line({x2right, y_pos}, {x1left, y_pos}, line_color, line_width);
    }

} // acmacs::tal::v3::Layout::draw_horizontal_line_between

// ----------------------------------------------------------------------

double acmacs::tal::v3::LayoutElement::pos_y_above(const Node& node, double vertical_step) const
{
    return vertical_step * (node.cumulative_vertical_offset_ - node.vertical_offset_ / 2.0);

} // acmacs::tal::v3::LayoutElement::pos_y_above

// ----------------------------------------------------------------------

double acmacs::tal::v3::LayoutElement::pos_y_below(const Node& node, double vertical_step) const
{
    if (node.last_next_leaf) // if node is not the last leaf
        return vertical_step * (node.last_next_leaf->cumulative_vertical_offset_ - node.last_next_leaf->vertical_offset_ / 2.0);
    else // last leaf
        return vertical_step * node.cumulative_vertical_offset_;

} // acmacs::tal::v3::LayoutElement::pos_y_below

// ----------------------------------------------------------------------

void acmacs::tal::v3::Gap::prepare(preparation_stage_t stage)
{
    if (stage == 1 && prepared_ < stage) {
        if (parameters().pixels.has_value())
            width_to_height_ratio() = *parameters().pixels / tal().draw().canvas_height();
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::Gap::prepare

// ----------------------------------------------------------------------
