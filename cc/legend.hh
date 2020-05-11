#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    namespace parameters
    {
        struct WorldMapDot
        {
            PointCoordinates coordinates{0.0, 0.0}; // {lat, long}: {0,0} - middle Africa, {-33.87,151.21} - Sydney, {49.25, -123.1} - Vancouver
            Color outline{WHITE};
            Color fill{BLACK};
            Pixels outline_width{1.0};
            Pixels size{3.0};
        };
    };

    class Tal;

    class Legend
    {
      public:
        Legend() = default;
        virtual ~Legend() = default;

        virtual void draw(acmacs::surface::Surface& surface) const = 0;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            bool show{true};
            PointCoordinates offset{0.0, 0.93};
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;

    };

    // ----------------------------------------------------------------------

    class LegendContinentMap : public Legend
    {
      public:
        using Legend::Legend;

        void draw(acmacs::surface::Surface& surface) const override;

        struct Parameters : public Legend::Parameters
        {
            Scaled size{0.15};
            parameters::Line equator{TRANSPARENT, Pixels{0.1}, surface::Dash::NoDash};
            parameters::Line tropics{TRANSPARENT, Pixels{0.1}, surface::Dash::Dash2};
            std::vector<parameters::WorldMapDot> dots;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;

    }; // class LegendContinentMap

    // ----------------------------------------------------------------------

    class LegendColoredByPos : public Legend
    {
      public:
        using Legend::Legend;

        void draw(acmacs::surface::Surface& surface) const override;

        struct Parameters : public Legend::Parameters
        {
            Scaled size{0.05};
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;

    }; // class LegendColoredByPos

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
