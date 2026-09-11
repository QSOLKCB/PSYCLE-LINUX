/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "newmachine.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
/* audio */
#include <exclusivelock.h>
/* platform */
#include "../../detail/portable.h"


/* NewMachineSearch */

/* implementation */
static void newmachinesearch_on_search_field_change(
	NewMachineSearch*, SearchField* sender);

void newmachinesearch_init(NewMachineSearch* self,
	psy_ui_Component* parent, PluginFilter* filter,
	const psy_ui_SVG* svg)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	self->filter = filter;
	searchfield_init(&self->search, &self->component, NULL, svg);
	psy_ui_component_set_align(searchfield_base(&self->search),
		psy_ui_ALIGN_TOP);
	psy_signal_connect(&self->search.signal_changed, self,
		newmachinesearch_on_search_field_change);
	searchfield_set_default_text(&self->search, "newmachine.search-plugin");
}

void newmachinesearch_set_restore_focus(NewMachineSearch* self,
	psy_ui_Component* restore_focus)
{
	assert(self);

	searchfield_set_restore_focus(&self->search, restore_focus);
}

void newmachinesearch_set_filter(NewMachineSearch* self,
	PluginFilter* filter)
{
	assert(self);

	self->filter = filter;
}

void newmachinesearch_on_search_field_change(NewMachineSearch* self,
	SearchField* sender)
{
	assert(self);

	if (self->filter) {
		pluginfilter_set_search_text(self->filter, searchfield_text(sender));
	}
}


/* NewMachineBar */

/* prototypes */
static void newmachinebar_on_rescan(NewMachineBar*, psy_ui_Component* sender);
static void newmachinebar_on_dir_config(NewMachineBar*,
	psy_ui_Component* sender);

