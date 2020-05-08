#pragma once

#include <memory>

#include "acmacs-base/settings.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/clades.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class TimeSeries;
    class LayoutElement;

    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings(Tal& tal) : tal_{tal} { update_env(); }

        bool apply_built_in(std::string_view name) override; // returns true if built-in command with that name found and applied

        void report_nodes(std::string_view prefix, std::string_view indent, const NodeSet& nodes) const;
        NodeSet select_nodes(const rjson::v3::value& criteria) const;

      private:
        Tal& tal_;

        Tree& tree() const { return tal_.tree(); }
        Draw& draw() const { return tal_.draw(); }

        enum class add_unique { no, yes };
        template <typename ElementType, typename ... Args> ElementType& add_element(Args&& ... args, add_unique uniq = add_unique::no);
        void init_element(LayoutElement& element);

        void apply_nodes() const;
        void update_env();
        void clade() const;
        void select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::v3::value& criteria) const;
        void ladderize();
        void margins();
        void outline(DrawOutline& draw_outline);
        void process_color_by(LayoutElementWithColoring& element);
        void add_gap();
        void add_tree();
        void add_time_series();
        void add_clades();
        void add_dash_bar();
        void add_dash_bar_clades();
        void add_title();
        void add_legend();
        void add_draw_aa_transitions();
        void add_draw_on_tree();
        void hz_sections();
        void hz_section_marker();
        void antigenic_maps();

        void process_legend(TimeSeries& time_series);
        void read_dash_parameters(LayoutElement::DashParameters& param);
        void read_label_parameters(const rjson::v3::value& source, LayoutElement::LabelParameters& param);
        void read_clade_parameters(const rjson::v3::value& source, Clades::CladeParameters& clade_parameters);
        void read_per_clade(Clades::Parameters& parameters);
        void read_text_parameters(const rjson::v3::value& source, LayoutElement::TextParameters& text_parameters) const;
        void read_line_parameters(const rjson::v3::value& source, LayoutElement::LineWithOffsetParameters& line_parameters) const;
        void read_line_parameters(const rjson::v3::value& source, LayoutElement::LineParameters& line_parameters) const;
        void read_dot_parameters(const rjson::v3::value& source, LayoutElement::WorldMapDotParameters& dot_parameters) const;

        void report_aa_at() const;

    }; // class Settings

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
