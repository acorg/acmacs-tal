#include "acmacs-tal/hz-sections.hh"
#include "acmacs-tal/tal-data.hh"
#include "acmacs-tal/tree-iterate.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::prepare(preparation_stage_t stage)
{
    if (stage == 2 && prepared_ < stage) {
        width_to_height_ratio() = 0.0;
        set_aa_transitions();
        // fmt::print(stderr, "DEBUG: hz-sections prepare width_to_height_ratio: {}\n", width_to_height_ratio());
        // auto& layout = tal().draw().layout();
    }
    LayoutElement::prepare(stage);

} // acmacs::tal::v3::HzSections::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::set_aa_transitions()
{
    auto pre = [this](const Node& node) {
        if (!node.aa_transitions_.empty()) {
            for (auto& section : sections_) {
                if (section.first->node_id.vertical >= node.first_leaf->node_id.vertical && section.last->node_id.vertical <= node.last_next_leaf->node_id.vertical)
                    section.aa_transitions.add_or_replace(node.aa_transitions_);
            }
        }
    };
    tree::iterate_pre(tal().tree(), pre);

} // acmacs::tal::v3::HzSections::set_aa_transitions

// ----------------------------------------------------------------------

void acmacs::tal::v3::HzSections::draw(acmacs::surface::Surface& /*surface*/) const
{
    // fmt::print(stderr, "DEBUG: HzSections width_to_height_ratio: {}\n", width_to_height_ratio());

    for (const auto& section : sections_) {
        fmt::print(stderr, "DEBUG: hz-section \"{}\" {} {} {} \"{}\"\n", section.id, section.first ? section.first->seq_id : seq_id_t{}, section.last ? section.last->seq_id : seq_id_t{}, section.aa_transitions, section.label);
    }

} // acmacs::tal::v3::HzSections::draw

// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
