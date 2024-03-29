#include "acmacs-base/rjson-v3-helper.hh"
#include "acmacs-tal/antigenic-maps.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/time-series.hh"

// ----------------------------------------------------------------------

bool acmacs::tal::v3::MapsSettings::apply_built_in(std::string_view name)
{
    using namespace std::string_view_literals;
    try {
        if (name == "antigenic-map-section"sv) {
            apply_antigenic_map_section();
            return true;
        }
        else if (name == "serum-circles-remove"sv) {
            antigenic_maps_.remove_serum_circles();
            return true;
        }
        return acmacs::mapi::Settings::apply_built_in(name);
    }
    catch (std::exception& err) {
        throw error{fmt::format("cannot apply \"{}\": {} while reading {}", name, err, format_toplevel())};
    }

} // acmacs::tal::v3::MapsSettings::apply_built_in

// ----------------------------------------------------------------------

void acmacs::tal::v3::MapsSettings::apply_antigenic_map_section()
{
    using namespace std::string_view_literals;
    const auto first_seq_id = rjson::v3::read_string(getenv("first"sv));
    if (!first_seq_id.has_value())
        throw error{"no \"first\" in \"antigenic-map-section\""};
    const auto& to_apply = getenv("apply"sv);
    if (!to_apply.is_array())
        throw error{"no or invalid \"apply\" in \"antigenic-map-section\""};
    const auto last_seq_id = rjson::v3::read_string(getenv("last"sv), "");
    if (const auto& current_section = antigenic_maps_.section();
        current_section.first && first_seq_id == current_section.first->seq_id && (last_seq_id.empty() || (current_section.last && last_seq_id == current_section.last->seq_id))) {
        apply("apply"sv);
    }

} // acmacs::tal::v3::MapsSettings::apply_antigenic_map_section

// ----------------------------------------------------------------------

map_elements::v1::Title& acmacs::tal::v3::MapsSettings::title()
{
    return chart_draw().map_elements().find_or_add<MapTitle>("title");

} // acmacs::tal::v3::MapsSettings::title

// ----------------------------------------------------------------------

bool acmacs::tal::v3::MapsSettings::apply_antigens()
{
    using namespace std::string_view_literals;

    acmacs::mapi::Settings::apply_antigens();
    if (const auto& fill = getenv("fill"); fill.is_object() && rjson::v3::read_bool(fill["time-series-color-scale"sv], false)) {
        antigenic_maps_.antigen_fill_time_series_color_scale(select_antigens(getenv("select"sv)));
    }
    return true;

} // acmacs::tal::v3::MapsSettings::apply_antigens

// ----------------------------------------------------------------------

bool acmacs::tal::v3::MapsSettings::apply_sera()
{
    acmacs::mapi::Settings::apply_sera();
    return true;

} // acmacs::tal::v3::MapsSettings::apply_sera

// ----------------------------------------------------------------------

bool acmacs::tal::v3::MapsSettings::select(const acmacs::chart::Antigens& /*antigens*/, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const
{
    using namespace std::string_view_literals;

    if (key == "in-tree"sv) {
        const auto in_tree_indexes = antigenic_maps_.chart_antigens_in_tree();
        if (rjson::v3::read_bool(value, false))
            indexes.keep(ReverseSortedIndexes{*in_tree_indexes});
        else
            indexes.remove(ReverseSortedIndexes{*in_tree_indexes});
        return true;
    }
    else if (key == "in-section"sv) {
        select_antigens_in_section(indexes, value);
        return true;
    }
    else
        return false;

} // acmacs::tal::v3::MapsSettings::select

// ----------------------------------------------------------------------

void acmacs::tal::v3::MapsSettings::select_antigens_in_section(acmacs::chart::PointIndexList& indexes, const rjson::v3::value& value) const
{
    value.visit([&indexes, this]<typename Val>(const Val& val) -> void {
        bool keep{false};
        acmacs::chart::PointIndexList in_section_indexes;
        if constexpr (std::is_same_v<Val, rjson::v3::detail::boolean>) {
            keep = val.template to<bool>();
            in_section_indexes = antigenic_maps_.chart_antigens_in_section(std::nullopt); // current section
        }
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::number>) {
            keep = true;
            in_section_indexes = antigenic_maps_.chart_antigens_in_section(val.template to<size_t>());
        }
        else
            throw acmacs::mapi::unrecognized{fmt::format("unrecognized \"in-section\" clause: {}", val)};
        if (keep)
            indexes.keep(ReverseSortedIndexes{*in_section_indexes});
        else
            indexes.remove(ReverseSortedIndexes{*in_section_indexes});
    });

} // acmacs::tal::v3::MapsSettings::select_antigens_in_section

