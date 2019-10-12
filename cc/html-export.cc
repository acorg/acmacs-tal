#include "acmacs-base/fmt.hh"
#include "acmacs-tal/html-export.hh"
#include "acmacs-tal/tree.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

namespace
{
    extern const char* sHeader;
    extern const char* sFooter;
    extern const char* sRowParentNode;
    extern const char* sRowLeaf;
} // namespace

static void add_nodes_old(fmt::memory_buffer& html, const acmacs::tal::v3::Node& tree, double edge_scale);

using prefix_t = std::vector<std::string>;
static void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, double edge_scale, prefix_t& prefix);

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::html_export(const Tree& tree)
{
    tree.cumulative_calculate();
    const double edge_scale = 1000.0 / tree.max_cumulative_shown().as_number();
    fmt::print(stderr, "DEBUG: max {} edge_scale {}\n", tree.max_cumulative_shown(), edge_scale);

    fmt::memory_buffer html;
    fmt::format_to(html, sHeader, fmt::arg("title", fmt::format("{} {}", tree.virus_type(), tree.lineage())));
    prefix_t prefix;
    add_nodes(html, tree, edge_scale, prefix);
    fmt::format_to(html, sFooter);
    return fmt::to_string(html);

} // acmacs::tal::v3::html_export

// ----------------------------------------------------------------------

void add_nodes(fmt::memory_buffer& html, const acmacs::tal::v3::Node& node, double edge_scale, prefix_t& prefix)
{
    if (node.is_leaf()) {
        fmt::format_to(html, "<li><table><tr>{prefix}<td><div class='leaf-edge' style='width: {edge}px;'></div></td><td class='seq-name'>{seq_id}</td></tr></table></li>",
                       fmt::arg("prefix", ::string::join("", prefix)), fmt::arg("edge", static_cast<int>(node.edge_length.as_number() * edge_scale)), fmt::arg("seq_id", node.seq_id));
    }
    else {

        for (auto& subnode : node.subtree)
            add_nodes(html, subnode, edge_scale, prefix);
    }

} // add_nodes

// ----------------------------------------------------------------------

void add_nodes_old(fmt::memory_buffer& html, const acmacs::tal::v3::Node& tree, double edge_scale)
{
    acmacs::tal::v3::EdgeLength parent_cumulative{0.0};

    const auto node_pre = [&html, &parent_cumulative, edge_scale](const auto& node) {
        // fmt::print(stderr, "DEBUG: + {} {}\n", node.subtree.size(), node.cumulative_edge_length);
        fmt::format_to(html, sRowParentNode, fmt::arg("parent_cumulative", static_cast<int>(parent_cumulative.as_number() * edge_scale)),
                       fmt::arg("edge", static_cast<int>(node.edge_length.as_number() * edge_scale)));
        parent_cumulative = node.cumulative_edge_length;
    };

    const auto node_post = [&parent_cumulative](const auto& node) { parent_cumulative -= node.edge_length; };

    const auto leaf = [&html, &parent_cumulative, edge_scale](const auto& node) {
        // fmt::print(stderr, "DEBUG: {} {}\n", node.seq_id, node.cumulative_edge_length);
        fmt::format_to(html, sRowLeaf, fmt::arg("seq_id", node.seq_id), fmt::arg("parent_cumulative", static_cast<int>(parent_cumulative.as_number() * edge_scale)),
                       fmt::arg("edge", static_cast<int>(node.edge_length.as_number() * edge_scale)));
    };

    acmacs::tal::v3::tree::iterate_leaf_pre_post(tree, leaf, node_pre, node_post);

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
   .parent-cumulative {{  }}
   .leaf-edge {{ height: 0.5em; border-bottom: 1px solid black; border-left: 1px solid black; }}
   .seq-name {{}}
   div.parent-cumulative {{ float: left; min-height: 1px; }}
   div.leaf-edge {{ float: left; height: 0.5em; border-bottom: 1px solid black; border-left: 1px solid black; }}
   div.seq-name {{}}
   ul.tree td {{ padding: 0; vertical-align: top; }}
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

    const char* sRowParentNode = R"(
    <li>
     <table>
      <tr>
       <td class='parent-cumulative' style='width: {parent_cumulative}px;'></td>
       <td><div class='leaf-edge' style='width: {edge}px;'></div></td>
      </tr>
     </table>
    </li>
)";

    const char* sRowLeaf = R"(
    <li>
     <table>
      <tr>
       <td class='parent-cumulative' style='width: {parent_cumulative}px;'></td>
       <td><div class='leaf-edge' style='width: {edge}px;'></div></td>
       <td class='seq-name'>{seq_id}</td>
      </tr>
     </table>
    </li>
)";

} // namespace
// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
