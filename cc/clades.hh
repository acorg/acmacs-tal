#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Clades : public LayoutElement
    {
      public:
        Clades(Tal& tal) : LayoutElement(tal, 0.0) {}

        void prepare(verbose verb) override;
        void draw(acmacs::surface::Surface& surface, verbose verb) const override;

        // ----------------------------------------------------------------------

        using slot_no_t = acmacs::named_size_t<struct acmacs_tal_Clades_slot_no_tag>;
        constexpr static const slot_no_t NoSlot{static_cast<size_t>(-1)};

        struct arrow_t : public LineParameters
        {
            Pixels arrow_width{3.0};
        };

        struct clade_section_t
        {
            const Node* first{nullptr};
            const Node* last{nullptr};
            std::string display_name;
            slot_no_t slot_no{0};
            LabelParameters label{BLACK, 0.7, vertical_position::middle, horizontal_position::left, {0.004, 0.0}, {}, NoRotation, LabelTetherParameters{}, TextStyle{}};
            arrow_t arrow;
            LineParameters horizontal_line;

            clade_section_t(const Node* frst, const Node* lst, std::string_view disp) : first{frst}, last{lst}, display_name{disp} {}
            constexpr node_id_t::value_type size() const
            {
                if (first && last)
                    return last->node_id_.vertical - first->node_id_.vertical + 1;
                else
                    return 0;
            }
        };

        struct clade_t
        {
            std::string name;
            std::vector<clade_section_t> sections;

            clade_t(std::string_view nam) : name{nam} {}
            bool intersects(const clade_t& rhs) const;
        };

        using clades_t = std::vector<clade_t>;

        // ----------------------------------------------------------------------

        struct SlotParameters
        {
            double width{0.02}; // relative to the clades area height
        };

        struct CladeParameters
        {
            std::string name;
            std::string display_name;
            bool hidden{false};
            unsigned short section_inclusion_tolerance{10};
            unsigned short section_exclusion_tolerance{5};
            slot_no_t slot_no{NoSlot};
            LabelParameters label;
            arrow_t arrow;
            LineParameters horizontal_line{GREY, Pixels{0.5}};
            double tree_top_gap{50.0}, tree_bottom_gap{50.0};
            bool time_series_top_separator{true}, time_series_bottom_separator{true};
        };

        struct Parameters
        {
            bool report{true};
            SlotParameters slot;
            CladeParameters all_clades;
            std::vector<CladeParameters> per_clade;
        };

        constexpr Parameters& parameters() { return parameters_; }
        const CladeParameters& parameters_for_clade(std::string_view name) const;

      private:
        Parameters parameters_;
        clades_t clades_;
        bool time_series_to_the_left_{false};

        void make_clades();
        void make_sections();
        void report_clades();
        void set_slots();
        void add_gaps_to_tree();
        void add_separators_to_time_series();
        size_t number_of_slots() const;
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
