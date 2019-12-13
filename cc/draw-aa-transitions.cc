#include "acmacs-tal/draw-aa-transitions.hh"

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::prepare()
{

} // acmacs::tal::v3::Legend::prepare

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::draw(acmacs::surface::Surface& /*surface*/) const
{
    // do nothing
    // draw_transitions() called by DrawTree::draw is used for drawing transitions

} // acmacs::tal::v3::LegendContinentMap::draw

// ----------------------------------------------------------------------

void acmacs::tal::v3::DrawAATransitions::draw_transitions(acmacs::surface::Surface& surface, const DrawTree& draw_tree) const
{
    fmt::print(stderr, "DEBUG: DrawAATransitions::draw_transitions\n");

} // acmacs::tal::v3::DrawAATransitions::draw_transitions

// ----------------------------------------------------------------------
/// Local Variables:
/// eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
/// End:
