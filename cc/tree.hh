#pragma once

#include <string>
#include <vector>
#include <tuple>

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

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

    constexpr const EdgeLength EdgeLengthNotSet{-1.0};

    // ----------------------------------------------------------------------

    class Node
    {
      public:
        using Subtree = std::vector<Node>;

        Node() = default;
        Node(SeqId a_seq_id, EdgeLength a_edge) : seq_id{a_seq_id}, edge_length{a_edge} {}

        bool is_leaf() const { return subtree.empty() && !seq_id.empty(); }

        Node& add_leaf(SeqId a_seq_id, EdgeLength a_edge) { return subtree.emplace_back(a_seq_id, a_edge); }
        Node& add_subtree() { return subtree.emplace_back(); }

        SeqId seq_id;
        EdgeLength edge_length{0.0};
        mutable EdgeLength cumulative_edge_length{EdgeLengthNotSet};
        Subtree subtree;

        bool hidden{false};
        std::string_view aa_sequence;
        std::string_view date;
        std::string_view continent;
        std::string_view country;
        std::vector<std::string_view> hi_names;

        // -------------------- not exported --------------------

        mutable size_t number_strains_in_subtree_{1};

        using ladderize_helper_t = std::tuple<EdgeLength, std::string_view, SeqId>; // edge, date, name
        ladderize_helper_t ladderize_helper_{EdgeLengthNotSet,{}, {}};


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
    };

    using NodeSet = NodeSetT<Node*>;
    using NodeConstSet = NodeSetT<const Node*>;


    // ----------------------------------------------------------------------

    class Tree : public Node
    {
      public:
        void data_buffer(std::string&& data) { data_buffer_ = std::move(data); }
        std::string_view data_buffer() const { return data_buffer_; }

        std::string_view virus_type() const { return virus_type_; }
        std::string_view lineage() const { return lineage_; }
        void virus_type(std::string_view virus_type) { virus_type_.assign(virus_type); }
        void lineage(std::string_view lineage) { lineage_.assign(lineage); }

        NodePath find_name(SeqId look_for) const;

        void match_seqdb(std::string_view seqdb_filename);

        enum class CumulativeReport { clusters, all };
        std::string report_cumulative(CumulativeReport report) const;
        void cumulative_calculate(bool recalculate = false) const;
        // void cumulative_reset() const;

        enum class Select { Init, Update };
        void select_cumulative(NodeConstSet& nodes, Select update, double cumulative_min) const;
        EdgeLength max_cumulative_shown() const;

        enum class Ladderize { None, MaxEdgeLength, NumberOfLeaves };
        void ladderize(Ladderize method);

        // re-roots tree making the parent of the leaf node with the passed name root
        void re_root(SeqId new_root);
        void re_root(const NodePath& new_root);

        void number_strains_in_subtree() const;

      private:
        std::string data_buffer_;
        std::string virus_type_;
        std::string lineage_;
    };

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
