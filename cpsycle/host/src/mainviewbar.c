/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "mainviewbar.h"
/* host */
#include "styles.h"
/* ui */
#include <trackercmds.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void mainviewbar_on_destroyed(MainViewBar*);
static void mainviewbar_init_layout(MainViewBar*);
static void mainviewbar_init_view_buttons(MainViewBar*);
static void mainviewbar_init_navigation(MainViewBar*, Workspace*);
static void mainviewbar_init_main_tabbar(MainViewBar*);
static void mainviewbar_init_view_tabbars(MainViewBar*);
static void mainviewbar_init_script_tabbar(MainViewBar*);
static void mainviewbar_on_maxminimize_view(MainViewBar*,
	psy_ui_Button* sender);
static void mainviewbar_on_toggle_scripts(MainViewBar*,
	psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MainViewBar* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			mainviewbar_on_destroyed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(mainviewbar_base(self), &vtable);
}

/* implementation */
void mainviewbar_init(MainViewBar* self, psy_ui_Component* parent,
	psy_ui_Component* pane, Workspace* workspace)
{
	assert(self);
	assert(pane);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	mainviewbar_init_layout(self);
	mainviewbar_init_navigation(self, workspace);	
	mainviewbar_init_main_tabbar(self);	
	mainviewbar_init_view_tabbars(self);
	mainviewbar_init_script_tabbar(self);
	mainviewbar_init_view_buttons(self);
	minmaximize_init(&self->min_maximize_, pane);
}

void mainviewbar_on_destroyed(MainViewBar* self)
{
	assert(self);

	minmaximize_dispose(&self->min_maximize_);
}

void mainviewbar_init_layout(MainViewBar* self)
{
	assert(self);

	psy_ui_component_set_margin(&self->component,
		psy_ui_margin_make_em(0.0, 0.0, 0.4, 0.0));
	psy_ui_component_init_align(&self->row_0_, &self->component,
		NULL, psy_ui_ALIGN_TOP);
	psy_ui_component_init_align(&self->row_1_, &self->component,
		NULL, psy_ui_ALIGN_TOP);
	psy_ui_component_init_align(&self->tab_bars_, &self->row_0_, NULL,
		psy_ui_ALIGN_LEFT);
}

void mainviewbar_init_view_buttons(MainViewBar* self)
{
	assert(self);

	psy_ui_component_init_align(&self->view_buttons_, &self->row_0_, NULL,
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_init_align(&self->view_buttons_client_, &self->view_buttons_, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->view_buttons_client_, psy_ui_ALIGN_LEFT,
		psy_ui_margin_zero());
	psy_ui_button_init_resource(&self->extract_left_, &self->view_buttons_client_,
		"img.icon-less");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->extract_left_), 0);
	psy_ui_button_init_resource(&self->extract_right_, & self->view_buttons_client_,
		"img.icon-more");	
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->extract_right_), 0);
	psy_ui_button_init_resource(&self->extract_bottom_, &self->view_buttons_client_,
		"img.icon-down");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->extract_bottom_), 0);
	psy_ui_button_init_resource(&self->extract_top_, &self->view_buttons_client_,
		"img.icon-up");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->extract_top_), 0);
	psy_ui_button_init_resource(&self->view_float_, &self->view_buttons_client_,
		"img.float");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->view_float_), 0);
	psy_ui_button_init_resource_connect(&self->maximize_btn_,
		&self->view_buttons_client_, "img.expand",
		self, mainviewbar_on_maxminimize_view);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->maximize_btn_), 0);
}

void mainviewbar_init_navigation(MainViewBar* self, Workspace* workspace)
{
	assert(self);

	navigation_init(&self->navigation_, &self->tab_bars_, workspace);
	psy_ui_component_set_align(navigation_base(&self->navigation_),
		psy_ui_ALIGN_LEFT);
}

void mainviewbar_init_script_tabbar(MainViewBar* self)
{	
	assert(self);

	psy_ui_tabbar_init(&self->script_tab_bar_, &self->row_1_);
	psy_ui_component_set_align(&self->script_tab_bar_.component,
		psy_ui_ALIGN_TOP);
	psy_ui_component_hide(&self->script_tab_bar_.component);	
	psy_ui_button_init_text_connect(&self->toggle_scripts_, &self->tab_bars_,
		"main.scripts", self, mainviewbar_on_toggle_scripts);
	psy_ui_component_set_align(psy_ui_button_base(&self->toggle_scripts_),
		psy_ui_ALIGN_LEFT);
}

void mainviewbar_add_minmaximze(MainViewBar* self, psy_ui_Component* component)
{
	assert(self);

	minmaximize_add(&self->min_maximize_, component);
}

void mainviewbar_toggle_minmaximze(MainViewBar* self)
{
	assert(self);

	minmaximize_toggle(&self->min_maximize_);
}

void mainviewbar_on_maxminimize_view(MainViewBar* self, psy_ui_Button* sender)
{
	assert(self);

	minmaximize_toggle(&self->min_maximize_);
}

void mainviewbar_init_main_tabbar(MainViewBar* self)
{
	psy_ui_Tab* tab;

	assert(self);

	psy_ui_tabbar_init(&self->tab_bar_, &self->tab_bars_);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tab_bar_),
		psy_ui_ALIGN_LEFT);		
	tab = psy_ui_tabbar_append(&self->tab_bar_, "main.machines",
		VIEW_ID_MACHINES);
	psy_ui_svg_copy(&tab->svg, psy_ui_app_svg(psy_ui_app(), "img.machines"));
	tab = psy_ui_tabbar_append(&self->tab_bar_, "main.patterns",
		VIEW_ID_PATTERNS);
	psy_ui_svg_copy(&tab->svg, psy_ui_app_svg(psy_ui_app(), "img.patterns"));		
	psy_ui_tabbar_append(&self->tab_bar_, "main.samples",
		VIEW_ID_SAMPLES);
	psy_ui_tabbar_append(&self->tab_bar_, "main.instruments",
		VIEW_ID_INSTRUMENTS);
	psy_ui_tabbar_append(&self->tab_bar_, "main.properties",
		VIEW_ID_SONGPROPERTIES);	
}

void mainviewbar_init_view_tabbars(MainViewBar* self)
{
	assert(self);

	psy_ui_notebook_init(&self->view_tab_bars_, &self->row_0_);
	psy_ui_notebook_set_page_not_found_index(&self->view_tab_bars_, 0);
	psy_ui_component_set_margin(&self->view_tab_bars_.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 4.0));
	psy_ui_component_set_align(&self->view_tab_bars_.component,
		psy_ui_ALIGN_LEFT);
	psy_ui_component_init(&self->empty_view_tab_bar_, psy_ui_notebook_base(
		&self->view_tab_bars_), NULL);
	psy_ui_component_set_id(&self->empty_view_tab_bar_, 0);
}

void mainviewbar_on_toggle_scripts(MainViewBar* self,
	psy_ui_Component* sender)
{
	assert(self);

	psy_ui_component_toggle_visibility(psy_ui_tabbar_base(
		&self->script_tab_bar_));
	psy_ui_component_align(psy_ui_component_parent(&self->component));	
	psy_ui_component_invalidate(
		psy_ui_component_parent(&self->component));
}
