#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class HzSections : public LayoutElement
    {
      public:
        HzSections(Tal& tal) : LayoutElement(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct Section
        {
            std::string id;
            // seq_id_t first;
            // seq_id_t last;
            bool shown{true};
            std::string label;
        };

        // ----------------------------------------------------------------------

        struct SectionParameters
        {
            std::string id;
            seq_id_t first;
            seq_id_t last;
            bool shown{true};
            std::string label;
        };

        struct Parameters
        {
            std::vector<SectionParameters> sections;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
        std::vector<Section> sections_;
    };

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
