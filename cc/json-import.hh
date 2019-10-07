#pragma once

#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class JsonImportError : public std::runtime_error { public: using std::runtime_error::runtime_error; };

    Tree json_import(std::string_view filename);
}

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
