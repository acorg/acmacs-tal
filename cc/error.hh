#pragma once

#include <stdexcept>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class error : public std::runtime_error
    {
      public:
        using std::runtime_error::runtime_error;
    };

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------
