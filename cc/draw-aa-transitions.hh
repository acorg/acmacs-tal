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

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        void draw_transitions(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const;

        // ----------------------------------------------------------------------

        struct Transition
        {
            const Node* node;
            parameters::Label label;
            std::vector<Size> name_sizes;
            PointCoordinates at_edge_line{PointCoordinates::zero2D};
            Viewport box;

            Transition(const Node* a_node, const parameters::Label& a_label) : node(a_node), label(a_label) {}
        };

        // ----------------------------------------------------------------------

        struct TransitionParameters
        {
            std::string node_id;
            parameters::Label label{BLACK, 0.01, parameters::vertical_position::top, parameters::horizontal_position::middle, {-0.04, 0.02}, {}, NoRotation, parameters::LabelTether{true, {BLACK, Pixels{0.3}}}, TextStyle{"monospace"}};
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
        mutable std::vector<Transition> transitions_;

        void calculate_boxes(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const;
        void report() const;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
