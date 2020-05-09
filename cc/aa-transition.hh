#pragma once

#include "acmacs-base/named-type.hh"
#include "seqdb-3/sequence.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    namespace draw_tree
    {
        struct AATransitionsParameters; // draw-tree.hh
    }

    class CommonAA : public acmacs::named_string_t<struct acmacs_tal_CommonAA_tag>
    {
      public:
        constexpr static const char NoCommon{'.'};
        constexpr static const char Any{'X'};
        CommonAA() = default;

        char at(seqdb::pos0_t pos) const { return *pos < size() ? get()[*pos] : NoCommon; }
        constexpr static bool is_common(char aa) { return aa != NoCommon && aa != Any; }
        bool is_common(seqdb::pos0_t pos) const { return is_common(at(pos)); }
        // bool is_no_common(seqdb::pos0_t pos) const { return at(pos) == NoCommon; }
        ssize_t num_common() const
        {
            return std::count_if(get().begin(), get().end(), [](char aa) { return aa != NoCommon; });
        }

        void update(seqdb::pos0_t pos, char aa);
        void update(acmacs::seqdb::sequence_aligned_ref_t seq);
        void update(const CommonAA& subtree);

        void set(seqdb::pos0_t pos, char aa)
        {
            if (*pos < size())
                get()[*pos] = aa;
        }
        void set_to_no_common(seqdb::pos0_t pos) { set(pos, NoCommon); }

        std::string report() const;
        std::string report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report = std::nullopt) const;
    };

    // ----------------------------------------------------------------------

    class AA_Transition
    {
      public:
        constexpr static const char Empty = ' ';
        AA_Transition() : left(Empty), right(Empty), pos{9999} /*, for_left(nullptr) */ {}
        AA_Transition(seqdb::pos0_t aPos, char aRight) : left(Empty), right(aRight), pos(aPos) /*, for_left(nullptr) */ {}
        std::string display() const { return fmt::format("{}{}{}", left, pos, right); }
        constexpr bool empty_left() const { return left == Empty; }
        constexpr bool empty_right() const { return right == Empty; }
        constexpr bool left_right_same() const { return left == right; }
        constexpr operator bool() const { return !empty_left() && !empty_right(); } //  ignore left_right_same flag to allow drawing them if "show-same-left-right-for-pos" is true

        char left;
        char right;
        seqdb::pos0_t pos;
        // const Node* for_left; // node used to set left part, for debugging transition labels

    }; // class AA_Transition

    // ----------------------------------------------------------------------

    class AA_Transitions
    {
      public:
        // returns if anything was removed
        template <typename Func> bool remove_if(Func predicate)
        {
            const auto start = std::remove_if(std::begin(data_), std::end(data_), predicate);
            const bool anything_to_remove = start != std::end(data_);
            if (anything_to_remove)
                data_.erase(start, std::end(data_));
            return anything_to_remove;
        }

        bool empty() const { return data_.empty(); }
        void add(seqdb::pos0_t pos, char right) { data_.emplace_back(pos, right); }
        bool remove(seqdb::pos0_t pos) { return remove_if([pos](const auto& en) { return en.pos == pos; }); }
        bool remove(seqdb::pos0_t pos, char right) { return remove_if([pos,right](const auto& en) { return en.pos == pos && en.right == right; }); }
        void remove_left_right_same(const draw_tree::AATransitionsParameters& parameters);
        void remove_empty_right() { remove_if([](const auto& en) { return en.empty_right(); }); }

        const AA_Transition* find(seqdb::pos0_t pos) const
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [pos](const auto& en) { return en.pos == pos; }); found != std::end(data_))
                return &*found;
            else
                return nullptr;
        }

        operator bool() const { return std::any_of(std::begin(data_), std::end(data_), [](const auto& en) -> bool { return en; }); }

        enum class show_empty_left { no, yes };
        std::string display(std::optional<seqdb::pos1_t> pos1 = std::nullopt, show_empty_left sel = show_empty_left::no) const;
        bool has(seqdb::pos1_t pos) const;
        std::vector<std::string> names() const;

        bool contains(std::string_view label) const
        {
            return std::find_if(std::begin(data_), std::end(data_), [label](const auto& en) { return en.display() == label; }) != std::end(data_);
        }

        void set_left(seqdb::sequence_aligned_ref_t seq)
        {
            for (auto& tr : data_)
                tr.left = seq.at(tr.pos);
        }

        void add_or_replace(const AA_Transition& transition);
        void add_or_replace(const AA_Transitions& transitions);

      private:
        std::vector<AA_Transition> data_;

    }; // class AA_Transitions

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::tal::AA_Transition> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const acmacs::tal::AA_Transition& tr, FormatCtx& ctx) { return fmt::formatter<std::string>::format(tr.display(), ctx); }
};

template <> struct fmt::formatter<acmacs::tal::AA_Transitions> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const acmacs::tal::AA_Transitions& tr, FormatCtx& ctx) { return fmt::formatter<std::string>::format(tr.display(), ctx); }
};


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
