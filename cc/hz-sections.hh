#pragma once

#include <numeric>

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Node;

    using hz_section_id_t = acmacs::named_string_t<struct acmacs_tal_hz_section_id_t_tag>;

    struct HzSection
    {
        HzSection(std::string_view a_id) : id{hz_section_id_t{a_id}} {}
        HzSection(std::string_view a_id, const Node* a_first, const Node* a_last, std::string_view a_label) : id{hz_section_id_t{a_id}}, first{a_first}, last{a_last}, label{a_label} {}

        hz_section_id_t id;
        const Node* first{nullptr};
        const Node* last{nullptr};
        bool shown{true};
        bool intersect{false};  // to report hz section intersection
        std::string label;
        std::string prefix; // A, B, C, etc.
        AA_Transitions aa_transitions{};
        std::optional<std::string> label_aa_transitions;

        std::string aa_transitions_format() const;
    };

    class HzSections : public LayoutElement
    {
      public:
        HzSections(Tal& tal) : LayoutElement(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct SectionParameters
        {
            SectionParameters(std::string_view a_id) : id{a_id} {}
            std::string id;
            seq_id_t first;
            seq_id_t last;
            bool shown{true};
            std::optional<std::string> label;
            std::optional<std::string> label_aa_transitions;
        };

        struct Parameters
        {
            bool report{true};
            std::vector<SectionParameters> sections;
            parameters::Line line{GREY, Pixels{1.0}, surface::Dash::NoDash};
            double tree_top_gap{0.01}, tree_bottom_gap{0.01}; // fraction of the tree area height

            SectionParameters& find_add_section(std::string_view id) { return HzSections::find_add_section(sections, id); }
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

        void add_section(HzSection&& section) { sections_.push_back(std::move(section)); }

        constexpr const auto& sections() const { return sections_; }
        size_t number_of_shown() const { return std::accumulate(std::begin(sections_), std::end(sections_), 0ul, [](size_t sum, const auto& section) { return sum + (section.shown ? 1 : 0); }); }

      private:
        Parameters parameters_;
        std::vector<HzSection> sections_;

        void update_from_parameters();
        void set_aa_transitions();
        void sort();
        void detect_intersect();
        void set_prefix();
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

    }; // class HzSections

    // ----------------------------------------------------------------------

    class HzSectionMarker : public LayoutElement
    {
      public:
        HzSectionMarker(Tal& tal) : LayoutElement(tal, 0.005) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            parameters::Line line{BLACK, Pixels{1.0}, surface::Dash::NoDash};
            double label_size{2.5}; // fraction of marker width
            Color label_color{BLACK};
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    }; // class HzSectionMarker

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
