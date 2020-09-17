#pragma once

#include "acmacs-base/named-type.hh"
#include "acmacs-base/counter.hh"
#include "seqdb-3/sequence.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    class Node;
    class Tree;

    namespace draw_tree
    {
        struct AATransitionsParameters; // draw-tree.hh
    }

    // ======================================================================

    class CommonAA
    {
      public:
        constexpr static const char NoCommon{'.'};
        constexpr static const char Any{'X'};

        constexpr static inline bool is_common(char aa) { return aa != NoCommon; }

        bool empty() const { return at_pos_.empty(); }
        size_t size() const { return at_pos_.size(); }

        char at(seqdb::pos0_t pos) const
        {
            if (at_pos_.size() <= *pos || at_pos_[*pos].size() != 1)
                return NoCommon;
            else
                return at_pos_[*pos].max().first;
        }

        char at(seqdb::pos0_t pos, double tolerance) const // tolerance: see AATransitionsParameters::non_common_tolerance in draw-tree.hh
        {
            if (at_pos_.size() <= *pos)
                return NoCommon;
            else if (const auto max = at_pos_[*pos].max(); (static_cast<double>(max.second) / static_cast<double>(at_pos_[*pos].total())) > tolerance)
                return max.first;
            else
                return NoCommon;
        }

        bool is_common(seqdb::pos0_t pos, double tolerance) const { return at(pos, tolerance) != NoCommon; }

        const auto& counter(seqdb::pos0_t pos) const { return at_pos_[*pos]; }

        ssize_t num_common() const
        {
            return std::count_if(at_pos_.begin(), at_pos_.end(), [](const auto& counter) { return counter.size() < 2; });
        }

        void update(acmacs::seqdb::sequence_aligned_ref_t seq);
        void update(const CommonAA& subtree);

        std::string report() const;
        std::string report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report = std::nullopt) const;

      private:
        using counter_t = CounterCharSome<' ', '`'>;
        std::vector<counter_t> at_pos_;

        void resize(size_t size)
        {
            if (at_pos_.size() < size)
                at_pos_.resize(size);
        }
    };

    // ======================================================================

    class AA_Transition
    {
      public:
        constexpr static const char Empty = ' ';
        AA_Transition() : left(Empty), right(Empty), pos{9999} {}
        AA_Transition(seqdb::pos0_t aPos, char aLeft, char aRight) : left(aLeft), right(aRight), pos(aPos) {}
        AA_Transition(seqdb::pos0_t aPos, char aRight) : left(Empty), right(aRight), pos(aPos) {}
        std::string display() const { return fmt::format("{}{}{}", left, pos, right); }
        constexpr bool empty_left() const { return left == Empty; }
        constexpr bool empty_right() const { return right == Empty; }
        constexpr bool left_right_same() const { return left == right; }
        constexpr bool has_data() const { return !empty_left() && !empty_right(); } //  ignore left_right_same flag to allow drawing them if "show-same-left-right-for-pos" is true
        bool pos_is_in(const std::vector<acmacs::seqdb::pos1_t>& selected_pos) const { return std::find(std::begin(selected_pos), std::end(selected_pos), pos) != std::end(selected_pos); }

        char left;
        char right;
        seqdb::pos0_t pos;

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
        auto size() const { return data_.size(); }
        void add(seqdb::pos0_t pos, char right) { data_.emplace_back(pos, right); }
        void add(seqdb::pos0_t pos, char left, char right) { data_.emplace_back(pos, left, right); }
        bool remove(seqdb::pos0_t pos)
        {
            return remove_if([pos](const auto& en) { return en.pos == pos; });
        }
        bool remove(seqdb::pos0_t pos, char right)
        {
            return remove_if([pos, right](const auto& en) { return en.pos == pos && en.right == right; });
        }
        void remove_left_right_same(const draw_tree::AATransitionsParameters& parameters);
        void remove_empty_right()
        {
            remove_if([](const auto& en) { return en.empty_right(); });
        }

        const AA_Transition* find(seqdb::pos0_t pos) const
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [pos](const auto& en) { return en.pos == pos; }); found != std::end(data_))
                return &*found;
            else
                return nullptr;
        }

        AA_Transition* find(seqdb::pos0_t pos)
        {
            if (const auto found = std::find_if(std::begin(data_), std::end(data_), [pos](const auto& en) { return en.pos == pos; }); found != std::end(data_))
                return &*found;
            else
                return nullptr;
        }

        bool has_data() const
        {
            return std::any_of(std::begin(data_), std::end(data_), [](const auto& en) -> bool { return en.has_data(); });
        }
        bool has_data_for(const std::vector<acmacs::seqdb::pos1_t>& selected_pos) const
        {
            return std::any_of(std::begin(data_), std::end(data_), [&selected_pos](const auto& en) -> bool { return en.has_data() && en.pos_is_in(selected_pos); });
        }

        std::vector<seqdb::pos0_t> all_pos0() const
        {
            std::vector<seqdb::pos0_t> all_pos(data_.size(), seqdb::pos0_t{99999});
            std::transform(std::begin(data_), std::end(data_), std::begin(all_pos), [](const auto& trans) { return trans.pos; });
            return all_pos;
        }

        enum class show_empty_left { no, yes };
        std::string display(std::optional<seqdb::pos1_t> pos1 = std::nullopt, show_empty_left sel = show_empty_left::no) const;
        std::string display_most_important(size_t num) const;
        bool has(seqdb::pos1_t pos) const;
        bool has_same_left_right() const
        {
            return std::any_of(std::begin(data_), std::end(data_), [](const auto& en) -> bool { return en.left_right_same(); });
        }
        std::vector<std::string> names(const std::vector<acmacs::seqdb::pos1_t>& selected_pos, std::string_view overwrite) const; // for all pos if selected_pos is empty

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

        void clear() { data_.clear(); }

      private:
        std::vector<AA_Transition> data_;

    }; // class AA_Transitions

    void reset_aa_transitions(Tree& tree);
    void update_aa_transitions(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
    void report_aa_transitions(const Node& root, const draw_tree::AATransitionsParameters& parameters);

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------

template <> struct fmt::formatter<acmacs::tal::AA_Transition> : fmt::formatter<std::string> {
    template <typename FormatCtx> auto format(const acmacs::tal::AA_Transition& tr, FormatCtx& ctx) { return fmt::formatter<std::string>::format(tr.display(), ctx); }
};

// "{}" - format all
// "{:3} - format 3 most important or all if total number of aa transitions <= 3
template <> struct fmt::formatter<acmacs::tal::AA_Transitions> : fmt::formatter<std::string> {

    template <typename ParseContext> constexpr auto parse(ParseContext& ctx)
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == ':')
            ++it;
        if (it != ctx.end() && *it != '}') {
            char* end{nullptr};
            most_important_ = std::strtoul(&*it, &end, 10);
            it = std::next(it, end - &*it);
        }
        return std::find(it, ctx.end(), '}');
    }

    template <typename FormatCtx> auto format(const acmacs::tal::AA_Transitions& tr, FormatCtx& ctx) { return fmt::formatter<std::string>::format(tr.display_most_important(most_important_), ctx); }

  private:
    size_t most_important_{0};
};


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
