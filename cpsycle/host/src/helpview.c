/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "helpview.h"
/* host */
#include "resources/resource.h"
#include "styles.h"


/* prototypes */
static void helpview_init_base(HelpView*, psy_ui_Component* parent);
static void helpview_init_tabbar(HelpView*, psy_ui_Component* tabbarparent,
	Workspace*);
static void helpview_init_sections(HelpView*, Workspace*);
static void helpview_on_tabbar_changed(HelpView*,
	psy_ui_TabBar* sender, uintptr_t index);
static void helpview_on_select_section(HelpView*, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options);
static uintptr_t helpview_section(const HelpView*);
static void helpview_on_focus(HelpView*, psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable helpview_vtable;
static bool helpview_vtable_initialized = FALSE;

static void helpview_vtable_init(HelpView* self)
{
	assert(self);

	if (!helpview_vtable_initialized) {
		helpview_vtable = *(self->component.vtable);		
		helpview_vtable.section =
			(psy_ui_fp_component_section)
			helpview_section;
		helpview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(helpview_base(self), &helpview_vtable);
}

/* implementation */
void helpview_init(HelpView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace* workspace)
{	
	assert(self);
	assert(workspace);

	self->workspace = workspace;
	helpview_init_base(self, parent);	
	helpview_init_tabbar(self, tabbarparent, workspace);
	helpview_init_sections(self, workspace);	
	psy_ui_tabbar_select(&self->tabbar, HELPVIEWSECTION_ABOUT);
}

void helpview_init_base(HelpView* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(helpview_base(self), parent, NULL);
	helpview_vtable_init(self);
	psy_ui_component_set_style_type(helpview_base(self), STYLE_SIDE_VIEW);
	psy_ui_component_set_id(helpview_base(self), VIEW_ID_HELPVIEW);
	psy_ui_component_set_title(helpview_base(self), "main.help");
	psy_signal_connect(&helpview_base(self)->signal_focus, self,
		helpview_on_focus);
	psy_signal_connect(&helpview_base(self)->signal_select_section, self,
		helpview_on_select_section);
}

void helpview_init_tabbar(HelpView* self, psy_ui_Component* tabbarparent,
	Workspace* workspace)
{	
	psy_ui_component_init_align(&self->bar, tabbarparent, NULL,
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_id(&self->bar, VIEW_ID_HELPVIEW);
	psy_ui_tabbar_init(&self->tabbar, &self->bar);
	psy_ui_component_set_align_expand(&self->bar, psy_ui_HEXPAND);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar),
		psy_ui_ALIGN_LEFT);
	psy_ui_tabbar_append_tabs(&self->tabbar, "help.help", "help.about",
		"help.greetings", NULL);
	psy_signal_connect(&self->tabbar.signal_change, self,
		helpview_on_tabbar_changed);
}

void helpview_init_sections(HelpView* self, Workspace* workspace)
{
	assert(self);

	psy_ui_notebook_init(&self->notebook, helpview_base(self));
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);	
	help_init(&self->help, psy_ui_notebook_base(&self->notebook),
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),
			"doc", PSYCLE_DOC_DEFAULT_DIR));		
	about_init(&self->about, psy_ui_notebook_base(&self->notebook), workspace);	
	greet_init(&self->greet, psy_ui_notebook_base(&self->notebook));	
}

void helpview_on_tabbar_changed(HelpView* self,
	psy_ui_TabBar* sender, uintptr_t index)
{
	assert(self);

	if (index != psy_INDEX_INVALID) {
		workspace_select_view(self->workspace, viewindex_make_section(
			VIEW_ID_HELPVIEW, index));
	}
}

void helpview_on_select_section(HelpView* self, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options)
{
	psy_ui_Component* view;

	assert(self);

	if (section != psy_INDEX_INVALID) {
		psy_ui_tabbar_mark(&self->tabbar, section);
		psy_ui_notebook_select(&self->notebook, section);
		view = psy_ui_notebook_active_page(&self->notebook);
		if (view) {
			psy_ui_component_set_focus(view);
		}
	}
}

uintptr_t helpview_section(const HelpView* self)
{
	assert(self);

	return psy_ui_notebook_page_index(&self->notebook);
}


void helpview_on_focus(HelpView* self, psy_ui_Component* sender)
{
	psy_ui_Component* view;

	assert(self);

	view = psy_ui_notebook_active_page(&self->notebook);
	if (view) {
		psy_ui_component_set_focus(view);
	}
}
