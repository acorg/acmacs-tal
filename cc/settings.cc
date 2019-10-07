#include "acmacs-base/read-file.hh"
#include "acmacs-tal/settings.hh"
#include "acmacs-tal/tree.hh"

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Settings::apply_built_in(std::string_view name) const
{
    if (name == "report-cumulative") {
        if (const auto output_filename = getenv("report-cumulative-output", ""); !output_filename.empty()) {
            acmacs::file::write(output_filename, tree_.report_cumulative());
        }
        // else {
        //     fmt::print(stderr, "DEBUG: report-cumulative-output empyt\n");
        // }
        return true;
    }
    return false;

} // acmacs::tal::v3::Settings::apply_built_in

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
