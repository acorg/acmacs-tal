#include "acmacs-base/color-distinct.hh"
#include "acmacs-tal/coloring.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

Color acmacs::tal::v3::ColoringUniform::color(const Node& /*node*/) const
{
    return color_;

} // acmacs::tal::v3::ColoringUniform::color

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::ColoringUniform::report() const
{
    return fmt::format("coloring uniform: {}", color_);

} // acmacs::tal::v3::ColoringUniform::report

// ----------------------------------------------------------------------

Color acmacs::tal::v3::ColoringByContinent::color(const Node& node) const
{
    using namespace std::string_view_literals;
    if (auto found = colors_.find(node.continent); found != colors_.end())
        return found->second;
    else if (auto found_unknown = colors_.find("UNKNOWN"sv); found_unknown != colors_.end())
        return found_unknown->second;
    else
        return PINK;

} // acmacs::tal::v3::ColoringByContinent::color

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::ColoringByContinent::report() const
{
    std::string result = fmt::format("coloring by continent:\n");
    for (const auto& [continent, color] : colors_)
        result += fmt::format("  {:17s}  {}\n", continent, color);
    return result;

} // acmacs::tal::v3::ColoringByContinent::report

// ----------------------------------------------------------------------

Color acmacs::tal::v3::ColoringByPos::color(const Node& node) const
{
    const auto aa = node.aa_sequence.at(pos_);
    if (aa == 'X')
        return BLACK;
    const auto color = colors_.emplace_not_replace(aa, acmacs::color::distinct(colors_.size())).second;
    // AD_DEBUG("ColoringByPos {} {}: {}", pos_, aa, color);
    return color;

} // acmacs::tal::v3::ColoringByPos::color

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::ColoringByPos::report() const
{
    std::string result = fmt::format("coloring by AA at {}:\n", pos_);
    for (const auto& [aa, color] : colors_)
        result += fmt::format("  {}  \"{}\"\n", aa, color);
    return result;

} // acmacs::tal::v3::ColoringByPos::report

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
