#pragma once

#include "acmacs-map-draw/draw.hh"
#include "acmacs-map-draw/mapi-settings.hh"
#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    struct HzSection;

    // ----------------------------------------------------------------------

    class MapsSettings : public acmacs::mapi::Settings
    {
      public:
        using acmacs::mapi::Settings::Settings;

      protected:
        bool select(const acmacs::chart::Antigens& antigens, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const override;
        bool select(const acmacs::chart::Sera& sera, acmacs::chart::PointIndexList& indexes, std::string_view key, const rjson::v3::value& value) const override;
    };

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

        constexpr acmacs::mapi::Settings& chart_draw_settings() { return chart_draw_settings_; }

      private:
        Parameters parameters_;
        size_t columns_{0}, rows_{0};
        ChartDraw chart_draw_;
        mutable MapsSettings chart_draw_settings_;

        void columns_rows();
        void draw_map(acmacs::surface::Surface& surface, const HzSection& section) const;

    }; // class AntigenicMaps

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
