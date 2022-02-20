#pragma once

#include <string>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class JsonExportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    class Tree;

    std::string json_export(const Tree& tree, size_t indent = 1);
}

// ----------------------------------------------------------------------
