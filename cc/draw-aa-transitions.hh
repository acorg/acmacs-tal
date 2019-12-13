#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class DrawTree;

    class DrawAATransitions : public LayoutElement
    {
      public:
        DrawAATransitions(Tal& tal) : LayoutElement(tal, 0.0) {}

        Position position() const override { return Position::absolute; }

        void prepare() override;
        void draw(acmacs::surface::Surface& surface) const override;

        void draw_transitions(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const;

        // ----------------------------------------------------------------------

        struct TransitionParameters
        {
            node_id_t node_id;
            LabelParameters label;
        };

        struct Parameters
        {
            std::vector<TransitionParameters> transitions;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
