#pragma once

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class ExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Tree;

    void export_tree(const Tree& tree, std::string_view filename);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
