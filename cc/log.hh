#pragma once

#include "acmacs-map-draw/log.hh"

// ----------------------------------------------------------------------

#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wexit-time-destructors"
#pragma GCC diagnostic ignored "-Wglobal-constructors"
#endif

namespace acmacs::log::inline v1
{
    const log_key_t clades{"clades"};
    const log_key_t coloring{"coloring"};
    const log_key_t tree{"tree"};
    const log_key_t hz_sections{"hz-sections"};
    const log_key_t time_series{"time-series"};

} // namespace acmacs::log::inline v1

#pragma GCC diagnostic pop

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