/* implementation */
void newmachinebar_init(NewMachineBar* self,
	psy_ui_Component* parent, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_style_type(&self->component, STYLE_DIALOG_BAR);
	psy_ui_component_set_default_align(&self->component,
		psy_ui_ALIGN_LEFT, psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_button_init_text_connect(&self->rescan, &self->component,
		"newmachine.rescan", self, newmachinebar_on_rescan);
	psy_ui_label_init_text(&self->desc, &self->component, "newmachine.in");
	psy_ui_button_init_text_connect(&self->directories, &self->component,
		"newmachine.plugin-directories", self, newmachinebar_on_dir_config);
	psy_ui_button_set_svg(&self->directories, psy_ui_app_svg(psy_ui_app(), "img.settings"));
	psy_ui_button_init_text(&self->cancel, &self->component, "Cancel");
	psy_ui_component_set_align(psy_ui_button_base(&self->cancel),
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_preferred_width(psy_ui_button_base(
		&self->cancel), psy_ui_value_make_ew(15.0));
	psy_ui_button_init_text(&self->add, &self->component, "OK");
	psy_ui_component_set_preferred_width(psy_ui_button_base(&self->add),
		psy_ui_value_make_ew(15.0));
	psy_ui_component_set_align(psy_ui_button_base(&self->add),
		psy_ui_ALIGN_RIGHT);
}

void newmachinebar_on_dir_config(NewMachineBar* self, psy_ui_Component* sender)
{
	assert(self);

	workspace_select_view(self->workspace, viewindex_make_all(
		VIEW_ID_SETTINGS, 3, 0, psy_INDEX_INVALID));
}

void newmachinebar_on_rescan(NewMachineBar* self,
	psy_ui_Component* sender)
{
	assert(self);

	if (!psy_audio_plugincatcher_scanning(
		&workspace_player(self->workspace)->plugincatcher)) {
		workspace_scan_plugins(self->workspace);
	}
}


/* NewMachineFilterGroup */

/* prototypes */
static void newmachinefiltergroup_on_button(NewMachineFilterGroup*,
	psy_ui_Button* sender);
static void newmachinefiltergroup_build(NewMachineFilterGroup*);
static void newmachinefiltergroup_select(NewMachineFilterGroup*,
	uintptr_t id, uintptr_t state);
static void newmachinefiltergroup_update(NewMachineFilterGroup*);
static void newmachinefiltergroup_on_desc(NewMachineFilterGroup*,
	psy_ui_Button* sender);

/* implementation */
void newmachinefiltergroup_init(NewMachineFilterGroup* self,
	psy_ui_Component* parent, PluginFilter* filter, PluginFilterGroup* group)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_init_align(&self->header, &self->component, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_button_init_connect(&self->desc, &self->header, self,
		newmachinefiltergroup_on_desc);
	psy_ui_button_set_text_alignment(&self->desc, psy_ui_ALIGNMENT_LEFT);
	psy_ui_component_set_align(&self->desc.component, psy_ui_ALIGN_TOP);
	psy_ui_component_init_align(&self->types, &self->component, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(&self->types,
		psy_ui_margin_make_em(0.5, 0.0, 0.0, 0.0));
	psy_ui_component_set_default_align(&self->types,
		psy_ui_ALIGN_TOP, psy_ui_margin_zero());
	newmachinefiltergroup_set_filter(self, filter, group);
}

void newmachinefiltergroup_set_filter(NewMachineFilterGroup* self,
	PluginFilter* filter, PluginFilterGroup* group)
{
	assert(self);

	self->filter = filter;
	self->filter_group = group;
	newmachinefiltergroup_build(self);
}

void newmachinefiltergroup_build(NewMachineFilterGroup* self)
{
	const psy_List* p;

	assert(self);

	psy_ui_component_clear(&self->types);
	if (!self->filter_group) {
		return;
	}
	for (p = self->filter_group->items; p != NULL; p = p->next) {
		const PluginFilterItem* curr;

		curr = (const PluginFilterItem*)p->entry;
		newmachinefiltergroup_add(self, curr->name, curr->key, curr->active);
	}
	psy_ui_button_set_text(&self->desc, self->filter_group->label);
}

void newmachinefiltergroup_add(NewMachineFilterGroup* self,
	const char* label, uintptr_t id, bool active)
{
	psy_ui_Button* button;

	assert(self);

	button = psy_ui_button_alloc_init(&self->types);
	psy_ui_component_set_id(psy_ui_button_base(button), id);
	psy_ui_button_set_text(button, label);
	psy_ui_button_set_text_alignment(button, psy_ui_ALIGNMENT_LEFT);
	psy_ui_button_allow_right_click(button);
	if (active) {
		psy_ui_component_add_style_state(psy_ui_button_base(button),
			psy_ui_STYLESTATE_SELECT);
	}
	psy_signal_connect(&button->signal_clicked, self,
		newmachinefiltergroup_on_button);
}

void newmachinefiltergroup_set(NewMachineFilterGroup* self,
	uintptr_t id, bool active)
{
	assert(self);

	if (active) {
		newmachinefiltergroup_mark(self, id);
	}
	else {
		newmachinefiltergroup_unmark(self, id);
	}
}

void newmachinefiltergroup_mark(NewMachineFilterGroup* self,
	uintptr_t id)
{
	psy_ui_Component* component;

	assert(self);

	component = psy_ui_component_by_id(&self->types, id, 0);
	if (component) {
		psy_ui_component_add_style_state(component,
			psy_ui_STYLESTATE_SELECT);
	}
}

void newmachinefiltergroup_unmark(NewMachineFilterGroup* self,
	uintptr_t id)
{
	psy_ui_Component* component;

	assert(self);

	component = psy_ui_component_by_id(&self->types, id, 0);
	if (component) {
		psy_ui_component_remove_style_state(component,
			psy_ui_STYLESTATE_SELECT);
	}
}

void newmachinefiltergroup_on_button(NewMachineFilterGroup* self,
	psy_ui_Button* sender)
{
	assert(self);

	newmachinefiltergroup_select(self, psy_ui_component_id(
		psy_ui_button_base(sender)), psy_ui_button_click_state(sender));
}

void newmachinefiltergroup_on_desc(NewMachineFilterGroup* self,
	psy_ui_Button* sender)
{
	assert(self);

	if (!self->filter_group) {
		return;
	}
	if (pluginfiltergroup_all_active(self->filter_group)) {
		pluginfiltergroup_deselect_all(self->filter_group);
	}
	else {
		pluginfiltergroup_select_all(self->filter_group);
	}
	newmachinefiltergroup_update(self);
	if (self->filter) {
		pluginfilter_notify(self->filter);
	}
}

void newmachinefiltergroup_select(NewMachineFilterGroup* self, uintptr_t id,
	uintptr_t state)
{
	assert(self);

	if (self->filter_group) {
		if (state == 1) {
			pluginfiltergroup_toggle(self->filter_group, id);
		}
		else {
			pluginfiltergroup_deselect_all(self->filter_group);
			pluginfiltergroup_select(self->filter_group, id);
		}
		newmachinefiltergroup_update(self);
	}
	if (self->filter) {
		pluginfilter_notify(self->filter);
	}
}

void newmachinefiltergroup_update(NewMachineFilterGroup* self)
{
	assert(self);

	if (self->filter_group) {
		psy_List* p;

		for (p = self->filter_group->items; p != NULL; p = p->next) {
			PluginFilterItem* item;

			item = (PluginFilterItem*)p->entry;
			newmachinefiltergroup_set(self, item->key, item->active);
		}
	}
}

/* NewMachineFilters */

/* prototypes */
static void newmachinefilters_on_toggle(NewMachineFilters*,
	psy_ui_Button* sender);

/* implementation */
void newmachinefilters_init(NewMachineFilters* self,
	psy_ui_Component* parent, PluginFilter* filter)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_init_align(&self->expand, &self->component, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(&self->expand, psy_ui_margin_make_em(
		0.0, 0.0, 1.0, 0.0));
	psy_ui_button_init_text_connect(&self->toggle, &self->expand,
		"newmachine.filters", self, newmachinefilters_on_toggle);
	psy_ui_component_set_align(&self->toggle.component, psy_ui_ALIGN_LEFT);	
	/* filters */
	psy_ui_component_init_align(&self->filters, &self->component, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_align_expand(&self->filters, psy_ui_HEXPAND);
	psy_ui_component_set_default_align(&self->filters,
		psy_ui_ALIGN_LEFT, psy_ui_margin_zero());
	psy_ui_component_hide(&self->filters);
	/* mode bar */
	newmachinefiltergroup_init(&self->mode, &self->filters,
		filter, (filter) ? &filter->mode : NULL);
	psy_ui_component_set_preferred_width(newmachinefiltergroup_base(
		&self->mode), psy_ui_value_make_pw(0.25));
	/* type bar */
	newmachinefiltergroup_init(&self->types, &self->filters,
		filter, (filter) ? &filter->types : NULL);
	psy_ui_component_set_preferred_width(newmachinefiltergroup_base(
		&self->types), psy_ui_value_make_pw(0.25));
	/* categeory bar */
	newmachinefiltergroup_init(&self->categories, &self->filters,
		filter, (filter) ? &filter->categories : NULL);
	psy_ui_component_set_preferred_width(newmachinefiltergroup_base(
		&self->categories), psy_ui_value_make_pw(0.25));
	/* sort bar */
	newmachinefiltergroup_init(&self->sort, &self->filters,
		filter, (filter) ? &filter->sort : NULL);
	psy_ui_component_set_preferred_width(newmachinefiltergroup_base(
		&self->sort), psy_ui_value_make_pw(0.25));
}

void newmachinefilters_on_toggle(NewMachineFilters* self,
	psy_ui_Button* sender)
{
	psy_ui_Component* base;

	assert(self);

	if (psy_ui_component_visible(&self->filters)) {
		psy_ui_component_hide(&self->filters);
	}
	else {
		psy_ui_component_show(&self->filters);
	}
	base = psy_ui_component_parent(psy_ui_component_parent(&self->component));
	psy_ui_component_align(base);
	psy_ui_component_invalidate(base);
}


/* NewMachinePlugins */

/* prototypes */
static void newmachineplugins_on_destroyed(NewMachinePlugins*);
static void newmachineplugins_init_plugin_view(NewMachinePlugins*,
	PluginFilter*);
static void newmachineplugins_on_plugin_cache_changed(
	NewMachinePlugins*, PluginScanThread* sender);
static void newmachineplugins_on_plugin_changed(
	NewMachinePlugins*, PluginsView* sender);
static void newmachineplugins_scroll_down(NewMachinePlugins*);
static void newmachineplugins_scroll_up(NewMachinePlugins*);

/* implementation */
void newmachineplugins_init(NewMachinePlugins* self, psy_ui_Component* parent,
	PluginFilter* filter, PluginScanThread* plugin_scan_thread)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	self->plugin_scan_thread = plugin_scan_thread;
	/* pane */
	psy_ui_component_init(&self->pane, &self->component, NULL);
	psy_ui_component_set_overflow(&self->pane, psy_ui_OVERFLOW_VSCROLL);
	psy_ui_component_set_scroll_step(&self->pane,
		psy_ui_size_make_em(0.0, 1.0));
	psy_ui_component_set_wheel_scroll(&self->pane, 4);
	/* filters */
	newmachinefilters_init(&self->filters, &self->pane, filter);
	psy_ui_component_set_align(&self->filters.component, psy_ui_ALIGN_TOP);
	/* plugins */
	newmachineplugins_init_plugin_view(self, filter);
	/* section scroll */
	psy_ui_scroller_init(&self->scroller, &self->component, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->pane);
	psy_ui_component_set_align(&self->pane, psy_ui_ALIGN_HCLIENT);
	psy_ui_component_set_tab_index(&self->scroller.component, 0);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);
	/* connect */
	psy_signal_connect(&plugin_scan_thread->signal_plugincachechanged,
		self, newmachineplugins_on_plugin_cache_changed);
}

void newmachineplugins_init_plugin_view(NewMachinePlugins* self,
	PluginFilter* filter)
{
	assert(self);

	pluginsview_init(&self->pluginview, &self->pane);
	psy_audio_plugins_update_filter_categories(
		&self->plugin_scan_thread->plugincatcher->plugins_,
		filter);
	pluginsview_set_plugins(&self->pluginview,
		&self->plugin_scan_thread->plugincatcher->plugins_);
	pluginsview_set_filter(&self->pluginview, filter);
	psy_ui_component_set_align(&self->pluginview.component, psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(&self->pluginview.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 1.0));
	psy_signal_connect(&self->pluginview.signal_changed, self,
		newmachineplugins_on_plugin_changed);
}

void newmachineplugins_on_plugin_cache_changed(NewMachinePlugins* self,
	PluginScanThread* sender)
{
	assert(self);

	pluginsview_set_plugins(&self->pluginview,
		&self->plugin_scan_thread->plugincatcher->plugins_);
}

void newmachineplugins_on_plugin_changed(NewMachinePlugins* self,
	PluginsView* sender)
{
	if (sender->scroll_dir > 0) {
		newmachineplugins_scroll_down(self);
	}
	else {
		newmachineplugins_scroll_up(self);
	}
}

void newmachineplugins_scroll_down(NewMachinePlugins* self)
{
	psy_ui_RealRectangle position;
	psy_ui_RealRectangle pluginview_position;
	psy_ui_RealSize client_size;
	psy_ui_RealRectangle client;
	double diff;
	uintptr_t index;

	index = newmachineselection_first(&self->pluginview.selection);
	if (index == psy_INDEX_INVALID) {
		return;
	}
	position = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0, pluginsview_index_to_px(
			&self->pluginview, index)),
		self->pluginview.item_size_px_);
	client_size = psy_ui_component_clientsize_px(&self->pane);
	pluginview_position = psy_ui_component_position(&self->pluginview.component);
	psy_ui_realrectangle_move(&position, psy_ui_realrectangle_topleft(
		&pluginview_position));
	client = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(
			psy_ui_component_scroll_left_px(&self->pane),
			psy_ui_component_scroll_top_px(&self->pane)),
		client_size);
	diff = client.bottom - position.bottom;
	if (diff < 0) {
		psy_ui_component_set_scroll_top_px(&self->pane,
			client.top - diff);
	}
}

