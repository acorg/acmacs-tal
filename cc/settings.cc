#include "acmacs-base/read-file.hh"
#include "acmacs-base/string-split.hh"
#include "acmacs-tal/settings.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::tree(Tree& tree)
{
    tree_ = &tree;
    update_env();

} // acmacs::tal::v3::Settings::tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::update_env()
{
    setenv_toplevel("virus-type", tree_->virus_type());
    setenv_toplevel("lineage", tree_->lineage());
    setenv_toplevel("tree-has-sequences", tree_->has_sequences());

} // acmacs::tal::v3::Settings::update_env

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Settings::apply_built_in(std::string_view name)
{
    try {
        // printenv();
        if (name == "aa-transitions") {
            tree().update_common_aa();
            tree().update_aa_transitions();
            if (getenv("report", false))
                tree().report_aa_transitions();
        }
        else if (name == "clades-reset") {
            tree().clades_reset();
        }
        else if (name == "clade") {
            clade();
        }
        else if (name == "ladderize") {
            if (const auto method = getenv("method", "number-of-leaves"); method == "number-of-leaves")
                tree().ladderize(Tree::Ladderize::NumberOfLeaves);
            else if (method == "max-edge-length")
                tree().ladderize(Tree::Ladderize::MaxEdgeLength);
            else
                throw acmacs::settings::error{fmt::format("unsupported ladderize method: {}", method)};
        }
        else if (name == "nodes") {
            apply_nodes();
        }
        else if (name == "re-root") {
            tree().re_root(SeqId{getenv("new-root", "re-root: new-root not specified")});
        }
        else if (name == "report-cumulative") {
            if (const auto output_filename = getenv("output", ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_cumulative(getenv("all", false) ? Tree::CumulativeReport::all : Tree::CumulativeReport::clusters));
        }
        else if (name == "report-time-series") {
            if (const auto output_filename = getenv("output", ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_time_series());
        }
        else if (name == "seqdb") {
            tree().match_seqdb(getenv("filename", ""));
            update_env();
        }
        else
            return acmacs::settings::Settings::apply_built_in(name);
        return true;
    }
    catch (std::exception& err) {
        throw error{fmt::format("cannot apply \"{}\": {}", name, err)};
    }

} // acmacs::tal::v3::Settings::apply_built_in

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::apply_nodes() const
{
    const auto selected = select_nodes(getenv("select"));
    fmt::print(stderr, "DEBUG: apply_nodes {}\n", selected.size());
    std::visit(
        [this, &selected]<typename T>(T && arg) {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                if (arg == "hide") {
                    for (Node* node : selected)
                        node->hidden = true;
                }
                else if (arg == "color") {
                    fmt::print(stderr, "DEBUG: apply color {}\n", getenv("tree-label", ""));
                    if (const auto tree_label = getenv("tree-label", ""); !tree_label.empty()) {
                        const Color color{tree_label};
                        for (Node* node : selected)
                            node->color_tree_label = color;
                    }
                    if (const auto time_series_dash = getenv("time-series-dash", ""); !time_series_dash.empty()) {
                        const Color color{time_series_dash};
                        for (Node* node : selected)
                            node->color_time_series_dash = color;
                    }
                }
                else if (arg == "report") {
                    report_nodes(fmt::format("INFO: {} selected nodes {}\n", selected.size(), getenv("select")), "  ", selected);
                }
                else
                    throw error{fmt::format("don't know how to apply for \"nodes\": {}", arg)};
            }
            else
                throw error{fmt::format("don't know how to apply for \"nodes\": {}", arg)};
        },
        getenv("apply").val_());

} // acmacs::tal::v3::Settings::apply_nodes

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::report_nodes(std::string_view prefix, std::string_view indent, const NodeSet& nodes) const
{
    fmt::print("{}", prefix);
    for (const auto* node : nodes) {
        if (node->is_leaf())
            fmt::print("{}{} [{}] cumul:{:.6f}\n", indent, node->seq_id, node->date, node->cumulative_edge_length.as_number());
        else
            fmt::print("{}(children: {}) cumul:{:.6f}\n", indent, node->subtree.size(), node->cumulative_edge_length.as_number());
    }

} // acmacs::tal::v3::Settings::report_nodes

// ----------------------------------------------------------------------

acmacs::tal::v3::NodeSet acmacs::tal::v3::Settings::select_nodes(const rjson::value& criteria) const
{
    NodeSet selected;
    bool report = false;
    rjson::for_each(criteria, [&selected, this, update = Tree::Select::init, &report](const std::string& key, const rjson::value& val) mutable {
        if (key == "all") {
            tree().select_all(selected, update);
        }
        else if (key == "aa") {
            tree().select_by_aa(selected, update, acmacs::seqdb::extract_aa_at_pos1_eq_list(val));
        }
        else if (key == "cumulative >=") {
            tree().select_if_cumulative_more_than(selected, update, val.to<double>());
        }
        else if (key == "date") {
            tree().select_by_date(selected, update, val[0].to<std::string_view>(), val[1].to<std::string_view>());
        }
        else if (key == "seq_id") {
            tree().select_by_seq_id(selected, update, val.to<std::string_view>());
        }
        else if (key == "report") {
            report = val.to<bool>();
        }
        else
            throw acmacs::settings::error{fmt::format("unrecognized select node criterium: {}", key)};
        if (key != "report")
            update = Tree::Select::update;
    });
    if (report)
        report_nodes(fmt::format("INFO: {} selected nodes {}\n", selected.size(), criteria), "  ", selected);
    return selected;

} // acmacs::tal::v3::Settings::select_nodes

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::clade() const
{
    using namespace std::string_view_literals;

    const auto clade_name = getenv("name", "");
    if (clade_name.empty())
        throw error{"empty clade name"};
    const auto display_name = getenv("display_name", clade_name);
    const auto report = getenv("report", false);

    if (const auto& substitutions = getenv("substitutions"); !substitutions.is_null())
        std::visit(
            [this,clade_name,display_name]<typename T>(T && arg) {
                if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                    tree().clade_set(clade_name, acmacs::string::split(std::forward<T>(arg), " "sv), display_name);
                }
                else if constexpr (std::is_same_v<std::decay_t<T>, rjson::array>) {
                    std::vector<std::string_view> substs;
                    try {
                        arg.copy_to(substs);
                    }
                    catch (rjson::error& /*err*/) {
                        throw error{fmt::format("invalid \"substitutions\" value: {}", arg)};
                    }
                    tree().clade_set(clade_name, substs, display_name);
                }
                else
                    throw error{fmt::format("invalid \"substitutions\" value: {}", arg)};
            },
            substitutions.val_());
    else
        throw error{"no \"substitutions\" provided"};

    if (report)
        tree().clade_report(clade_name);

} // acmacs::tal::v3::Settings::clade

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
