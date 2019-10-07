#pragma once

#include "acmacs-base/settings.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings(Tree& tree) : tree_{tree} {}
        virtual bool apply_built_in(std::string_view name) const; // returns true if built-in command with that name found and applied

        void report_nodes(std::string_view prefix, std::string_view indent, const NodeConstSet& nodes) const;
        NodeConstSet select_nodes(const rjson::value& criteria) const;
        NodeConstSet select_and_report_nodes(const rjson::value& criteria, bool report) const;

      private:
        Tree& tree_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
