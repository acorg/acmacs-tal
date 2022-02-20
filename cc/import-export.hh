#pragma once

#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tal;
    class Tree;

    struct ImportError : public std::runtime_error { using std::runtime_error::runtime_error; };
    struct ExportError : public std::runtime_error { using std::runtime_error::runtime_error; };

    struct ExportOptions
    {
        bool add_aa_substitution_labels{false}; // newick export, SARS
    };

    void import_tree(std::string_view filename, Tree& tree);
    void export_tree(std::string_view filename, const Tree& tree, const ExportOptions& options);

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
