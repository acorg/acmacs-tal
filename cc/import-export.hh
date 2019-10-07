#pragma once

#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class ImportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class ExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    Tree import_tree(std::string_view filename);

    void export_tree(const Tree& tree, std::string_view filename);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
