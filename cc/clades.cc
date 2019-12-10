#include "acmacs-tal/clades.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/draw-tree.hh"
#include "acmacs-tal/time-series.hh"

// ----------------------------------------------------------------------

bool acmacs::tal::v3::Clades::clade_t::intersects(const clade_t& rhs) const
{
    return true;

} // acmacs::tal::v3::Clades::clade_t::intersects

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::prepare()
{
    if (!prepared_) {
        auto& layout = tal_.draw().layout();
        time_series_to_the_left_ = layout.index_of(layout.find<TimeSeries>()) < layout.index_of(this);

        make_clades();
        if (width_to_height_ratio() <= 0.0)
            width_to_height_ratio() = (number_of_slots() + 1) * parameters_.slot.width;
    }
    LayoutElement::prepare();

} // acmacs::tal::v3::Clades::prepare

// ----------------------------------------------------------------------

const acmacs::tal::v3::Clades::CladeParameters& acmacs::tal::v3::Clades::parameters_for_clade(std::string_view name) const
{
    if (auto found = std::find_if(std::begin(parameters_.per_clade), std::end(parameters_.per_clade), [name](const auto& for_clade) { return for_clade.name == name; }); found != std::end(parameters_.per_clade))
        return *found;
    else
        return parameters_.all_clades;

} // acmacs::tal::v3::Clades::parameters_for_clade

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::make_clades()
{
    make_sections();
    set_slots();
    add_gaps_to_the_tree();
    report_clades();

} // acmacs::tal::v3::Clades::make_clades

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::make_sections()
{
    const auto& tree_clades = tal_.tree().clades();
    for (const auto& tree_clade : tree_clades) {
        const auto& clade_param = parameters_for_clade(tree_clade.name);
        if (!clade_param.hidden) {
            auto& clade = clades_.emplace_back(tree_clade.name);
            for (const auto& tree_section : tree_clade.sections) {
                auto& section = clade.sections.emplace_back(tree_section.first, tree_section.last, tree_clade.display_name);
                section.label = clade_param.label;
                section.arrow = clade_param.arrow;
                section.horizontal_line = clade_param.horizontal_line;
                if (!clade_param.display_name.empty())
                    section.display_name = clade_param.display_name;
            }

            // merge sections
            size_t merge_to = 0;
            for (size_t sec_no = 0; sec_no < (clade.sections.size() - 1); ++sec_no) {
                if ((clade.sections[sec_no + 1].first->node_id_.vertical - clade.sections[sec_no].last->node_id_.vertical) <= clade_param.section_inclusion_tolerance) {
                    clade.sections[merge_to].last = clade.sections[sec_no + 1].last;
                    clade.sections[sec_no + 1].first = nullptr;
                }
                else
                    merge_to = sec_no + 1;
            }
            clade.sections.erase(std::remove_if(std::begin(clade.sections), std::end(clade.sections), [](const auto& sec) { return sec.first == nullptr; }), std::end(clade.sections));

            // remove small sections
            const auto is_section_small = [tol = clade_param.section_exclusion_tolerance](const auto& sec) { return sec.size() <= tol; };
            if (const size_t num_small_sections = static_cast<size_t>(std::count_if(std::begin(clade.sections), std::end(clade.sections), is_section_small)); num_small_sections < clade.sections.size())
                clade.sections.erase(std::remove_if(std::begin(clade.sections), std::end(clade.sections), is_section_small), std::end(clade.sections));
        }
    }

} // acmacs::tal::v3::Clades::make_sections

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::set_slots()
{
    std::vector<clade_t*> clade_refs(clades_.size());
    std::transform(std::begin(clades_), std::end(clades_), std::begin(clade_refs), [](auto& clad) { return &clad; });
    // smallest clade first (by its longest section)
    std::sort(std::begin(clade_refs), std::end(clade_refs), [](const clade_t* c1, const clade_t* c2) {
        const auto cmp = [](const auto& s1, const auto& s2) { return s1.size() < s2.size(); };
        return std::max_element(std::begin(c1->sections), std::end(c1->sections), cmp)->size() < std::max_element(std::begin(c2->sections), std::end(c2->sections), cmp)->size();
    });
    size_t slot_no{0};
    for (size_t clade_no = 0; clade_no < clade_refs.size(); ++clade_no) {
        if (clade_no > 0 && clade_refs[clade_no]->intersects(*clade_refs[clade_no - 1]))
            ++slot_no;
        for (auto& section : clade_refs[clade_no]->sections)
            section.slot_no = slot_no_t{slot_no};
    }

} // acmacs::tal::v3::Clades::set_slots

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::add_gaps_to_the_tree()
{
    for (const auto& clade : clades_) {
        const auto& clade_param = parameters_for_clade(clade.name);
        for (const auto& section : clade.sections) {
            if (clade_param.tree_top_gap > 0.0 && section.first->vertical_offset_ <= Node::default_vertical_offset)
                section.first->vertical_offset_ += clade_param.tree_top_gap;
            if (clade_param.tree_bottom_gap > 0.0) {
                if (const auto next_leaf = tal_.tree().next_leaf(section.last); next_leaf && next_leaf->vertical_offset_ <= Node::default_vertical_offset) {
                    next_leaf->vertical_offset_ += clade_param.tree_bottom_gap;
                }
            }
        }
    }

} // acmacs::tal::v3::Clades::add_gaps_to_the_tree

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::report_clades()
{
    if (parameters_.report) {
        for (const auto& clade : clades_) {
            fmt::print("Clade {} ({})\n", clade.name, clade.sections.size());
            for (size_t section_no = 0; section_no < clade.sections.size(); ++section_no) {
                const auto& section = clade.sections[section_no];
                fmt::print("  {} [{}] slot:{} {} {} .. {} {}\n", section.display_name, section.size(), section.slot_no, section.first->node_id_, section.first->seq_id, section.last->node_id_,
                           section.last->seq_id);
                if (section_no < (clade.sections.size() - 1))
                    fmt::print("   gap {}\n", clade.sections[section_no + 1].first->node_id_.vertical - section.last->node_id_.vertical - 1);
            }
        }
    }

} // acmacs::tal::v3::Clades::report_clades

