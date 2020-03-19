#pragma once

#include <string>
#include <vector>
#include <tuple>
#include <optional>
#include <algorithm>

#include "acmacs-base/debug.hh"
#include "acmacs-base/named-type.hh"
#include "acmacs-base/counter.hh"
#include "acmacs-base/date.hh"
#include "acmacs-base/color.hh"
#include "acmacs-base/flat-set.hh"
#include "seqdb-3/seqdb.hh"
#include "acmacs-tal/aa-transition.hh"

// ----------------------------------------------------------------------

namespace acmacs::chart
{
    class Chart;
}

namespace acmacs::tal::inline v3
{
    class Node;

    class error : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };

    using seq_id_t = acmacs::seqdb::seq_id_t; // string, not string_view to support populate_with_nuc_duplicates

    using EdgeLength = named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>;
    // class EdgeLength : public named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>
    // {
    //   public:
    // };

    using NodePath = acmacs::named_vector_t<const Node*, struct acmacs_tal_NodePath_tag>;

    constexpr const EdgeLength EdgeLengthNotSet{-1.0};

    struct ladderize_helper_t
    {
        EdgeLength edge{EdgeLengthNotSet};
        std::string_view date;
        seq_id_t seq_id;
    };

    struct node_id_t
    {
        using value_type = unsigned short;
        constexpr static const value_type NotSet{static_cast<value_type>(-1)};

        value_type vertical{NotSet};
        value_type horizontal{NotSet};
        constexpr bool empty() const { return vertical == NotSet; }
        constexpr bool operator<(const node_id_t& rhs) const { return vertical < rhs.vertical; }
    };

    // ----------------------------------------------------------------------

    class Node
    {
      public:
        using Subtree = std::vector<Node>;

        Node() = default;
        Node(const seq_id_t& a_seq_id, EdgeLength a_edge) : edge_length{a_edge}, seq_id{a_seq_id} {}

        bool is_leaf() const noexcept { return subtree.empty(); } // seq_id may contain generated node name used for debugging

        Node& add_leaf(const seq_id_t& a_seq_id, EdgeLength a_edge) { return subtree.emplace_back(a_seq_id, a_edge); }
        Node& add_subtree() { return subtree.emplace_back(); }

        const Node& find_first_leaf() const;

        // all nodes
        EdgeLength edge_length{0.0};
        mutable EdgeLength cumulative_edge_length{EdgeLengthNotSet};
        bool hidden{false};

        // leaf node only
        seq_id_t seq_id;
        acmacs::seqdb::ref ref;
        acmacs::seqdb::sequence_aligned_ref_t aa_sequence;
        acmacs::seqdb::sequence_aligned_ref_t nuc_sequence;
        std::string_view strain_name;  // from seqdb
        std::string_view date;
        std::string_view continent;
        std::string_view country;
        std::vector<std::string_view> hi_names;
        acmacs::flat_set_t<std::string> clades;

        // middle node only
        Subtree subtree;
        std::vector<std::string_view> aa_substs;

        // -> export
        double edge_line_width_scale{1.0};
        Color color_edge_line{BLACK};
        double label_scale{1.0};
        std::optional<Color> label_color;

        // -------------------- not exported --------------------
        Node* first_prev_leaf{nullptr}; // first leaf for non-leaf nodes, prev leaf for leaves, nullptr for the top of the tree
        Node* last_next_leaf{nullptr}; // last leaf for non-leaf nodes, next leaf for leaves, nullptr for the last leaf
        enum class leaf_position { first, middle, last, single };
        leaf_position leaf_pos{leaf_position::middle};
        node_id_t node_id;            // includes vertical leaf number for leaves

        // all nodes
        ladderize_helper_t ladderize_helper_;
        // leaf node only
        mutable std::optional<size_t> antigen_index_in_chart_;
        mutable std::vector<std::tuple<size_t, bool, bool>> serum_index_in_chart_; // serum_no, reassortant matches, passage_type matches

        // middle node only
        mutable CommonAA common_aa_;
        mutable AA_Transitions aa_transitions_;
        mutable const Node* node_for_left_aa_transitions_{nullptr};
        Subtree to_populate;    // populate_with_nuc_duplicates()

        // -------------------- drawing support --------------------
        mutable double cumulative_vertical_offset_{0.0};
        constexpr static const double default_vertical_offset{1.0};
        mutable double vertical_offset_{default_vertical_offset}; // mutalbe for adjusting via const Node* in clade sections

        bool children_are_shown() const { return !hidden && (subtree.empty() || std::any_of(std::begin(subtree), std::end(subtree), [](const auto& node) { return node.children_are_shown(); })); }
        void remove_aa_transition(seqdb::pos0_t pos, char right) const;
        std::vector<const Node*> shown_children() const;

        void hide();
        void populate(const acmacs::seqdb::ref& a_ref, const acmacs::seqdb::Seqdb& seqdb);

        size_t number_leaves_in_subtree() const
        {
            if (!is_leaf())
                return last_next_leaf->node_id.vertical - first_prev_leaf->node_id.vertical + 1;
            else
                return 0;
        }

        // char aa_at(seqdb::pos0_t pos0) const { return is_leaf() ? aa_sequence.at(pos0) : common_aa_.at(pos0); }

        // double distance_from_previous = -1; // for hz sections auto-detection
        // std::string aa_at;          // see make_aa_at()
        // AA_Transitions aa_transitions;
        // size_t hz_section_index = HzSectionNoIndex;
        // Color mark_with_line = ColorNoChange;
        // Pixels mark_with_line_width{0};
        // std::optional<size_t> chart_antigen_index;
        // size_t matched_antigens = 0; // for parent nodes only
        // std::optional<size_t> mark_with_label;

    }; // class Node

    // ----------------------------------------------------------------------

    template <typename N> class NodeSetT : public std::vector<N>
    {
      public:
        NodeSetT() = default;

        void add(const NodeSetT<N>& another) { std::copy(std::begin(another), std::end(another), std::back_inserter(*this)); }
        void filter(const NodeSetT<N>& another) { this->erase(std::remove_if(this->begin(), this->end(), [&another](const auto& en) { return std::find(std::begin(another), std::end(another), en) == std::end(another); }), this->end()); }
    };

    using NodeSet = NodeSetT<Node*>;
    // using NodeConstSet = NodeSetT<const Node*>;

    // ----------------------------------------------------------------------

    class Tree : public Node
    {
      public:
        void erase();

        void data_buffer(std::string&& data) { data_buffer_ = std::move(data); }
        std::string_view data_buffer() const { return data_buffer_; }

        std::string_view virus_type() const { return virus_type_; }
        std::string_view lineage() const { return lineage_; }
        void virus_type(std::string_view virus_type) { virus_type_.assign(virus_type); }
        void lineage(std::string_view lineage) { lineage_.assign(lineage); }

        bool has_sequences() const;

        NodePath find_path_by_seq_id(const seq_id_t& look_for) const;
        const Node* find_node_by_seq_id(const seq_id_t& look_for) const;

        void match_seqdb(std::string_view seqdb_filename);
        void populate_with_nuc_duplicates();

        std::string report_cumulative() const;
        void cumulative_calculate(bool recalculate = false) const;
        // void cumulative_reset() const;
        void branches_by_edge();

        enum class Select { init, update };
        enum class Descent { no, yes };
        void select_if_cumulative_more_than(NodeSet& nodes, Select update, double cumulative_min, Descent descent = Descent::no);
        void select_if_edge_more_than(NodeSet& nodes, Select update, double edge_min);
        void select_if_edge_more_than_mean_edge_of(NodeSet& nodes, Select update, double fraction_or_number);
        EdgeLength max_cumulative_shown() const;

        void select_all(NodeSet& nodes, Select update);
        void select_by_date(NodeSet& nodes, Select update, std::string_view start, std::string_view end);
        void select_by_seq_id(NodeSet& nodes, Select update, std::string_view regexp);
        void select_by_country(NodeSet& nodes, Select update, std::string_view country);
        void select_by_continent(NodeSet& nodes, Select update, std::string_view continent);
        void select_by_location(NodeSet& nodes, Select update, std::string_view location);
        void select_by_aa(NodeSet& nodes, Select update, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos1);
        void select_by_nuc(NodeSet& nodes, Select update, const acmacs::seqdb::nucleotide_at_pos1_eq_list_t& nuc_at_pos1);
        void select_matches_chart_antigens(NodeSet& nodes, Select update);

        enum class serum_match_t { name, reassortant, passage_type };
        void select_matches_chart_sera(NodeSet& nodes, Select update, serum_match_t match_type);

        void hide(const NodeSet& nodes);

        void just_imported();

        enum class Ladderize { None, MaxEdgeLength, NumberOfLeaves };
        void ladderize(Ladderize method);

        // re-roots tree making the parent of the leaf node with the passed name root
        void re_root(const seq_id_t& new_root);
        void re_root(const NodePath& new_root);

        void report_first_last_leaves(size_t min_number_of_leaves) const;

        // ----------------------------------------------------------------------

        Counter<std::string_view> stat_by_month() const;
        std::pair<date::year_month_day, date::year_month_day> suggest_time_series_start_end(const Counter<std::string_view>& stat) const;

        enum class report_size { brief, detailed };
        std::string report_time_series(report_size rs) const;

        // ----------------------------------------------------------------------

        void update_common_aa() const;
        void report_common_aa() const;
        void update_aa_transitions() const;
        void report_aa_transitions() const;

        // ----------------------------------------------------------------------

        struct clade_section_t
        {
            clade_section_t(const Node* node) : first{node}, last{node} {}
            const Node* first;
            const Node* last;
        };

        struct clade_t
        {
            clade_t(std::string_view nam, std::string_view disp) : name{nam}, display_name{disp.empty() ? nam : disp} {}
            std::string name;
            std::string display_name;
            std::vector<clade_section_t> sections;
        };

        using clades_t = std::vector<clade_t>;

        // constexpr clades_t& clades() { return clades_; }
        constexpr const clades_t& clades() const { return clades_; }

        void clades_reset();
        void clade_set(std::string_view name, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos, std::string_view display_name);
        void clade_set(std::string_view name, const acmacs::seqdb::nucleotide_at_pos1_eq_list_t& nuc_at_pos, std::string_view display_name);
        void clade_report(std::string_view name={}) const;

        // ----------------------------------------------------------------------

        void match(const acmacs::chart::Chart& chart) const;

        // drawing support
        double compute_cumulative_vertical_offsets(); // returns tree height

      private:
        const clade_t* find_clade(std::string_view name) const;
        clade_t& find_or_add_clade(std::string_view name, std::string_view display_name = {});
        std::vector<const Node*> sorted_by_edge() const;
        double mean_edge_of(double fraction_or_number) const; // nodes sorted by edge, longest nodes (fraction of all or by number) taken and their mean edge calculated
        void set_first_last_next_node_id();

        std::string data_buffer_;
        std::string virus_type_;
        std::string lineage_;
        clades_t clades_;
        mutable bool chart_matched_{false};
        mutable seqdb::pos0_t longest_sequence_{0};
        bool nodes_hidden_{false};

    }; // class Tree

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ======================================================================

