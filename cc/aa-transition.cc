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

std::string acmacs::tal::v3::CommonAA::report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report) const
{
    fmt::memory_buffer out;
    size_t num_common = 0;
    for (seqdb::pos0_t pos{0}; *pos < size(); ++pos) {
        if (is_common(pos) && !parent.is_common(pos) && (!pos_to_report || pos_to_report == pos)) {
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

void acmacs::tal::v3::AA_Transitions::add_or_replace(const AA_Transition& to_add)
{
    if (const auto found = std::find_if(std::begin(data_), std::end(data_), [&to_add](const auto& en) { return en.pos == to_add.pos; }); found != std::end(data_))
        *found = to_add;
    else
        data_.push_back(to_add);

} // acmacs::tal::v3::AA_Transitions::add_or_replace

// ----------------------------------------------------------------------

void acmacs::tal::v3::AA_Transitions::add_or_replace(const AA_Transitions& transitions)
{
    for (const auto& transition : transitions.data_)
        add_or_replace(transition);

} // acmacs::tal::v3::AA_Transitions::add_or_replace

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::AA_Transitions::display(std::optional<seqdb::pos1_t> pos1, show_empty_left sel) const
{
    fmt::memory_buffer output;
    for (const auto& en : data_) {
        if ((sel == show_empty_left::yes || !en.empty_left()) && !en.empty_right() && (!pos1 || *pos1 == en.pos))
            fmt::format_to(output, " {}", en);
    }
    if (const auto result = fmt::to_string(output); result.size() > 1)
        return result.substr(1);
    else
        return result;

} // acmacs::tal::v3::AA_Transitions::display

// ----------------------------------------------------------------------

bool acmacs::tal::v3::AA_Transitions::has(seqdb::pos1_t pos) const
{
    for (const auto& en : data_) {
        if (en.pos == pos && !en.empty_right())
            return true;
    }
    return false;

} // acmacs::tal::v3::AA_Transitions::has

// ----------------------------------------------------------------------

std::vector<std::string> acmacs::tal::v3::AA_Transitions::names() const
{
    std::vector<std::string> result;
    for (const auto& en : data_) {
        if (!en.empty_left())
            result.push_back(en.display());
    }
    return result;

} // acmacs::tal::v3::AA_Transitions::names

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
