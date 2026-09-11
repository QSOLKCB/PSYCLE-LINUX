/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pluginsview.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
#include <trackercmds.h>
/* audio */
#include <plugincatcher.h>
/* platform */
#include "../../detail/portable.h"


/* NewMachineSelection */
void newmachineselection_init(NewMachineSelection* self)
{
	assert(self);
	
	self->items = NULL;
}

void newmachineselection_dispose(NewMachineSelection* self)
{
	assert(self);
	
	psy_list_free(self->items);
	self->items = NULL;
}

void newmachineselection_clear(NewMachineSelection* self)
{
	assert(self);
	
	psy_list_free(self->items);
	self->items = NULL;
}

void newmachineselection_select(NewMachineSelection* self, uintptr_t index)
{
	assert(self);
	
	if (!psy_list_find_entry(self->items, (void*)index)) {
		psy_list_append(&self->items, (void*)index);
	}
}

void newmachineselection_single_select(NewMachineSelection* self,
	uintptr_t index)
{
	assert(self);
	
	newmachineselection_clear(self);
	psy_list_append(&self->items, (void*)index);
}

void newmachineselection_select_all(NewMachineSelection* self, uintptr_t size)
{
	uintptr_t i;

	newmachineselection_clear(self);
	for (i = 0; i < size; ++i) {
		psy_list_append(&self->items, (void*)i);
	}	
}

void newmachineselection_deselect(NewMachineSelection* self, uintptr_t index)
{
	psy_List* p;

	p = psy_list_find_entry(self->items, (void*)index);
	if (p) {
		psy_list_remove(&self->items, p);
	}	
}

void newmachineselection_toggle(NewMachineSelection* self, uintptr_t index)
{
	psy_List* p;

	p = psy_list_find_entry(self->items, (void*)index);
	if (p) {
		psy_list_remove(&self->items, p);
	} else {
		psy_list_append(&self->items, (void*)index);
	}
}

bool newmachineselection_is_selected(const NewMachineSelection* self,
	uintptr_t index)
{
	return psy_list_find_entry(self->items, (void*)index) != NULL;
}

uintptr_t newmachineselection_first(const NewMachineSelection* self)
{
	if (self->items) {
		return (uintptr_t)self->items->entry;
	}
	return psy_INDEX_INVALID;
}

bool newmachineselection_valid(const NewMachineSelection* self)
{
	return (newmachineselection_first(self) != psy_INDEX_INVALID);
}

/* PluginsView */

static void plugindisplayname(const psy_audio_MachineInfo*, char* text);
static uintptr_t plugintype(const psy_audio_MachineInfo*, char* text);
static uintptr_t pluginmode(const psy_audio_MachineInfo*, char* text);

/* prototypes */
static void pluginsview_on_destroyed(PluginsView*);
static void pluginsview_connect_input_handler(PluginsView*, InputHandler*);
static void pluginsview_on_draw(PluginsView*, psy_ui_Graphics*);
static void pluginsview_draw_item(PluginsView*, psy_ui_Graphics*,
	const psy_audio_MachineInfo*, psy_ui_RealPoint cp, bool sel);
