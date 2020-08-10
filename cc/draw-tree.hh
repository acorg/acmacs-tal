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
            enum class method { derek, eu_20200514 };

            enum method method{method::derek};
            bool calculate{false};
            bool report{false};
            bool debug{false};
            std::optional<seqdb::pos1_t> report_pos;
            size_t report_number_leaves_threshold{20};
            std::optional<seqdb::pos1_t> show_same_left_right_for_pos;
        };

        struct Parameters
        {
            AATransitionsParameters aa_transitions;
        };
    }

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


}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
