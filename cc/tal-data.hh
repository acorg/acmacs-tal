#pragma once

#include "acmacs-tal/tree.hh"
#include "acmacs-tal/draw.hh"
#include "acmacs-tal/import-export.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
    using ChartP = std::shared_ptr<Chart>;
}

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Settings;

    class Tal
    {
      public:
        Tal() = default;

        void import_tree(std::string_view filename);
        void import_chart(std::string_view filename);
        void export_tree(std::string_view filename, const ExportOptions& options);

        void reset();
        void prepare();

        constexpr Tree& tree() { return tree_; }
        constexpr const Tree& tree() const { return tree_; }
        bool chart_present() const { return static_cast<bool>(chart_); } // g++9 does not like constexpr here
        const acmacs::chart::Chart& chart() const { return *chart_; } // g++9 does not like constexpr here
        acmacs::chart::ChartP chartp() const { return chart_; }
        constexpr Draw& draw() { return draw_; }
        constexpr const Draw& draw() const { return draw_; }

        constexpr Settings& settings() { return *settings_; }
        constexpr const Settings& settings() const { return *settings_; }

      private:
        Tree tree_;
        acmacs::chart::ChartP chart_;
        Draw draw_;
        Settings* settings_{nullptr};

        friend class Settings;

    }; // class Tal

} // namespace acmacs

// ----------------------------------------------------------------------
