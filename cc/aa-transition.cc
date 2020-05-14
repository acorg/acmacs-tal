#include "acmacs-tal/aa-transition.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

namespace acmacs::tal::inline v3
{
    // ----------------------------------------------------------------------
    // AA subst calculation before 2020-05-13
    // returns length of the longest sequence found under root
    static seqdb::pos0_t update_common_aa(Node& root);
    static void report_common_aa(const Node& root, std::optional<seqdb::pos1_t> pos_to_report, size_t number_leaves_threshold);
    static void update_aa_transitions_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
    static void update_aa_transitions_before_20200513(Tree& tree, const draw_tree::AATransitionsParameters& parameters);
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::update_aa_transitions(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    update_aa_transitions_before_20200513(tree, parameters);

} // acmacs::tal::v3::update_aa_transitions

// ======================================================================

void acmacs::tal::v3::update_aa_transitions_20200514(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{

} // acmacs::tal::v3::update_aa_transitions_20200514

// ======================================================================

acmacs::seqdb::pos0_t acmacs::tal::v3::update_common_aa(Node& root)
{
    seqdb::pos0_t longest_sequence{0};
    tree::iterate_post(root, [&longest_sequence](Node& node) {
        for (auto& child : node.subtree) {
            if (!child.hidden) {
                if (child.is_leaf()) {
                    node.common_aa_.update(child.aa_sequence);
                    longest_sequence = std::max(longest_sequence, seqdb::pos0_t{child.aa_sequence.size()});
                }
                else
                    node.common_aa_.update(child.common_aa_);
            }
        }
    });
    return longest_sequence;

} // acmacs::tal::v3::update_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::report_common_aa(const Node& root, std::optional<seqdb::pos1_t> pos_to_report, size_t number_leaves_threshold)
{
    AD_INFO("common AA");
    root.number_leaves_in_subtree();
    tree::iterate_pre_parent(root, [&pos_to_report, number_leaves_threshold](const Node& node, const Node& parent) {
        if (node.number_leaves_in_subtree() >= number_leaves_threshold) {
            if (const auto rep = node.common_aa_.report(parent.common_aa_, pos_to_report); !rep.empty())
                fmt::print(stderr, "    node:{:4.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::report_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::update_aa_transitions_before_20200513(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    AD_DEBUG_IF(debug_from(parameters.debug), "update_aa_transitions_before_20200513");

    tree.cumulative_calculate();
    const auto longest_sequence = update_common_aa(tree);

    const auto aa_at = [](const Node& node, seqdb::pos0_t pos) {
        if (node.is_leaf())
            return node.aa_sequence.at(pos);
        else
            return node.common_aa_.at(pos);
    };

    tree::iterate_post(tree, [aa_at, longest_sequence, &parameters](Node& node) {
        for (seqdb::pos0_t pos{0}; pos < longest_sequence; ++pos) {
            const auto dbg = debug_from(parameters.debug && parameters.report_pos && pos == *parameters.report_pos);
            const auto common_aa_at = node.common_aa_.at(pos);
            AD_DEBUG_IF(dbg, "update_aa_transitions_before_20200513 counting {} node:{:4.3s} leaves:{:4d} common-aa:{} is_no_common:{}", pos, node.node_id, node.number_leaves_in_subtree(), common_aa_at,
                        common_aa_at == CommonAA::NoCommon);
            if (common_aa_at == CommonAA::NoCommon) {
                CounterChar counter;
                for (auto& child : node.subtree) {
                    if (const auto aa = aa_at(child, pos); CommonAA::is_common(aa)) {
                        child.aa_transitions_.add(pos, aa);
                        counter.count(aa);
                    }
                    else if (const auto found = child.aa_transitions_.find(pos); found) {
                        counter.count(found->right);
                    }
                }
                AD_DEBUG_IF(dbg, "  update_aa_transitions_before_20200513 counting {} node:{:4.3s} leaves:{:4d} counter: {}", pos, node.node_id, node.number_leaves_in_subtree(), counter);
                if (const auto [max_aa, max_count] = counter.max(); max_count > 1) {
                    node.remove_aa_transition(pos, max_aa);
                    node.aa_transitions_.add(pos, max_aa);
                }
            }
            // else {
            //     if (node.aa_transitions_.has(pos))
            //         AD_WARNING("update_aa_transitions_before_20200513 (the only common) has for pos {}", pos);
            // }
        }
    });

    const std::vector<const Node*> sorted_leaf_nodes = tree.sorted_by_cumulative_edge(Tree::leaves_only::yes); // bigger cumul length first

    AD_DEBUG_IF(debug_from(parameters.debug), "update_aa_transitions_before_20200513 adding left part");
    // add left part to aa transitions (Derek's algorithm)
    auto add_left_part = [&sorted_leaf_nodes, &parameters](Node& node) {
        const auto dbg = debug_from(parameters.debug && parameters.report_pos && node.aa_transitions_.has(*parameters.report_pos));
        AD_DEBUG_IF(dbg, "update_aa_transitions_before_20200513 (add left) {:4.3s} aa-transitions: {}  empty:{}", node.node_id,
                    node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes), node.aa_transitions_.empty());
        if (!node.aa_transitions_.empty()) {
            const auto node_left_edge = node.cumulative_edge_length - node.edge_length;

            const auto node_for_left = std::upper_bound(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), node_left_edge,
                                                        [](const auto& a_node_left_edge, const auto* nd) { return a_node_left_edge > nd->cumulative_edge_length; });
            // const auto node_for_left = std::find_if(std::begin(sorted_leaf_nodes), std::end(sorted_leaf_nodes), [node_left_edge](const auto* nd) { return nd->cumulative_edge_length <
            // node_left_edge; });
            if (node_for_left != std::end(sorted_leaf_nodes)) {
                node.aa_transitions_.set_left((*node_for_left)->aa_sequence);
                node.node_for_left_aa_transitions_ = *node_for_left;
                AD_DEBUG_IF(dbg, "update_aa_transitions_before_20200513 (add left) {:4.3s} {} node-for-left: {} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes), node.node_for_left_aa_transitions_->node_id,
                            node.node_for_left_aa_transitions_->seq_id);
            }
            else {
                AD_DEBUG_IF(dbg, "update_aa_transitions_before_20200513 (add left) no node for left {:4.3s} {}", node.node_id,
                            node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes));
            }
        }

        node.aa_transitions_.remove_left_right_same(parameters);
        node.aa_transitions_.remove_empty_right();
    };
    tree::iterate_leaf_pre(tree, add_left_part, add_left_part);

} // acmacs::tal::v3::update_aa_transitions_before_20200513

// ----------------------------------------------------------------------

void acmacs::tal::v3::report_aa_transitions(const Node& root, const draw_tree::AATransitionsParameters& parameters)
{
    report_common_aa(root, parameters.report_pos, parameters.report_number_leaves_threshold);
    AD_INFO("AA transitions");
    root.number_leaves_in_subtree();
    tree::iterate_pre(root, [&parameters](const Node& node) {
        if (node.number_leaves_in_subtree() >= parameters.report_number_leaves_threshold) {
            if (const auto rep = node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes); !rep.empty())
                fmt::print(stderr, "   {:5.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::report_aa_transitions

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

void acmacs::tal::v3::AA_Transitions::remove_left_right_same(const draw_tree::AATransitionsParameters& parameters)
{
    // AD_DEBUG_IF(debug_from(!data_.empty()), "remove_left_right_same {}", display());
    remove_if([&parameters](const auto& en) { return en.left_right_same() && (!parameters.show_same_left_right_for_pos || en.pos != *parameters.show_same_left_right_for_pos); });
    // AD_DEBUG_IF(debug_from(!data_.empty()), "  --> {}", display());

} // acmacs::tal::v3::AA_Transitions::remove_left_right_same

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

std::vector<std::string> acmacs::tal::v3::AA_Transitions::names(const std::vector<acmacs::seqdb::pos1_t>& selected_pos) const
{
    std::vector<std::string> result;
    for (const auto& en : data_) {
        if (!en.empty_left() && (selected_pos.empty() || en.pos_is_in(selected_pos)))
            result.push_back(en.display());
    }
    return result;

} // acmacs::tal::v3::AA_Transitions::names

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
