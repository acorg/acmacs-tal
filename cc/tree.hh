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
#include "seqdb-3/aa-at-pos.hh"
#include "seqdb-3/seq-id.hh"
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

    using SeqId = acmacs::named_string_view_t<struct acmacs_tal_SeqId_tag>;

    using EdgeLength = named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>;
    // class EdgeLength : public named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>
    // {
    //   public:
    // };

    using NodePath = acmacs::named_vector_t<const Node*, struct acmacs_tal_NodePath_tag>;

    using ladderize_helper_t = std::tuple<EdgeLength, std::string_view, SeqId>; // edge, date, name

    constexpr const EdgeLength EdgeLengthNotSet{-1.0};

    // ----------------------------------------------------------------------

    class Node
    {
      public:
        using Subtree = std::vector<Node>;

        Node() = default;
        Node(SeqId a_seq_id, EdgeLength a_edge) : edge_length{a_edge}, seq_id{a_seq_id} {}

        bool is_leaf() const { return subtree.empty(); } // seq_id may contain generated node name used for debugging

        Node& add_leaf(SeqId a_seq_id, EdgeLength a_edge) { return subtree.emplace_back(a_seq_id, a_edge); }
        Node& add_subtree() { return subtree.emplace_back(); }

        const Node& first_leaf() const;

        // all nodes
        EdgeLength edge_length{0.0};
        mutable EdgeLength cumulative_edge_length{EdgeLengthNotSet};
        bool hidden{false};

        // leaf node only
        SeqId seq_id;
        acmacs::seqdb::sequence_aligned_ref_t aa_sequence;
        std::string_view date;
        std::string_view continent;
        std::string_view country;
        std::vector<std::string_view> hi_names;
        acmacs::flat_set_t<std::string> clades;

        Color color_tree_label{BLACK}; // -> export
        Color color_time_series_dash{BLACK}; // -> export

        // middle node only
        Subtree subtree;
        std::vector<std::string_view> aa_substs;

        // -------------------- not exported --------------------
        // all nodes
        mutable size_t number_leaves_in_subtree_{1};
        ladderize_helper_t ladderize_helper_{EdgeLengthNotSet,{}, {}};
        // leaf node only
        mutable size_t row_no_;
        mutable std::optional<size_t> antigen_index_in_chart_;
        mutable std::vector<std::tuple<size_t, bool, bool>> serum_index_in_chart_; // serum_no, reassortant matches, passage_type matches
        // middle node only
        mutable CommonAA common_aa_;
        mutable AA_Transitions aa_transitions_;
        mutable const Node* node_for_left_aa_transitions_{nullptr};

        bool children_are_shown() const { return !hidden && (subtree.empty() || std::any_of(std::begin(subtree), std::end(subtree), [](const auto& node) { return node.children_are_shown(); })); }
        void remove_aa_transition(seqdb::pos0_t pos, char right) const;

        // char aa_at(seqdb::pos0_t pos0) const { return is_leaf() ? aa_sequence.at(pos0) : common_aa_.at(pos0); }

        // double distance_from_previous = -1; // for hz sections auto-detection
        // std::string continent;
        // std::string aa_at;          // see make_aa_at()
        // AA_Transitions aa_transitions;
        // bool shown = true;
        // size_t line_no = 0;
        // size_t hz_section_index = HzSectionNoIndex;
        // double vertical_pos = -1;
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

        bool has_sequences() const { return !first_leaf().aa_sequence.empty(); }

        NodePath find_name(SeqId look_for) const;

        void match_seqdb(std::string_view seqdb_filename);

        std::string report_cumulative() const;
        void cumulative_calculate(bool recalculate = false) const;
        // void cumulative_reset() const;
        void branches_by_edge() const;

        enum class Select { init, update };
        enum class Descent { no, yes };
        void select_if_cumulative_more_than(NodeSet& nodes, Select update, double cumulative_min, Descent descent = Descent::no);
        EdgeLength max_cumulative_shown() const;

        void select_all(NodeSet& nodes, Select update);
        void select_by_date(NodeSet& nodes, Select update, std::string_view start, std::string_view end);
        void select_by_seq_id(NodeSet& nodes, Select update, std::string_view regexp);
        void select_by_aa(NodeSet& nodes, Select update, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& aa_at_pos1);
        void select_matches_chart_antigens(NodeSet& nodes, Select update);

        enum class serum_match_t { name, reassortant, passage_type };
        void select_matches_chart_sera(NodeSet& nodes, Select update, serum_match_t match_type);

        void hide(const NodeSet& nodes);

        enum class Ladderize { None, MaxEdgeLength, NumberOfLeaves };
        void ladderize(Ladderize method);

        // re-roots tree making the parent of the leaf node with the passed name root
        void re_root(SeqId new_root);
        void re_root(const NodePath& new_root);

        void number_leaves_in_subtree() const;

        Counter<std::string_view> stat_by_month() const;
        std::pair<date::year_month_day, date::year_month_day> suggest_time_series_start_end(const Counter<std::string_view>& stat) const;
        std::string report_time_series() const;

        void update_common_aa() const;
        void report_common_aa() const;
        void update_aa_transitions() const;
        void report_aa_transitions() const;

        void clades_reset();
        void clade_set(std::string_view name, const acmacs::seqdb::amino_acid_at_pos1_eq_list_t& substitutions, std::string_view display_name, size_t inclusion_tolerance, size_t exclusion_tolerance);
        void clade_report(std::string_view name={}) const;

        void match(const acmacs::chart::Chart& chart) const;

      private:
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

        const clade_t* find_clade(std::string_view name) const
        {
            if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [name](const auto& cl) { return cl.name == name; }); found != std::end(clades_))
                return &*found;
            else
                return nullptr;
        }

        clade_t& find_clade(std::string_view name, std::string_view display_name = {})
        {
            if (auto found = std::find_if(std::begin(clades_), std::end(clades_), [name](const auto& cl) { return cl.name == name; }); found != std::end(clades_))
                return *found;
            else
                return clades_.emplace_back(name, display_name);
        }

        std::string data_buffer_;
        std::string virus_type_;
        std::string lineage_;
        mutable bool row_no_set_{false};
        clades_t clades_;
        mutable bool chart_matched_{false};

        void set_row_no() const;
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
