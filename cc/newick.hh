#pragma once

#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class NewickImportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    Tree newick_import(std::string_view filename);
    std::string newick_export(const Tree& tree);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
