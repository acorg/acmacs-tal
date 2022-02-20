#pragma once

#include "acmacs-base/time-series.hh"
#include "acmacs-base/flat-map.hh"
#include "acmacs-tal/layout.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;

    class TimeSeries : public LayoutElementWithColoring
    {
      public:
        TimeSeries(Tal& tal) : LayoutElementWithColoring(tal, 0.0) {}

        void prepare(preparation_stage_t stage) override;
        void draw(acmacs::surface::Surface& surface) const override;

        void add_horizontal_line_above(const Node* node, const parameters::Line& line, bool warn_if_present);

        // ----------------------------------------------------------------------

        struct SlotSeparatorParameters : parameters::Line
        {
            SlotSeparatorParameters() : parameters::Line{BLACK, Pixels{0.5}, surface::Dash::NoDash} {}
        };

        struct SlotLabelParameters
        {
            Rotation rotation{Rotation90DegreesAnticlockwise}; // Rotation90DegreesAnticlockwise, Rotation90DegreesClockwise
            Color color{BLACK};
            double scale{0.7};                         // relative to parameters_.slot.width
            double offset{0.002}; // relative to the time series area height
        };

        struct SlotParameters
        {
            double width{0.01}; // relative to the time series area height
            std::array<SlotSeparatorParameters, 12> separator; // left separator in each month, only the first element used for non-monthly ts
            std::array<Color, 12> background{TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT, TRANSPARENT};
            SlotLabelParameters label;
        };

        struct PerNodeParameters
        {
            std::optional<Color> color;
            std::optional<double> width; // fraction of slot width
            std::optional<Pixels> line_width;
        };

        enum class color_scale_type { bezier_gradient };
        struct ColorScaleParameters
        {
            bool show{true};
            color_scale_type type{color_scale_type::bezier_gradient};
            std::array<Color, 3> colors{Color{0x440154}, Color{0x40ffff}, Color{0xfde725}};
            double offset{0.008};
            double height{0.01};
        };

        struct LegendParameters
        {
            bool show{true};
            double scale{0.012}; // relative to the time series area height
            double offset{0.022}; // relative to the time series area height
            double gap_scale{1.1}; // relative to scale
            ColoringByPosBase::legend_show_count show_count{ColoringByPosBase::legend_show_count::yes};
            ColoringByPosBase::legend_show_pos show_pos{ColoringByPosBase::legend_show_pos::yes};
            double count_scale{0.3}; // relative to scale
            Color pos_color{BLACK};
            Color count_color{GREY};
        };

        struct Parameters
        {
            acmacs::time_series::parameters time_series;
            SlotParameters slot;
            parameters::Dash dash;
            small_map_with_unique_keys_t<seq_id_t, PerNodeParameters> per_nodes;
            ColorScaleParameters color_scale;
            std::string report;
            LegendParameters legend;
        };

        virtual Parameters& parameters() { return parameters_; }
        virtual const Parameters& parameters() const { return parameters_; }

        acmacs::color::Modifier color_for(date::year_month_day date) const;

      protected:
        struct dash_t
        {
            Color color{PINK};
            Pixels line_width{0.0};
            double width{0.0};
            size_t slot{0};
            double y;
        };

        const auto& series() const { return series_; }
        auto& dashes() { return dashes_; }

        virtual void set_width_to_height_ratio();

        virtual void prepare_coloring();
        virtual void prepare_dash(size_t slot_no, const Node& leaf);

        virtual void draw_background_separators(acmacs::surface::Surface& surface) const;
        virtual void draw_labels(acmacs::surface::Surface& surface) const;
        virtual void draw_legend(acmacs::surface::Surface& surface) const;

      private:
        struct horizontal_line_t : public parameters::Line
        {
            const Node* node;

            constexpr horizontal_line_t(const Node* a_node, const parameters::Line& a_line) : parameters::Line(a_line), node(a_node) {}
        };

        Parameters parameters_;
        acmacs::time_series::series series_;
        std::vector<horizontal_line_t> horizontal_lines_;
        std::vector<Color> color_scale_;
        std::vector<dash_t> dashes_;

        void prepare_dashes();
        void make_color_scale();
        void draw_color_scale(acmacs::surface::Surface& surface) const;

        std::pair<std::string, std::string> labels(const acmacs::time_series::slot& slot) const;
        size_t slot_month(const acmacs::time_series::slot& slot) const;
        void draw_horizontal_lines(acmacs::surface::Surface& surface, const DrawTree* draw_tree) const;

    };

    // ----------------------------------------------------------------------

    class TimeSeriesWithShift : public TimeSeries
    {
      public:
        using TimeSeries::TimeSeries;

        struct Parameters : public TimeSeries::Parameters
        {
            size_t shift;
        };

        Parameters& parameters() override { return parameters_; }
        const Parameters& parameters() const override { return parameters_; }

        void add_coloring(std::unique_ptr<Coloring> coloring) { coloring_.push_back(std::move(coloring)); }


      protected:
        void set_width_to_height_ratio() override;

        void prepare_coloring() override;
        void prepare_dash(size_t slot_no, const Node& leaf) override;

        void draw_background_separators(acmacs::surface::Surface& surface) const override;
        void draw_labels(acmacs::surface::Surface& surface) const override;
        void draw_legend(acmacs::surface::Surface& surface) const override;

      private:
        Parameters parameters_;
        std::vector<std::unique_ptr<Coloring>> coloring_;

        size_t number_of_slots() const { return series().size() + (coloring_.size() - 1) * parameters().shift; }
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
