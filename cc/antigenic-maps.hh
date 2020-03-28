#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    struct HzSection;

    class AntigenicMaps : public LayoutElement
    {
      public:
        AntigenicMaps(Tal& tal) : LayoutElement(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            double gap{20};
            size_t columns{0};
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
        size_t columns_{0}, rows_{0};

        void columns_rows();
        void draw_map(acmacs::surface::Surface& surface, const HzSection& section) const;

    }; // class AntigenicMaps

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