// ----------------------------------------------------------------------

size_t acmacs::tal::v3::Clades::number_of_slots() const
{
    slot_no_t max_slot_no{0};
    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections)
            max_slot_no = std::max(max_slot_no, section.slot_no);
    }
    return *max_slot_no + 1;

} // acmacs::tal::v3::Clades::number_of_slots

// ----------------------------------------------------------------------

void acmacs::tal::v3::Clades::draw(acmacs::surface::Surface& surface) const
{
    const auto* draw_tree = tal_.draw().layout().find<DrawTree>();
    if (!draw_tree) {
        fmt::print(stderr, "WARNING: No tree section in layout, cannot draw clades\n");
        return;
    }
    const auto vertical_step = draw_tree->vertical_step();
    const auto& viewport = surface.viewport();

    const TextStyle text_style{};

    for (const auto& clade : clades_) {
        for (const auto& section : clade.sections) {
            const auto pos_x = time_series_to_the_left_ ?
                    (viewport.left() + parameters_.slot.width * (*section.slot_no + 1)) :
                    (viewport.right() - parameters_.slot.width * (*section.slot_no + 1));
            // arrow
            surface.double_arrow({pos_x, vertical_step * section.first->cumulative_vertical_offset_},
                                 {pos_x, vertical_step * section.last->cumulative_vertical_offset_},
                                 section.arrow.color, section.arrow.line_width, section.arrow.arrow_width);

            // label
            const Scaled label_size{parameters_.slot.width * section.label.scale};
            const auto text_size = surface.text_size(section.display_name, label_size, text_style);
            double vertical_pos;
            switch (section.label.position) {
                case vertical_position::top:
                    vertical_pos = vertical_step * section.first->cumulative_vertical_offset_ + text_size.height;
                    break;
                case vertical_position::middle:
                    vertical_pos = vertical_step * (section.first->cumulative_vertical_offset_ + section.last->cumulative_vertical_offset_) / 2.0 + text_size.height / 2.0;
                    break;
                case vertical_position::bottom:
                    vertical_pos = vertical_step * section.last->cumulative_vertical_offset_;
                    break;
            }
            const auto text_pos_x = time_series_to_the_left_ ? (pos_x + section.label.horizontal_offset) : (pos_x - text_size.width - section.label.horizontal_offset);
            surface.text({text_pos_x, vertical_pos}, section.display_name, section.label.color, label_size, text_style, section.label.rotation);

            // horizontal lines
            const auto left = time_series_to_the_left_ ? viewport.left() : pos_x;
            const auto right = time_series_to_the_left_ ? pos_x : viewport.right();
            surface.line({left, vertical_step * section.first->cumulative_vertical_offset_},
                         {right, vertical_step * section.first->cumulative_vertical_offset_},
                         section.horizontal_line.color, section.horizontal_line.line_width);
            surface.line({left, vertical_step * section.last->cumulative_vertical_offset_},
                         {right, vertical_step * section.last->cumulative_vertical_offset_},
                         section.horizontal_line.color, section.horizontal_line.line_width);
        }
    }

} // acmacs::tal::v3::Clades::draw

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
