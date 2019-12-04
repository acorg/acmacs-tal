#pragma once

#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;
    class Tree;

    struct ImportError : public std::runtime_error { using std::runtime_error::runtime_error; };
    struct ExportError : public std::runtime_error { using std::runtime_error::runtime_error; };

    void import_tree(std::string_view filename, Tree& tree);
    void export_tree(std::string_view filename, const Tree& tree);

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
