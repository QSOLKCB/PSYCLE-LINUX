/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "mainviews.h"
/* host */
#include "cmdsgeneral.h"
#include "resources/resource.h"
#include "styles.h"
/* ui */
#include <uisplitbar.h>
/* container */
#include <trackercmds.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void mainviews_on_destroyed(MainViews*);
static void mainviews_init_views(MainViews*);
static void mainviews_init_file_view(MainViews*);
static void mainviews_on_extract_left(MainViews*, psy_ui_Button* sender);
static void mainviews_on_extract_right(MainViews*, psy_ui_Button* sender);
static void mainviews_on_extract_top(MainViews*, psy_ui_Button* sender);
static void mainviews_on_extract_bottom(MainViews*, psy_ui_Button* sender);
static void mainviews_dock_extract(MainViews* self, psy_ui_AlignType);
static void mainviews_extract(MainViews*, psy_ui_AlignType);
static void mainviews_dock(MainViews*, psy_ui_Component* page);
static void mainviews_on_float(MainViews*, psy_ui_Button* sender);
static void mainviews_float(MainViews*);
static void mainviews_on_view_selected(MainViews*, Workspace* sender,
	uintptr_t view_id, uintptr_t section, uintptr_t options);
static void mainviews_on_tabbar_changed(MainViews*, psy_ui_TabBar* sender,
	uintptr_t tabindex);
static void mainviews_on_script_tabbar_changed(MainViews*,
	psy_ui_TabBar* sender, uintptr_t tabindex);
static void mainviews_select_view(MainViews*, ViewIndex);
static void mainviews_align(MainViews*);
// static bool mainviews_on_input(MainViews*, InputHandler* sender);
// static bool mainviews_handle_command(MainViews*, psy_EventDriverCmd);
// static void mainviews_connect_input_handler(MainViews*, InputHandler*);
// static void mainviews_on_navhandler_selected(MainViews*, psy_ui_NavHandler*);


/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MainViews* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);		
		vtable.on_destroyed =
			(psy_ui_fp_component)
			mainviews_on_destroyed;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(mainviews_base(self), &vtable);
}


/* implementation */
void mainviews_init(MainViews* self, psy_ui_Component* parent,
	psy_ui_Component* pane, Workspace* workspace)
{	
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->workspace = workspace;
	self->cfg_ = workspace_cfg(workspace);
	mainviewbar_init(&self->mainviewbar, &self->component, pane, workspace);	
	psy_signal_connect(&self->mainviewbar.extract_left_.signal_clicked,
		self, mainviews_on_extract_left);
	psy_signal_connect(&self->mainviewbar.extract_right_.signal_clicked,
		self, mainviews_on_extract_right);
	psy_signal_connect(&self->mainviewbar.extract_top_.signal_clicked,
		self, mainviews_on_extract_top);
	psy_signal_connect(&self->mainviewbar.extract_bottom_.signal_clicked,
		self, mainviews_on_extract_bottom);
	psy_signal_connect(&self->mainviewbar.view_float_.signal_clicked,
		self, mainviews_on_float);
	psy_ui_component_set_align(mainviewbar_base(&self->mainviewbar),
		psy_ui_ALIGN_TOP);	
	psy_ui_notebook_init(&self->notebook, &self->component);
	self->notebook.page_not_found_index = VIEW_ID_FLOATED;
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	psy_ui_emptyviewpage_init(&self->empty_page,
		psy_ui_notebook_base(&self->notebook));
	self->workspace->hostmachinecallback.ui_ = psy_ui_notebook_base(&self->notebook);
	mainviews_init_views(self);
	links_init(&self->links);
	psy_signal_connect(&self->mainviewbar.tab_bar_.signal_change, self,
		mainviews_on_tabbar_changed);
	psy_signal_connect(&self->mainviewbar.script_tab_bar_.signal_change, self,
		mainviews_on_script_tabbar_changed);
	psy_signal_connect(&self->workspace->signal_view_selected, self,
		mainviews_on_view_selected);			
}

