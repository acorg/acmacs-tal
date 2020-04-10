#include "acmacs-base/read-file.hh"
#include "acmacs-base/range-v3.hh"
#include "acmacs-base/string.hh"
#include "locationdb/locdb.hh"
#include "acmacs-virus/virus-name-parse.hh"
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

template <typename ElementType, typename... Args> ElementType& acmacs::tal::v3::Settings::add_element(Args&&... args, add_unique uniq)
{
    using namespace std::string_view_literals;
    static size_t uniq_id{0};   // thread unsafe!
    const LayoutElementId element_id{getenv("id"sv, uniq == add_unique::yes ? fmt::format("-unique-{}", ++uniq_id) : std::string{})};
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
        if (lineage.empty())
            setenv_toplevel("virus-type/lineage", virus_type);
        else
            setenv_toplevel("virus-type/lineage", fmt::format("{}/{}", virus_type, ::string::capitalize(lineage.substr(0, 3))));
    }
    setenv_toplevel("tree-has-sequences", tal_.tree().has_sequences());
    setenv_toplevel("chart-present", tal_.chart_present());
    if (tal_.chart_present())
        setenv_toplevel("chart-assay", tal_.chart().info()->assay().hi_or_neut());

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
            // Timeit time_update_common_aa(">>>> update_common_aa: ", verb == verbose::yes ? report_time::yes : report_time::no);
            tree().update_common_aa();
            // time_update_common_aa.report();
            // Timeit time_update_aa_transitions(">>>> update_aa_transitions: ", verb == verbose::yes ? report_time::yes : report_time::no);
            tree().update_aa_transitions();
            // time_update_aa_transitions.report();
            if (getenv("report"sv, false))
                tree().report_aa_transitions();
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
            add_legend();
        else if (name == "margins"sv)
            margins();
        else if (name == "nodes"sv)
            apply_nodes();
        else if (name == "populate-with-nuc-duplicates"sv)
            tree().populate_with_nuc_duplicates();
        else if (name == "re-root"sv)
            tree().re_root(seq_id_t{getenv("new-root"sv, "re-root: new-root not specified")});
        else if (name == "report-cumulative"sv) {
            // tree().branches_by_edge();
            if (const auto output_filename = getenv("output"sv, ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_cumulative());
        }
        else if (name == "report-time-series"sv) {
            if (const auto output_filename = getenv("output"sv, ""); !output_filename.empty())
                acmacs::file::write(output_filename, tree().report_time_series(Tree::report_size::detailed));
            else
                AD_INFO("{}", tree().report_time_series(Tree::report_size::brief));
        }
        else if (name == "seqdb"sv) {
            tree().match_seqdb(getenv("filename"sv, ""));
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

    const auto selected = select_nodes(getenv("select"));

    const auto apply_one = [this, &selected](std::string_view key, const rjson::value& value) {
        if (key == "hide") {
            if (value.is_bool()) {
                if (value.to<bool>())
                    tree().hide(selected);
            }
            else
                throw error{fmt::format("unrecognized value for \"{}\" operation on the selected nodes", key)};
        }
        else if (key == "line") {
            if (auto* draw_on_tree = draw().layout().find<DrawOnTree>(); draw_on_tree) {
                LayoutElement::LineWithOffsetParameters line;
                read_line_parameters(value, line);
                for (Node* node : selected)
                    draw_on_tree->parameters().per_node.push_back({node->seq_id, {}, line});
            }
        }
        else if (key == "tree-label-color") {
            const Color color{value.to<std::string_view>()};
            for (Node* node : selected)
                node->label_color = color;
        }
        else if (key == "time-series-dash") {
            if (auto* time_series = draw().layout().find<TimeSeries>(); time_series) {
                for (Node* node : selected) {
                    auto& entry = time_series->parameters().per_nodes.emplace_not_replace(node->seq_id).second;
                    rjson::copy_if_not_null(value.get("color"sv), entry.color);
                    rjson::copy_if_not_null(value.get("width"sv), entry.width);
                    rjson::copy_if_not_null(value.get("line_width_pixels"sv), entry.line_width);
                }
            }
            else {
                throw error{"time_series element is not added yet, use \"nodes\" mod after \"layout\""};
            }
        }
        else if (key == "text") {
            if (auto* draw_on_tree = draw().layout().find<DrawOnTree>(); draw_on_tree) {
                LayoutElement::TextParameters text;
                read_text_parameters(value, text);
                for (Node* node : selected)
                    draw_on_tree->parameters().per_node.push_back({node->seq_id, text, {}});
            }
        }
        else if (key == "tree-label-scale") {
            const auto scale{value.to<double>()};
            for (Node* node : selected)
                node->label_scale = scale;
        }
        else if (key == "tree-edge-line-color") {
            const Color color{value.to<std::string_view>()};
            for (Node* node : selected)
                node->color_edge_line = color;
        }
        else if (key == "tree-edge-line-width") {
            const auto line_width_scale = value.to<double>();
            for (Node* node : selected)
                node->edge_line_width_scale = line_width_scale;
        }
        else if (key == "report") {
            report_nodes(fmt::format("INFO {} selected nodes {}\n", selected.size(), getenv("select")), "  ", selected);
        }
    };

    const auto apply_value = [apply_one](const rjson::value& value_val) {
        std::visit(
            [apply_one]<typename ArgX>(ArgX && arg) {
                using Arg = std::decay_t<ArgX>;
                if constexpr (std::is_same_v<Arg, std::string>)
                    apply_one(arg, true);
                else if constexpr (std::is_same_v<Arg, rjson::object>)
                    arg.for_each([apply_one](std::string_view key, const rjson::value& val) { apply_one(key, val); });
                else
                    throw error{fmt::format("don't know how to apply for \"nodes\": {}", arg)};
            },
            value_val.val_());
    };

    const rjson::value& to_apply = getenv("apply");
    std::visit(
        [apply_value, &to_apply]<typename ArgX>(ArgX && arg) {
            using Arg = std::decay_t<ArgX>;
            if constexpr (std::is_same_v<Arg, rjson::array>)
                arg.for_each([apply_value](const rjson::value& elt) { apply_value(elt); });
            else
                apply_value(to_apply);
        },
        to_apply.val_());

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
    if (const auto debug_outline = getenv("debug-outline"sv); !debug_outline.is_null()) {
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
                    rjson::copy_if_not_null(val.get("width"sv), draw_outline.outline_width);
                }
                else {
                    AD_WARNING("unrecognized debug-outline value: {}", val);
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
            fmt::print("{}{} {} [{}] edge:{:.6f} cumul:{:.6f}\n", indent, node->node_id, node->seq_id, node->date, node->edge_length.as_number(), node->cumulative_edge_length.as_number());
        else
            fmt::print("{}{} (children: {}) edge:{:.6f} cumul:{:.6f}\n", indent, node->node_id, node->subtree.size(), node->edge_length.as_number(), node->cumulative_edge_length.as_number());
    }

} // acmacs::tal::v3::Settings::report_nodes

// ----------------------------------------------------------------------

acmacs::tal::v3::NodeSet acmacs::tal::v3::Settings::select_nodes(const rjson::value& criteria) const
{
    NodeSet selected;
    bool report = false;
    rjson::for_each(criteria, [&selected, this, update = Tree::Select::init, &report](std::string_view key, const rjson::value& val) mutable {
        if (key == "all") {
            tree().select_all(selected, update);
        }
        else if (key == "aa") {
            tree().select_by_aa(selected, update, acmacs::seqdb::extract_aa_at_pos1_eq_list(substitute(val)));
        }
        else if (key == "country") {
            tree().select_by_country(selected, update, substitute(val).to<std::string_view>());
        }
        else if (key == "cumulative >=") {
            tree().select_if_cumulative_more_than(selected, update, substitute(val).to<double>());
        }
        else if (key == "date") {
            tree().select_by_date(selected, update, val[0].to<std::string_view>(), val[1].to<std::string_view>());
        }
        else if (key == "edge >=") {
            tree().select_if_edge_more_than(selected, update, substitute(val).to<double>());
        }
        else if (key == "edge >= mean_edge of") {
            tree().select_if_edge_more_than_mean_edge_of(selected, update, substitute(val).to<double>());
        }
        else if (key == "location") {
            tree().select_by_location(selected, update, substitute(val).to<std::string_view>());
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
            if (const auto match_type = substitute(val).to<std::string_view>(); match_type == "reassortant")
                mt = Tree::serum_match_t::reassortant;
            else if (match_type == "passage")
                mt = Tree::serum_match_t::passage_type;
            else if (match_type != "name")
                AD_WARNING("unrecognized \"matches-chart-serum\" value: \"{}\", \"name\" assumed", match_type);
            tree().select_matches_chart_sera(selected, update, mt);
        }
        else if (key == "seq_id") {
            tree().select_by_seq_id(selected, update, substitute(val).to<std::string_view>());
        }
        else if (key == "report") {
            report = substitute(val).to<bool>();
        }
        else if (key == "vaccine") {
            select_vaccine(selected, update, substitute(val));
        }
        else
            throw acmacs::settings::error{fmt::format("unrecognized select node criterium: {}", key)};
        if (key != "report")
            update = Tree::Select::update;
    });
    if (report)
        report_nodes(fmt::format("INFO {} selected nodes {}\n", selected.size(), criteria), "  ", selected);
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

    if (const auto& aa_at_pos = getenv("aa"sv); !aa_at_pos.is_null()) {
        AD_LOG(acmacs::log::clades, "settings \"{}\" aa: {}", clade_name, aa_at_pos);
        tree().clade_set(clade_name, acmacs::seqdb::extract_aa_at_pos1_eq_list(aa_at_pos), display_name);
    }
    else if (const auto& nuc_at_pos = getenv("nuc"sv); !nuc_at_pos.is_null()) {
        AD_LOG(acmacs::log::clades, "settings \"{}\" nuc: {}", clade_name, nuc_at_pos);
        tree().clade_set(clade_name, acmacs::seqdb::extract_nuc_at_pos1_eq_list(nuc_at_pos), display_name);
    }
    else
        throw error{"neither \"aa\" nor \"nuc\" provided"};

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
            AD_WARNING("unrecognized \"color_by\": {}, uniform(PINK) assumed", key);
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

void acmacs::tal::v3::Settings::add_gap()
{
    using namespace std::string_view_literals;

    auto& gap_element = add_element<Gap>(add_unique::yes);
    auto& param = gap_element.parameters();

    if (const auto& pixels_value = getenv("pixels"sv); !pixels_value.is_null())
        param.pixels = pixels_value.to<double>();

} // acmacs::tal::v3::Settings::add_gap

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_tree()
{
    auto& tree_element = add_element<DrawTree>();
    process_color_by(tree_element);
    add_element<HzSections>();

} // acmacs::tal::v3::Settings::add_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_time_series()
{
    using namespace std::string_view_literals;

    auto& element = add_element<TimeSeries>();
    auto& param = element.parameters();

    process_color_by(element);
    read_dash_parameters(param.dash);

    acmacs::time_series::update(rjson::object{{"start"sv, getenv("start"sv)}, {"end"sv, getenv("end"sv)}, {"interval"sv, getenv("interval"sv)}}, param.time_series);

    if (const auto& slot_val = getenv("slot"sv); !slot_val.is_null()) {
        rjson::copy_if_not_null(slot_val.get("width"sv), param.slot.width);

        if (const auto& separator = slot_val.get("separator"sv); !separator.is_null()) {
            if (const auto& width_pixels = separator.get("width_pixels"sv); !width_pixels.is_null()) {
                rjson::copy_if_not_null(width_pixels, param.slot.separator[0].line_width);
                for (auto mp = std::next(std::begin(param.slot.separator)); mp != std::end(param.slot.separator); ++mp)
                    mp->line_width = param.slot.separator[0].line_width;
            }
            if (const auto& color = separator.get("color"sv); !color.is_null()) {
                rjson::copy_if_not_null(color, param.slot.separator[0].color);
                for (auto mp = std::next(std::begin(param.slot.separator)); mp != std::end(param.slot.separator); ++mp)
                    mp->color = param.slot.separator[0].color;
            }
            rjson::for_each(separator.get("per_month"sv), [&param](const rjson::value& for_month) {
                if (const auto& month = for_month.get("month"sv); !month.is_null()) {
                    const auto month_no = month.to<size_t>() - 1;
                    rjson::copy_if_not_null(for_month.get("width_pixels"sv), param.slot.separator[month_no].line_width);
                    rjson::copy_if_not_null(for_month.get("color"sv), param.slot.separator[month_no].color);
                }
            });
        }

        if (const auto& background = slot_val.get("background"sv); !background.is_null()) {
            if (const auto& color = background.get("color"sv); !color.is_null()) {
                rjson::copy_if_not_null(color, param.slot.background[0]);
                for (auto mp = std::next(std::begin(param.slot.background)); mp != std::end(param.slot.background); ++mp)
                    *mp = param.slot.background[0];
            }
            rjson::for_each(background.get("per_month"sv), [&param](const rjson::value& for_month) {
                if (const auto& month = for_month.get("month"sv); !month.is_null()) {
                    const auto month_no = month.to<size_t>() - 1;
                    rjson::copy_if_not_null(for_month.get("color"sv), param.slot.background[month_no]);
                }
            });
        }

        rjson::copy_if_not_null(slot_val.get("label"sv, "color"sv), param.slot.label.color);
        rjson::copy_if_not_null(slot_val.get("label"sv, "scale"sv), param.slot.label.scale);
        rjson::copy_if_not_null(slot_val.get("label"sv, "offset"sv), param.slot.label.offset);
        rjson::call_if_not_null<std::string_view>(slot_val.get("label"sv, "rotation"sv), [&param](auto rot) {
            if (rot == "clockwise")
                param.slot.label.rotation = Rotation90DegreesClockwise;
            else if (rot == "anticlockwise" || rot == "counterclockwise")
                param.slot.label.rotation = Rotation90DegreesAnticlockwise;
            else
                AD_WARNING("unrecognzied label rotation value in the time series parameters: \"{}\"", rot);
        });
    }

    if (const auto& color_scale_val = getenv("color-scale"sv); !color_scale_val.is_null()) {
        rjson::copy_if_not_null(color_scale_val.get("show"sv), param.color_scale.show);
        if (const auto& type_val = color_scale_val.get("type"sv); !type_val.is_null()) {
            if (const auto type = type_val.to<std::string_view>(); type == "bezier_gradient" || type == "bezier-gradient")
                param.color_scale.type = TimeSeries::color_scale_type::bezier_gradient;
            else
                AD_WARNING("unrecognized time-series color-scale type (\"bezier-gradient\" expected): {}", type);
        }
        switch (param.color_scale.type) {
            case TimeSeries::color_scale_type::bezier_gradient:
                if (const auto& colors_val = color_scale_val.get("colors"sv); !colors_val.is_null()) {
                    if (colors_val.is_array() && colors_val.size() == 3)
                        rjson::transform(colors_val, std::begin(param.color_scale.colors), [](const rjson::value& val) { return Color{val.to<std::string_view>()}; });
                    else
                        AD_WARNING("invalid number of colors in time-series color-scale colors (3 strings expected): {}", colors_val);
                }
                break;
        }
        rjson::copy_if_not_null(color_scale_val.get("offset"sv), param.color_scale.offset);
        rjson::copy_if_not_null(color_scale_val.get("height"sv), param.color_scale.height);
    }

} // acmacs::tal::v3::Settings::add_time_series

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_dash_parameters(LayoutElement::DashParameters& param)
{
    using namespace std::string_view_literals;

    if (const auto& dash_val = getenv("dash"sv); !dash_val.is_null()) {
        rjson::copy_if_not_null(dash_val.get("width"sv), param.width);
        rjson::copy_if_not_null(dash_val.get("line_width_pixels"sv), param.line_width);
    }

} // acmacs::tal::v3::Settings::read_dash_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_clade_parameters(const rjson::value& source, Clades::CladeParameters& clade_parameters)
{
    using namespace std::string_view_literals;
    rjson::copy_if_not_null(source.get("display_name"sv), clade_parameters.display_name);
    if (const auto& shown = source.get("shown"sv); !shown.is_null())
        clade_parameters.hidden = !shown.template to<bool>();
    else if (const auto& show = source.get("show"sv); !show.is_null())
        clade_parameters.hidden = !show.template to<bool>();
    else
        rjson::copy_if_not_null(source.get("hidden"sv), clade_parameters.hidden);
    rjson::copy_if_not_null(source.get("section_inclusion_tolerance"sv), clade_parameters.section_inclusion_tolerance);
    rjson::copy_if_not_null(source.get("section_exclusion_tolerance"sv), clade_parameters.section_exclusion_tolerance);
    rjson::copy_if_not_null(source.get("slot"sv), clade_parameters.slot_no);

    read_label_parameters(source.get("label"sv), clade_parameters.label);

    rjson::copy_if_not_null(source.get("arrow"sv, "color"sv), clade_parameters.arrow.color);
    rjson::copy_if_not_null(source.get("arrow"sv, "line_width"sv), clade_parameters.arrow.line_width);
    rjson::copy_if_not_null(source.get("arrow"sv, "arrow_width"sv), clade_parameters.arrow.arrow_width);

    read_line_parameters(source.get("horizontal_line"sv), clade_parameters.horizontal_line);

    rjson::copy_if_not_null(source.get("top_gap"sv), clade_parameters.tree_top_gap);
    rjson::copy_if_not_null(source.get("bottom_gap"sv), clade_parameters.tree_bottom_gap);
    rjson::copy_if_not_null(source.get("time_series_top_separator"sv), clade_parameters.time_series_top_separator);
    rjson::copy_if_not_null(source.get("time_series_bottom_separator"sv), clade_parameters.time_series_bottom_separator);

} // acmacs::tal::v3::Settings::read_clade_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_per_clade(Clades::Parameters& parameters)
{
    using namespace std::string_view_literals;
    rjson::for_each(getenv("per_clade"sv), [this,&parameters](const rjson::value& for_clade) {
        if (const auto& name = for_clade.get("name"sv); !name.is_null())
            read_clade_parameters(for_clade, parameters.find_or_add_pre_clade(name.to<std::string_view>()));
    });

} // acmacs::tal::v3::Settings::read_per_clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_clades()
{
    AD_LOG(acmacs::log::settings, "clades");

    using namespace std::string_view_literals;

    auto& element = add_element<Clades>();
    auto& param = element.parameters();

    getenv_copy_if_present("report"sv, param.report);

    if (const auto& slot_val = substitute(getenv("slot"sv)); !slot_val.is_null()) {
        rjson::copy_if_not_null(substitute(slot_val.get("width"sv)), param.slot.width);
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
    read_line_parameters(substitute(getenv("line"sv)), param.line);
    rjson::copy_if_not_null(getenv("top_gap"sv), param.tree_top_gap);
    rjson::copy_if_not_null(getenv("bottom_gap"sv), param.tree_bottom_gap);

    rjson::for_each(getenv("sections"sv), [&param](const rjson::value& for_section) {
        if (const auto& id = for_section.get("id"sv); !id.is_null()) {
            auto& section = param.find_add_section(id.to<std::string_view>());
            section.shown = rjson::get_or(for_section, "show"sv, true);
            rjson::copy_if_not_null(for_section.get("first"sv), section.first);
            rjson::copy_if_not_null(for_section.get("last"sv), section.last);
            rjson::copy_if_not_null(for_section.get("label"sv), section.label);
        }
        else if (for_section.get("?id"sv).is_null() && for_section.get("? id"sv).is_null())
            AD_WARNING("settings hz-section without \"id\" ignored: {}", for_section);
    });

} // acmacs::tal::v3::Settings::hz_sections

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::hz_section_marker()
{
    using namespace std::string_view_literals;

    auto& element = add_element<HzSectionMarker>();
    auto& param = element.parameters();

    read_line_parameters(substitute(getenv("line"sv)), param.line);

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

void acmacs::tal::v3::Settings::read_label_parameters(const rjson::value& source, LayoutElement::LabelParameters& param)
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::copy_if_not_null(substitute(source.get("color"sv)), param.color);
        rjson::copy_if_not_null(substitute(source.get("scale"sv)), param.scale);
        rjson::call_if_not_null<std::string_view>(substitute(source.get("vertical_position"sv)), [&param](auto position) {
            if (position == "middle"sv)
                param.vpos = Clades::vertical_position::middle;
            else if (position == "top"sv)
                param.vpos = Clades::vertical_position::top;
            else if (position == "bottom"sv)
                param.vpos = Clades::vertical_position::bottom;
            else
                AD_WARNING("unrecognized clade label position: \"{}\"", position);
        });
        rjson::call_if_not_null<std::string_view>(substitute(source.get("horizontal_position"sv)), [&param](auto position) {
            if (position == "middle"sv)
                param.hpos = Clades::horizontal_position::middle;
            else if (position == "left"sv)
                param.hpos = Clades::horizontal_position::left;
            else if (position == "right"sv)
                param.hpos = Clades::horizontal_position::right;
            else
                AD_WARNING("unrecognized clade label position: \"{}\"", position);
        });
        rjson::copy_if_not_null(source.get("offset"sv), param.offset);
        rjson::copy_if_not_null(substitute(source.get("text"sv)), param.text);
        rjson::call_if_not_null<double>(substitute(source.get("rotation_degrees"sv)), [&param](auto rotation_degrees) { param.rotation = RotationDegrees(rotation_degrees); });

        rjson::copy_if_not_null(source.get("tether"sv, "show"sv), param.tether.show);
        rjson::copy_if_not_null(source.get("tether"sv, "color"sv), param.tether.line.color);
        rjson::copy_if_not_null(source.get("tether"sv, "line_width"sv), param.tether.line.line_width);

        rjson::copy_if_not_null(source.get("text_style"sv, "font"sv), param.text_style.font_family);
        rjson::copy_if_not_null(source.get("text_style"sv, "slant"sv), param.text_style.slant);
        rjson::copy_if_not_null(source.get("text_style"sv, "weight"sv), param.text_style.weight);
    }

} // acmacs::tal::v3::Settings::read_label_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_dash_bar()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DashBar>();
    auto& param = element.parameters();

    read_dash_parameters(param.dash);

    rjson::for_each(getenv("nodes"sv), [this, &param](const rjson::value& entry) {
        auto& for_nodes = param.for_nodes.emplace_back();
        for_nodes.nodes = select_nodes(entry.get("select"sv));
        rjson::copy_if_not_null(substitute(entry.get("color"sv)), for_nodes.color);
    });

    rjson::for_each(getenv("labels"sv), [this, &param](const rjson::value& label_data) {
        read_label_parameters(label_data, param.labels.emplace_back());
    });

} // acmacs::tal::v3::Settings::add_dash_bar

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_dash_bar_clades()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DashBarClades>();
    auto& param = element.parameters();

    read_dash_parameters(param.dash);

    rjson::for_each(getenv("clades"sv), [this, &param](const rjson::value& for_clade) {
        auto& clade = param.clades.emplace_back();
        rjson::copy_if_not_null(for_clade.get("name"sv), clade.name);
        rjson::copy_if_not_null(for_clade.get("color"sv), clade.color);
        read_label_parameters(for_clade.get("label"sv), clade.label);
    });

} // acmacs::tal::v3::Settings::add_dash_bar_clades

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_title()
{
    using namespace std::string_view_literals;

    auto& element = add_element<Title>();
    auto& param = element.parameters();

    const rjson::object source{
        {"text", getenv("text"sv)},
        {"offset", getenv("offset"sv)},
        {"color", getenv("color"sv)},
        {"size", getenv("size"sv)}
    };
    read_text_parameters(source, param);

} // acmacs::tal::v3::Settings::add_title

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_text_parameters(const rjson::value& source, LayoutElement::TextParameters& text_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::copy_if_not_null(source.get("text"sv), text_parameters.text);
        rjson::copy(source.get("offset"sv), text_parameters.offset);
        rjson::copy_if_not_null(source.get("absolute_x"sv), text_parameters.absolute_x);
        rjson::copy_if_not_null(source.get("color"sv), text_parameters.color);
        rjson::copy_if_not_null(source.get("size"sv), text_parameters.size);
    }

} // acmacs::tal::v3::Settings::read_text_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_line_parameters(const rjson::value& source, LayoutElement::LineParameters& line_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        rjson::copy_if_not_null(source.get("color"sv), line_parameters.color);
        rjson::copy_if_not_null(source.get("line_width"sv), line_parameters.line_width);

        if (const auto& dash_val = source.get("dash"sv); !dash_val.is_null()) {
            const auto dash = dash_val.to<std::string_view>();
            if (dash.empty() || dash == "no" || dash == "no-dash" || dash == "no_dash")
                line_parameters.dash = surface::Dash::NoDash;
            else if (dash == "dash1")
                line_parameters.dash = surface::Dash::Dash1;
            else if (dash == "dash2")
                line_parameters.dash = surface::Dash::Dash2;
            else if (dash == "dash3")
                line_parameters.dash = surface::Dash::Dash3;
        }
    }

} // acmacs::tal::v3::Settings::read_line_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_line_parameters(const rjson::value& source, LayoutElement::LineWithOffsetParameters& line_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        read_line_parameters(source, static_cast<LayoutElement::LineParameters&>(line_parameters));
        rjson::copy(source.get("c1"sv), line_parameters.offset[0]);
        rjson::copy(source.get("c2"sv), line_parameters.offset[1]);
        rjson::copy_if_not_null(source.get("absolute_x"sv), line_parameters.absolute_x);
    }

} // acmacs::tal::v3::Settings::read_line_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::read_dot_parameters(const rjson::value& source, LayoutElement::WorldMapDotParameters& dot_parameters) const
{
    using namespace std::string_view_literals;

    if (!source.is_null()) {
        if (const auto& loc = source.get("location"sv); !loc.is_null()) {
            try {
                const auto found = acmacs::locationdb::get().find_or_throw(loc.to<std::string_view>());
                dot_parameters.coordinates = PointCoordinates{found.latitude(), found.longitude()};
            }
            catch (std::exception&) {
                AD_WARNING("\"location\" for world map dot not found: {}", source);
            }
        }
        rjson::copy(source.get("coordinates"sv), dot_parameters.coordinates);
        rjson::copy_if_not_null(source.get("outline"sv), dot_parameters.outline);
        rjson::copy_if_not_null(source.get("fill"sv), dot_parameters.fill);
        rjson::copy_if_not_null(source.get("outline_width"sv), dot_parameters.outline_width);
        rjson::copy_if_not_null(source.get("size"sv), dot_parameters.size);
    }

} // acmacs::tal::v3::Settings::read_dot_parameters

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_legend()
{
    using namespace std::string_view_literals;

    std::string legend_type{"world-map"};
    getenv_copy_if_present("type"sv, legend_type);

    if (legend_type == "world-map") {
        auto& element = add_element<LegendContinentMap>();
        auto& param = element.parameters();
        rjson::copy(getenv("offset"sv), param.offset);
        getenv_extract_copy_if_present<double>("size"sv, param.size);
        read_line_parameters(getenv("equator"sv), param.equator);
        read_line_parameters(getenv("tropics"sv), param.tropics);
        rjson::for_each(getenv("dots"sv), [&param, this](const rjson::value& for_dot) { read_dot_parameters(for_dot, param.dots.emplace_back()); });
    }
    else
        AD_WARNING("unrecognized legend type: \"{}\"", legend_type);

} // acmacs::tal::v3::Settings::add_legend

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_draw_aa_transitions()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DrawAATransitions>();
    auto& param = element.parameters();

    getenv_copy_if_present("minimum_number_leaves_in_subtree"sv, param.minimum_number_leaves_in_subtree);
    getenv_copy_if_present("text_line_interleave"sv, param.text_line_interleave);

    // ----------------------------------------------------------------------

    enum class ignore_name { no, yes };
    const auto read_node_parameters = [this](const rjson::value& source, DrawAATransitions::TransitionParameters& parameters, ignore_name ign) {
        if (ign == ignore_name::no)
            rjson::copy_if_not_null(source.get("node_id"sv), parameters.node_id);
        read_label_parameters(source.get("label"sv), parameters.label);
    };

    if (const auto& all_nodes_val = getenv("all_nodes"sv); !all_nodes_val.is_null())
        read_node_parameters(all_nodes_val, param.all_nodes, ignore_name::yes);
    rjson::for_each(getenv("per_node"sv), [&param,read_node_parameters](const rjson::value& for_node) {
        param.per_node.push_back(param.all_nodes);
        read_node_parameters(for_node, param.per_node.back(), ignore_name::no);
    });

} // acmacs::tal::v3::Settings::add_draw_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::add_draw_on_tree()
{
    using namespace std::string_view_literals;

    auto& element = add_element<DrawOnTree>();
    auto& param = element.parameters();

    rjson::for_each(getenv("texts"sv), [&param,this](const rjson::value& text_entry) {
        auto& text_param = param.texts.emplace_back();
        read_text_parameters(text_entry, text_param);
    });

} // acmacs::tal::v3::Settings::add_draw_on_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Settings::select_vaccine(NodeSet& nodes, Tree::Select update, const rjson::value& criteria) const
{
    const auto names = ranges::to<std::vector<std::string>>(
        acmacs::whocc::vaccine_names(acmacs::virus::type_subtype_t{getenv("virus-type", "")}, acmacs::virus::lineage_t{getenv("lineage", "")})
        | ranges::views::filter([vaccine_type=acmacs::whocc::Vaccine::type_from_string(rjson::get_or(criteria, "type", "any"))](const auto& en) { return vaccine_type == acmacs::whocc::vaccine_type::any || en.type == vaccine_type; })
        | ranges::views::transform([](const auto& en) { return en.name; }));

    AD_LOG(acmacs::log::vaccines, "{}", names);
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
    // report_nodes(fmt::format("INFO select_vaccine: {} selected nodes {}\n", nodes.size(), criteria), "  ", nodes);

} // acmacs::tal::v3::Settings::select_vaccine

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
