#pragma once

#include "acmacs-map-draw/draw.hh"
#include "acmacs-map-draw/mapi-settings.hh"
#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    struct HzSection;
    class AntigenicMaps;

    // ----------------------------------------------------------------------

    class MapsSettings : public acmacs::mapi::Settings
    {
      public:
        MapsSettings(AntigenicMaps& antigenic_maps, ChartDraw& chart_draw) : acmacs::mapi::Settings(chart_draw), antigenic_maps_{antigenic_maps} {}

        bool apply_built_in(std::string_view name) override; // returns true if built-in command with that name found and applied

      protected:
        bool apply_antigens() override;
        bool apply_sera() override;
        bool select(const acmacs::chart::Antigens& antigens, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const override;
        bool select(const acmacs::chart::Sera& sera, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const override;
        map_elements::v1::Title& title() override;

      private:
        AntigenicMaps& antigenic_maps_;

        void select_antigens_in_section(acmacs::chart::PointIndexList& indexes, const rjson::v3::value& value) const;
        void apply_antigenic_map_section();
    };

    // ----------------------------------------------------------------------

    class MapTitle : public map_elements::v1::Title
    {
      public:
        MapTitle(AntigenicMaps& antigenic_maps) : map_elements::v1::Title(), antigenic_maps_{antigenic_maps} {}

     protected:
        std::string update_line_before_drawing(std::string_view line, const ChartDraw& aChartDraw) const override;

      private:
        AntigenicMaps& antigenic_maps_;

    }; // class MapTitle

    // ----------------------------------------------------------------------

    class AntigenicMaps : public LayoutElement
    {
      public:
        AntigenicMaps(Tal& tal);

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            double gap_between_maps{20}; // pixels
            size_t columns{0};
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

        constexpr auto& maps_settings() { return maps_settings_; }

        acmacs::chart::PointIndexList chart_antigens_in_tree() const;
        acmacs::chart::PointIndexList chart_antigens_in_section(std::optional<size_t> section_no) const; // current_section_no_ if nullopt
        void antigen_fill_time_series_color_scale(const acmacs::chart::PointIndexList& indexes);

        const HzSection& current_section() const;

      private:
        Parameters parameters_;
        size_t columns_{0}, rows_{0};
        ChartDraw chart_draw_;
        mutable MapsSettings maps_settings_;
        mutable std::optional<size_t> current_section_no_; // during drawing

        void columns_rows();
        void draw_map(acmacs::surface::Surface& surface, size_t section_no) const;

    }; // class AntigenicMaps

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
