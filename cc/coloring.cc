#include "acmacs-base/color-distinct.hh"
#include "acmacs-base/color-amino-acid.hh"
#include "acmacs-draw/surface.hh"
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

Color acmacs::tal::v3::ColoringByPosBase::color(const Node& node) const
{
    try {
        if (const auto aa = node.aa_sequence.at(pos_); aa != ' ')
            return colors_.get(aa).color;
        else
            return GREY;        // sequence is shorter than pos or no sequence available at all
    }
    catch (std::out_of_range&) {
        throw coloring_error{fmt::format("ColoringByPosBase: no color for {} {}, forgot to call ColoringByPosBase::prepare?", pos_, node.aa_sequence.at(pos_))};
    }

} // acmacs::tal::v3::ColoringByPosBase::color

// ----------------------------------------------------------------------

size_t acmacs::tal::v3::ColoringByPosBase::total_count() const
{
    return std::accumulate(std::begin(colors_), std::end(colors_), 0ul, [](size_t sum, const auto& color_count) { return sum + color_count.second.count; });

} // acmacs::tal::v3::ColoringByPosBase::total_count

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::ColoringByPosBase::report() const
{
    std::string result = fmt::format("coloring by AA at {}:\n", pos());
    const auto total = static_cast<double>(total_count());
    for (const auto& [aa, color_count] : colors_)
        result += fmt::format("  {}  {:12s} {:6d} {:4.1f}%\n", aa, fmt::format("\"{}\"", color_count.color), color_count.count, static_cast<double>(color_count.count) / total * 100.0);
    return result;

} // acmacs::tal::v3::ColoringByPosBase::report

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosBase::sort_colors_by_frequency()
{
    colors().sort([](const auto& e1, const auto& e2) -> bool {
        // X is last regardles of its frequency
        if (e1.first == 'X')
            return false;
        if (e2.first == 'X')
            return true;
        return e1.second.count > e2.second.count;
    });

} // acmacs::tal::v3::ColoringByPosBase::sort_colors_by_frequency

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosBase::draw_legend(acmacs::surface::Surface& surface, const PointCoordinates& origin, legend_layout layout, Color title_color, Scaled text_size, double interleave,
                                                     legend_show_count show_count, legend_show_pos show_pos, double count_scale, Color count_color) const
{
    auto text_origin{origin};

    if (show_pos == legend_show_pos::yes) {
        const auto pos_text{fmt::format("{}", pos())};
        surface.text(text_origin, pos_text, title_color, text_size);
        if (layout == legend_layout::horizontal)
            text_origin.x(text_origin.x() + surface.text_size(pos_text, text_size).width - *text_size);
    }

    const auto total_percent = static_cast<double>(total_count()) / 100.0;
    const auto count_text_size{text_size * count_scale};
    const auto [aa_height, aa_width] = surface.text_size(std::string(1, 'W'), text_size);
    for (const auto& [aa, color_count] : colors()) {
        if (aa != ' ') { // ignore data for absent or short sequences, it is ugly and misleading in the legend
            switch (layout) {
                case legend_layout::vertical:
                    text_origin.y(text_origin.y() + *text_size * (1.0 + interleave));
                    break;
                case legend_layout::horizontal:
                    text_origin.x(text_origin.x() + *text_size * (1.0 + interleave));
                    break;
            }
            const auto aa_t{fmt::format("{}", aa)};
            surface.text(text_origin, aa_t, color_count.color, text_size);
            if (show_count == legend_show_count::yes) {
                const auto count_x{text_origin.x() + aa_width * 1.1};
                surface.text({count_x, text_origin.y() - aa_height + *count_text_size * 1.45}, fmt::format("{:.1f}%", static_cast<double>(color_count.count) / total_percent), count_color,
                             count_text_size);
                surface.text({count_x, text_origin.y()}, fmt::format("{}", color_count.count), count_color, count_text_size);
            }
        }
    }

} // acmacs::tal::v3::ColoringByPosBase::draw_legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosAAColors::prepare(const Node& node)
{
    if (const auto aa = node.aa_sequence.at(pos()); aa != ' ') { // ' ' means sequence is shorter than pos or no sequence available at all
        auto& entry = colors().emplace_not_replace(aa, acmacs::amino_acid_color(aa));
        ++entry.second.count;
    }

} // acmacs::tal::v3::ColoringByPosAAColors::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosAAColors::prepare()
{
    sort_colors_by_frequency();

} // acmacs::tal::v3::ColoringByPosAAColors::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosAAFrequency::prepare(const Node& node)
{
    auto& entry = colors().emplace_not_replace(node.aa_sequence.at(pos()), color_count_t{PINK, 0});
    ++entry.second.count;

} // acmacs::tal::v3::ColoringByPosAAFrequency::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::ColoringByPosAAFrequency::prepare()
{
    sort_colors_by_frequency();

    size_t color_no{0};
    for (auto& [aa, color_count] : colors()) {
        if (aa == 'X') {
            color_count.color = BLACK;
        }
        else {
            if (color_no < color_order_.size())
                color_count.color = color_order_[color_no];
            else
                color_count.color = acmacs::color::distinct(color_no - color_order_.size());
            ++color_no;
        }
    }

} // acmacs::tal::v3::ColoringByPosAAFrequency::prepare

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