// ----------------------------------------------------------------------

bool acmacs::tal::v3::MapsSettings::select(const acmacs::chart::Sera& /*sera*/, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const
{
    using namespace std::string_view_literals;
    if (key == "in-section"sv) {
        select_sera_in_section(indexes, value);
        return true;
    }
    else
        return false;

} // acmacs::tal::v3::MapsSettings::select

// ----------------------------------------------------------------------

void acmacs::tal::v3::MapsSettings::select_sera_in_section(acmacs::chart::PointIndexList& indexes, const rjson::v3::value& value) const
{
    value.visit([&indexes, this]<typename Val>(const Val& val) -> void {
        bool keep{false};
        acmacs::chart::PointIndexList in_section_indexes;
        if constexpr (std::is_same_v<Val, rjson::v3::detail::boolean>) {
            keep = val.template to<bool>();
            in_section_indexes = antigenic_maps_.chart_sera_in_section(std::nullopt); // current section
        }
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::number>) {
            keep = true;
            in_section_indexes = antigenic_maps_.chart_sera_in_section(val.template to<size_t>());
        }
        else
            throw acmacs::mapi::unrecognized{fmt::format("unrecognized \"in-section\" clause: {}", val)};
        if (keep)
            indexes.keep(ReverseSortedIndexes{*in_section_indexes});
        else
            indexes.remove(ReverseSortedIndexes{*in_section_indexes});
    });

} // acmacs::tal::v3::MapsSettings::select_sera_in_section

// ----------------------------------------------------------------------