static void pluginsview_on_align(PluginsView*);
static void pluginsview_on_preferred_size(PluginsView*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void pluginsview_on_key_down(PluginsView*, psy_ui_KeyboardEvent*);
static bool pluginsview_on_input(PluginsView*, InputHandler* sender);
static bool pluginsview_handle_command(PluginsView*, psy_EventDriverCmd );
static void pluginsview_on_mouse_down(PluginsView*, psy_ui_MouseEvent*);
static void pluginsview_on_mouse_up(PluginsView*, psy_ui_MouseEvent*);
static void pluginsview_on_mouse_double_click(PluginsView*, psy_ui_MouseEvent*);
static uintptr_t pluginsview_px_to_index(PluginsView*, psy_ui_RealPoint);
static uintptr_t pluginsview_num_lines(const PluginsView*);
static void pluginsview_on_filter_changed(PluginsView*, PluginFilter* sender);
static void pluginsview_filter(PluginsView*);
static void pluginsview_prev_items(PluginsView*, uintptr_t num_items);
static void pluginsview_advance_items(PluginsView*, uintptr_t num_items);
static uintptr_t pluginsview_num_items(const PluginsView*);
static uintptr_t pluginsview_num_columns(PluginsView* self, psy_ui_Value width);
static psy_ui_RealSize pluginsview_item_size_px(const PluginsView*);

/* vtable */
static psy_ui_ComponentVtable pluginsview_vtable;
static psy_ui_ComponentVtable pluginsview_super_vtable;
static bool pluginsview_vtable_initialized = FALSE;

static void pluginsview_vtable_init(PluginsView* self)
{
	assert(self);
	
	if (!pluginsview_vtable_initialized) {
		pluginsview_vtable = *(self->component.vtable);
		pluginsview_super_vtable = pluginsview_vtable;
		pluginsview_vtable.on_destroyed =
			(psy_ui_fp_component)
			pluginsview_on_destroyed;
		pluginsview_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			pluginsview_on_draw;
		pluginsview_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			pluginsview_on_key_down;
		pluginsview_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			pluginsview_on_mouse_down;
		pluginsview_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			pluginsview_on_mouse_up;
		pluginsview_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			pluginsview_on_mouse_double_click;
		pluginsview_vtable.onalign =
			(psy_ui_fp_component)
			pluginsview_on_align;
		pluginsview_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			pluginsview_on_preferred_size;
		pluginsview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(pluginsview_base(self), &pluginsview_vtable);
}

/* implementation */
void pluginsview_init(PluginsView* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	pluginsview_vtable_init(self);	
	self->filter = NULL;	
	self->num_cols_ = 1;
	self->item_size_ = psy_ui_size_make_em(42.0, 1.5);
	self->item_size_px_ = psy_ui_realsize_make(100, 12);
	self->scroll_dir = 1;
	psy_signal_init(&self->signal_selected);
	psy_signal_init(&self->signal_changed);		
	psy_audio_plugins_init(&self->plugins_);
	psy_audio_plugins_init(&self->curr_plugins_);		
	psy_ui_component_set_tab_index(&self->component, 0);	
	newmachineselection_init(&self->selection);	
	pluginsview_connect_input_handler(self, psy_ui_app_input_handler(
		psy_ui_app()));
}

void pluginsview_on_destroyed(PluginsView* self)
{	
	assert(self);
	
	psy_signal_dispose(&self->signal_selected);
	psy_signal_dispose(&self->signal_changed);
	pluginsview_clear(self);
	if (self->filter) {
		psy_signal_disconnect(&self->filter->signal_changed, self,
			pluginsview_on_filter_changed);
	}
	newmachineselection_dispose(&self->selection);
	psy_audio_plugins_dispose(&self->plugins_);
	psy_audio_plugins_dispose(&self->curr_plugins_);	
}

void pluginsview_connect_input_handler(PluginsView* self, InputHandler* input_handler)
{
	assert(self);

	if (!input_handler) {
		return;
	}
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, &self->component, (fp_inputhandler_input)pluginsview_on_input);
}

void pluginsview_clear(PluginsView* self)
{	
	assert(self);
	
	psy_audio_plugins_clear(&self->plugins_);	
	pluginsview_reset_current(self);
}

void pluginsview_reset_current(PluginsView* self)
{		
	assert(self);
	
	newmachineselection_clear(&self->selection);	
	psy_audio_plugins_clear(&self->curr_plugins_);
	psy_audio_plugins_copy(&self->curr_plugins_, &self->plugins_);	
}

void pluginsview_set_plugins(PluginsView* self, const psy_audio_Plugins*
	plugins)
{
	assert(self);
	
	pluginsview_clear(self);
	if (plugins) {		
		psy_audio_plugins_copy(&self->plugins_, plugins);
	}
	pluginsview_reset_current(self);
	psy_ui_component_invalidate(&self->component);
}

void pluginsview_set_filter(PluginsView* self, PluginFilter* filter)
{	
	assert(self);
	
	if (self->filter) {
		psy_signal_disconnect(&self->filter->signal_changed, self,
			pluginsview_on_filter_changed);
	}
	self->filter = filter;
	if (self->filter) {
		psy_signal_connect(&self->filter->signal_changed, self,
			pluginsview_on_filter_changed);
	}
	pluginsview_reset_current(self);
}

void pluginsview_on_draw(PluginsView* self, psy_ui_Graphics* g)
{	
	psy_ui_RealRectangle clip;		
	uintptr_t index;	
	psy_ui_RealPoint cp;
	const psy_List* p;
	uintptr_t curr_col;
		
	assert(self);	
				
	clip = psy_ui_graphics_cliprect(g);
	index = pluginsview_px_to_index(self, psy_ui_realpoint_make(0.0, clip.top));
	if (index == psy_INDEX_INVALID) {
		return;
	}
	psy_ui_realpoint_init_all(&cp, 0.0,
		floor(clip.top / self->item_size_px_.height) *
			self->item_size_px_.height);
	p = psy_list_at_const(psy_audio_plugins_begin(&self->curr_plugins_), index);
	curr_col  = 0;
	for (; p != NULL; psy_list_next_const(&p), ++index) {			
		pluginsview_draw_item(self, g, (const psy_audio_MachineInfo*)
			psy_list_entry_const(p), cp,
			newmachineselection_is_selected(&self->selection, index));
		cp.x += self->item_size_px_.width;
		++curr_col;
		if (curr_col >= self->num_cols_) {
			cp = psy_ui_realpoint_make(0.0, cp.y + self->item_size_px_.height);
			if (cp.y > clip.bottom) {
				break;
			}
			curr_col = 0;
		}			
	}
}

void pluginsview_draw_item(PluginsView* self, psy_ui_Graphics* g,
	const psy_audio_MachineInfo* plugin, psy_ui_RealPoint cp, bool sel)
{
	char text[128];	
	psy_ui_Style* itemstyle;
	psy_ui_Colour bgcolour;
	const psy_ui_TextMetric* tm;

	assert(self);
	
	if (sel) {
		itemstyle = psy_ui_style(psy_ui_STYLE_LIST_ITEM_SELECT);
	} else {
		itemstyle = psy_ui_style(psy_ui_STYLE_LIST_ITEM);
	}
	if (itemstyle->background.colour.mode.transparent) {
		bgcolour = psy_ui_component_background_colour(&self->component);
	} else {
		bgcolour = psy_ui_style_background_colour(itemstyle);
	}
	if (!itemstyle->colour.mode.transparent) {
		psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(itemstyle));
	} else {
		psy_ui_graphics_set_text_colour(g, psy_ui_component_colour(&self->component));
	}
	if (!itemstyle->background.colour.mode.transparent) {
		psy_ui_graphics_draw_solid_rectangle(g,
			psy_ui_realrectangle_make(
				cp,
				psy_ui_realsize_make(self->item_size_px_.width - 5,
				self->item_size_px_.height)),
			bgcolour);
	}	
	plugindisplayname(plugin, text);	
	psy_ui_graphics_textout(g, psy_ui_realpoint_make(cp.x, cp.y + 2), text,
		psy_strlen(text));
	plugintype(plugin, text);
	tm = psy_ui_component_textmetric(&self->component);
	psy_ui_graphics_textout(g, psy_ui_realpoint_make(cp.x +
		self->item_size_px_.width - tm->tmAveCharWidth * 7, cp.y + 2),
		text, psy_strlen(text));
	if (pluginmode(plugin, text) == psy_audio_MACHMODE_FX) {
		psy_ui_graphics_set_text_colour(g, psy_ui_colour_make(0x00B1C8B0));
	} else {		
		psy_ui_graphics_set_text_colour(g, psy_ui_colour_make(0x00D1C5B6));
	}	
	psy_ui_graphics_textout(g, psy_ui_realpoint_make(cp.x +
		self->item_size_px_.width - tm->tmAveCharWidth * 10.0, cp.y + 2),
		text, psy_strlen(text));
}

