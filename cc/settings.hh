#pragma once

#include <memory>

#include "acmacs-base/settings.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    using ChartP = std::shared_ptr<Chart>;
}

namespace acmacs::tal::inline v3
{
    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings() = default;

        void tree(Tree& tree);
        Tree& tree() const { if (!tree_) throw error{"tree was not set"}; return *tree_; }

        void chart(const acmacs::chart::ChartP& chart);
        const acmacs::chart::Chart& chart() const { if (!chart_) throw error{"chart was not set"}; return *chart_; }

        bool apply_built_in(std::string_view name) override; // returns true if built-in command with that name found and applied

        void report_nodes(std::string_view prefix, std::string_view indent, const NodeSet& nodes) const;
        NodeSet select_nodes(const rjson::value& criteria) const;

      private:
        Tree* tree_;
        acmacs::chart::ChartP chart_;

        void apply_nodes() const;
        void update_env();
        void clade() const;
        void select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::value& criteria) const;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
