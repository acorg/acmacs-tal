#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class Title : public LayoutElement
    {
      public:
        Title(Tal& tal) : LayoutElement(tal, 0.0) {}

        Position position() const override { return Position::absolute; }

        // void prepare(verbose verb) override;
        void draw(acmacs::surface::Surface& surface, verbose verb) const override;

        // ----------------------------------------------------------------------

        struct Parameters : TextParameters
        {
            Parameters() : TextParameters{{}, PointCoordinates{0.0, -0.005}, BLACK, Scaled{0.015}} {}
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;

    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
