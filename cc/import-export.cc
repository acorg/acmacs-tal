#include "acmacs-base/read-file.hh"
#include "acmacs-base/fmt.hh"
#include "acmacs-tal/import-export.hh"
#include "acmacs-tal/newick.hh"
#include "acmacs-tal/json-import.hh"
#include "acmacs-tal/json-export.hh"
#include "acmacs-tal/html-export.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::import_tree(std::string_view filename, Tree& tree)
{
    fs::path filepath{filename};
    auto ext = filepath.extension();
    if (ext == ".xz" || ext == ".bz2")
        ext = filepath.stem().extension();
    if (ext == ".newick") {
        try {
            newick_import(filename, tree);
        }
        catch (NewickImportError& err) {
            throw ImportError{fmt::format("cannot import from newick: {}", err)};
        }
    }
    else if (filename == "-" || ext == ".json") {
        try {
            json_import(filename, tree);
        }
        catch (JsonImportError& err) {
            throw ImportError{fmt::format("cannot import from json: {}", err)};
        }
    }
    else
        throw ImportError{fmt::format("cannot infer import method from filename: {}", filename)};
    tree.just_imported();

} // acmacs::tal::v3::import_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::export_tree(std::string_view filename, const Tree& tree)
{
    fs::path filepath{filename};
    auto ext = filepath.extension();
    if (ext == ".xz" || ext == ".bz2")
        ext = filepath.stem().extension();
    std::string exported;
    if (ext == ".newick") {
        try {
            exported = newick_export(tree);
        }
        catch (NewickExportError& err) {
            throw ExportError{fmt::format("cannot export to newick: {}", err)};
        }
    }
    else if (filename == "-" || filename == "=" || ext == ".json") {
        try {
            exported = json_export(tree);
        }
        catch (JsonExportError& err) {
            throw ExportError{fmt::format("cannot export to json: {}", err)};
        }
    }
    else if (ext == ".html") {
        try {
            exported = html_export(tree);
        }
        catch (JsonExportError& err) {
            throw ExportError{fmt::format("cannot export to html: {}", err)};
        }
    }
    else
        throw ExportError{fmt::format("cannot infer export method from filename: {}", filename)};
    acmacs::file::write(filename, exported);

} // acmacs::tal::v3::export_tree

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
