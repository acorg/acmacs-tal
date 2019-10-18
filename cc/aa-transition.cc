#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::CommonAA::update(seqdb::pos0_t pos, char aa)
{
    if (aa != Any) {
        if (at(pos) == Any)
            set(pos, aa);
        else if (at(pos) != aa)
            set_to_no_common(pos);
    }

} // acmacs::tal::v3::CommonAA::update

// ----------------------------------------------------------------------

void acmacs::tal::v3::CommonAA::update(acmacs::seqdb::sequence_aligned_ref_t seq)
{
    if (empty()) {
        get().assign(*seq);
    }
    else {
        if (seq.size() < size())
            get().resize(seq.size());
        for (seqdb::pos0_t pos{0}; *pos < size(); ++pos)
            update(pos, seq[*pos]);
    }

} // acmacs::tal::v3::CommonAA::update

// ----------------------------------------------------------------------

void acmacs::tal::v3::CommonAA::update(const CommonAA& subtree)
{
    if (empty()) {
        *this = subtree;
    }
    else {
        if (subtree.size() < size())
            get().resize(subtree.size());
        for (seqdb::pos0_t pos{0}; *pos < size(); ++pos)
            update(pos, subtree[*pos]);
    }

} // acmacs::tal::v3::CommonAA::update

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::CommonAA::report() const
{
    fmt::memory_buffer out;
    for (seqdb::pos0_t pos{0}; *pos < size(); ++pos) {
        if (is_common(pos))
            fmt::format_to(out, " {}{}", pos, at(pos));
    }
    return fmt::format("common:{} {}", num_common(), fmt::to_string(out));

} // acmacs::tal::v3::CommonAA::report

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::CommonAA::report(const CommonAA& parent) const
{
    fmt::memory_buffer out;
    size_t num_common = 0;
    for (seqdb::pos0_t pos{0}; *pos < size(); ++pos) {
        if (is_common(pos) && !parent.is_common(pos)) {
            fmt::format_to(out, " {}{}", pos, at(pos));
            ++num_common;
        }
    }
    if (num_common == 0)
        return std::string{};
    else
        return fmt::format("common:{} {}", num_common, fmt::to_string(out));

} // acmacs::tal::v3::CommonAA::report

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::AA_Transitions::display(bool show_empty_left) const
{
    fmt::memory_buffer output;
    for (const auto& en : data_) {
        // if (show_empty_left || !en.empty_left())
            fmt::format_to(output, " {}", en);
    }
    if (const auto result = fmt::to_string(output); result.size() > 1)
        return result.substr(1);
    else
        return result;

} // acmacs::tal::v3::AA_Transitions::display

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