void plugindisplayname(const psy_audio_MachineInfo* plugin, char* text)
{	
	const char* label;

	label = plugin->shortname;
	if (strcmp(label, "") == 0) {
		label = "unknown";
	}
	psy_snprintf(text, 128, "%s", label);
}

uintptr_t plugintype(const psy_audio_MachineInfo* plugin, char* text)
{	
	uintptr_t rv;
	
	rv = (uintptr_t)plugin->type;
	switch (rv) {
		case psy_audio_PLUGIN:
			strcpy(text, "psy");
		break;
		case psy_audio_LUA:
			strcpy(text, "lua");
		break;
		case psy_audio_VST:
			strcpy(text, "vst");
		break;
		case psy_audio_VSTFX:
			strcpy(text, "vst");
		break;
		case psy_audio_LADSPA:
			strcpy(text, "lad");
			break;
		case psy_audio_LV2:
			strcpy(text, "lv2");
			break;
		default:
			strcpy(text, "int");
		break;
	}
	return rv;
}

uintptr_t pluginmode(const psy_audio_MachineInfo* plugin, char* text)
{			
	uintptr_t rv;

	rv = (uintptr_t)plugin->mode;
	strcpy(text, (rv == psy_audio_MACHMODE_FX) ? "fx" : "gn");
	return rv;
}

