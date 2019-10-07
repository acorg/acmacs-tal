#pragma once

#include <string>
#include <vector>

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    using SeqId = std::string_view;

    using EdgeLength = named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>;
    // class EdgeLength : public named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>
    // {
    //   public:
    // };

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

        std::string_view aa_sequence;
        std::string_view date;
        std::string_view continent;
        std::string_view country;
        std::vector<std::string_view> hi_names;

        // size_t number_strains = 1;
        // double ladderize_max_edge_length = 0;
        // std::string ladderize_max_date;
        // std::string ladderize_max_name_alphabetically;
        // double cumulative_edge_length = -1;
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

        void match_seqdb(std::string_view seqdb_filename);

        enum class CumulativeReport { clusters, all };
        std::string report_cumulative(CumulativeReport report) const;
        void cumulative_calculate() const;
        void cumulative_reset() const;

        enum class Select { Init, Update };
        void select_cumulative(NodeConstSet& nodes, Select update, double cumulative_min) const;

        enum class Ladderize { MaxEdgeLength, NumberOfLeaves };
        void ladderize(Ladderize method);

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
