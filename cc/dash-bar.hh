#pragma once

#include "acmacs-base/flat-map.hh"
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

        virtual void draw_label(acmacs::surface::Surface& surface, std::string_view text, Color color, const parameters::Label& label_param) const;

        // ----------------------------------------------------------------------

        struct Parameters
        {
            bool report{false};
            parameters::Dash dash{1.0, Pixels{0.5}}; // width, line_width
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
            std::vector<parameters::Label> labels;
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
            parameters::Label label{BLACK, 0.01, parameters::vertical_position::middle, parameters::horizontal_position::left, {-0.002, 0.0}, {}, NoRotation, parameters::LabelTether{}, TextStyle{}};
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

    // ----------------------------------------------------------------------

    class DashBarAAAt : public DashBarBase
    {
      public:
        DashBarAAAt(Tal& tal) : DashBarBase(tal) {}

        // void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        // ----------------------------------------------------------------------

        struct Parameters : public DashBarBase::Parameters
        {
            seqdb::pos1_t pos{193};
            small_map_with_unique_keys_t<char, Color> colors_by_aa;
            std::vector<Color> colors_by_frequency;
            small_map_with_unique_keys_t<char, parameters::Label> labels_by_aa;
            std::vector<parameters::Label> labels_by_frequency;
        };

        constexpr Parameters& parameters() { return parameters_; }
        constexpr const Parameters& parameters() const { return parameters_; }

      private:
        Parameters parameters_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
