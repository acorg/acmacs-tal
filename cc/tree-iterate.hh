#pragma once

#include "acmacs-base/enumerate.hh"
#include "acmacs-base/fmt.hh"

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3::tree
{

    // ----------------------------------------------------------------------

    template <typename N, typename F1> inline void iterate_leaf(N&& node, F1 f_name)
    {
        if (node.is_leaf()) {
            f_name(std::forward<N>(node));
        }
        else {
            for (auto& subnode : node.subtree)
                iterate_leaf(subnode, f_name);
        }
    }

    // ----------------------------------------------------------------------

    // stops iterating if f_name returns true
    template <typename N, typename F1> inline bool iterate_leaf_stop(N&& node, F1 f_name)
    {
        bool stop = false;
        if (node.is_leaf()) {
            stop = f_name(std::forward<N>(node));
        }
        else {
            for (auto& subnode : node.subtree) {
                if ((stop = iterate_leaf_stop(subnode, f_name)))
                    break;
            }
        }
        return stop;
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F3> inline void iterate_leaf_post(N&& node, F1 f_name, F3 f_subtree_post)
    {
        if (node.is_leaf()) {
            f_name(std::forward<N>(node));
        }
        else {
            for (auto& subnode : node.subtree)
                iterate_leaf_post(subnode, f_name, f_subtree_post);
            f_subtree_post(std::forward<N>(node));
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F2> inline void iterate_leaf_pre(N&& node, F1 f_name, F2 f_subtree_pre)
    {
        if (node.is_leaf()) {
            f_name(std::forward<N>(node));
        }
        else {
            f_subtree_pre(std::forward<N>(node));
            for (auto& subnode : node.subtree)
                iterate_leaf_pre(subnode, f_name, f_subtree_pre);
        }
    }

    // ----------------------------------------------------------------------

    // Stop descending the tree if f_subtree_pre returned false
    template <typename N, typename F1, typename F2> inline void iterate_leaf_pre_stop(N&& node, F1 f_name, F2 f_subtree_pre)
    {
        if (node.is_leaf()) {
            f_name(std::forward<N>(node));
        }
        else {
            if (f_subtree_pre(std::forward<N>(node))) {
                for (auto& subnode : node.subtree)
                    iterate_leaf_pre_stop(subnode, f_name, f_subtree_pre);
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F3> inline void iterate_pre(N&& node, F3 f_subtree_pre)
    {
        if (!node.is_leaf()) {
            f_subtree_pre(std::forward<N>(node));
            for (auto& subnode : node.subtree)
                iterate_pre(subnode, f_subtree_pre);
        }
    }

    // template <typename N, typename F3> inline void iterate_pre(N&& node, F3 f_subtree_pre, size_t index)
    // {
    //     if (!node.is_leaf()) {
    //         f_subtree_pre(std::forward<N>(node), index);
    //         for (auto [no, subnode] : acmacs::enumerate(node.subtree))
    //             iterate_pre(subnode, f_subtree_pre, no);
    //     }
    // }

    // ----------------------------------------------------------------------

    template <typename N, typename P, typename F1> inline void iterate_pre_parent(N&& node, P&& parent, F1 f_subtree_pre)
    {
        if (!node.is_leaf()) {
            f_subtree_pre(std::forward<N>(node), std::forward<P>(parent));
            for (auto& subnode : node.subtree)
                iterate_pre_parent(subnode, std::forward<N>(node), f_subtree_pre);
        }
    }

    template <typename N, typename F1> inline void iterate_pre_parent(N&& node, F1 f_subtree_pre)
    {
        if (!node.is_leaf()) {
            for (auto& subnode : node.subtree)
                iterate_pre_parent(subnode, std::forward<N>(node), f_subtree_pre);
        }
    }

    // ----------------------------------------------------------------------

    // template <typename N, typename F3> inline void iterate_pre_path(N&& node, F3 f_subtree_pre, std::string path = std::string{})
    // {
    //     constexpr const char path_parts[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    //     if (!node.is_leaf()) {
    //         f_subtree_pre(std::forward<N>(node), path);
    //         auto subpath_p = std::begin(path_parts);
    //         std::string subpath_infix;
    //         for (auto& node : node.subtree) {
    //             iterate_pre_path(node, f_subtree_pre, path + subpath_infix + *subpath_p);
    //             ++subpath_p;
    //             if (!*subpath_p) {
    //                 subpath_p = std::begin(path_parts);
    //                 subpath_infix += '-';
    //             }
    //         }
    //     }
    // }

    template <typename N, typename F3> inline void iterate_pre_path(N&& node, F3 f_subtree_pre, std::string path = std::string{})
    {
        if (!node.is_leaf()) {
            f_subtree_pre(std::forward<N>(node), path);
            for (auto [no, subnode] : acmacs::enumerate(node.subtree)) {
                iterate_pre_path(node, f_subtree_pre, fmt::format("{}-{}", path, no));
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F3> inline void iterate_post(N&& node, F3 f_subtree_post)
    {
        if (!node.is_leaf()) {
            for (auto& subnode : node.subtree)
                iterate_post(subnode, f_subtree_post);
            f_subtree_post(std::forward<N>(node));
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F2, typename F3> inline void iterate_leaf_pre_post(N&& node, F1 f_leaf, F2 f_subtree_pre, F3 f_subtree_post)
    {
        if (node.is_leaf()) {
            f_leaf(std::forward<N>(node));
        }
        else {
            f_subtree_pre(std::forward<N>(node));
            for (auto& subnode : node.subtree)
                iterate_leaf_pre_post(subnode, f_leaf, f_subtree_pre, f_subtree_post);
            f_subtree_post(std::forward<N>(node));
        }
    }

    template <typename N, typename F1, typename F2, typename F3, typename F4> inline void iterate_stop_leaf_pre_post(N&& node, F1 f_stop, F2 f_leaf, F3 f_subtree_pre, F4 f_subtree_post)
    {
        if (!f_stop(std::forward<N>(node))) {
            if (node.is_leaf()) {
                f_leaf(std::forward<N>(node));
            }
            else {
                f_subtree_pre(std::forward<N>(node));
                for (auto& subnode : node.subtree)
                    iterate_stop_leaf_pre_post(subnode, f_stop, f_leaf, f_subtree_pre, f_subtree_post);
                f_subtree_post(std::forward<N>(node));
            }
        }
    }

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3::tree

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
