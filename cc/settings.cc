#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/string.hh"
#include "acmacs-virus/virus-name.hh"
#include "acmacs-whocc-data/vaccines.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-tal/settings.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/clades.hh"

// ----------------------------------------------------------------------

template <typename ElementType, typename ... Args> ElementType& acmacs::tal::v3::Settings::add_element(Args&& ... args)
{
    using namespace std::string_view_literals;
    auto element_p = std::make_unique<ElementType>(std::forward<Args>(args) ...);
    auto& element = *element_p;
    draw().layout().add(std::move(element_p));
    getenv_copy_if_present("width_to_height_ratio"sv, element.width_to_height_ratio());
    outline(element.outline());
    return element;

} // acmacs::tal::v3::Settings::add_element

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::update_env()
{
    setenv_toplevel("virus-type", tal_.tree().virus_type());
    setenv_toplevel("lineage", tal_.tree().lineage());
    setenv_toplevel("tree-has-sequences", tal_.tree().has_sequences());
    setenv_toplevel("chart-present", tal_.chart_present());
    if (tal_.chart_present()) {
        setenv_toplevel("chart-assay", tal_.chart().info()->assay().hi_or_neut());
    }

} // acmacs::tal::v3::Settings::update_env

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Settings::apply_built_in(std::string_view name, verbose verb)
{
    using namespace std::string_view_literals;
    try {
        // printenv();
        if (name == "aa-transitions"sv) {
            tree().update_common_aa();
            tree().update_aa_transitions();
            if (getenv("report"sv, false))
                tree().report_aa_transitions();
        }
        else if (name == "clades-reset"sv)
            tree().clades_reset();
        else if (name == "clade"sv)
            clade();
        else if (name == "clades"sv)
            add_clades();
        else if (name == "gap"sv)
            add_element<Gap>();
        else if (name == "ladderize"sv)
            ladderize();
        else if (name == "margins"sv)
            margins();
        else if (name == "nodes"sv)
            apply_nodes();
        else if (name == "re-root"sv)
            tree().re_root(SeqId{getenv("new-root"sv, "re-root: new-root not specified")});
        else if (name == "report-cumulative"sv) {
            // tree().branches_by_edge();
            if (const auto output_filename = getenv("output"sv, ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_cumulative());
        }
        else if (name == "report-time-series"sv) {
            if (const auto output_filename = getenv("output"sv, ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_time_series());
        }
        else if (name == "seqdb"sv) {
            tree().match_seqdb(getenv("filename"sv, ""));
            update_env();
        }
        else if (name == "time_series"sv)
            add_time_series();
        else if (name == "tree"sv)
            add_tree();
        else
            return acmacs::settings::Settings::apply_built_in(name, verb);
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

void acmacs::tal::v3::Settings::ladderize()
{
    if (const auto method = getenv("method", "number-of-leaves"); method == "number-of-leaves")
        tree().ladderize(Tree::Ladderize::NumberOfLeaves);
    else if (method == "max-edge-length")
        tree().ladderize(Tree::Ladderize::MaxEdgeLength);
    else
        throw acmacs::settings::error{fmt::format("unsupported ladderize method: {}", method)};

} // acmacs::tal::v3::Settings::ladderize

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::margins()
{
    using namespace std::string_view_literals;
    getenv_copy_if_present("left"sv, draw().margins().left);
    getenv("right"sv, draw().margins().right);
    getenv("top"sv, draw().margins().top);
    getenv("bottom"sv, draw().margins().bottom);
    outline(draw().outline());

} // acmacs::tal::v3::Settings::margins

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::outline(DrawOutline& draw_outline)
{
    using namespace std::string_view_literals;
    if (const auto debug_outline = getenv("debug_outline"sv); !debug_outline.is_null()) {
        std::visit(
            [&draw_outline]<typename T>(T && val) {
                if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
                    draw_outline.outline = val;
                }
                else if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                    draw_outline.outline = true;
                    draw_outline.outline_color = Color{val};
                }
                else if constexpr (std::is_same_v<std::decay_t<T>, rjson::object>) {
                    draw_outline.outline = rjson::get_or(val.get("show"sv), true);
                    rjson::copy_if_not_null(val.get("color"sv), draw_outline.outline_color);
                    if (const auto& wid = val.get("width"sv); !wid.is_null())
                        draw_outline.outline_width = Pixels{wid.template to<double>()};
                }
                else {
                    fmt::print(stderr, "WARNING: unrecognized debug_outline value: {}\n", val);
                }
            },
            debug_outline.val_());
    }

} // acmacs::tal::v3::Settings::outline

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
            if (!tal_.chart_present())
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(tal_.chart());
            tree().select_matches_chart_antigens(selected, update);
        }
        else if (key == "matches-chart-serum") {
            if (!tal_.chart_present())
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(tal_.chart());
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
    using namespace std::string_view_literals;
    const auto clade_name = getenv("name"sv, "");
    if (clade_name.empty())
        throw error{"empty clade name"};
    const auto display_name = getenv("display_name"sv, clade_name);
    const auto report = getenv("report"sv, false);
    // const auto inclusion_tolerance = getenv("inclusion_tolerance"sv, getenv("clade_section_inclusion_tolerance"sv, 10UL));
    // const auto exclusion_tolerance = getenv("exclusion_tolerance"sv, getenv("clade_section_exclusion_tolerance"sv, 5UL));

    if (const auto& substitutions = getenv("substitutions"sv); !substitutions.is_null())
        tree().clade_set(clade_name, acmacs::seqdb::extract_aa_at_pos1_eq_list(substitutions), display_name);
    else
        throw error{"no \"substitutions\" provided"};

    if (report)
        tree().clade_report(clade_name);

} // acmacs::tal::v3::Settings::clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::process_color_by(LayoutElementWithColoring& element)
{
    using namespace std::string_view_literals;

    const auto color_by = [](std::string_view key, const rjson::value& fields) -> std::unique_ptr<Coloring> {
        if (key == "continent") {
            auto cb = std::make_unique<ColoringByContinent>();
            for (const auto& continent : {"EUROPE"sv, "CENTRAL-AMERICA"sv, "MIDDLE-EAST"sv, "NORTH-AMERICA"sv, "AFRICA"sv, "ASIA"sv, "RUSSIA"sv, "AUSTRALIA-OCEANIA"sv, "SOUTH-AMERICA"sv}) {
                if (const auto& val = fields.get(continent); val.is_string())
                    cb->set(continent, Color{val.to<std::string>()});
            }
            return cb;
        }
        else if (key == "pos") {
            return std::make_unique<ColoringByPos>(acmacs::seqdb::pos1_t{rjson::get_or(fields, "pos", 192)});
        }
        else if (key == "uniform") {
            return std::make_unique<ColoringUniform>(Color{rjson::get_or(fields, "color", "black")});
        }
        else {
            fmt::print(stderr, "WARNING: unrecognized \"color_by\": {}, uniform(PINK) assumed\n", key);
            return std::make_unique<ColoringUniform>(PINK);
        }
    };

    const auto& cb_val = getenv("color_by"sv);
    auto coloring = std::visit(
        [color_by,&cb_val]<typename T>(T && arg) -> std::unique_ptr<Coloring> {
            if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
                return color_by(arg, rjson::object{});
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, rjson::object>) {
                return color_by(rjson::get_or(cb_val, "N", "uniform"), cb_val);
            }
            else if constexpr (!std::is_same_v<std::decay_t<T>, rjson::null> && !std::is_same_v<std::decay_t<T>, rjson::const_null>)
                throw error{"unsupported value type for \"color_by\""};
            else
                return {};
        },
        cb_val.val_());
    if (coloring)
        element.coloring(std::move(coloring));

} // acmacs::tal::v3::Settings::process_color_by

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_tree()
{
    auto& element = add_element<DrawTree>(tal_);
    process_color_by(element);

} // acmacs::tal::v3::Settings::add_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_time_series()
{
    auto& element = add_element<TimeSeries>(tal_);
    process_color_by(element);
    read_time_series_parameters(element);

} // acmacs::tal::v3::Settings::add_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_time_series_parameters(TimeSeries& time_series)
{
    using namespace std::string_view_literals;

    auto& param = time_series.parameters();
    acmacs::time_series::update(rjson::object{{"start"sv, getenv("start"sv)}, {"end"sv, getenv("end"sv)}, {"interval"sv, getenv("interval"sv)}}, param.time_series);

    if (const auto& dash_val = getenv("dash"sv); !dash_val.is_null()) {
        if (const auto& width = dash_val.get("width"sv); !width.is_null())
            param.dash.width = width.to<double>();
        if (const auto& line_width_pixels = dash_val.get("line_width_pixels"sv); !line_width_pixels.is_null())
            param.dash.line_width = Pixels{line_width_pixels.to<double>()};
    }

    if (const auto& slot_val = getenv("slot"sv); !slot_val.is_null()) {
        if (const auto& width = slot_val.get("width"sv); !width.is_null())
            param.slot.width = width.to<double>();
        if (const auto& separator_val = slot_val.get("separator"sv); !separator_val.is_null()) {
            if (const auto& width = separator_val.get("width_pixels"sv); !width.is_null())
                param.slot.separator.width = width.to<double>();
            if (const auto& color = separator_val.get("color"sv); !color.is_null())
                param.slot.separator.color = Color{color.to<std::string_view>()};
        }
        if (const auto& label_val = slot_val.get("label"sv); !label_val.is_null()) {
            if (const auto& rotation = label_val.get("rotation"sv); !rotation.is_null()) {
                if (const auto rot = rotation.to<std::string_view>(); rot == "clockwise")
                    param.slot.label.rotation = Rotation90DegreesClockwise;
                else if (rot == "anticlockwise" || rot == "counterclockwise")
                    param.slot.label.rotation = Rotation90DegreesAnticlockwise;
                else
                    fmt::print(stderr, "WARNING: unrecognzied label rotation value in the time series parameters: \"{}\"\n", rot);
            }
            if (const auto& color = label_val.get("color"sv); !color.is_null())
                param.slot.label.color = Color{color.to<std::string_view>()};
            if (const auto& scale = label_val.get("scale"sv); !scale.is_null())
                param.slot.label.scale = scale.to<double>();
            if (const auto& offset = label_val.get("offset"sv); !offset.is_null())
                param.slot.label.offset = offset.to<double>();
        }
    }

} // acmacs::tal::v3::Settings::read_time_series_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_clades()
{
    auto& element = add_element<Clades>(tal_);

} // acmacs::tal::v3::Settings::add_clades

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
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
