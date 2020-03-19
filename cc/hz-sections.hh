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
            Section(std::string_view a_id) : id{section_id_t{a_id}} {}
            Section(std::string_view a_id, const Node* a_first, const Node* a_last, std::string_view a_label) : id{section_id_t{a_id}}, first{a_first}, last{a_last}, label{a_label} {}
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
            SectionParameters(std::string_view a_id) : id{a_id} {}
            std::string id;
            seq_id_t first;
            seq_id_t last;
            bool shown{true};
            std::string label;
        };

        struct Parameters
        {
            bool report{true};
            std::vector<SectionParameters> sections;
            LineParameters line;
            double tree_top_gap{50.0}, tree_bottom_gap{50.0};

            SectionParameters& find_add_section(std::string_view id)
            {
                return HzSections::find_add_section(sections, id);
            }
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

        void add_section(Section&& section) { sections_.push_back(std::move(section)); }

      private:
        Parameters parameters_;
        std::vector<Section> sections_;

        void update_from_parameters();
        void set_aa_transitions();
        void sort();
        void add_gaps_to_tree();
        void add_separators_to_time_series();
        void report() const;

        template <typename Sec> inline static Sec& find_add_section(std::vector<Sec>& sections, std::string_view id)
            {
                if (auto found = std::find_if(std::begin(sections), std::end(sections), [id](const auto& section) { return section.id == id; }); found != std::end(sections))
                    return *found;
                else
                    return sections.emplace_back(id);
            }
    };

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
