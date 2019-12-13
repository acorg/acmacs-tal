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

        struct Transition
        {
            const Node* node;
            LabelParameters label;

            Transition(const Node* a_node, const LabelParameters& a_label) : node(a_node), label(a_label) {}
        };

        // ----------------------------------------------------------------------

        struct TransitionParameters
        {
            std::string node_id;
            LabelParameters label{BLACK, 0.01, vertical_position::top, horizontal_position::middle, {-0.04, 0.02}, {}, NoRotation, LabelTetherParameters{true, {BLACK, Pixels{0.3}}}, TextStyle{"monospace"}};
        };

        struct Parameters
        {
            size_t minimum_number_leaves_in_subtree{20};
            double text_line_interleave{0.3}; // fraction of the text height
            TransitionParameters all_nodes;
            std::vector<TransitionParameters> per_node;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }
        const TransitionParameters& parameters_for_node(std::string_view node_id) const;

      private:
        Parameters parameters_;
        std::vector<Transition> transitions_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