// {:4.3} -> {:>4d}.{:<3d}
template <> struct fmt::formatter<acmacs::tal::node_id_t>
{
    template <typename ParseContext> constexpr auto parse(ParseContext& ctx) -> decltype(ctx.begin())
    {
        auto it = ctx.begin();
        if (it != ctx.end() && *it == ':')
            ++it;
        if (it != ctx.end() && *it != '}' && *it != '.') {
            char* end{nullptr};
            v_size_ = std::strtoul(&*it, &end, 10);
            it = std::next(it, end - &*it);
        }
        if (it != ctx.end() && *it == '.')
            ++it;
        if (it != ctx.end() && *it != '}') {
            char* end{nullptr};
            h_size_ = std::strtoul(&*it, &end, 10);
            it = std::next(it, end - &*it);
        }
        while (it != ctx.end() && *it != '}')
            ++it;
        return it;
    }

    template <typename FormatCtx> auto format(const acmacs::tal::node_id_t& node_id, FormatCtx& ctx)
    {
        if (v_size_.has_value())
            format_to(ctx.out(), "{:>{}d}.", node_id.vertical, *v_size_);
        else
            format_to(ctx.out(), "{}.", node_id.vertical);
        if (h_size_.has_value())
            format_to(ctx.out(), "{:<{}d}", node_id.horizontal, *h_size_);
        else
            format_to(ctx.out(), "{}", node_id.horizontal);
        return ctx.out();
    }

    std::optional<size_t> v_size_, h_size_;
};

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
