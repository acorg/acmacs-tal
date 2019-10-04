#include "acmacs-tal/newick.hh"

// ----------------------------------------------------------------------

acmacs::tal::v3::Tree acmacs::tal::v3::newick_import(std::string_view filename)
{
    return Tree{};

} // acmacs::tal::v3::newick_import

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::newick_export(const Tree& /*tree*/)
{
    throw std::runtime_error("Not Implemented Yet");

} // acmacs::tal::v3::newick_export

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
