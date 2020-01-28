#pragma once

#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw.hh"

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
        void prepare(verbose verb);
        void export_tree(std::string_view filename, verbose verb);

        constexpr Tree& tree() { return tree_; }
        constexpr const Tree& tree() const { return tree_; }
        bool chart_present() const { return static_cast<bool>(chart_); } // g++9 does not like constexpr here
        const acmacs::chart::Chart& chart() const { return *chart_; } // g++9 does not like constexpr here
        constexpr Draw& draw() { return draw_; }
        constexpr const Draw& draw() const { return draw_; }

      private:
        Tree tree_;
        acmacs::chart::ChartP chart_;
        Draw draw_;
    };
} // namespace acmacs

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