void newmachineplugins_scroll_up(NewMachinePlugins* self)
{
	psy_ui_RealRectangle position;
	psy_ui_RealRectangle pluginview_position;
	psy_ui_RealSize client_size;
	psy_ui_RealRectangle client;
	uintptr_t index;

	index = newmachineselection_first(&self->pluginview.selection);
	if (index == psy_INDEX_INVALID) {
		return;
	}
	position = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0, pluginsview_index_to_px(
			&self->pluginview, index)),
		self->pluginview.item_size_px_);
	pluginview_position = psy_ui_component_position(&self->pluginview.component);
	psy_ui_realrectangle_move(&position, psy_ui_realrectangle_topleft(
		&pluginview_position));
	client_size = psy_ui_component_clientsize_px(&self->pane);
	client = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(
			psy_ui_component_scroll_left_px(&self->pane),
			psy_ui_component_scroll_top_px(&self->pane)),
		client_size);
	if (position.top < client.top) {
		psy_ui_component_set_scroll_top_px(&self->pane,
			position.top);
	}
	psy_ui_component_invalidate(&self->pane);
}


/* NewMachine */

/* prototypes */
static void newmachine_on_destroyed(NewMachine*);
static void newmachine_init_client(NewMachine*, psy_ui_Component* parent);
static void newmachine_on_plugin_cache_changed(NewMachine*, PluginScanThread*);
static void newmachine_on_mouse_down(NewMachine*, psy_ui_MouseEvent*);
static void newmachine_on_focus(NewMachine*, psy_ui_Component* sender);
static void newmachine_on_scan_start(NewMachine*, PluginScanThread*);
static void newmachine_on_scan_end(NewMachine*, PluginScanThread*);
static void newmachine_on_plugin_scan_progress(NewMachine*, PluginScanThread*,
	int progress);
