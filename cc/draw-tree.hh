#pragma once

#include "acmacs-tal/layout.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class DrawTree : public LayoutElementWithColoring
    {
      public:
        DrawTree(Tal& tal) : LayoutElementWithColoring(tal, 0.7) {}

        void prepare(verbose verb) override;
        void draw(acmacs::surface::Surface& surface, verbose verb) const override;

        constexpr double vertical_step() const { return vertical_step_; }
        constexpr double horizontal_step() const { return horizontal_step_; }

      private:
        const double height_{1.0};
        double vertical_step_{0};
        double horizontal_step_{0};
    };

    // ----------------------------------------------------------------------

    class DrawOnTree : public LayoutElement
    {
      public:
        DrawOnTree(Tal& tal) : LayoutElement(tal, 0.0) {}

        void draw(acmacs::surface::Surface& surface, verbose verb) const override;
        void draw_on_tree(acmacs::surface::Surface& surface, const DrawTree& draw_tree, verbose verb) const;

        // ----------------------------------------------------------------------

        // relative to node
        struct PerNodeParameters
        {
            SeqId seq_id;
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
