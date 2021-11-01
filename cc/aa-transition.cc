#include "acmacs-base/string-join.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-tal/aa-transition.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3
{
    // ----------------------------------------------------------------------
    // AA subst calculation derek-2016
    // returns length of the longest sequence found under root
    // static void report_common_aa(const Node& root, std::optional<seqdb::pos1_t> pos_to_report, size_t number_leaves_threshold);

} // namespace acmacs::tal::inline v3

// ----------------------------------------------------------------------

void acmacs::tal::v3::reset_aa_transitions(Tree& tree)
{
    const auto reset_node = [](Node& node) { node.aa_transitions_.clear(); };

    tree::iterate_leaf_pre(tree, reset_node, reset_node);

} // acmacs::tal::v3::reset_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::detail::update_common_aa(Tree& tree, seqdb::pos0_t longest_aa_sequence, size_t number_of_aas)
{
    tree.resize_common_aa(*longest_aa_sequence, number_of_aas);

    const Timeit ti{"update_common_aa"};
    size_t max_count{0};
    tree::iterate_post(tree, [&max_count](Node& node) {
        for (auto& child : node.subtree) {
            if (!child.hidden) {
                if (child.is_leaf())
                    node.common_aa_->update(child.aa_sequence);
                else
                    node.common_aa_->update(*child.common_aa_);
            }
        }
        max_count = std::max(max_count, node.common_aa_.max_count());
    });
    AD_INFO("update_common_aa: max_count: {}", max_count);
}

// ----------------------------------------------------------------------

void acmacs::tal::v3::update_aa_transitions(Tree& tree, const draw_tree::AATransitionsParameters& parameters)
{
    switch (parameters.method) {
        case draw_tree::AATransitionsParameters::method::eu_20210503:
            detail::update_aa_transitions_eu_20210503(tree, parameters);
            break;
        case draw_tree::AATransitionsParameters::method::eu_20200915:
            detail::update_aa_transitions_eu_20200915(tree, parameters);
            break;
        case draw_tree::AATransitionsParameters::method::eu_20200514:
            detail::update_aa_transitions_eu_20200514(tree, parameters);
            break;
        // case draw_tree::AATransitionsParameters::method::eu_20200909:
        //     throw std::runtime_error{"aa subst method eu_20200909 commented out"};
        //     // update_aa_transitions_eu_20200909(tree, parameters);
        //     // break;
        case draw_tree::AATransitionsParameters::method::derek_2016:
            detail::update_aa_transitions_derek_2016(tree, parameters);
            break;
    }

} // acmacs::tal::v3::update_aa_transitions

// ======================================================================

// void acmacs::tal::v3::report_common_aa(const Node& /*root*/, std::optional<seqdb::pos1_t> /*pos_to_report*/, size_t /*number_leaves_threshold*/)
// {
//     AD_WARNING("report_common_aa not implemented");
//     // AD_INFO("common AA");
//     // root.number_leaves_in_subtree();
//     // tree::iterate_pre_parent(root, [&pos_to_report, number_leaves_threshold](const Node& node, const Node& parent) {
//     //     if (node.number_leaves_in_subtree() >= number_leaves_threshold) {
//     //         if (const auto rep = node.common_aa_->report(parent.common_aa_, pos_to_report); !rep.empty())
//     //             fmt::print(stderr, "    node:{:4.3} (children:{} leaves:{}) {}\n", node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
//     //     }
//     // });

// } // acmacs::tal::v3::report_common_aa

// ----------------------------------------------------------------------

void acmacs::tal::v3::report_aa_transitions(const Node& root, const draw_tree::AATransitionsParameters& parameters)
{
    // not implemented report_common_aa(root, parameters.report_pos, parameters.report_number_leaves_threshold);
    AD_INFO("AA transitions");
    root.number_leaves_in_subtree();
    tree::iterate_pre(root, [&parameters](const Node& node) {
        if (node.number_leaves_in_subtree() >= parameters.report_number_leaves_threshold) {
            if (const auto rep = node.aa_transitions_.display(parameters.report_pos, AA_Transitions::show_empty_left::yes); !rep.empty())
                fmt::print(stderr, fmt::runtime("   {:5.3} (children:{} leaves:{}) {}\n"), node.node_id, node.subtree.size(), node.number_leaves_in_subtree(), rep);
        }
    });

} // acmacs::tal::v3::report_aa_transitions

