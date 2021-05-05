#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;
    class Legend;

    namespace draw_tree
    {
        struct AATransitionsParameters
        {
            enum class method { derek_2016, eu_20200514, eu_20200915, eu_20210503 };

            enum method method { method::eu_20200915 };
            bool calculate{false};
            bool report{false};
            bool debug{false};
            std::optional<seqdb::pos1_t> report_pos;
            size_t report_number_leaves_threshold{20};
            std::optional<seqdb::pos1_t> show_same_left_right_for_pos;
            // if in the intermediate node most freq aa occupies more that this value (relative to total), consider the most freq aa to be common in this node
            double non_common_tolerance{0.6};
            std::vector<double> non_common_tolerance_per_pos; // negative value means use non_common_tolerance

            double non_common_tolerance_for(seqdb::pos0_t pos) const
            {
                if (non_common_tolerance_per_pos.size() <= *pos || non_common_tolerance_per_pos[*pos] < 0.0)
                    return non_common_tolerance;
                else
                    return non_common_tolerance_per_pos[*pos];
            }
        };

        struct Parameters
        {
            AATransitionsParameters aa_transitions;
            Scaled node_id_text_size{1e-4};
        };
    } // namespace draw_tree

    // ----------------------------------------------------------------------

    class DrawTree : public LayoutElementWithColoring
    {
      public:
        DrawTree(Tal& tal);

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        constexpr double vertical_step() const { return vertical_step_; }
        constexpr double horizontal_step() const { return horizontal_step_; }

        constexpr draw_tree::Parameters& parameters() { return parameters_; }
        constexpr const draw_tree::Parameters& parameters() const { return parameters_; }

        void legend(std::unique_ptr<Legend>&& a_legend);

      private:
        draw_tree::Parameters parameters_;
        const double height_{1.0};
        double vertical_step_{0};
        double horizontal_step_{0};
        std::unique_ptr<Legend> legend_;
    };

    // ----------------------------------------------------------------------

    class DrawOnTree : public LayoutElement
    {
      public:
        DrawOnTree(Tal& tal) : LayoutElement(tal, 0.0) {}

        void draw(acmacs::surface::Surface& surface) const override;
        void draw_on_tree(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const;

        // ----------------------------------------------------------------------

        // relative to node
        struct PerNodeParameters
        {
            seq_id_t seq_id;
            parameters::Text text;
            parameters::LineWithOffset line;
        };

        struct Parameters
        {
            // bool report{false};
            std::vector<parameters::Text> texts;
            std::vector<PerNodeParameters> per_node;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    };

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
