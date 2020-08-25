#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/string.hh"
#include "acmacs-base/rjson-v3-helper.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-normalize.hh"
#include "acmacs-whocc-data/vaccines.hh"
#include "acmacs-chart-2/chart.hh"
#include "acmacs-tal/log.hh"
#include "acmacs-tal/settings.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"
#include "acmacs-tal/title.hh"
#include "acmacs-tal/legend.hh"
#include "acmacs-tal/dash-bar.hh"
#include "acmacs-tal/draw-aa-transitions.hh"
#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/antigenic-maps.hh"

// ----------------------------------------------------------------------

inline void extract_coordinates(const rjson::v3::value& source, acmacs::PointCoordinates& target)
{
    if (!source.is_null()) {
        if (source.size() == *target.number_of_dimensions()) {
            for (acmacs::number_of_dimensions_t dim{0}; dim < target.number_of_dimensions(); ++dim)
                target[dim] = source[*dim].to<double>();
        }
        else
            AD_WARNING("invalid number of elements in offset: {}, not extracted", source);
    }
}

// ======================================================================

template <typename ElementType, typename... Args> ElementType& acmacs::tal::v3::Settings::add_element(Args&&... args, add_unique uniq)
{
    using namespace std::string_view_literals;
    static size_t uniq_id{0};   // thread unsafe!

    const auto id = getenv_to_string("id"sv);
    const LayoutElementId element_id{(id.empty() && uniq == add_unique::yes) ? fmt::format("-unique-{}", ++uniq_id) : id};
    if (auto* found = draw().layout().find<ElementType>(element_id); found) {
        init_element(*found);
        return *found;
    }
    else {

        auto element_p = std::make_unique<ElementType>(tal_, std::forward<Args>(args)...);
        auto& element = *element_p;
        element.id(element_id);
        draw().layout().add(std::move(element_p));
        init_element(element);
        return element;
    }

} // acmacs::tal::v3::Settings::add_element

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::init_element(LayoutElement& element)
{
    using namespace std::string_view_literals;
    getenv_copy_if_present("width-to-height-ratio"sv, element.width_to_height_ratio());
    outline(element.outline());

} // acmacs::tal::v3::Settings::init_element

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::update_env()
{
    const auto virus_type = tal_.tree().virus_type();
    if (!virus_type.empty()) {  // might be updated later upon seqdb matching
        const auto lineage = tal_.tree().lineage();
        AD_LOG(acmacs::log::settings, "tree virus type: \"{}\" lineage: \"\"", virus_type, lineage);
        setenv_toplevel("virus-type", virus_type);
        setenv_toplevel("lineage", lineage);
        if (virus_type != "B" || lineage.empty())
            setenv_toplevel("virus-type/lineage", virus_type);
        else
            setenv_toplevel("virus-type/lineage", fmt::format("{}/{}", virus_type, ::string::capitalize(lineage.substr(0, 3))));
    }
    setenv_toplevel("tree-has-sequences", tal_.tree().has_sequences());
    setenv_toplevel("chart-present", tal_.chart_present());
    if (tal_.chart_present()) {
        auto info{tal_.chart().info()};
        setenv_toplevel("chart-assay", info->assay().HI_or_Neut());
        setenv_toplevel("chart-lab", info->lab());
        setenv_toplevel("chart-rbc", info->rbc_species());
        setenv_toplevel("chart-date", info->date(chart::Info::Compute::Yes));
    }

} // acmacs::tal::v3::Settings::update_env

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Settings::apply_built_in(std::string_view name)
{
    // Timeit time_apply(fmt::format(">>>> applying {}: ", name), verb == verbose::yes ? report_time::yes : report_time::no);
    using namespace std::string_view_literals;
    try {
        // printenv();
        if (name == "antigenic-maps"sv)
            antigenic_maps();
        else if (name == "aa-transitions"sv) {
            if (DrawTree* draw_tree = draw().layout().find_draw_tree(throw_error::no); draw_tree) {
                auto& param = draw_tree->parameters().aa_transitions;
                param.calculate = true;
                param.report = getenv_or("report"sv, false);
                param.debug = getenv_or("debug"sv, true);
                if (const auto& pos = getenv("pos"sv); !pos.is_null())
                    param.report_pos = pos.to<seqdb::pos1_t>();
                param.report_number_leaves_threshold = getenv_or("number-leaves-threshold"sv, 20ul);
                if (const auto& pos = getenv("show-same-left-right-for-pos"sv); !pos.is_null())
                    param.show_same_left_right_for_pos = pos.to<seqdb::pos1_t>();
            }
            else
                AD_WARNING("\"aa-transitions\" requested but no darw_tree found");
        }
        else if (name == "clades-reset"sv)
            tree().clades_reset();
        else if (name == "clade"sv) // acmacs-whocc-data/conf/clades.json
            clade();
        else if (name == "clades"sv)
            add_clades();
        else if (name == "dash-bar"sv)
            add_dash_bar();
        else if (name == "dash-bar-clades"sv)
            add_dash_bar_clades();
        else if (name == "draw-aa-transitions"sv)
            add_draw_aa_transitions();
        else if (name == "draw-on-tree"sv)
            add_draw_on_tree();
        else if (name == "gap"sv)
            add_gap();
        else if (name == "hz-sections"sv || name == "hz_sections"sv)
            hz_sections();
        else if (name == "hz-section-marker"sv)
            hz_section_marker();
        else if (name == "ladderize"sv)
            ladderize();
        else if (name == "legend"sv)
            AD_WARNING("\"legend\" is deprecated! use \"legend\" inside \"tree\"");
        else if (name == "margins"sv)
            margins();
        else if (name == "nodes"sv)
            apply_nodes();
        else if (name == "populate-with-nuc-duplicates"sv)
            tree().populate_with_nuc_duplicates();
        else if (name == "re-root"sv)
            tree().re_root(seq_id_t{getenv_or("new-root"sv, "re-root: new-root not specified"sv)});
        else if (name == "report-branches-by-cumulative-edge"sv)
            tree().branches_by_cumulative_edge();
        else if (name == "report-branches-by-edge"sv)
            tree().branches_by_edge();
        else if (name == "report-cumulative"sv) {
            // tree().branches_by_edge();
            if (const auto output_filename = getenv_or("output"sv, ""sv); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_cumulative(getenv_or("max"sv, 0ul))); // 0 - report all
        }
        else if (name == "report-aa-at"sv) {
            report_aa_at();
        }
        else if (name == "seqdb"sv) {
            tree().match_seqdb(getenv_or("filename"sv, ""sv));
            update_env();
        }
        else if (name == "time-series"sv)
            add_time_series();
        else if (name == "title"sv)
            add_title();
        else if (name == "tree"sv)
            add_tree();
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
    using namespace std::string_view_literals;

    const auto selected = select_nodes(getenv("select"sv));

    const auto apply_one = [this, &selected](std::string_view key, const rjson::v3::value& value) {
        if (key == "hide"sv) {
            if (rjson::v3::read_bool(value, false))
                tree().hide(selected, Tree::hide_if_too_many_leaves::no);
        }
        else if (key == "line"sv) {
            if (auto* draw_on_tree = draw().layout().find<DrawOnTree>(); draw_on_tree) {
                parameters::LineWithOffset line;
                read_line_parameters(value, line);
                for (Node* node : selected)
                    draw_on_tree->parameters().per_node.push_back({node->seq_id, {}, line});
            }
        }
        else if (key == "tree-label-color"sv) {
            const Color color{value.to<std::string_view>()};
            for (Node* node : selected)
                node->label_color = color;
        }
        else if (key == "time-series-dash"sv) {
            if (auto* time_series = draw().layout().find<TimeSeries>(); time_series) {
                for (Node* node : selected) {
                    auto& entry = time_series->parameters().per_nodes.emplace_not_replace(node->seq_id).second;
                    rjson::v3::copy_if_not_null(value["color"sv], entry.color);
                    rjson::v3::copy_if_not_null(value["width"sv], entry.width);
                    rjson::v3::copy_if_not_null(value["line_width_pixels"sv], entry.line_width);
                }
            }
            else {
                throw error{"time_series element is not added yet, use \"nodes\" mod after \"layout\""};
            }
        }
        else if (key == "text"sv) {
            if (auto* draw_on_tree = draw().layout().find<DrawOnTree>(); draw_on_tree) {
                parameters::Text text;
                read_text_parameters(value, text);
                for (Node* node : selected)
                    draw_on_tree->parameters().per_node.push_back({node->seq_id, text, {}});
            }
        }
        else if (key == "tree-label-scale"sv) {
            const auto scale{value.to<double>()};
            for (Node* node : selected)
                node->label_scale = scale;
        }
        else if (key == "tree-edge-line-color"sv) {
            const Color color{value.to<std::string_view>()};
            for (Node* node : selected)
                node->color_edge_line = color;
        }
        else if (key == "tree-edge-line-width"sv) {
            const auto line_width_scale = value.to<double>();
            for (Node* node : selected)
                node->edge_line_width_scale = line_width_scale;
        }
        else if (key == "report"sv) {
            AD_INFO("{} selected nodes {}\n{}\n", selected.size(), getenv("select"sv), report_nodes("  ", selected));
        }
    };

    const auto apply_value = [apply_one](const rjson::v3::value& value_val) {
        value_val.visit([apply_one, &value_val]<typename Arg>(const Arg& arg) {
            if constexpr (std::is_same_v<Arg, rjson::v3::detail::string>)
                apply_one(arg.template to<std::string_view>(), rjson::v3::detail::boolean{true});
            else if constexpr (std::is_same_v<Arg, rjson::v3::detail::object>) {
                for (const auto& [key, val] : arg)
                    apply_one(key, val);
            }
            else
                throw error{fmt::format("don't know how to apply for \"nodes\": {}", value_val)};
        });
    };

    const rjson::v3::value& to_apply = getenv("apply"sv);
    to_apply.visit([apply_value, &to_apply]<typename Arg>(const Arg& arg) {
        if constexpr (std::is_same_v<Arg, rjson::v3::detail::array>) {
            for (const rjson::v3::value& elt : arg)
                apply_value(elt);
        }
        else
            apply_value(to_apply);
    });

} // acmacs::tal::v3::Settings::apply_nodes

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::ladderize()
{
    using namespace std::string_view_literals;
    if (const auto method = getenv_or("method"sv, "number-of-leaves"sv); method == "number-of-leaves"sv)
        tree().ladderize(Tree::Ladderize::NumberOfLeaves);
    else if (method == "max-edge-length"sv)
        tree().ladderize(Tree::Ladderize::MaxEdgeLength);
    else
        throw acmacs::settings::error{fmt::format("unsupported ladderize method: {}", method)};

} // acmacs::tal::v3::Settings::ladderize

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::margins()
{
    using namespace std::string_view_literals;
    getenv_copy_if_present("left"sv, draw().margins().left);
    getenv_copy_if_present("right"sv, draw().margins().right);
    getenv_copy_if_present("top"sv, draw().margins().top);
    getenv_copy_if_present("bottom"sv, draw().margins().bottom);
    outline(draw().outline());

} // acmacs::tal::v3::Settings::margins

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::outline(DrawOutline& draw_outline)
{
    using namespace std::string_view_literals;
    if (const auto debug_outline = getenv("debug-outline"sv); !debug_outline.is_null()) {
        debug_outline.visit(
            [&draw_outline,&debug_outline]<typename T>(const T& val) {
                if constexpr (std::is_same_v<T, rjson::v3::detail::boolean>) {
                    draw_outline.outline = val.template to<bool>();
                }
                else if constexpr (std::is_same_v<T, rjson::v3::detail::string>) {
                    draw_outline.outline = true;
                    draw_outline.outline_color = Color{val.template to<std::string_view>()};
                }
                else if constexpr (std::is_same_v<T, rjson::v3::detail::object>) {
                    draw_outline.outline = rjson::v3::get_or(val["show"sv], true);
                    rjson::v3::copy_if_not_null(val["color"sv], draw_outline.outline_color);
                    rjson::v3::copy_if_not_null(val["width"sv], draw_outline.outline_width);
                }
                else {
                    AD_WARNING("unrecognized debug-outline value: {}", debug_outline);
                }
            });
    }

} // acmacs::tal::v3::Settings::outline

// ----------------------------------------------------------------------

std::string acmacs::tal::v3::Settings::report_nodes(std::string_view indent, const NodeSet& nodes) const
{
    fmt::memory_buffer output;
    if (nodes.size() < 100) {
        for (const auto* node : nodes) {
            if (node->is_leaf())
                fmt::format_to(output, "{}{} {} [{}] edge:{:.6f} cumul:{:.6f}\n", indent, node->node_id, node->seq_id, node->date, node->edge_length.as_number(), node->cumulative_edge_length.as_number());
            else
                fmt::format_to(output, "{}{} (children: {}) edge:{:.6f} cumul:{:.6f}\n", indent, node->node_id, node->subtree.size(), node->edge_length.as_number(), node->cumulative_edge_length.as_number());
        }
    }
    return fmt::to_string(output);

} // acmacs::tal::v3::Settings::report_nodes

// ----------------------------------------------------------------------

acmacs::tal::v3::NodeSet acmacs::tal::v3::Settings::select_nodes(const rjson::v3::value& criteria) const
{
    using namespace std::string_view_literals;
    NodeSet selected;
    bool report = false;
    auto update = Tree::Select::init;
    for (const auto& [key, val] : criteria.object()) {
        if (key == "all"sv) {
            tree().select_all(selected, update);
        }
        else if (key == "aa"sv) {
            tree().select_by_aa(selected, update, acmacs::seqdb::extract_aa_at_pos1_eq_list(substitute_to_value(val)));
        }
        else if (key == "country"sv) {
            tree().select_by_country(selected, update, substitute_to_value(val).to<std::string_view>());
        }
        else if (key == "cumulative >="sv) {
            tree().select_if_cumulative_more_than(selected, update, substitute_to_value(val).to<double>());
        }
        else if (key == "date"sv) {
            tree().select_by_date(selected, update, val[0].to<std::string_view>(), val[1].to<std::string_view>());
        }
        else if (key == "edge >="sv) {
            tree().select_if_edge_more_than(selected, update, substitute_to_value(val).to<double>());
        }
        else if (key == "edge >= mean_edge of"sv) {
            tree().select_if_edge_more_than_mean_edge_of(selected, update, substitute_to_value(val).to<double>());
        }
        else if (key == "location"sv) {
            tree().select_by_location(selected, update, substitute_to_value(val).to<std::string_view>());
        }
        else if (key == "matches-chart-antigen"sv) {
            if (!tal_.chart_present())
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(tal_.chart());
            tree().select_matches_chart_antigens(selected, update);
        }
        else if (key == "matches-chart-serum"sv) {
            if (!tal_.chart_present())
                throw acmacs::settings::error{"cannot select node that matches chart antigen: no chart given"};
            tree().match(tal_.chart());
            Tree::serum_match_t mt{Tree::serum_match_t::name};
            if (const auto match_type = substitute_to_value(val).to<std::string_view>(); match_type == "reassortant"sv)
                mt = Tree::serum_match_t::reassortant;
            else if (match_type == "passage"sv)
                mt = Tree::serum_match_t::passage_type;
            else if (match_type != "name"sv)
                AD_WARNING("unrecognized \"matches-chart-serum\" value: \"{}\", \"name\" assumed", match_type);
            tree().select_matches_chart_sera(selected, update, mt);
        }
        else if (key == "seq_id"sv) {
            tree().select_by_seq_id(selected, update, substitute_to_value(val).to<std::string_view>());
        }
        else if (key == "report"sv) {
            report = substitute_to_value(val).to<bool>();
        }
        else if (key == "top-cumulative-gap"sv) {
            tree().select_by_top_cumulative_gap(selected, update, substitute_to_value(val).to<double>());
        }
        else if (key == "vaccine"sv) {
            select_vaccine(selected, update, substitute_to_value(val));
        }
        else
            throw acmacs::settings::error{fmt::format("unrecognized select node criterium: {}", key)};
        if (key != "report"sv)
            update = Tree::Select::update;
    }
    if (report)
        AD_INFO("{} selected nodes {} {}\n", selected.size(), criteria, report_nodes("  ", selected));
    return selected;

} // acmacs::tal::v3::Settings::select_nodes

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::clade() const
{
    using namespace std::string_view_literals;
    const auto clade_name = getenv_or("name"sv, ""sv);
    if (clade_name.empty())
        throw error{"empty clade name"};
    const auto display_name = getenv_or("display_name"sv, clade_name);
    const auto report = getenv_or("report"sv, false);
    // const auto inclusion_tolerance = getenv("inclusion_tolerance"sv, getenv("clade_section_inclusion_tolerance"sv, 10UL));
    // const auto exclusion_tolerance = getenv("exclusion_tolerance"sv, getenv("clade_section_exclusion_tolerance"sv, 5UL));

    if (const auto& aa_at_pos = getenv("aa"sv); !aa_at_pos.is_null()) {
        AD_LOG(acmacs::log::clades, "settings \"{}\" aa: {}", clade_name, aa_at_pos);
        tree().clade_set_by_aa_at_pos(clade_name, acmacs::seqdb::extract_aa_at_pos1_eq_list(aa_at_pos), display_name);
    }
    else if (const auto& nuc_at_pos = getenv("nuc"sv); !nuc_at_pos.is_null()) {
        AD_LOG(acmacs::log::clades, "settings \"{}\" nuc: {}", clade_name, nuc_at_pos);
        tree().clade_set_by_nuc_at_pos(clade_name, acmacs::seqdb::extract_nuc_at_pos1_eq_list(nuc_at_pos), display_name);
    }
    else
        throw error{"neither \"aa\" nor \"nuc\" provided"};

    if (report)
        tree().clade_report(clade_name);

} // acmacs::tal::v3::Settings::clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_tree()
{
    auto& tree_element = add_element<DrawTree>();
    process_color_by(tree_element);
    process_tree_legend(tree_element);
    add_element<HzSections>();

} // acmacs::tal::v3::Settings::add_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::process_color_by(LayoutElementWithColoring& element)
{
    using namespace std::string_view_literals;

    const auto color_by_pos_aa_colors = [this](const rjson::v3::value& fields) -> std::unique_ptr<Coloring> {
        const auto& pos_v = substitute_to_value(fields["pos"sv]);
        if (pos_v.is_null())
            throw error{"\"pos\" is not in \"color-by\" with \"N\": \"pos-aa-colors\""};
        return std::make_unique<ColoringByPosAAColors>(acmacs::seqdb::pos1_t{pos_v.to<acmacs::seqdb::pos1_t>()});
    };

    const auto color_by_pos_aa_frequency = [this](const rjson::v3::value& fields) -> std::unique_ptr<Coloring> {
        const auto& pos_v = substitute_to_value(fields["pos"sv]);
        if (pos_v.is_null())
            throw error{"\"pos\" is not in \"color-by\" with \"N\": \"pos-aa-frequency\""};
        auto coloring_by_pos = std::make_unique<ColoringByPosAAFrequency>(acmacs::seqdb::pos1_t{pos_v.to<acmacs::seqdb::pos1_t>()});
        if (const auto& colors_v = substitute_to_value(fields.get("colors"sv)); !colors_v.is_null()) {
            if (colors_v.is_array()) {
                for (const auto& en : colors_v.array())
                    coloring_by_pos->add_color(en.to<Color>());
            }
            else
                AD_WARNING("color-by \"colors\" must be array: {}", fields);
        }
        return coloring_by_pos;
    };

    const auto color_by = [color_by_pos_aa_colors, color_by_pos_aa_frequency](std::string_view key, const rjson::v3::value& fields) -> std::unique_ptr<Coloring> {
        if (key == "continent"sv) {
            auto cb = std::make_unique<ColoringByContinent>();
            for (const auto& continent : {"EUROPE"sv, "CENTRAL-AMERICA"sv, "MIDDLE-EAST"sv, "NORTH-AMERICA"sv, "AFRICA"sv, "ASIA"sv, "RUSSIA"sv, "AUSTRALIA-OCEANIA"sv, "SOUTH-AMERICA"sv}) {
                if (const auto& val = fields.get(continent); val.is_string())
                    cb->set(continent, Color{val.to<std::string_view>()});
            }
            return cb;
        }
        else if (key == "pos-aa-colors"sv) {
            return color_by_pos_aa_colors(fields);
        }
        else if (key == "pos-aa-frequency"sv) {
            return color_by_pos_aa_frequency(fields);
        }
        else if (key == "uniform"sv) {
            return std::make_unique<ColoringUniform>(Color{rjson::v3::get_or(fields, "color"sv, "black"sv)});
        }
        else {
            AD_WARNING("unrecognized \"color_by\": {}, uniform(PINK) assumed", key);
            return std::make_unique<ColoringUniform>(PINK);
        }
    };

    const auto& cb_val = getenv("color-by"sv, "color_by"sv);
    auto coloring = cb_val.visit([color_by, &cb_val]<typename T>(const T& arg) -> std::unique_ptr<Coloring> {
        if constexpr (std::is_same_v<T, rjson::v3::detail::string>) {
            return color_by(arg.template to<std::string_view>(), rjson::v3::detail::object{});
        }
        else if constexpr (std::is_same_v<T, rjson::v3::detail::object>) {
            return color_by(rjson::v3::get_or(cb_val, "N"sv, "uniform"sv), cb_val);
        }
        else if constexpr (!std::is_same_v<T, rjson::v3::detail::null>)
            throw error{"unsupported value type for \"color-by\""};
        else
            return {};
    });
    if (coloring)
        element.coloring(std::move(coloring));

} // acmacs::tal::v3::Settings::process_color_by

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::process_tree_legend(DrawTree& tree)
{
    using namespace std::string_view_literals;

    const auto& legend_v = getenv("legend"sv);
    if (const auto legend_type = rjson::v3::get_or(legend_v, "type"sv, tree.legend_type()); legend_type == "world-map"sv) {
        // AD_DEBUG("legend: world-map");
        auto legend{std::make_unique<LegendContinentMap>()};
        auto& param = legend->parameters();
        param.show = rjson::v3::get_or(legend_v, "show"sv, true); // shown by default
        extract_coordinates(legend_v["offset"sv], param.offset);
        if (const auto& world_map = legend_v["world-map"sv]; !world_map.is_null()) {
            rjson::v3::copy_if_not_null(world_map["size"sv], param.size);
            read_line_parameters(world_map["equator"sv], param.equator);
            read_line_parameters(world_map["tropics"sv], param.tropics);
            for (const rjson::v3::value& for_dot : world_map["dots"sv].array())
                read_dot_parameters(for_dot, param.dots.emplace_back());
        }
        tree.legend(std::move(legend));
    }
    else if (legend_type == "color-by-pos-aa-colors"sv || legend_type == "color-by-pos-aa-frequency"sv) {
        // AD_DEBUG("legend: color_by-pos-aa-frequency");
        auto legend{std::make_unique<LegendColoredByPos>()};
        auto& param = legend->parameters();
        param.show = rjson::v3::get_or(legend_v, "show"sv, true); // shown by default
        extract_coordinates(legend_v["offset"sv], param.offset);
        if (const auto& color_by_pos = legend_v["color-by-pos"sv]; !color_by_pos.is_null()) {
            rjson::v3::copy_if_not_null(color_by_pos["text-size"sv], param.text_size);
            rjson::v3::copy_if_not_null(color_by_pos["title-color"sv], param.title_color);
            rjson::v3::copy_if_not_null(color_by_pos["interleave"sv], param.interleave);
            if (const auto& count_v = color_by_pos["count"sv]; !count_v.is_null()) {
                rjson::v3::copy_if_not_null(count_v["show"sv], param.show_count);
                rjson::v3::copy_if_not_null(count_v["scale"sv], param.count_scale);
                rjson::v3::copy_if_not_null(count_v["color"sv], param.count_color);
            }
        }
        tree.legend(std::move(legend));
    }
    else if (legend_type == "none"sv) {
        // AD_DEBUG("legend: none");
    }
    else {
        AD_WARNING("unrecognized legend type \"{}\", tree lgend not shown", legend_type);
    }

} // acmacs::tal::v3::Settings::process_tree_legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_gap()
{
    using namespace std::string_view_literals;

    auto& gap_element = add_element<Gap>(add_unique::yes);
    auto& param = gap_element.parameters();

    if (const auto& pixels_value = getenv("pixels"sv); !pixels_value.is_null())
        param.pixels = pixels_value.to<double>();

} // acmacs::tal::v3::Settings::add_gap

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_time_series()
{
    using namespace std::string_view_literals;

    auto& element = add_element<TimeSeries>();
    auto& param = element.parameters();

    process_color_by(element);
    process_legend(element);
    read_dash_parameters(param.dash);

    if (const auto& interval_v = getenv("interval"sv); !interval_v.is_null()) {
        std::optional<std::string_view> interval{std::nullopt};
        if (interval_v.is_object()) {
            for (const auto& interval_n : {"year"sv, "month"sv, "week"sv, "day"sv}) {
                if (const auto& num = interval_v[interval_n]; !num.is_null()) {
                    interval = interval_n;
                    param.time_series.number_of_intervals = num.to<date::period_diff_t>();
                    break;
                }
            }
            if (!interval.has_value())
                AD_WARNING("unrecognized interval specification: {}, month assumed", interval_v);
        }
        else if (interval_v.is_string())
            interval = interval_v.to<std::string_view>();
        else
            AD_WARNING("unrecognized interval specification: {}, month assumed", interval_v);
        if (interval)
            param.time_series.intervl = acmacs::time_series::interval_from_string(*interval);
    }

    if (const auto& start = getenv("start"sv); !start.is_null())
        param.time_series.first = date::from_string(start.to<std::string_view>(), date::allow_incomplete::yes, date::throw_on_error::yes);
    if (const auto& end = getenv("end"sv); !end.is_null())
        param.time_series.after_last = date::from_string(end.to<std::string_view>(), date::allow_incomplete::yes, date::throw_on_error::yes);
    rjson::v3::copy_if_not_null(getenv("report"sv), param.report);

    if (const auto& slot_val = getenv("slot"sv); !slot_val.is_null()) {
        rjson::v3::copy_if_not_null(slot_val["width"sv], param.slot.width);

        if (const auto& separator = slot_val["separator"sv]; !separator.is_null()) {
            if (const auto& width_pixels = separator["width_pixels"sv]; !width_pixels.is_null()) {
                rjson::v3::copy_if_not_null(width_pixels, param.slot.separator[0].line_width);
                for (auto mp = std::next(std::begin(param.slot.separator)); mp != std::end(param.slot.separator); ++mp)
                    mp->line_width = param.slot.separator[0].line_width;
            }
            if (const auto& color = separator["color"sv]; !color.is_null()) {
                rjson::v3::copy_if_not_null(color, param.slot.separator[0].color);
                for (auto mp = std::next(std::begin(param.slot.separator)); mp != std::end(param.slot.separator); ++mp)
                    mp->color = param.slot.separator[0].color;
            }
            for (const rjson::v3::value& for_month : separator["per_month"sv].array()) {
                if (const auto& month = for_month["month"sv]; !month.is_null()) {
                    const auto month_no = month.to<size_t>() - 1;
                    rjson::v3::copy_if_not_null(for_month["width_pixels"sv], param.slot.separator[month_no].line_width);
                    rjson::v3::copy_if_not_null(for_month["color"], param.slot.separator[month_no].color);
                }
            }
        }

        if (const auto& background = slot_val["background"sv]; !background.is_null()) {
            if (const auto& color = background["color"sv]; !color.is_null()) {
                rjson::v3::copy_if_not_null(color, param.slot.background[0]);
                for (auto mp = std::next(std::begin(param.slot.background)); mp != std::end(param.slot.background); ++mp)
                    *mp = param.slot.background[0];
            }
            for (const rjson::v3::value& for_month : background["per_month"sv].array()) {
                if (const auto& month = for_month["month"sv]; !month.is_null()) {
                    const auto month_no = month.to<size_t>() - 1;
                    rjson::v3::copy_if_not_null(for_month["color"sv], param.slot.background[month_no]);
                }
            }
        }

        rjson::v3::copy_if_not_null(slot_val.get("label"sv, "color"sv), param.slot.label.color);
        rjson::v3::copy_if_not_null(slot_val.get("label"sv, "scale"sv), param.slot.label.scale);
        rjson::v3::copy_if_not_null(slot_val.get("label"sv, "offset"sv), param.slot.label.offset);
        if (const auto& rot_v = slot_val.get("label"sv, "rotation"sv); !rot_v.is_null()) {
            if (const auto rot = rot_v.to<std::string_view>(); rot == "clockwise"sv)
                param.slot.label.rotation = Rotation90DegreesClockwise;
            else if (rot == "anticlockwise"sv || rot == "counterclockwise"sv)
                param.slot.label.rotation = Rotation90DegreesAnticlockwise;
            else
                AD_WARNING("unrecognzied label rotation value in the time series parameters: \"{}\"", rot);
        }
    }

    if (const auto& color_scale_val = getenv("color-scale"sv); !color_scale_val.is_null()) {
        rjson::v3::copy_if_not_null(color_scale_val["show"sv], param.color_scale.show);
        if (const auto& type_val = color_scale_val["type"sv]; !type_val.is_null()) {
            if (const auto type = type_val.to<std::string_view>(); type == "bezier_gradient" || type == "bezier-gradient"sv)
                param.color_scale.type = TimeSeries::color_scale_type::bezier_gradient;
            else
                AD_WARNING("unrecognized time-series color-scale type (\"bezier-gradient\" expected): {}", type);
        }
        switch (param.color_scale.type) {
            case TimeSeries::color_scale_type::bezier_gradient:
                if (const auto& colors_val = color_scale_val["colors"sv]; !colors_val.is_null()) {
                    if (colors_val.is_array() && colors_val.size() == 3) {
                        for (size_t index = 0; index < colors_val.size(); ++index)
                            param.color_scale.colors[index] = Color{colors_val[index].to<std::string_view>()};
                    }
                    else
                        AD_WARNING("invalid number of colors in time-series color-scale colors (3 strings expected): {}", colors_val);
                }
                break;
        }
        rjson::v3::copy_if_not_null(color_scale_val["offset"sv], param.color_scale.offset);
        rjson::v3::copy_if_not_null(color_scale_val["height"sv], param.color_scale.height);
    }

} // acmacs::tal::v3::Settings::add_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::process_legend(TimeSeries& time_series)
{
    using namespace std::string_view_literals;

    if (const auto& legend = getenv("legend"sv); !legend.is_null()) {
        auto& legend_param = time_series.parameters().legend;
        rjson::v3::copy_if_not_null(legend["show"sv], legend_param.show);
        rjson::v3::copy_if_not_null(legend["scale"sv], legend_param.scale);
        rjson::v3::copy_if_not_null(legend["offset"sv], legend_param.offset);
        rjson::v3::copy_if_not_null(legend["gap_scale"sv], legend_param.gap_scale);
        rjson::v3::copy_if_not_null(legend["count_scale"sv], legend_param.count_scale);
        rjson::v3::copy_if_not_null(legend["pos_color"sv], legend_param.pos_color);
        rjson::v3::copy_if_not_null(legend["count_color"sv], legend_param.count_color);
    }

} // acmacs::tal::v3::Settings::process_legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_dash_parameters(parameters::Dash& param)
{
    using namespace std::string_view_literals;

    if (const auto& dash_val = getenv("dash"sv); !dash_val.is_null()) {
        rjson::v3::copy_if_not_null(dash_val["width"sv], param.width);
        rjson::v3::copy_if_not_null(dash_val["line_width_pixels"sv], param.line_width);
    }

} // acmacs::tal::v3::Settings::read_dash_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_clade_parameters(const rjson::v3::value& source, Clades::CladeParameters& clade_parameters)
{
    using namespace std::string_view_literals;

    const auto& display_name = source["display_name"sv];
    display_name.visit([&clade_parameters, &display_name]<typename Arg>(const Arg& arg) {
        if constexpr (std::is_same_v<Arg, rjson::v3::detail::string>) {
            clade_parameters.display_name.emplace_back(arg.template to<std::string_view>());
        }
        else if constexpr (std::is_same_v<Arg, rjson::v3::detail::array>) {
            for (const auto& dn : arg)
                clade_parameters.display_name.emplace_back(dn.template to<std::string_view>());
        }
        else if constexpr (!std::is_same_v<Arg, rjson::v3::detail::null>)
            throw error{fmt::format("clade \"display_name\": invalid value ({}), array of strings or string expected", display_name)};
    });

    source["show"sv].visit([&clade_parameters]<typename Arg>(const Arg& arg) {
        if constexpr (std::is_same_v<Arg, rjson::v3::detail::boolean>) {
            clade_parameters.hidden.clear();
            clade_parameters.hidden.push_back(!arg);
        }
        else if constexpr (std::is_same_v<Arg, rjson::v3::detail::array>) {
            clade_parameters.hidden.clear();
            for (const auto& show : arg)
                clade_parameters.hidden.push_back(!show);
        }
        else if constexpr (!std::is_same_v<Arg, rjson::v3::detail::null>)
            throw error{"clade \"show\": invalid value, array of bools or bool expected"};
    });
    // AD_DEBUG("read_clade_parameters \"{}\" hidden {}", clade_parameters.name, clade_parameters.hidden);

    source["slot"sv].visit([&clade_parameters]<typename Arg>(const Arg& arg) {
            if constexpr (std::is_same_v<Arg, rjson::v3::detail::number>) {
                clade_parameters.slot_no.clear();
                clade_parameters.slot_no.push_back(arg.template to<Clades::slot_no_t>());
            }
            else if constexpr (std::is_same_v<Arg, rjson::v3::detail::array>) {
                clade_parameters.slot_no.clear();
                for (const auto& slot : arg)
                    clade_parameters.slot_no.push_back(slot.template to<Clades::slot_no_t>());
            }
            else if constexpr (!std::is_same_v<Arg, rjson::v3::detail::null>)
                throw error{"clade \"slot\": invalid value, array of ints or int expected"};
    });

    const auto& label_v = source["label"sv];
    label_v.visit([&clade_parameters, this, &label_v]<typename Arg>(const Arg& arg) {
            if constexpr (std::is_same_v<Arg, rjson::v3::detail::object>) {
                clade_parameters.label.clear();
                read_label_parameters(label_v, clade_parameters.label.emplace_back());
            }
            else if constexpr (std::is_same_v<Arg, rjson::v3::detail::array>) {
                clade_parameters.label.clear();
                for (const auto& label : arg)
                    read_label_parameters(label, clade_parameters.label.emplace_back());
            }
            else if constexpr (!std::is_same_v<Arg, rjson::v3::detail::null>)
                throw error{"clade \"label\": invalid value, array or object expected"};
    });

    rjson::v3::copy_if_not_null(source["section_inclusion_tolerance"sv], clade_parameters.section_inclusion_tolerance);
    rjson::v3::copy_if_not_null(source["section_exclusion_tolerance"sv], clade_parameters.section_exclusion_tolerance);

    rjson::v3::copy_if_not_null(source.get("arrow"sv, "color"sv), clade_parameters.arrow.color);
    rjson::v3::copy_if_not_null(source.get("arrow"sv, "line_width"sv), clade_parameters.arrow.line_width);
    rjson::v3::copy_if_not_null(source.get("arrow"sv, "arrow_width"sv), clade_parameters.arrow.arrow_width);

    read_line_parameters(source["horizontal_line"sv], clade_parameters.horizontal_line);

    rjson::v3::copy_if_not_null(source["top-gap"sv], clade_parameters.tree_top_gap);
    rjson::v3::copy_if_not_null(source["bottom-gap"sv], clade_parameters.tree_bottom_gap);
    rjson::v3::copy_if_not_null(source["time_series_top_separator"sv], clade_parameters.time_series_top_separator);
    rjson::v3::copy_if_not_null(source["time_series_bottom_separator"sv], clade_parameters.time_series_bottom_separator);

} // acmacs::tal::v3::Settings::read_clade_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_per_clade(Clades::Parameters& parameters)
{
    using namespace std::string_view_literals;
    for (const rjson::v3::value& for_clade : getenv("per_clade"sv).array()) {
        if (const auto& name = for_clade["name"sv]; !name.is_null())
            read_clade_parameters(for_clade, parameters.find_or_add_pre_clade(name.to<std::string_view>()));
    }

} // acmacs::tal::v3::Settings::read_per_clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_clades()
{
    AD_LOG(acmacs::log::settings, "clades");

    using namespace std::string_view_literals;

    auto& element = add_element<Clades>();
    auto& param = element.parameters();

    getenv_copy_if_present("report"sv, param.report);

    if (const auto& slot_val = substitute_to_value(getenv("slot"sv)); !slot_val.is_null()) {
        rjson::v3::copy_if_not_null(substitute_to_value(slot_val["width"sv]), param.slot.width);
    }

    // ----------------------------------------------------------------------

    if (const auto& all_clades_val = getenv("all_clades"sv); !all_clades_val.is_null())
        read_clade_parameters(all_clades_val, param.all_clades);
    read_per_clade(param);

} // acmacs::tal::v3::Settings::add_clades

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::hz_sections()
{
    using namespace std::string_view_literals;

    auto& element = add_element<HzSections>();
    auto& param = element.parameters();

    getenv_copy_if_present("report"sv, param.report);
    read_line_parameters(substitute_to_value(getenv("line"sv)), param.line);
    rjson::v3::copy_if_not_null(getenv("top-gap"sv), param.tree_top_gap);
    rjson::v3::copy_if_not_null(getenv("bottom-gap"sv), param.tree_bottom_gap);

    for (const rjson::v3::value& for_section : getenv("sections"sv).array()) {
        if (const auto& id = for_section["id"sv]; !id.is_null()) {
            auto& section = param.find_add_section(id.to<std::string_view>());
            section.shown = rjson::v3::get_or(for_section, "show"sv, true);
            rjson::v3::copy_if_not_null(for_section["first"sv], section.first);
            rjson::v3::copy_if_not_null(for_section["last"sv], section.last);
            rjson::v3::copy_if_not_null(for_section["label"sv], section.label);
            rjson::v3::copy_if_not_null(for_section["aa_transitions"sv], section.label_aa_transitions);
        }
        else if (for_section["?id"sv].is_null() && for_section["? id"sv].is_null())
            AD_WARNING("settings hz-section without \"id\" ignored: {}", for_section);
    }

} // acmacs::tal::v3::Settings::hz_sections

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::hz_section_marker()
{
    using namespace std::string_view_literals;

    auto& element = add_element<HzSectionMarker>();
    auto& param = element.parameters();

    read_line_parameters(substitute_to_value(getenv("line"sv)), param.line);

} // acmacs::tal::v3::Settings::hz_section_marker

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::antigenic_maps()
{
    using namespace std::string_view_literals;

    auto& element = add_element<AntigenicMaps>();
    auto& param = element.parameters();

    getenv_copy_if_present("gap-between-maps"sv, param.gap_between_maps);
    getenv_copy_if_present("columns"sv, param.columns);

} // acmacs::tal::v3::Settings::antigenic_maps

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_label_parameters(const rjson::v3::value& source, parameters::Label& param)
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::v3::copy_if_not_null(substitute_to_value(source["color"sv]), param.color);
        rjson::v3::copy_if_not_null(substitute_to_value(source["scale"sv]), param.scale);
        if (const auto& position_v = substitute_to_value(source["vertical_position"sv]); !position_v.is_null()) {
            if (const auto position = position_v.to<std::string_view>(); position == "middle"sv)
                param.vpos = parameters::vertical_position::middle;
            else if (position == "top"sv)
                param.vpos = parameters::vertical_position::top;
            else if (position == "bottom"sv)
                param.vpos = parameters::vertical_position::bottom;
            else
                AD_WARNING("unrecognized clade label position: \"{}\"", position);
        }
        if (const auto& position_v = substitute_to_value(source["horizontal_position"sv]); !position_v.is_null()) {
            if (const auto position = position_v.to<std::string_view>(); position == "middle"sv)
                param.hpos = parameters::horizontal_position::middle;
            else if (position == "left"sv)
                param.hpos = parameters::horizontal_position::left;
            else if (position == "right"sv)
                param.hpos = parameters::horizontal_position::right;
            else
                AD_WARNING("unrecognized clade label position: \"{}\"", position);
        }
        if (const auto& offset_v = source["offset"sv]; !offset_v.is_null()) {
            if (offset_v.size() == param.offset.size()) {
                for (size_t index = 0; index < param.offset.size(); ++index)
                    param.offset[index] = offset_v[index].to<double>();
            }
            else
                AD_WARNING("Invalid label offset, two numbers expected: {}", source);
        }
        rjson::v3::copy_if_not_null(substitute_to_value(source["text"sv]), param.text);
        if (const auto& rotation_degrees_v = substitute_to_value(source["rotation_degrees"sv]); !rotation_degrees_v.is_null())
            param.rotation = RotationDegrees(rotation_degrees_v.to<double>());

        rjson::v3::copy_if_not_null(source.get("tether"sv, "show"sv), param.tether.show);
        rjson::v3::copy_if_not_null(source.get("tether"sv, "color"sv), param.tether.line.color);
        rjson::v3::copy_if_not_null(source.get("tether"sv, "line_width"sv), param.tether.line.line_width);

        rjson::v3::copy_if_not_null(source.get("text_style"sv, "font"sv), param.text_style.font_family);
        rjson::v3::copy_if_not_null(source.get("text_style"sv, "slant"sv), param.text_style.slant);
        rjson::v3::copy_if_not_null(source.get("text_style"sv, "weight"sv), param.text_style.weight);
    }

} // acmacs::tal::v3::Settings::read_label_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_dash_bar()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DashBar>();
    auto& param = element.parameters();

    read_dash_parameters(param.dash);

    for (const rjson::v3::value& entry : getenv("nodes"sv).array()) {
        auto& for_nodes = param.for_nodes.emplace_back();
        for_nodes.nodes = select_nodes(entry["select"sv]);
        rjson::v3::copy_if_not_null(substitute_to_value(entry["color"sv]), for_nodes.color);
    }

    for (const rjson::v3::value& label_data : getenv("labels"sv).array())
        read_label_parameters(label_data, param.labels.emplace_back());

} // acmacs::tal::v3::Settings::add_dash_bar

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_dash_bar_clades()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DashBarClades>();
    auto& param = element.parameters();

    read_dash_parameters(param.dash);

    for (const rjson::v3::value& for_clade : getenv("clades"sv).array()) {
        auto& clade = param.clades.emplace_back();
        rjson::v3::copy_if_not_null(for_clade["name"sv], clade.name);
        rjson::v3::copy_if_not_null(for_clade["color"sv], clade.color);
        read_label_parameters(for_clade["label"sv], clade.label);
    }

} // acmacs::tal::v3::Settings::add_dash_bar_clades

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_title()
{
    using namespace std::string_view_literals;

    auto& element = add_element<Title>();
    auto& param = element.parameters();

    param.text = getenv_to_string("text"sv, toplevel_only::no, if_no_substitution_found::leave_as_is);
    extract_coordinates(getenv("offset"sv), param.offset);
    rjson::v3::copy_if_not_null(getenv("color"sv), param.color);
    rjson::v3::copy_if_not_null(getenv("size"sv), param.size);

} // acmacs::tal::v3::Settings::add_title

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_text_parameters(const rjson::v3::value& source, parameters::Text& text_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::v3::copy_if_not_null(source["text"sv], text_parameters.text);
        extract_coordinates(source["offset"sv], text_parameters.offset);
        rjson::v3::copy_if_not_null(source["absolute_x"sv], text_parameters.absolute_x);
        rjson::v3::copy_if_not_null(source["color"sv], text_parameters.color);
        rjson::v3::copy_if_not_null(source["size"sv], text_parameters.size);
    }

} // acmacs::tal::v3::Settings::read_text_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_line_parameters(const rjson::v3::value& source, parameters::Line& line_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::v3::copy_if_not_null(source["color"sv], line_parameters.color);
        rjson::v3::copy_if_not_null(source["line_width"sv], line_parameters.line_width);

        if (const auto& dash_val = source["dash"sv]; !dash_val.is_null()) {
            const auto dash = dash_val.to<std::string_view>();
            if (dash.empty() || dash == "no" || dash == "no-dash" || dash == "no_dash"sv)
                line_parameters.dash = surface::Dash::NoDash;
            else if (dash == "dash1"sv)
                line_parameters.dash = surface::Dash::Dash1;
            else if (dash == "dash2"sv)
                line_parameters.dash = surface::Dash::Dash2;
            else if (dash == "dash3"sv)
                line_parameters.dash = surface::Dash::Dash3;
        }
    }

} // acmacs::tal::v3::Settings::read_line_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_line_parameters(const rjson::v3::value& source, parameters::LineWithOffset& line_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        read_line_parameters(source, static_cast<parameters::Line&>(line_parameters));
        extract_coordinates(source["c1"sv], line_parameters.offset[0]);
        extract_coordinates(source["c2"sv], line_parameters.offset[1]);
        rjson::v3::copy_if_not_null(source["absolute_x"sv], line_parameters.absolute_x);
    }

} // acmacs::tal::v3::Settings::read_line_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_dot_parameters(const rjson::v3::value& source, parameters::WorldMapDot& dot_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        if (const auto& loc = source["location"sv]; !loc.is_null()) {
            try {
                const auto found = acmacs::locationdb::get().find_or_throw(loc.to<std::string_view>());
                dot_parameters.coordinates = PointCoordinates{found.latitude(), found.longitude()};
            }
            catch (std::exception&) {
                AD_WARNING("\"location\" for world map dot not found: {}", source);
            }
        }
        extract_coordinates(source["coordinates"sv], dot_parameters.coordinates);
        rjson::v3::copy_if_not_null(source["outline"sv], dot_parameters.outline);
        rjson::v3::copy_if_not_null(source["fill"sv], dot_parameters.fill);
        rjson::v3::copy_if_not_null(source["outline_width"sv], dot_parameters.outline_width);
        rjson::v3::copy_if_not_null(source["size"sv], dot_parameters.size);
    }

} // acmacs::tal::v3::Settings::read_dot_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_draw_aa_transitions()
{
    using namespace std::string_view_literals;

    if (DrawTree* draw_tree = draw().layout().find_draw_tree(throw_error::no); draw_tree) {
        auto& aa_transitions = draw_tree->parameters().aa_transitions;
        aa_transitions.calculate = true;
        if (const auto& method_v = getenv("method"sv); !method_v.is_null()) {
            const auto method = method_v.to<std::string_view>();
            if (method.empty() || method == "derek"sv)
                aa_transitions.method = draw_tree::AATransitionsParameters::method::derek;
            else if (method == "eu_20200514"sv || method == "eu-20200514"sv)
                aa_transitions.method = draw_tree::AATransitionsParameters::method::eu_20200514;
            else
                throw error{"\"draw-aa-transitions\": invalid \"method\" (\"derek\" or \"eu_20200514\" expected)"};
        }
    }

    auto& element = add_element<DrawAATransitions>();
    auto& param = element.parameters();

    getenv_copy_if_present("show"sv, param.show);
    getenv_copy_if_present("minimum_number_leaves_in_subtree"sv, param.minimum_number_leaves_in_subtree);
    getenv_copy_if_present("text_line_interleave"sv, param.text_line_interleave);

    getenv("only-for"sv).visit([&param, this]<typename Val>(const Val& value) { // draw only for the specified pos, if list is absent or empty, draw for all pos
        if constexpr (std::is_same_v<Val, rjson::v3::detail::array>) {
            for (const auto& pos_v : value)
                param.only_for_pos.emplace_back(substitute_to_value(pos_v).template to<size_t>());
        }
        else if constexpr (std::is_same_v<Val, rjson::v3::detail::number>)
            param.only_for_pos.emplace_back(value.template to<size_t>());
        else if constexpr (!std::is_same_v<Val, rjson::v3::detail::null>)
            throw error{"\"draw-aa-transitions\": invalid \"only-for\", array or number expected"};
    });

    // ----------------------------------------------------------------------

    enum class ignore_name { no, yes };
    const auto read_node_parameters = [this](const rjson::v3::value& source, DrawAATransitions::TransitionParameters& parameters, ignore_name ign) {
        if (ign == ignore_name::no)
            rjson::v3::copy_if_not_null(source["node_id"sv], parameters.node_id);
        read_label_parameters(source["label"sv], parameters.label);
    };

    if (const auto& all_nodes_val = getenv("all_nodes"sv); !all_nodes_val.is_null())
        read_node_parameters(all_nodes_val, param.all_nodes, ignore_name::yes);
    for (const rjson::v3::value& for_node : getenv("per_node"sv).array()) {
        param.per_node.push_back(param.all_nodes);
        read_node_parameters(for_node, param.per_node.back(), ignore_name::no);
    }

} // acmacs::tal::v3::Settings::add_draw_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_draw_on_tree()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DrawOnTree>();
    auto& param = element.parameters();

    for (const rjson::v3::value& text_entry : getenv("texts"sv).array()) {
        auto& text_param = param.texts.emplace_back();
        read_text_parameters(text_entry, text_param);
    }

} // acmacs::tal::v3::Settings::add_draw_on_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::report_aa_at() const
{
    using namespace std::string_view_literals;

    const auto extract = []<typename From>(const From& from) -> size_t {
        if constexpr (std::is_same_v<From, rjson::v3::detail::string>)
            return acmacs::string::from_chars<size_t>(from.template to<std::string_view>());
        else if constexpr (std::is_same_v<From, rjson::v3::detail::number>)
            return from.template to<size_t>();
        throw std::exception{};
    };

    std::vector<acmacs::seqdb::pos1_t> pos_to_report;
    try {
        substitute_to_value(getenv("pos")).visit([&pos_to_report, extract]<typename Val>(const Val& value) {
            if constexpr (std::is_same_v<Val, rjson::v3::detail::string> || std::is_same_v<Val, rjson::v3::detail::number>) {
                pos_to_report.emplace_back(extract(value));
            }
            else if constexpr (std::is_same_v<Val, rjson::v3::detail::array>) {
                for (const auto& subval : value)
                    pos_to_report.emplace_back(subval.visit([extract]<typename Sub>(const Sub& sub) -> size_t { return extract(sub); }));
            }
            else
                throw std::exception{};
        });
    }
    catch (std::exception&) {
        const auto& pos = getenv("pos");
        throw error{AD_FORMAT("unsupported value for report_pos_at \"pos\": {} substituted: {}", pos, substitute_to_value(pos))};
    }

    const auto output = tree().report_aa_at(pos_to_report, substitute_to_value(getenv("names"sv)).to<bool>());
    if (const auto output_filename = getenv_or("output"sv, ""sv); !output_filename.empty() && output_filename != "-"sv)
        acmacs::file::write(output_filename, output);
    else
        AD_INFO("AA at:\n{}", output);

} // acmacs::tal::v3::Settings::report_aa_at

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::v3::value& criteria) const
{
    using namespace std::string_view_literals;

    const acmacs::virus::type_subtype_t virus_type{getenv_or("virus-type"sv, ""sv)};
    const acmacs::virus::lineage_t lineage{getenv_or("lineage"sv, ""sv)};
    AD_LOG(acmacs::log::vaccines, "select_vaccine virus-type: \"{}\" lineage: \"{}\"", virus_type, lineage);

    const auto names = ranges::to<std::vector<std::string>>(
        acmacs::whocc::vaccine_names(virus_type, lineage)
        | ranges::views::filter([vaccine_type=acmacs::whocc::Vaccine::type_from_string(rjson::v3::get_or(criteria, "type"sv, "any"sv))](const auto& en) { return vaccine_type == acmacs::whocc::vaccine_type::any || en.type == vaccine_type; })
        | ranges::views::transform([](const auto& en) { return en.name; }));

    AD_LOG(acmacs::log::vaccines, "{}", names);
    NodeSet selected_nodes;
    for (const auto& name : names) {
        NodeSet some_nodes;
        tree().select_by_seq_id(some_nodes, Tree::Select::init, *acmacs::seqdb::make_seq_id(name));
        selected_nodes.add(some_nodes);
    }

    if (const auto passage = ::string::lower(rjson::v3::get_or(criteria, "passage"sv, ""sv)); !passage.empty()) {
        const auto exclude_by_passage = [&passage](const auto* node) {
            const auto name = acmacs::virus::name::parse(node->seq_id);
            if (passage == "cell"sv)
                return !name.passage.is_cell();
            else if (passage == "egg"sv)
                return !name.passage.is_egg();
            else if (passage == "reassortant"sv)
                return name.reassortant.empty();
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
    // AD_INFO("{} selected nodes {}\n", selected.size(), getenv("select"sv), report_nodes("  ", nodes));

} // acmacs::tal::v3::Settings::select_vaccine

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
