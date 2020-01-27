#include "acmacs-base/fmt.hh"
#include "acmacs-base/string.hh"
#include "acmacs-tal/html-export.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

namespace
{
    extern const char* sHeader;
    extern const char* sFooter;
} // namespace

using prefix_t = std::vector<std::string>;
static void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, const acmacs::tal::v3::Node& parent, double edge_scale, prefix_t& prefix, bool last);

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::html_export(const Tree& tree)
{
    tree.cumulative_calculate();
    const double edge_scale = 1000.0 / tree.max_cumulative_shown().as_number();
    fmt::print(stderr, "DEBUG: max {} edge_scale {}\n", tree.max_cumulative_shown(), edge_scale);

    fmt::memory_buffer html;
    fmt::format_to(html, sHeader, fmt::arg("title", fmt::format("{} {}", tree.virus_type(), tree.lineage())));
    prefix_t prefix;
    add_nodes(html, tree, tree, edge_scale, prefix, false);
    fmt::format_to(html, sFooter);
    return fmt::to_string(html);

} // acmacs::tal::v3::html_export

// ----------------------------------------------------------------------

void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, const acmacs::tal::v3::Node& /*parent*/, double edge_scale, prefix_t& prefix, bool last)
{
    if (node.is_leaf()) {
        fmt::format_to(html,
                       "<li><table><tr>{prefix}<td><div class='node-edge {node_edge_last}' style='width: {edge}px;'></div></td><td class='seq-name' style='color: "
                       "{color_tree_label}'>{seq_id}</td></tr></table></li>\n",
                       fmt::arg("prefix", ::string::join("", prefix)), fmt::arg("node_edge_last", last ? "node-edge-last" : ""),
                       fmt::arg("edge", static_cast<int>(node.edge_length.as_number() * edge_scale)), fmt::arg("seq_id", node.seq_id),
                       fmt::arg("color_tree_label", "black" /*node.color_tree_label.to_hex_string()*/));
    }
    else {
        const auto edge = static_cast<int>(node.edge_length.as_number() * edge_scale);
        fmt::format_to(html, "<li><table><tr>{prefix}<td><div class='node-edge {node_edge_last}' style='width: {edge}px;'></div></td>", fmt::arg("prefix", ::string::join("", prefix)),
                       fmt::arg("node_edge_last", last ? "node-edge-last" : ""), fmt::arg("edge", edge));
        // if (node.number_leaves_in_subtree_ >= 20) {
            // if (const auto rep = node.common_aa_.report(parent.common_aa_); !rep.empty())
            //     fmt::format_to(html, "<td class='common-aa'>leaves:{} {}</td>", node.number_leaves_in_subtree_, rep);
        if (const auto rep = node.aa_transitions_.display(); !rep.empty())
            fmt::format_to(html, "<td class='common-aa'>{}leaves:{} {} -- left:{}</td>", node.seq_id.empty() ? std::string{} : fmt::format("[{}] ", node.seq_id), node.number_leaves_in_subtree_, rep, node.node_for_left_aa_transitions_ ? node.node_for_left_aa_transitions_->seq_id : std::string_view{});
        // }
        fmt::format_to(html, "</tr></table></li>\n");
        prefix.push_back(fmt::format("<td class='subnode subnode-middle'><div style='width: {edge}px;'></div></td>", fmt::arg("edge", edge)));
        for (auto subnode = std::begin(node.subtree); subnode != std::end(node.subtree); ++subnode) {
            if (subnode->children_are_shown()) {
                const auto sub_last = std::next(subnode) == std::end(node.subtree);
                if (sub_last)
                    prefix.back() = ::string::replace(prefix.back(), "subnode-middle", "subnode-last");
                add_nodes(html, *subnode, node, edge_scale, prefix, sub_last);
            }
        }
        prefix.pop_back();
    }

} // add_nodes

// ----------------------------------------------------------------------

namespace
{
    const char* sHeader = R"(
<!DOCTYPE html>
<html>
 <head>
  <title>{title}</title>
  <style>
   ul.tree {{ list-style: none; margin: 0; padding: 0; }}
   ul.tree > li {{ white-space: nowrap; }}
   ul.tree table {{ border-collapse: collapse; }}
   ul.tree td {{ padding: 0; vertical-align: top; }}
   ul.tree td.subnode-middle {{ border-right: 1px solid black; }}
   ul.tree td.subnode-last {{ border-right: none; }}
   ul.tree td.common-aa {{ color: blue; font-size: 0.8em; }}
   ul.tree div.node-edge {{ height: 0.5em; border-bottom: 1px solid black; }}
   ul.tree div.node-edge-last {{ border-left: 1px solid black; }}
  </style>
 </head>
 <body>
  <h1>{title}</h1>
  <ul class="tree">
)";

    const char* sFooter = R"(
  </ul>
 </body>
<html>
)";

} // namespace
// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
