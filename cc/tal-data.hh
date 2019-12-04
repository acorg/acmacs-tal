#pragma once

#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    using ChartP = std::shared_ptr<Chart>;
}

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal
    {
      public:
        Tal() = default;

        void import_tree(std::string_view filename);
        void import_chart(std::string_view filename);
        void export_tree(std::string_view filename);

        Tree& tree() { return tree_; }
        const Tree& tree() const { return tree_; }
        bool chart_present() const { return static_cast<bool>(chart_); }
        const acmacs::chart::Chart& chart() const { return *chart_; }

      private:
        Tree tree_;
        acmacs::chart::ChartP chart_;
    };
} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