void pluginsview_on_align(PluginsView* self)
{		
	psy_ui_Size size;
	
	assert(self);
		
	self->item_size_px_ = pluginsview_item_size_px(self);	
	size = psy_ui_component_scroll_size(&self->component);
	self->num_cols_ = pluginsview_num_columns(self, size.width);	
}

void pluginsview_on_preferred_size(PluginsView* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{		
	assert(self);
		
	if (limit) {
		rv->width = limit->width;			
	} else {
		rv->width = psy_ui_value_make_px(65535.0);
	}
	self->num_cols_ = pluginsview_num_columns(self, rv->width);
	rv->height = psy_ui_mul_value_real(self->item_size_.height,
		(double)pluginsview_num_lines(self));	
}

uintptr_t pluginsview_num_columns(PluginsView* self, psy_ui_Value width)
{
	const psy_ui_TextMetric* tm;	
		
	assert(self);
	
	tm = psy_ui_component_textmetric(&self->component);		
	return (uintptr_t)(psy_ui_value_px(&width, tm, NULL) /
		psy_ui_value_px(&self->item_size_.width, tm, NULL));
}

void pluginsview_on_key_down(PluginsView* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_CONTROL) {
		psy_ui_keyboardevent_stop_propagation(ev);		
	}
	if (pluginsview_num_items(self) == 0) {
		return;
	}
	if (!newmachineselection_valid(&self->selection)) {
		newmachineselection_single_select(&self->selection, 0);
		psy_ui_component_invalidate(&self->component);
		psy_signal_emit(&self->signal_changed, self, 0);
		return;
	}
	switch (psy_ui_keyboardevent_keycode(ev)) {
	case psy_ui_KEY_A:
		if (psy_ui_keyboardevent_ctrl_key(ev)) {
			newmachineselection_select_all(&self->selection,
				pluginsview_num_items(self));
			psy_ui_component_invalidate(&self->component);
			psy_signal_emit(&self->signal_changed, self, 0);
		}
		break;
	case psy_ui_KEY_RETURN:
		psy_signal_emit(&self->signal_selected, self, 0);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;	
	case psy_ui_KEY_PRIOR:
		pluginsview_prev_items(self, self->num_cols_ * 4);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;
	case psy_ui_KEY_NEXT:
		pluginsview_advance_items(self, self->num_cols_ * 4);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;
	case psy_ui_KEY_LEFT:
		pluginsview_prev_items(self, 1);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;
	case psy_ui_KEY_RIGHT:
		pluginsview_advance_items(self, 1);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;
	default:
		break;
	}
}

