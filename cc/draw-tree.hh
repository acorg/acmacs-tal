#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class DrawTree : public LayoutElementWithColoring
    {
      public:
        DrawTree(Tal& tal) : LayoutElementWithColoring(tal, 0.7) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        constexpr double vertical_step() const { return vertical_step_; }
        constexpr double horizontal_step() const { return horizontal_step_; }

        struct AATransitionsParameters
        {
            bool report{false};
            std::optional<seqdb::pos1_t> pos;
            size_t number_leaves_threshold{20};
        };

        struct Parameters
        {
            AATransitionsParameters aa_transitions;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
        const double height_{1.0};
        double vertical_step_{0};
        double horizontal_step_{0};
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
            TextParameters text;
            LineWithOffsetParameters line;
        };

        struct Parameters
        {
            // bool report{false};
            std::vector<TextParameters> texts;
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
