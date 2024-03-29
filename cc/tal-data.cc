#include "acmacs-base/filesystem.hh"
#include "acmacs-chart-2/factory-import.hh"
#include "acmacs-base/timeit.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::import_tree(std::string_view filename)
{
    const Timeit ti{"importing tree"};
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

void acmacs::tal::v3::Tal::reset()
{
    reset_aa_transitions(tree());
    draw().reset();

} // acmacs::tal::v3::Tal::reset

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::prepare()
{
    draw().prepare();

} // acmacs::tal::v3::Tal::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::Tal::export_tree(std::string_view filename, const ExportOptions& options)
{
    const fs::path filepath{filename};
    const auto ext = filepath.extension();
    if (ext == ".pdf") {
        draw().export_pdf(filename);
    }
    else {
        acmacs::tal::export_tree(filename, tree_, options);
    }

} // acmacs::tal::v3::Tal::export_tree

// ----------------------------------------------------------------------