static void newmachine_on_plugin_category_changed(NewMachine*, NewMachineDetail*
	sender);
static void newmachine_emit_selected_plugin(NewMachine*);
static void newmachine_on_add_selected_plugins(NewMachine*,
	psy_ui_Button* sender);
static void newmachine_on_cancel(NewMachine*, psy_ui_Button* sender);
static void newmachine_add_machine(NewMachine*, const psy_audio_MachineInfo*);
static void newmachine_on_plugin_selected(NewMachine*, PluginsView* sender);
static void newmachine_on_plugin_changed(NewMachine*, PluginsView* parent);
static void newmachine_add_selection(NewMachine*);

/* vtable */
static psy_ui_ComponentVtable newmachine_vtable;
static bool newmachine_vtable_initialized = FALSE;

static void newmachine_vtable_init(NewMachine* self)
{
	assert(self);

	if (!newmachine_vtable_initialized) {
		newmachine_vtable = *(self->component.vtable);
		newmachine_vtable.on_destroyed =
			(psy_ui_fp_component)
			newmachine_on_destroyed;
		newmachine_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			newmachine_on_mouse_down;
		newmachine_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &newmachine_vtable);
}

/* implementation */
void newmachine_init(NewMachine* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	newmachine_vtable_init(self);
	self->workspace = workspace;
	self->restore_view = viewindex_make_section(VIEW_ID_MACHINES,
		SECTION_ID_MACHINEVIEW_WIRES);
	pluginfilter_init(&self->filter);
	psy_audio_plugins_update_filter_categories(
		&self->workspace->player_.plugincatcher.plugins_, &self->filter);
	psy_ui_component_set_id(&self->component,
		SECTION_ID_MACHINEVIEW_NEWMACHINE);
	psy_ui_component_set_style_type(&self->component, STYLE_CLIENT_VIEW);
	psy_ui_notebook_init(&self->notebook, &self->component);
	newmachine_init_client(self, psy_ui_notebook_base(&self->notebook));
	pluginscanview_init(&self->scanview, psy_ui_notebook_base(
		&self->notebook), &self->workspace->plugin_scan_thread_);
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	psy_ui_notebook_select(&self->notebook, 0);
}

