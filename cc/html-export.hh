#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class HtmlExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Tree;

    std::string html_export(const Tree& tree);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
