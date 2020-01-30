#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class Legend : public LayoutElement
    {
      public:
        Legend(Tal& tal) : LayoutElement(tal, 0.0) {}

        Position position() const override { return Position::absolute; }

        // void prepare(verbose verb) override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            PointCoordinates offset{0.0, 0.93};
            Scaled size{0.15};
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

        void draw(acmacs::surface::Surface& surface, verbose verb) const override;

        struct Parameters : public Legend::Parameters
        {
            LineParameters equator{TRANSPARENT, Pixels{0.1}, surface::Dash::NoDash};
            LineParameters tropics{TRANSPARENT, Pixels{0.1}, surface::Dash::Dash2};
            std::vector<DotParameters> dots;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;

    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