void newmachine_on_destroyed(NewMachine* self)
{
	assert(self);

	pluginfilter_dispose(&self->filter);
}

void newmachine_init_client(NewMachine* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init_align(&self->client, parent, NULL,
		psy_ui_ALIGN_CLIENT);
	/* search */
	newmachinesearch_init(&self->search, &self->client, &self->filter,
		psy_ui_app_svg(psy_ui_app(), "search"));		
	psy_ui_component_set_align(newmachinesearch_base(&self->search),
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(newmachinesearch_base(&self->search),
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));
	/* bar */
	newmachinebar_init(&self->bar, &self->component, self->workspace);
	psy_ui_component_set_align(newmachinebar_base(&self->bar),
		psy_ui_ALIGN_BOTTOM);
	/* plugin details */
	newmachinedetail_init(&self->detail, &self->client, self->workspace);
	psy_ui_component_set_align(&self->detail.component, psy_ui_ALIGN_RIGHT);
	psy_signal_connect(&self->detail.signal_categorychanged, self,
		newmachine_on_plugin_category_changed);
	/* plugin section */
	newmachineplugins_init(&self->plugins, &self->client,
		&self->filter, &self->workspace->plugin_scan_thread_);
	psy_ui_component_set_align(&self->plugins.component, psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->plugins.pluginview.signal_selected,
		self, newmachine_on_plugin_selected);
	psy_signal_connect(&self->plugins.pluginview.signal_changed,
		self, newmachine_on_plugin_changed);
	newmachinesearch_set_restore_focus(&self->search,
		&self->plugins.pluginview.component);
	/* connect to signals */
	psy_signal_connect(
		&self->workspace->plugin_scan_thread_.signal_plugincachechanged,
		self, newmachine_on_plugin_cache_changed);
	psy_signal_connect(&self->component.signal_focus,
		self, newmachine_on_focus);
	psy_signal_connect(&self->workspace->plugin_scan_thread_.signal_scanstart,
		self, newmachine_on_scan_start);
	psy_signal_connect(&self->workspace->plugin_scan_thread_.signal_scanend,
		self, newmachine_on_scan_end);
	psy_signal_connect(&self->bar.add.signal_clicked,
		self, newmachine_on_add_selected_plugins);
	psy_signal_connect(&self->bar.cancel.signal_clicked,
		self, newmachine_on_cancel);
}

