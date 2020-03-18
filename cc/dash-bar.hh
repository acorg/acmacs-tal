#pragma once

#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class DashBarBase : public LayoutElement
    {
      public:
        DashBarBase(Tal& tal) : LayoutElement(tal, 0.009) {}

        // void prepare(preparation_stage_t stage) override;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            bool report{false};
            DashParameters dash{1.0, Pixels{0.5}}; // width, line_width
        };

    };

    // ----------------------------------------------------------------------

    class DashBar : public DashBarBase
    {
      public:
        DashBar(Tal& tal) : DashBarBase(tal) {}

        // void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct NodeParameters
        {
            NodeSet nodes;
            Color color{PINK};
        };

        struct Parameters : public DashBarBase::Parameters
        {
            std::vector<NodeParameters> for_nodes;
            std::vector<LabelParameters> labels;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    };

    // ----------------------------------------------------------------------

    class DashBarClades : public DashBarBase
    {
      public:
        DashBarClades(Tal& tal) : DashBarBase(tal) {}

        // void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct CladeParameters
        {
            std::string name;
            Color color{PINK};
            LabelParameters label{BLACK, 0.01, vertical_position::middle, horizontal_position::left, {-0.002, 0.0}, {}, NoRotation, LabelTetherParameters{}, TextStyle{}};
        };

        struct Parameters : public DashBarBase::Parameters
        {
            std::vector<CladeParameters> clades;
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