bool pluginsview_on_input(PluginsView* self, InputHandler* sender)
{
	assert(self);

	return pluginsview_handle_command(self, inputhandler_cmd(sender));
}

bool pluginsview_handle_command(PluginsView* self, psy_EventDriverCmd cmd)
{
	assert(self);
		
	switch (psy_eventdrivercmd_id(&cmd)) {	
	case CMD_NAVDOWN:
		pluginsview_advance_items(self, self->num_cols_);
		return TRUE;
	case CMD_NAVSELECT:
		psy_signal_emit(&self->signal_selected, self, 0);
		return TRUE;
	case CMD_NAVUP:
		pluginsview_prev_items(self, self->num_cols_);		
		return TRUE;
	/* case psy_ui_KEY_PRIOR:
		pluginsview_prev_items(self, self->num_cols_ * 4);
		psy_ui_keyboardevent_stop_propagation(ev);
		break;
	case psy_ui_KEY_NEXT:
		pluginsview_advance_items(self, self->num_cols_ * 4);
		psy_ui_keyboardevent_stop_propagation(ev);
		break; */
	case CMD_NAVLEFT:
		pluginsview_prev_items(self, 1);		
		return TRUE;
	case CMD_NAVRIGHT:
		pluginsview_advance_items(self, 1);		
		return TRUE;
	default:
		return FALSE;
	}
}

void pluginsview_prev_items(PluginsView* self, uintptr_t num_items)	
{
	assert(self);
	
	if (pluginsview_num_items(self) == 0) {
		return;
	}
	if (newmachineselection_first(&self->selection) >= num_items) {
		uintptr_t index;
					
		index = newmachineselection_first(&self->selection) - num_items;													
		newmachineselection_single_select(&self->selection, index);
		psy_ui_component_invalidate(&self->component);
		self->scroll_dir = -(intptr_t)(num_items);
		psy_signal_emit(&self->signal_changed, self, 0);
	}
}

void pluginsview_advance_items(PluginsView* self, uintptr_t num_items)
{	
	assert(self);
	
	if (pluginsview_num_items(self) == 0) {
		return;
	}
	if (newmachineselection_first(&self->selection) + num_items <
			pluginsview_num_items(self)) {
		newmachineselection_single_select(&self->selection,
			newmachineselection_first(&self->selection) + num_items);
		self->scroll_dir = num_items;
		psy_ui_component_invalidate(&self->component);
		psy_signal_emit(&self->signal_changed, self, 0);
	}
}

uintptr_t pluginsview_num_lines(const PluginsView* self)
{
	assert(self);
	
	if (self->num_cols_ == 0) {
		return 1;
	}
	return (psy_audio_plugins_size(&self->curr_plugins_) / self->num_cols_) + 1;	
}

uintptr_t pluginsview_num_items(const PluginsView* self)
{
	assert(self);
	
	return psy_audio_plugins_size(&self->curr_plugins_);
}