acmacs::tal::v3::AntigenicMaps::AntigenicMaps(Tal& tal)
    : LayoutElement(tal, 0.0), chart_draw_{tal.chartp(), 0}, maps_settings_{*this, chart_draw_}
{

} // acmacs::tal::v3::AntigenicMaps::AntigenicMaps

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::prepare(preparation_stage_t stage)
{
    using namespace std::string_view_literals;

    if (stage == 2 && prepared_ < stage) {
        tal().tree().match(tal().chart());
        tal().draw().layout().prepare_element<HzSections>(stage);
        columns_rows();
        // acmacs::log::enable("settings"sv);
        maps_settings_.apply_first({"/tal-mapi"sv, "mapi"sv, "mapi-default"sv}, acmacs::settings::v3::throw_if_nothing_applied::yes);
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::AntigenicMaps::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::columns_rows()
{
    if (const auto* hz_sections = tal().draw().layout().find<HzSections>(); hz_sections && !hz_sections->sections().empty()) {
        const auto num_sections = hz_sections->number_of_shown();
        if (parameters().columns == 0)
            columns_ = num_sections / 3 + (num_sections % 3 ? 1 : 0);
        else
            columns_ = parameters().columns;
        // if (columns_ < 2)
        //     rows_ = num_sections;
        // else
        rows_ = 3;
        width_to_height_ratio() = static_cast<double>(columns_) / 3.0;
    }
    else {
        columns_ = 0;
        rows_ = 0;
        width_to_height_ratio() = 0.0;
    }

} // acmacs::tal::v3::AntigenicMaps::columns_rows

// ----------------------------------------------------------------------

const acmacs::tal::v3::HzSection& acmacs::tal::v3::AntigenicMaps::section(std::optional<size_t> section_no) const
{
    if (section_no.has_value())
        return tal().draw().layout().find<HzSections>()->sections().at(*section_no);

    current_section_.validate();
    return *current_section_.section;

} // acmacs::tal::v3::AntigenicMaps::section

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::remove_serum_circles()
{
    chart_draw_.remove_serum_circles();

} // acmacs::tal::v3::AntigenicMaps::remove_serum_circles

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::draw(acmacs::surface::Surface& surface) const
{
    using namespace std::string_view_literals;
    if (columns_ && rows_) {
        const auto& viewport = surface.viewport();
        const auto gap = surface.convert(Pixels{parameters().gap_between_maps}).value();
        const auto map_size = viewport.size.height / static_cast<double>(rows_) - gap * static_cast<double>(rows_ - 1) / static_cast<double>(rows_);
        const auto& hz_sections = tal().draw().layout().find<HzSections>()->sections();
        for (size_t section_no{0}, map_no{0}; section_no < hz_sections.size(); ++section_no) {
            if (const auto& section = hz_sections[section_no]; section.shown) {
                fmt::print(stderr, "\n\n");
                AD_INFO("========================================================\n>>> SECTION {} {}\n\n", section_no, static_cast<char>(map_no + 'A'));
                current_section_.set(section_no, section);
                const auto left = static_cast<double>(map_no % columns_) * (map_size + gap);
                const auto top = static_cast<double>(map_no / columns_) * (map_size + gap);
                draw_map(surface.subsurface({left, top}, Scaled{map_size}, Size{1.0, 1.0}, true), section);
                current_section_.reset();
                ++map_no;
            }
        }
    }

} // acmacs::tal::v3::AntigenicMaps::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::draw_map(acmacs::surface::Surface& surface, const HzSection& section) const
{
    using namespace std::string_view_literals;

    MapsSettings::environment_push subenv(maps_settings_);
    maps_settings_.setenv("section-prefix"sv, section.prefix);
    maps_settings_.setenv("section-label"sv, section.label);
    maps_settings_.setenv("section-aa-transitions"sv, section.aa_transitions_format());

    AD_INFO("prefix:\"{}\" label:\"{}\" aa-transitions:\"{}\"", section.prefix, section.label, section.aa_transitions_format());

    maps_settings_.apply("antigenic-map"sv);

    chart_draw_.calculate_viewport();
    acmacs::draw::DrawElementsToSurface painter(surface.subsurface({0.0, 0.0}, Scaled{1.0}, chart_draw_.viewport("AntigenicMaps::draw_map"sv), true));
    chart_draw_.draw(painter);

} // acmacs::tal::v3::AntigenicMaps::draw_map

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::tal::v3::AntigenicMaps::chart_antigens_in_tree() const
{
    return tal().tree().chart_antigens_in_tree();

} // acmacs::tal::v3::AntigenicMaps::chart_antigens_in_tree

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::tal::v3::AntigenicMaps::chart_antigens_in_section(std::optional<size_t> section_no) const
{
    const auto& sect = section(section_no);
    return tal().tree().chart_antigens_in_section(sect.first, sect.last);

} // acmacs::tal::v3::AntigenicMaps::chart_antigens_in_section

// ----------------------------------------------------------------------

acmacs::chart::PointIndexList acmacs::tal::v3::AntigenicMaps::chart_sera_in_section(std::optional<size_t> section_no) const
{
    const auto& sect = section(section_no);
    return tal().tree().chart_sera_in_section(sect.first, sect.last);

} // acmacs::tal::v3::AntigenicMaps::chart_sera_in_section

// ----------------------------------------------------------------------

void acmacs::tal::v3::AntigenicMaps::antigen_fill_time_series_color_scale(const acmacs::chart::PointIndexList& indexes)
{
    if (const auto* time_series = tal().draw().layout().find<TimeSeries>(); time_series) {
        auto antigens{chart_draw_.chart().antigens()};
        for (const auto index : indexes) {
            const auto date = date::from_string(antigens->at(index)->date(), date::allow_incomplete::yes, date::throw_on_error::no);
            acmacs::PointStyleModified style;
            style.fill(time_series->color_for(date));
            chart_draw_.modify(index, style);
        }
    }
    else
        AD_WARNING("cannot color by \"time-series-color-scale\": time series not available");

} // acmacs::tal::v3::AntigenicMaps::antigen_fill_time_series_color_scale

// ----------------------------------------------------------------------
