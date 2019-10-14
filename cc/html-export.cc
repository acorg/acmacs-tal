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
static void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, double edge_scale, prefix_t& prefix, bool last);

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::html_export(const Tree& tree)
{
    tree.cumulative_calculate();
    const double edge_scale = 1000.0 / tree.max_cumulative_shown().as_number();
    fmt::print(stderr, "DEBUG: max {} edge_scale {}\n", tree.max_cumulative_shown(), edge_scale);

    fmt::memory_buffer html;
    fmt::format_to(html, sHeader, fmt::arg("title", fmt::format("{} {}", tree.virus_type(), tree.lineage())));
    prefix_t prefix;
    add_nodes(html, tree, edge_scale, prefix, false);
    fmt::format_to(html, sFooter);
    return fmt::to_string(html);

} // acmacs::tal::v3::html_export

// ----------------------------------------------------------------------

void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, double edge_scale, prefix_t& prefix, bool last)
{
    if (node.is_leaf()) {
        fmt::format_to(html, "<li><table><tr>{prefix}<td><div class='node-edge {node_edge_last}' style='width: {edge}px;'></div></td><td class='seq-name'>{seq_id}</td></tr></table></li>",
                       fmt::arg("prefix", ::string::join("", prefix)), fmt::arg("node_edge_last", last ? "node-edge-last" : ""),
                       fmt::arg("edge", static_cast<int>(node.edge_length.as_number() * edge_scale)), fmt::arg("seq_id", node.seq_id));
    }
    else {
        const auto edge = static_cast<int>(node.edge_length.as_number() * edge_scale);
        fmt::format_to(html, "<li><table><tr>{prefix}<td><div class='node-edge {node_edge_last}' style='width: {edge}px;'></div></td></tr></table></li>",
                       fmt::arg("prefix", ::string::join("", prefix)), fmt::arg("node_edge_last", last ? "node-edge-last" : ""), fmt::arg("edge", edge));
        prefix.push_back(fmt::format("<td class='subnode subnode-middle'><div style='width: {edge}px;'></div></td>", fmt::arg("edge", edge)));
        for (auto subnode = std::begin(node.subtree); subnode != std::end(node.subtree); ++subnode) {
            if (!subnode->hidden) {
                const auto sub_last = std::next(subnode) == std::end(node.subtree);
                if (sub_last)
                    prefix.back() = ::string::replace(prefix.back(), "subnode-middle", "subnode-last");
                add_nodes(html, *subnode, edge_scale, prefix, sub_last);
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
