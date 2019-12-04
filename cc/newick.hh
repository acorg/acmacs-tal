#pragma once

#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tree;

    class NewickImportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };
    class NewickExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    void newick_import(std::string_view filename, Tree& tree);
    std::string newick_export(const Tree& tree);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
