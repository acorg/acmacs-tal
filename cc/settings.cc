#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-whocc-data/vaccines.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-tal/settings.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::tree(Tree& tree)
{
    tree_ = &tree;
    update_env();

} // acmacs::tal::v3::Settings::tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::chart(const acmacs::chart::ChartP& chart)
{
    chart_ = chart;
    update_env();

} // acmacs::tal::v3::Settings::chart

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::update_env()
{
    setenv_toplevel("virus-type", tree_->virus_type());
    setenv_toplevel("lineage", tree_->lineage());
    setenv_toplevel("tree-has-sequences", tree_->has_sequences());
    setenv_toplevel("chart-present", static_cast<bool>(chart_));
    if (chart_) {
        setenv_toplevel("chart-assay", chart_->info()->assay().hi_or_neut());
    }

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
                    tree().hide(selected);
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
        else if (key == "matches-chart-antigen") {
            if (!chart_)
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(chart());
            tree().select_matches_chart_antigens(selected, update);
        }
        else if (key == "matches-chart-serum") {
            if (!chart_)
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(chart());
            Tree::serum_match_t mt{Tree::serum_match_t::name};
            if (const auto match_type = val.to<std::string_view>(); match_type == "reassortant")
                mt = Tree::serum_match_t::reassortant;
            else if (match_type == "passage")
                mt = Tree::serum_match_t::passage_type;
            else if (match_type != "name")
                fmt::print(stderr, "WARNING: unrecognized \"matches-chart-serum\" value: \"{}\", \"name\" assumed\n", match_type);
            tree().select_matches_chart_sera(selected, update, mt);
        }
        else if (key == "seq_id") {
            tree().select_by_seq_id(selected, update, val.to<std::string_view>());
        }
        else if (key == "report") {
            report = val.to<bool>();
        }
        else if (key == "vaccine") {
            select_vaccine(selected, update, val);
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
    const auto clade_name = getenv("name", "");
    if (clade_name.empty())
        throw error{"empty clade name"};
    const auto display_name = getenv("display_name", clade_name);
    const auto report = getenv("report", false);
    const auto inclusion_tolerance = getenv("inclusion_tolerance", getenv("clade_section_inclusion_tolerance", 10UL));
    const auto exclusion_tolerance = getenv("exclusion_tolerance", getenv("clade_section_exclusion_tolerance",  5UL));

    if (const auto& substitutions = getenv("substitutions"); !substitutions.is_null())
        tree().clade_set(clade_name, acmacs::seqdb::extract_aa_at_pos1_eq_list(substitutions), display_name, inclusion_tolerance, exclusion_tolerance);
    else
        throw error{"no \"substitutions\" provided"};

    if (report)
        tree().clade_report(clade_name);

} // acmacs::tal::v3::Settings::clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::value& criteria) const
{
    const auto names = ranges::to<std::vector<std::string>>(
        acmacs::whocc::vaccine_names(acmacs::virus::type_subtype_t{getenv("virus-type", "")}, acmacs::virus::lineage_t{getenv("lineage", "")})
        | ranges::views::filter([vaccine_type=acmacs::whocc::Vaccine::type_from_string(rjson::get_or(criteria, "type", "any"))](const auto& en) { return vaccine_type == acmacs::whocc::vaccine_type::any || en.type == vaccine_type; })
        | ranges::views::transform([](const auto& en) { return en.name; }));

    fmt::print(stderr, "DEBUG: select_vaccine {}\n", names);
    NodeSet selected_nodes;
    for (const auto& name : names) {
        NodeSet some_nodes;
        tree().select_by_seq_id(some_nodes, Tree::Select::init, *acmacs::seqdb::make_seq_id(name));
        selected_nodes.add(some_nodes);
    }

    if (const auto passage = ::string::lower(rjson::get_or(criteria, "passage", "")); !passage.empty()) {
        const auto exclude_by_passage = [&passage](const auto* node) {
            const auto& name_parts = acmacs::virus::parse_name(node->seq_id, acmacs::virus::parse_name_f::none);
            if (passage == "cell")
                return !name_parts.passage.is_cell();
            else if (passage == "egg")
                return !name_parts.passage.is_egg();
            else if (passage == "reassortant")
                return name_parts.reassortant.empty();
            else
                return false;
        };
        selected_nodes.erase(std::remove_if(std::begin(selected_nodes), std::end(selected_nodes), exclude_by_passage), std::end(selected_nodes));
    }

    switch (update) {
      case Tree::Select::init:
          nodes = selected_nodes;
          break;
      case Tree::Select::update:
          nodes.filter(selected_nodes);
          break;
    }
    // report_nodes(fmt::format("INFO: select_vaccine: {} selected nodes {}\n", nodes.size(), criteria), "  ", nodes);

} // acmacs::tal::v3::Settings::select_vaccine

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