void newmachine_on_plugin_changed(NewMachine* self, PluginsView* sender)
{
	assert(self);

	newmachinedetail_set_plugin(&self->detail, pluginsview_selected(sender));
}

void newmachine_on_plugin_selected(NewMachine* self, PluginsView* sender)
{
	assert(self);

	newmachine_add_selection(self);
}

void newmachine_on_add_selected_plugins(NewMachine* self, psy_ui_Button* sender)
{
	assert(self);

	newmachine_add_selection(self);
}

void newmachine_add_selection(NewMachine* self)
{
	PluginsView* view;

	assert(self);

	view = &self->plugins.pluginview;
	if (newmachineselection_valid(&view->selection)) {
		const psy_List* p;
		uintptr_t count;

		p = newmachineselection_begin_const(&view->selection);
		for (count = 0; p != NULL; p = p->next, ++count) {
			const psy_audio_MachineInfo* plugin;
			uintptr_t sel;

			sel = (uintptr_t)psy_list_entry_const(p);
			plugin = pluginsview_plugin(view, sel);
			if (plugin) {
				self->workspace->insert.count = count;
				newmachine_add_machine(self, plugin);
			}
		}
		machineinsert_reset(&self->workspace->insert);
		workspace_select_view(self->workspace, self->restore_view);
	}
}

void newmachine_on_cancel(NewMachine* self, psy_ui_Button* sender)
{
	assert(self);

	workspace_select_view(self->workspace, self->restore_view);
}