void pluginsview_on_mouse_down(PluginsView* self, psy_ui_MouseEvent* ev)
{	
	assert(self);
	
	self->multidrag = FALSE;
	if (psy_ui_mouseevent_button(ev) == 1) {
		uintptr_t index;		
		
		index = pluginsview_px_to_index(self, psy_ui_mouseevent_pt(ev));
		if (index != psy_INDEX_INVALID) {			
			if (psy_ui_mouseevent_ctrl_key(ev)) {
				newmachineselection_toggle(&self->selection, index);
			} else {
				if (psy_list_size(self->selection.items) > 1 &&
					newmachineselection_is_selected(&self->selection, index)) {
					self->dragpt = psy_ui_mouseevent_pt(ev);
					self->multidrag = TRUE;
					self->dragindex = index;
				} else {
					newmachineselection_single_select(&self->selection, index);
				}
			}			
			psy_ui_component_invalidate(&self->component);
			psy_signal_emit(&self->signal_changed, self, 0);
			psy_ui_component_set_focus(&self->component);
		}
	}
	pluginsview_super_vtable.on_mouse_down(&self->component, ev);
}

void pluginsview_on_mouse_up(PluginsView* self, psy_ui_MouseEvent* ev)
{
	psy_ui_RealPoint pt;
	
	assert(self);

	pt = psy_ui_mouseevent_pt(ev);
	if (self->multidrag && psy_ui_realpoint_equal(&self->dragpt, &pt)) {
		newmachineselection_single_select(&self->selection, self->dragindex);
		psy_ui_component_invalidate(&self->component);
		psy_signal_emit(&self->signal_changed, self, 0);
		psy_ui_component_set_focus(&self->component);
	}
}

uintptr_t pluginsview_px_to_index(PluginsView* self, psy_ui_RealPoint pt)
{	
	uintptr_t index;
		
	assert(self);	
			
	index = (uintptr_t)floor(pt.y / self->item_size_px_.height) * self->num_cols_ +
		(uintptr_t)floor(pt.x / self->item_size_px_.width);
	if (index < pluginsview_num_items(self)) {
		return index;
	}	
	return psy_INDEX_INVALID;	
}

double pluginsview_index_to_px(PluginsView* self, uintptr_t index)
{		
	assert(self);	
			
	if (index != psy_INDEX_INVALID) {
		return (index / self->num_cols_) * self->item_size_px_.height;
	}
	return 0;
}

psy_ui_RealSize pluginsview_item_size_px(const PluginsView* self)
{
	assert(self);
	
	return psy_ui_size_px(&self->item_size_, psy_ui_component_textmetric(
		&self->component), NULL);
}

void pluginsview_on_mouse_double_click(PluginsView* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if ((psy_ui_mouseevent_button(ev) == 1) &&
			newmachineselection_valid(&self->selection)) {
		psy_signal_emit(&self->signal_selected, self, 0);		
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void pluginsview_on_filter_changed(PluginsView* self, PluginFilter* sender)
{	
	assert(self);
			
	pluginsview_filter(self);
}

void pluginsview_filter(PluginsView* self)
{
	assert(self);
	
	if (self->filter) {
		psy_audio_Plugins sort;
		
		psy_audio_plugins_filter(&self->plugins_, self->filter,
			&self->curr_plugins_);
		psy_audio_plugins_init(&sort);
		psy_audio_plugins_copy(&sort, &self->curr_plugins_);
		psy_audio_plugins_sort(&sort, self->filter, &self->curr_plugins_);
		psy_audio_plugins_dispose(&sort);
		psy_signal_emit(&self->signal_changed, self, 0);		
		psy_ui_component_align_full(psy_ui_component_parent(
			psy_ui_component_parent(&self->component)));
		psy_ui_component_update_overflow(psy_ui_component_parent(
			psy_ui_component_parent(&self->component)));		
		psy_ui_component_invalidate(psy_ui_component_parent(
			psy_ui_component_parent(&self->component)));
	}
}

const psy_audio_MachineInfo* pluginsview_selected(PluginsView* self)
{	
	assert(self);

	return pluginsview_plugin(self, newmachineselection_first(
		&self->selection));	
}

const psy_audio_MachineInfo* pluginsview_plugin(const PluginsView* self,
	uintptr_t index)
{
	assert(self);

	if (index != psy_INDEX_INVALID) {
		return psy_audio_plugins_at_const(&self->curr_plugins_, index);
	}
	return NULL;
}
