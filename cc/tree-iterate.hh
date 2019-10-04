#pragma once

// ----------------------------------------------------------------------

namespace acmacs::tal::inline v3::tree
{

    // ----------------------------------------------------------------------

    template <typename N, typename F1> inline void iterate_leaf(N && aNode, F1 && f_name)
    {
        if (aNode.is_leaf()) {
            f_name(std::forward<N>(aNode));
        }
        else {
            for (auto& node : aNode.subtree) {
                iterate_leaf(node, std::forward<F1>(f_name));
            }
        }
    }

    // ----------------------------------------------------------------------

    // stops iterating if f_name returns true
    template <typename N, typename F1> inline bool iterate_leaf_stop(N && aNode, F1 && f_name)
    {
        bool stop = false;
        if (aNode.is_leaf()) {
            stop = f_name(std::forward<N>(aNode));
        }
        else {
            for (auto& node : aNode.subtree) {
                if ((stop = iterate_leaf_stop(node, f_name)))
                    break;
            }
        }
        return stop;
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F3> inline void iterate_leaf_post(N && aNode, F1 && f_name, F3 && f_subtree_post)
    {
        if (aNode.is_leaf()) {
            f_name(std::forward<N>(aNode));
        }
        else {
            for (auto& node : aNode.subtree) {
                iterate_leaf_post(node, std::forward<F1>(f_name), std::forward<F3>(f_subtree_post));
            }
            f_subtree_post(std::forward<N>(aNode));
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F2> inline void iterate_leaf_pre(N && aNode, F1 && f_name, F2 && f_subtree_pre)
    {
        if (aNode.is_leaf()) {
            f_name(std::forward<N>(aNode));
        }
        else {
            f_subtree_pre(std::forward<N>(aNode));
            for (auto& node : aNode.subtree) {
                iterate_leaf_pre(node, std::forward<F1>(f_name), std::forward<F2>(f_subtree_pre));
            }
        }
    }

    // ----------------------------------------------------------------------

    // Stop descending the tree if f_subtree_pre returned false
    template <typename N, typename F1, typename F2> inline void iterate_leaf_pre_stop(N && aNode, F1 && f_name, F2 && f_subtree_pre)
    {
        if (aNode.is_leaf()) {
            f_name(std::forward<N>(aNode));
        }
        else {
            if (f_subtree_pre(std::forward<N>(aNode))) {
                for (auto& node : aNode.subtree) {
                    iterate_leaf_pre_stop(node, std::forward<F1>(f_name), std::forward<F2>(f_subtree_pre));
                }
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F3> inline void iterate_pre(N && aNode, F3 && f_subtree_pre)
    {
        if (!aNode.is_leaf()) {
            f_subtree_pre(std::forward<N>(aNode));
            for (auto& node : aNode.subtree) {
                iterate_pre(node, std::forward<F3>(f_subtree_pre));
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F3> inline void iterate_pre_path(N && aNode, F3 && f_subtree_pre, std::string path = std::string{})
    {
        constexpr const char path_parts[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        if (!aNode.is_leaf()) {
            f_subtree_pre(std::forward<N>(aNode), path);
            auto subpath_p = std::begin(path_parts);
            std::string subpath_infix;
            for (auto& node : aNode.subtree) {
                iterate_pre_path(node, std::forward<F3>(f_subtree_pre), path + subpath_infix + *subpath_p);
                ++subpath_p;
                if (!*subpath_p) {
                    subpath_p = std::begin(path_parts);
                    subpath_infix += '-';
                }
            }
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F3> inline void iterate_post(N && aNode, F3 && f_subtree_post)
    {
        if (!aNode.is_leaf()) {
            for (auto& node : aNode.subtree) {
                iterate_post(node, std::forward<F3>(f_subtree_post));
            }
            f_subtree_post(std::forward<N>(aNode));
        }
    }

    // ----------------------------------------------------------------------

    template <typename N, typename F1, typename F2, typename F3> inline void iterate_leaf_pre_post(N && aNode, F1 && f_name, F2 && f_subtree_pre, F3 && f_subtree_post)
    {
        if (aNode.is_leaf()) {
            f_name(std::forward<N>(aNode));
        }
        else {
            f_subtree_pre(std::forward<N>(aNode));
            for (auto node = aNode.subtree.begin(); node != aNode.subtree.end(); ++node) {
                iterate_leaf_pre_post(*node, std::forward<F1>(f_name), std::forward<F2>(f_subtree_pre), std::forward<F3>(f_subtree_post));
            }
            f_subtree_post(std::forward<N>(aNode));
        }
    }

    // ----------------------------------------------------------------------

} // namespace acmacs::tal::inlinev3::tree

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
