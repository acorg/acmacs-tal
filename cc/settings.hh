#pragma once

#include "acmacs-base/settings.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tree;

    class Settings : public acmacs::settings::Settings
    {
      public:
        Settings(Tree& tree) : tree_{tree} {}
        virtual bool apply_built_in(std::string_view name) const; // returns true if built-in command with that name found and applied

      private:
        Tree& tree_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