void mainviews_init_views(MainViews* self)
{
	machineview_init(&self->machineview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);
#ifdef PSYCLE_USE_PATTERN_VIEW
	patternview_init(&self->patternview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		psycleconfig_patview(self->cfg_),
		self->workspace);
//		self->patternview.trackerview.nav_back = &self->mainviewbar.component;
#endif		
	samplesview_init(&self->samplesview, psy_ui_notebook_base(
		&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);
	instrumentview_init(&self->instrumentsview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);
	songpropertiesview_init(&self->songpropertiesview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);
	propertiesview_init(&self->settingsview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
			&self->workspace->cfg_.config, 3, TRUE,
			self->workspace);	
	psy_ui_component_set_id(&self->settingsview.component, VIEW_ID_SETTINGS);
	psy_ui_component_set_title(&self->settingsview.component, "main.settings");	
#ifdef PSYCLE_USE_STYLE_VIEW	
	styleview_init(&self->styleview,	
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);	
#endif		
	helpview_init(&self->helpview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);
	renderview_init(&self->renderview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		self->workspace);	
	confirmbox_init(&self->confirm,
		psy_ui_notebook_base(&self->notebook));
	self->workspace->confirm = &self->confirm;
	psy_ui_component_set_id(confirmbox_base(&self->confirm),
		VIEW_ID_CONFIRM);
	mainviews_init_file_view(self);	
}

void mainviews_init_file_view(MainViews* self)
{
	assert(self);
	
	/* ft2 style file view */
	fileview_init(&self->fileview,
		psy_ui_notebook_base(&self->notebook),
		psy_ui_notebook_base(&self->mainviewbar.view_tab_bars_),
		psy_dir_config());
	self->fileview.song_link = fileview_add_link(&self->fileview, "Songs", 
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),
			"songs", PSYCLE_SONGS_DEFAULT_DIR));	
	filechooser_set_file_view(&self->workspace->file_chooser, &self->fileview);	
	fileview_set_directory(&self->fileview,	
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),
			"songs", PSYCLE_SONGS_DEFAULT_DIR));	
}

void mainviews_on_destroyed(MainViews* self)
{
	assert(self);
	
	links_dispose(&self->links);
}

void mainviews_on_extract_left(MainViews* self, psy_ui_Button* sender)
{
	mainviews_dock_extract(self, psy_ui_ALIGN_LEFT);
}

void mainviews_on_extract_right(MainViews* self, psy_ui_Button* sender)
{
	mainviews_dock_extract(self, psy_ui_ALIGN_RIGHT);	
}

void mainviews_on_extract_top(MainViews* self, psy_ui_Button* sender)
{
	mainviews_dock_extract(self, psy_ui_ALIGN_TOP);
}

void mainviews_on_extract_bottom(MainViews* self, psy_ui_Button* sender)
{
	mainviews_dock_extract(self, psy_ui_ALIGN_BOTTOM);
}

void mainviews_dock_extract(MainViews* self, psy_ui_AlignType align)
{
	ViewIndex view;
	psy_ui_Component* page;

	view = workspace_current_view(self->workspace);
	page = psy_ui_component_by_id(&self->component, view.id,
		psy_ui_NONE_RECURSIVE);
	if (page) {
		mainviews_dock(self, page);
	}
	else {
		mainviews_extract(self, align);
	}
}

void mainviews_extract(MainViews* self, psy_ui_AlignType align)
{
	psy_ui_Component* page;	
	uintptr_t curr_index;

	page = psy_ui_notebook_active_page(&self->notebook);	
	if (!page) {
		return;
	}
	curr_index = psy_ui_component_index(page);
	if (curr_index != psy_INDEX_INVALID && curr_index > 1) {
		curr_index = psy_ui_component_index(page) - 1;
	} else {
		curr_index = psy_INDEX_INVALID;
	}
	if (page) {
		psy_ui_Splitter* splitter;

		psy_ui_component_set_parent(page, &self->component);
		psy_ui_component_set_align(page, align);
		psy_ui_component_set_preferred_size(page, psy_ui_size_make_em(80.0, 20.0));
		splitter = psy_ui_splitter_allocinit(&self->component);
		psy_ui_component_set_align(&splitter->component, align);
		psy_ui_component_align_invalidate(&self->component);				
	}
	if (curr_index != psy_INDEX_INVALID) {
		psy_ui_notebook_select(&self->notebook, curr_index);
	} else {
		psy_ui_notebook_select_by_component_id(&self->notebook, VIEW_ID_FLOATED);
	}
}

