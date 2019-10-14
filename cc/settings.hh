#pragma once

#include "acmacs-base/settings.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings() = default;

        void tree(Tree& tree);
        Tree& tree() const { if (!tree_) throw error{"tree was not set"}; return *tree_; }

        virtual bool apply_built_in(std::string_view name) const; // returns true if built-in command with that name found and applied

        void report_nodes(std::string_view prefix, std::string_view indent, const NodeSet& nodes) const;
        NodeSet select_nodes(const rjson::value& criteria) const;
        NodeSet select_and_report_nodes(const rjson::value& criteria, bool report) const;

      private:
        Tree* tree_;

        void apply_nodes() const;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
