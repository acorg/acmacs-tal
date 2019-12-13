#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class DashBar : public LayoutElement
    {
      public:
        DashBar(Tal& tal) : LayoutElement(tal, 0.009) {}

        // void prepare() override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            bool report{false};
            DashParameters dash{1.0, Pixels{0.5}}; // width, line_width
        };

    };

    // ----------------------------------------------------------------------

    class DashBarClades : public DashBar
    {
      public:
        DashBarClades(Tal& tal) : DashBar(tal) {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct CladeParameters
        {
            std::string name;
            Color color{PINK};
            LabelParameters label{BLACK, 0.01, vertical_position::middle, horizontal_position::left, {-0.002, 0.0}, {}, NoRotation};
        };

        struct Parameters : public DashBar::Parameters
        {
            std::vector<CladeParameters> clades;
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
