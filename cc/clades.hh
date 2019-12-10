#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class Clades : public LayoutElement
    {
      public:
        Clades(Tal& tal) : LayoutElement(0.0), tal_{tal} {}

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        bool is_clades() override { return true; }

        // ----------------------------------------------------------------------

        using slot_no_t = acmacs::named_size_t<struct acmacs_tal_Clades_slot_no_tag>;
        constexpr static const slot_no_t NoSlot{static_cast<size_t>(-1)};

        enum class vertical_position { top, middle, bottom };

        struct label_t
        {
            Rotation rotation{NoRotation};
            Color color{BLACK};
            double scale{0.5};                         // relative to parameters_.slot.width
            vertical_position position{vertical_position::middle};
            double vertical_offset{0}; // relative to the area height
            double horizonltal_offset{0}; // relative to the area height
        };

        struct arrow_t
        {
            Color color{BLACK};
            Pixels line_width{1.0};
            Pixels arrow_width{3.0};
        };

        struct clade_section_t
        {
            const Node* first{nullptr};
            const Node* last{nullptr};
            std::string display_name;
            slot_no_t slot_no{0};
            label_t label;
            arrow_t arrow;

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
            clade_t(std::string_view nam) : name{nam} {}
            std::string name;
            std::vector<clade_section_t> sections;
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
            bool hidden{false};
            size_t section_inclusion_tolerance{10};
            size_t section_exclusion_tolerance{5};
            std::string display_name;
            slot_no_t slot_no{NoSlot};
            label_t label;
            arrow_t arrow;
            double tree_top_gap{10.0}, tree_bottom_gap{10.0};
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
        Tal& tal_;
        Parameters parameters_;
        clades_t clades_;

        void make_clades();
        void make_sections();
        void report_clades();
        void set_slots();
        void add_gaps_to_the_tree();
        size_t number_of_slots() const;
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