void newmachine_add_machine(NewMachine* self, const psy_audio_MachineInfo*
	machine_info)
{
	psy_audio_MachineFactory* machine_factory;
	psy_audio_Machine* machine;

	assert(self);

	machine_factory = &self->workspace->player_.machinefactory;
	machine = psy_audio_machinefactory_make_info(machine_factory, machine_info);
	if (machine) {
		const MachinesInsert* insert;
		psy_audio_Machines* machines;
		uintptr_t mac_id;		

		insert = &self->workspace->insert;
		machines = psy_audio_song_machines(workspace_song(self->workspace));
		if (insert->replace_mac != psy_INDEX_INVALID &&
			psy_audio_machine_mode(machine) == psy_audio_MACHMODE_FX) {
			psy_audio_machine_set_bus(machine);
		}
		if (insert->replace_mac != psy_INDEX_INVALID) {
			mac_id = insert->replace_mac;
			if (psy_audio_machines_at(machines, insert->replace_mac)) {
				double x;
				double y;
				psy_audio_Connections connections;				

				psy_audio_connections_init(&connections);
				psy_audio_exclusivelock_enter();				
				psy_audio_connections_copy(&connections, &machines->connections);				
				psy_audio_machine_position(psy_audio_machines_at(machines,
					insert->replace_mac), &x, &y);
				psy_audio_machines_remove(machines, insert->replace_mac, FALSE);
				psy_audio_machine_set_position(machine, x, y);
				psy_audio_machines_insert(machines, insert->replace_mac, machine);
				psy_audio_connections_copy(&machines->connections, &connections);
				psy_audio_connections_dispose(&connections);
				psy_audio_exclusivelock_leave();
			} else {
				psy_ui_RealSize client_size;
				psy_ui_RealSize size_px;
				psy_ui_Style* style;

				client_size = psy_ui_component_scroll_size_px(&self->component);
				size_px = psy_ui_realsize_zero();
				if (psy_audio_machine_mode(machine) == psy_audio_MACHMODE_FX) {
					style = psy_ui_style(STYLE_MV_EFFECT);
				}
				else {
					style = psy_ui_style(STYLE_MV_GENERATOR);
				}
				if (style) {
					psy_ui_Size size;

					size = psy_ui_style_size(style);
					size_px = psy_ui_size_px(&size, psy_ui_component_textmetric(
						newmachine_base(self)), NULL);
				}
				psy_audio_machines_position_random(machines, machine,
					client_size.width, client_size.height,
					size_px.width, size_px.height);
				psy_audio_machines_insert(machines, insert->replace_mac, machine);
			}			
		} else {
			mac_id = psy_audio_machines_append(machines, machine);
			psy_audio_machines_select(machines, mac_id);
		}
		psy_audio_machines_rewire(machines, mac_id, insert->wire);
	}
	else {
		psy_Logger* logger;

		logger = psy_ui_component_logger(&self->component, psy_ui_LOG_TERMINAL);
		if (logger) {
			psy_logger_error(logger, machine_factory->errstr);
		}
	}
}

void newmachine_on_plugin_cache_changed(NewMachine* self,
	PluginScanThread* sender)
{
	assert(self);

	newmachinedetail_set_plugin(&self->detail, NULL);
	psy_audio_plugins_update_filter_categories(
		&self->workspace->player_.plugincatcher.plugins_, &self->filter);
	psy_ui_component_align_full(&self->client);
	psy_ui_component_invalidate(&self->client);
}

void newmachine_on_focus(NewMachine* self, psy_ui_Component* sender)
{
	assert(self);

	psy_ui_component_set_focus(&self->plugins.pluginview.component);
}

void newmachine_on_scan_start(NewMachine* self, PluginScanThread* sender)
{
	assert(self);

	pluginscanview_reset(&self->scanview);
	psy_ui_notebook_select(&self->notebook, 1);
}

void newmachine_on_scan_end(NewMachine* self, PluginScanThread* sender)
{
	assert(self);

	pluginscanview_stop(&self->scanview);
	psy_ui_notebook_select(&self->notebook, 0);
}

void newmachine_on_mouse_down(NewMachine* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_mouseevent_stop_propagation(ev);
}

void newmachine_on_plugin_category_changed(NewMachine* self,
	NewMachineDetail* sender)
{
	assert(self);

	psy_audio_plugins_update_filter_categories(
		&self->workspace->player_.plugincatcher.plugins_, &self->filter);
}
