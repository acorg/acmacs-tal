#pragma once

#include <string>
#include <vector>

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    using EdgeLength = double;
    using SeqId = std::string;

    class Node
    {
      public:
        using Subtree = std::vector<Node>;

        Node() = default;

        bool is_leaf() const { return subtree.empty() && !seq_id.empty(); }

        EdgeLength edge_length = 0;
        EdgeLength cumulative_edge_length = -1;
        SeqId seq_id;
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
    };

} // namespace acmacs::tal::inlinev3

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