// ======================================================================

void acmacs::tal::v3::AA_Transitions::remove_left_right_same(const draw_tree::AATransitionsParameters& parameters, [[maybe_unused]] const Node& node)
{
    // AD_DEBUG(!data_.empty(), "remove_left_right_same {}", display());
    remove_if([&parameters /*, &node */](const auto& en) {
        // const auto dbg = en.left_right_same() && parameters.debug && parameters.report_pos && en.pos == *parameters.report_pos;
        // AD_DEBUG(dbg, "remove_left_right_same {:5.3} {}", node.node_id, en.display());
        return en.left_right_same() && (!parameters.show_same_left_right_for_pos || en.pos != *parameters.show_same_left_right_for_pos);
    });
    // AD_DEBUG(!data_.empty(), "  --> {}", display());

} // acmacs::tal::v3::AA_Transitions::remove_left_right_same

// ----------------------------------------------------------------------

void acmacs::tal::v3::AA_Transitions::add_or_replace(const AA_Transition& to_add)
{
    data_.erase(std::remove_if(std::begin(data_), std::end(data_), [&to_add](const auto& en) { return en.pos == to_add.pos; }), std::end(data_));
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
    if (data_.empty())
        return {};
    std::vector<const AA_Transition*> res;
    for (const auto& en : data_) {
        if ((sel == show_empty_left::yes || !en.empty_left()) && !en.empty_right() && (!pos1 || *pos1 == en.pos))
            res.push_back(&en);
    }
    return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat->display(); });

} // acmacs::tal::v3::AA_Transitions::display

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::AA_Transitions::display_most_important(size_t num) const
{
    if (data_.empty())
        return {};
    std::vector<const AA_Transition*> res;
    for (const auto& en : data_) {
        if (!en.empty_left() && !en.empty_right())
            res.push_back(&en);
    }
    if (num == 0 || res.size() < num)
        return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat->display(); });

    // HA recepter binding domain is 63 - 286 https://www.ncbi.nlm.nih.gov/pmc/articles/PMC3020035/
    auto to_remove{res.size() - num};
    for (auto resp = res.begin(); resp != res.end() && to_remove > 0; ++resp) { // removing entries that are out of recepter binding domain
        if ((**resp).pos < seqdb::pos1_t{63} || (**resp).pos > seqdb::pos1_t{286}) {
            *resp = nullptr;
            --to_remove;
        }
    }
    for (auto resp = res.begin(); resp != res.end() && to_remove > 0; ++resp) { // removing initial (and not removed before) entries
        if (*resp) {
            *resp = nullptr;
            --to_remove;
        }
    }
    return acmacs::string::join(acmacs::string::join_space, res.begin(), res.end(), [](const auto* aat) { return aat ? aat->display() : std::string{}; });

} // acmacs::tal::v3::AA_Transitions::display_most_important

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

std::vector<std::string> acmacs::tal::v3::AA_Transitions::names(const std::vector<acmacs::seqdb::pos1_t>& selected_pos, std::string_view overwrite) const
{
    std::vector<std::string> result;
    if (overwrite.empty()) {
        for (const auto& en : data_) {
            if (!en.empty_left() && (selected_pos.empty() || en.pos_is_in(selected_pos)))
                result.push_back(en.display());
        }
    }
    else {
        const auto res = acmacs::string::split(overwrite, " ", acmacs::string::Split::StripRemoveEmpty);
        result.resize(res.size());
        std::transform(std::begin(res), std::end(res), std::begin(result), [](std::string_view src) { return std::string{src}; });
    }
    return result;

} // acmacs::tal::v3::AA_Transitions::names

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
