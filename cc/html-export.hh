#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class HtmlExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Tree;

    std::string html_export(const Tree& tree);
    std::string names_export(const Tree& tree);
    std::string text_export(const Tree& tree);
}

// ----------------------------------------------------------------------
