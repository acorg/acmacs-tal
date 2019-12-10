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

        // ----------------------------------------------------------------------

        using slot_no_t = acmacs::named_size_t<struct acmacs_tal_Clades_slot_no_tag>;

        struct label_t
        {
            Rotation rotation{NoRotation};
            Color color{BLACK};
            double scale{1.0};                         // relative to parameters_.slot.width
            // double offset{0.002}; // relative to the time series area height
        };

        struct arrow_t
        {
            Color color{BLACK};
            Pixels line_width{1};
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

        struct Parameters
        {
            SlotParameters slot;
        };

        constexpr Parameters& parameters() { return parameters_; }

      private:
        Tal& tal_;
        Parameters parameters_;
        clades_t clades_;

        void make_clades();
        size_t number_of_slots() const;
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
