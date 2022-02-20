#pragma once

#include <string_view>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Tree;

    struct JsonImportError : public std::runtime_error { using std::runtime_error::runtime_error; };

    void json_import(std::string_view filename, Tree& tree);
}

// ----------------------------------------------------------------------
