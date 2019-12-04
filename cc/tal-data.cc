#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/import-export.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::import_tree(std::string_view filename)
{
    if (!filename.empty()) {
        tree_.erase();
        acmacs::tal::import_tree(filename, tree_);
    }

} // acmacs::tal::v3::Tal::import_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::import_chart(std::string_view filename)
{
    if (!filename.empty()) {
        chart_ = acmacs::chart::import_from_file(filename);

    }
} // acmacs::tal::v3::Tal::import_chart

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::export_tree(std::string_view filename)
{
    fs::path filepath{filename};
    const auto ext = filepath.extension();
    if (ext == ".pdf") {
    }
    else {
        acmacs::tal::export_tree(filename, tree_);
    }

} // acmacs::tal::v3::Tal::export_tree

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
