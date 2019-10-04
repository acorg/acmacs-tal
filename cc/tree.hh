#pragma once

#include <string>
#include <vector>

#include "acmacs-base/named-type.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    using EdgeLength = named_double_from_string_t<struct acmacs_tal_EdgeLength_tag>;
    using SeqId = std::string_view;

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
        EdgeLength cumulative_edge_length{-1.0};
        Subtree subtree;

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

    class Tree : public Node
    {
      public:

        void data_buffer(std::string&& data) { data_buffer_ = std::move(data); }
        std::string_view data_buffer() const { return data_buffer_; }

      private:
        std::string data_buffer_;
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
