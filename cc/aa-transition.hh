#pragma once

#include "acmacs-base/named-type.hh"
// #include "acmacs-base/counter.hh"
#include "seqdb-3/sequence.hh"
#include "acmacs-tal/aa-counter.hh"

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

    // constexpr const char NoCommon{'.'};
    constexpr const char NoCommon = AACounter::nothing;
    constexpr const char Any{'X'};
    constexpr inline bool is_common(char aa) { return aa != NoCommon; }

    class CommonAA
    {
      public:
        CommonAA(size_t number_of_positions, size_t number_of_aa) : at_pos_(number_of_positions, number_of_aa) {}
        bool empty(seqdb::pos0_t pos) const { return at_pos_.empty(low_mem_mode() ? 0ul : *pos); }
        size_t allocated() const { return at_pos_.allocated(); }
        size_t max_count() const { return at_pos_.max_count(); }

        bool low_mem_mode() const { return at_pos_.low_mem_mode(); }

        char at(seqdb::pos0_t pos) const
        {
            if (!low_mem_mode())
                return at_pos_.max(*pos);
            else
                return at_pos_.max(0); // support for eu-20200915-low-mem
        }

        template <bool dbg = false> char at(seqdb::pos0_t pos, double tolerance) const // tolerance: see AATransitionsParameters::non_common_tolerance_for() in draw-tree.hh
        {
            if (low_mem_mode())
                pos = seqdb::pos0_t{0}; // for update_aa_transitions_eu_20200915_per_pos
            const auto total = at_pos_.total(*pos);
            if (const auto& max = at_pos_.max_count(*pos); (static_cast<double>(max.count) / static_cast<double>(total)) > tolerance) {
                if constexpr (dbg)
                    AD_DEBUG(total > 100, "                    common:{} @{} <- at_pos_[pos].max():{} total:{} max.second/total: {} > tolerance", max.aa, pos, max.count, total,
                             static_cast<double>(max.count) / static_cast<double>(total));
                return max.aa;
            }
            else {
                if constexpr (dbg)
                    AD_DEBUG(total > 100, "                    common:- @{} <- at_pos_[pos].max():{} total:{} max.second/total: {} <= tolerance", pos, max.count, total,
                             static_cast<double>(max.count) / static_cast<double>(total));
                return NoCommon;
            }
        }

        bool is_common(seqdb::pos0_t pos, double tolerance) const { return at(pos, tolerance) != NoCommon; }

        std::string report_sorted_max_first(seqdb::pos0_t pos, std::string_view format) const { return at_pos_.report_sorted_max_first(*pos, format); }

        // const auto& counter(seqdb::pos0_t pos) const { return at_pos_[*pos]; }

        // ssize_t num_common() const
        // {
        //     return std::count_if(at_pos_.begin(), at_pos_.end(), [](const auto& counter) { return counter.size() < 2; });
        // }

        void update(acmacs::seqdb::sequence_aligned_ref_t seq)
        {
            for (AACounter::pos_t pos0{0}; pos0 < *seq.size(); ++pos0) {
                if (const auto aa = seq[pos0]; aa != Any)
                    at_pos_.count(pos0, aa);
            }
        }

        void update(const CommonAA& subtree)
        {
            at_pos_.add(subtree.at_pos_);
        }

        // std::string report() const;
        // std::string report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report = std::nullopt) const;

        // void resize(size_t size)
        // {
        //     if (__builtin_expect(size <= AACounter::number_of_positions, 1)) // [[likely]]
        //         ;
        //     else // [[unlikely]]
        //         throw std::runtime_error{fmt::format("CommonAA::resize {}: change number_of_positions in aa-counter.hh:15", size)};
        // }

      private:
        AACounter at_pos_;
    };

    class CommonAA_Ptr
    {
      public:
        CommonAA_Ptr() = default;
        CommonAA_Ptr(const CommonAA_Ptr&) {} // not copied
        CommonAA_Ptr(CommonAA_Ptr&&) = default;
        CommonAA_Ptr& operator=(CommonAA_Ptr&&) = default;

        void create(size_t number_of_positions, size_t number_of_aa) { data_ = std::make_unique<CommonAA>(number_of_positions, number_of_aa); }
        size_t allocated() const { return data_ ? data_->allocated() : 0; }
        size_t max_count() const { return data_ ? data_->max_count() : 0; }

        CommonAA* operator->() { return data_.get(); }
        const CommonAA* operator->() const { return data_.get(); }
        const CommonAA& operator*() const { return *data_; }

      private:
        std::unique_ptr<CommonAA> data_{nullptr};
    };

    // class CommonAA
    // {
    //   public:
    //     // constexpr static const char NoCommon{'.'};
    //     // constexpr static const char Any{'X'};

    //     // constexpr static inline bool is_common(char aa) { return aa != NoCommon; }

    //     // bool empty() const { return at_pos_.empty(); }
    //     // size_t size() const { return at_pos_.size(); }

    //     char at(seqdb::pos0_t pos) const
    //     {
    //         if (at_pos_.size() <= *pos || at_pos_[*pos].size() != 1)
    //             return NoCommon;
    //         else
    //             return at_pos_[*pos].max().first;
    //     }

    //     template <bool dbg = false> char at(seqdb::pos0_t pos, double tolerance) const // tolerance: see AATransitionsParameters::non_common_tolerance_for() in draw-tree.hh
    //     {
    //         // AD_DEBUG(dbg, "                CommonAA.at(pos:{}, tolerance:{}): at_pos_.size():{}", pos, tolerance, at_pos_.size());
    //         if (at_pos_.size() <= *pos) {
    //             if constexpr (dbg)
    //                 AD_DEBUG("                    common:- @{} <- at_pos_.size():{} <= pos", pos, at_pos_.size());
    //             return NoCommon;
    //         }
    //         else if (const auto max = at_pos_[*pos].max(); (static_cast<double>(max.second) / static_cast<double>(at_pos_[*pos].total())) > tolerance) {
    //             if constexpr (dbg)
    //                 AD_DEBUG(at_pos_[*pos].total() > 100, "                    common:{} @{} <- at_pos_[pos].max():{} at_pos_[*pos].total():{} max.second/total: {} > tolerance", max.first, pos,
    //                 max,
    //                          at_pos_[*pos].total(), static_cast<double>(max.second) / static_cast<double>(at_pos_[*pos].total()));
    //             return max.first;
    //         }
    //         else {
    //             if constexpr (dbg)
    //                 AD_DEBUG(at_pos_[*pos].total() > 100, "                    common:- @{} <- at_pos_[pos].max():{} at_pos_[*pos].total():{} max.second/total: {} <= tolerance", max, pos,
    //                          at_pos_[*pos].total(), static_cast<double>(max.second) / static_cast<double>(at_pos_[*pos].total()));
    //             return NoCommon;
    //         }
    //     }

    //     bool is_common(seqdb::pos0_t pos, double tolerance) const { return at(pos, tolerance) != NoCommon; }

    //     const auto& counter(seqdb::pos0_t pos) const { return at_pos_[*pos]; }

    //     ssize_t num_common() const
    //     {
    //         return std::count_if(at_pos_.begin(), at_pos_.end(), [](const auto& counter) { return counter.size() < 2; });
    //     }

    //     void update(acmacs::seqdb::sequence_aligned_ref_t seq)
    //     {
    //         for (size_t pos0{0}; pos0 < *seq.size(); ++pos0) {
    //             if (const auto aa = seq[pos0]; aa != Any)
    //                 at_pos_[pos0].count(aa);
    //         }
    //     }

    //     void update(const CommonAA& subtree)
    //     {
    //         if (at_pos_.empty()) {
    //             at_pos_ = subtree.at_pos_;
    //         }
    //         else {
    //             for (size_t pos{0}; pos < at_pos_.size(); ++pos)
    //                 at_pos_[pos].update(subtree.at_pos_[pos]);
    //         }
    //     }

    //     // std::string report() const;
    //     // std::string report(const CommonAA& parent, std::optional<seqdb::pos1_t> pos_to_report = std::nullopt) const;

    //     void resize(size_t size)
    //     {
    //         // if (at_pos_.size() < size)
    //         //     throw std::runtime_error{fmt::format("CommonAA<{}>::resize {}: change in aa_transition.hh:21", SIZE, size)};
    //         // if (at_pos_.size() < size)
    //         at_pos_.resize(size);
    //     }

    //   private:
    //     // using counter_t = CounterCharSome<'-', '['>;
    //     // std::vector<counter_t> at_pos_;
    // };

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
        void remove_left_right_same(const draw_tree::AATransitionsParameters& parameters, const Node& node);
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

        auto begin() const { return data_.begin(); }
        auto end() const { return data_.end(); }
        auto begin() { return data_.begin(); }
        auto end() { return data_.end(); }

      private:
        std::vector<AA_Transition> data_;

    }; // class AA_Transitions

    void reset_aa_transitions(Tree& tree);
    void update_aa_transitions(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
    void report_aa_transitions(const Node& root, const draw_tree::AATransitionsParameters& parameters);

    namespace detail
    {
        void update_common_aa(Tree& tree, seqdb::pos0_t longest_aa_sequence, size_t number_of_aas);
        void update_common_aa_for_pos(Tree& tree, seqdb::pos0_t pos, size_t number_of_aas);
        void update_aa_transitions_eu_20210503(Tree& tree, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20210503.cc
        void update_aa_transitions_eu_20200915(Tree& tree, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20200915.cc
        void update_aa_transitions_eu_20200915_per_pos(Tree& tree, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20200915.cc
        void update_aa_transitions_eu_20200915_stage_3(Tree& tree, seqdb::pos0_t pos, const seqdb::sequence_aligned_ref_t& root_sequence, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20200915.cc
        void update_aa_transitions_eu_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20200915.cc
        void update_aa_transitions_derek_2016(Tree& tree, const draw_tree::AATransitionsParameters& parameters); // aa-transition-20200915.cc
    }

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
