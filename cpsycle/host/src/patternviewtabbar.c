/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewtabbar.h"
/* host */
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes*/
static void patternviewtabbar_on_tabbar_change(PatternViewTabBar*,
	psy_ui_Component* sender, uintptr_t tab_index);

/* implementation */
void patternviewtabbar_init(PatternViewTabBar* self, psy_ui_Component* parent,
	psy_Configuration* patconfig, Workspace* workspace)
{
	psy_ui_Tab* tab;

	assert(self);

	psy_ui_component_init_align(&self->component, parent, NULL,
		psy_ui_ALIGN_LEFT);	
	self->workspace = workspace;
	psy_ui_component_set_id(&self->component, VIEW_ID_PATTERNS);
	self->patconfig = patconfig;	
	psy_ui_tabbar_init(&self->tabbar, &self->component);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar),
		psy_ui_ALIGN_LEFT);
	psy_ui_tabbar_append_tabs(&self->tabbar, "pv.tracker", "pv.roll",
		"pv.split", "pv.vert", "pv.horz", NULL);
	tab = psy_ui_tabbar_tab(&self->tabbar, 2);
	if (tab) {
		psy_ui_tab_set_mode(tab, psy_ui_TABMODE_LABEL);
	}
	tab = psy_ui_tabbar_tab(&self->tabbar, 0);
	if (tab) {
		psy_ui_Margin margin;

		margin = psy_ui_component_margin(&tab->component);
		margin.left = psy_ui_value_make_ew(1.0);
		psy_ui_component_set_margin(&tab->component, margin);
	}	
	psy_ui_tabbar_select(&self->tabbar, 0);
	psy_ui_button_init(&self->context_button, &self->component);
	psy_ui_button_set_svg(&self->context_button, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(
		&self->context_button));
	psy_ui_component_set_align(psy_ui_button_base(&self->context_button),
		psy_ui_ALIGN_RIGHT);
	psy_signal_connect(&self->tabbar.signal_change, self,
		patternviewtabbar_on_tabbar_change);
}

void patternviewtabbar_on_tabbar_change(PatternViewTabBar* self,
	psy_ui_Component* sender, uintptr_t tab_index)
{
	ViewIndex curr;
	
	assert(self);

	curr = workspace_current_view(self->workspace);
	if (tab_index <= PATTERN_DISPLAYMODE_NUM) {
		const static PatternDisplayMode display[] = {
			PATTERN_DISPLAYMODE_TRACKER, PATTERN_DISPLAYMODE_PIANOROLL,
			PATTERN_DISPLAYMODE_NUM,
			PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_VERTICAL,
			PATTERN_DISPLAYMODE_TRACKER_PIANOROLL_HORIZONTAL };
		psy_Property* p;		

		p = psy_configuration_at(self->patconfig, "patterndisplay");
		if (p) {
			psy_property_set_item_int(p, display[tab_index]);
		}		
	}	
	if (curr.id != VIEW_ID_PATTERNS || curr.section != tab_index) {	
		workspace_add_view(self->workspace, viewindex_make_section(
			VIEW_ID_PATTERNS, tab_index));
	}
}
