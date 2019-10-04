#include "acmacs-base/read-file.hh"
#include "acmacs-tal/export.hh"
#include "acmacs-tal/newick.hh"
#include "acmacs-tal/json-export.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::export_tree(const Tree& tree, std::string_view filename)
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
    else
        throw ExportError{fmt::format("cannot infer export method from filename: {}", filename)};
    acmacs::file::write(filename, exported);

} // acmacs::tal::v3::export_tree

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
