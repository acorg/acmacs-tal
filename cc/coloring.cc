#include "acmacs-tal/coloring.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

Color acmacs::tal::v3::ColoringUniform::color(const Node& /*node*/) const
{
    return color_;

} // acmacs::tal::v3::ColoringUniform::color

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

Color acmacs::tal::v3::ColoringByPos::color(const Node& node) const
{
    const auto aa = node.aa_sequence.at(pos_);
    if (aa == 'X')
        return BLACK;
    if (auto found = colors_.find(aa); found != colors_.end())
        return found->second;
    const auto color = Color::distinct(colors_.size());
    fmt::print(stderr, "DEBUG: ColoringByPos {} {}: {}\n", pos_, aa, color.to_string());
    colors_.emplace(aa, color);
    return color;

    // static bool reported{false};
    // if (!reported) {
    //     fmt::print(stderr, "WARNING: ColoringByPos not implemented\n");
    //     reported = true;
    // }
    // return PINK;

} // acmacs::tal::v3::ColoringByPos::color

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End: