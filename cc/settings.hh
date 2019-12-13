#pragma once

#include <memory>

#include "acmacs-base/settings.hh"
#include "acmacs-tal/tal-data.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class TimeSeries;

    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings(Tal& tal) : tal_{tal} { update_env(); }

        bool apply_built_in(std::string_view name, verbose verb) override; // returns true if built-in command with that name found and applied

        void report_nodes(std::string_view prefix, std::string_view indent, const NodeSet& nodes) const;
        NodeSet select_nodes(const rjson::value& criteria) const;

      private:
        Tal& tal_;

        Tree& tree() const { return tal_.tree(); }
        Draw& draw() { return tal_.draw(); }

        template <typename ElementType, typename ... Args> ElementType& add_element(Args&& ... args);

        void apply_nodes() const;
        void update_env();
        void clade() const;
        void select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::value& criteria) const;
        void ladderize();
        void margins();
        void outline(DrawOutline& draw_outline);
        void process_color_by(LayoutElementWithColoring& element);
        void add_tree();
        void add_time_series();
        void add_clades();
        void add_dash_bar_clades();
        void add_title();
        void add_legend();

        void read_dash_parameters(LayoutElement::DashParameters& param);
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