void mainviews_dock(MainViews* self, psy_ui_Component* page)
{
	psy_ui_Component* splitter;
	uintptr_t index;

	index = psy_ui_component_index(page);
	index++;
	splitter = psy_ui_component_at(&self->component, index);
	if (splitter) {
		psy_ui_component_remove(&self->component, splitter);
	}
	psy_ui_component_set_parent(page, &self->notebook.component);
	psy_ui_component_set_align(page, psy_ui_ALIGN_CLIENT);
	psy_ui_component_align_invalidate(&self->component);	
	psy_ui_notebook_select_by_component_id(&self->notebook, page->id);
}

void mainviews_on_float(MainViews* self, psy_ui_Button* sender)
{
	mainviews_float(self);
}

void mainviews_float(MainViews* self)
{	
	psy_ui_Component* page;

	assert(self);

	page = psy_ui_notebook_active_page(&self->notebook);
	if (page) {				
		page->kbd_ = workspace_kbd_driver(self->workspace);
		page->frame_size_ = psy_ui_size_make_em(140.0, 30.0);
		psy_ui_component_float(page);		
	}
}

void mainviews_on_view_selected(MainViews* self, Workspace* sender,
	uintptr_t view_id, uintptr_t section, uintptr_t options)
{	
	assert(self);

	mainviews_select_view(self, viewindex_make_all(
		view_id, section, options, psy_INDEX_INVALID));
}

void mainviews_on_tabbar_changed(MainViews* self, psy_ui_TabBar* sender,
	uintptr_t tabindex)
{	
	psy_ui_Tab* tab;
		
	assert(self);

	tab = psy_ui_tabbar_tab(sender, tabindex);
	if (!tab) {
		return;
	}
	workspace_select_view(self->workspace, viewindex_make(psy_ui_component_id(
		&tab->component)));	
}

void mainviews_select_view(MainViews* self, ViewIndex view_index)
{
	psy_ui_Component* view;

	assert(self);

	if (view_index.id == psy_INDEX_INVALID) {
		view = psy_ui_notebook_active_page(&self->notebook);
	}
	else {
		psy_ui_notebook_select_by_component_id(&self->notebook,
			view_index.id);
		view = psy_ui_notebook_active_page(&self->notebook);
	}
	if (view) {
		psy_ui_component_select_section(view, view_index.section, view_index.option);
		if (view_index.id != psy_INDEX_INVALID) {
			psy_ui_notebook_select_by_component_id
				(&self->mainviewbar.view_tab_bars_,
				view_index.id);
			psy_ui_tabbar_mark_id(&self->mainviewbar.tab_bar_, view_index.id);
			view_index.section = psy_ui_component_section(view);			
			workspace_add_view(self->workspace, view_index);
			psy_ui_component_set_focus(view);
		}
		mainviews_align(self);		
	}
}

void mainviews_on_script_tabbar_changed(MainViews* self,
	psy_ui_TabBar* sender, uintptr_t tabindex)
{
	const Link* link;

	link = links_at(&self->links, tabindex);
	if (link) {
		psy_audio_Machine* machine;
		
		machine = psy_audio_machinefactory_make_machine_from_path(
			&self->workspace->player_.machinefactory, psy_audio_LUA,
			link_path(link), 0, (uintptr_t)(void*)&self->notebook.component);
		// if (machine) {
		//	psy_audio_machine_set_host_view(machine, &self->notebook.component);
		// }
	}
}

void mainviews_add_link(MainViews* self, Link* link)
{
	links_add(&self->links, link);
	psy_ui_tabbar_append(&self->mainviewbar.script_tab_bar_,
		link->label_, psy_INDEX_INVALID);
}

void mainviews_align(MainViews* self)
{
	assert(self);
	
	if (psy_ui_component_draw_visible(mainviews_base(self))) {
		psy_ui_component_align_invalidate(mainviews_base(self));
	}
}
