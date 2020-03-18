#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Node;

    class HzSections : public LayoutElement
    {
      public:
        HzSections(Tal& tal) : LayoutElement(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        using section_id_t = acmacs::named_string_t<struct acmacs_tal_section_id_t_tag>;

        struct Section
        {
            section_id_t id;
            const Node* first{nullptr};
            const Node* last{nullptr};
            bool shown{true};
            std::string label;
            AA_Transitions aa_transitions{};
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

        void add_section(Section&& section) { sections_.push_back(std::move(section)); }

      private:
        Parameters parameters_;
        std::vector<Section> sections_;

        void set_aa_transitions();
        void sort();
    };

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
