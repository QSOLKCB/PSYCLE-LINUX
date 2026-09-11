/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uilistbox.h"
/* local */
#include "uiapp.h"
#include "trackercmds.h"
/* platform */
#include "../../detail/portable.h"


/* ListBoxItem */

/* implementation */
void psy_ui_listitem_init(psy_ui_ListItem* self, const char* text)
{
	assert(self);
	
	if (text) {
		self->text = psy_strdup(text);
	} else {
		self->text = psy_strdup("");
	}
	self->selected = FALSE;
	self->id = psy_INDEX_INVALID;
}

void psy_ui_listitem_dispose(psy_ui_ListItem* self)
{
	assert(self);
	
	free(self->text);
	self->text = NULL;
}

psy_ui_ListItem* psy_ui_listitem_alloc(void)
{
	return (psy_ui_ListItem*)malloc(sizeof(psy_ui_ListItem));
}

psy_ui_ListItem* psy_ui_listitem_alloc_init(const char* text)
{
	psy_ui_ListItem* rv;
	
	rv = psy_ui_listitem_alloc();
	if (rv) {
		psy_ui_listitem_init(rv, text);
	}
	return rv;
}

void psy_ui_listitem_set_text(psy_ui_ListItem* self, const char* text)
{
	assert(self);
	
	psy_strreset(&self->text, text);
}


/* ListBoxPane */

/* prototypes*/
static void psy_ui_listboxpane_on_destroyed(psy_ui_ListBoxPane*);
static void psy_ui_listboxpane_on_draw(psy_ui_ListBoxPane*,
	psy_ui_Graphics*);
static void psy_ui_listboxpane_on_preferred_size(psy_ui_ListBoxPane*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static void psy_ui_listboxpane_on_mouse_down(psy_ui_ListBoxPane*,
	psy_ui_MouseEvent*);
static void psy_ui_listboxpane_on_size(psy_ui_ListBoxPane*);
static uintptr_t psy_ui_listboxpane_count(const psy_ui_ListBoxPane*);
static bool psy_ui_listboxpane_on_input(psy_ui_ListBoxPane*,
	InputHandler* sender);
static bool psy_ui_listboxpane_handle_command(psy_ui_ListBoxPane*,
	psy_EventDriverCmd);


/* vtable */
static psy_ui_ComponentVtable psy_ui_listboxpane_vtable;
static bool psy_ui_listboxpane_vtable_initialized = FALSE;

static void psy_ui_listboxpane_vtable_init(psy_ui_ListBoxPane* self)
{
	if (!psy_ui_listboxpane_vtable_initialized) {
		psy_ui_listboxpane_vtable = *(self->component.vtable);
		psy_ui_listboxpane_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_listboxpane_on_destroyed;
		psy_ui_listboxpane_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_listboxpane_on_draw;
		psy_ui_listboxpane_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_listboxpane_on_preferred_size;
		psy_ui_listboxpane_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_listboxpane_on_mouse_down;
		psy_ui_listboxpane_vtable.onsize =
			(psy_ui_fp_component)
			psy_ui_listboxpane_on_size;
		psy_ui_listboxpane_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_listboxpane_vtable);
}

/* implementation */
void psy_ui_listboxpane_init(psy_ui_ListBoxPane* self, psy_ui_Component*
	parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_listboxpane_vtable_init(self);	
	psy_table_init(&self->items);
	self->selindex = psy_INDEX_INVALID;
	self->charnumber = 0;
	self->multiselect = FALSE;
	psy_signal_init(&self->signal_selchanged);
	psy_ui_component_set_wheel_scroll(&self->component, 4);
	psy_ui_component_set_overflow(&self->component, psy_ui_OVERFLOW_VSCROLL);
	psy_ui_component_set_style_type_focus(&self->component, psy_ui_STYLE_FOCUS);
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS, psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_listboxpane_on_input);
}

void psy_ui_listboxpane_on_destroyed(psy_ui_ListBoxPane* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_selchanged);
	psy_table_dispose_all(&self->items, (psy_fp_disposefunc)
		psy_ui_listitem_dispose);
	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),		
		self, psy_ui_listboxpane_on_input);
}

void psy_ui_listboxpane_on_size(psy_ui_ListBoxPane* self)
{
	const psy_ui_TextMetric* tm;
	intptr_t line_height;

	assert(self);
	
	tm = psy_ui_component_textmetric(&self->component);
	/*! @todo */
	line_height = (intptr_t)(tm->tmHeight * 1.2);
	psy_ui_component_set_scroll_step_height(&self->component,
		psy_ui_value_make_px((double)line_height));
}

void psy_ui_listboxpane_on_draw(psy_ui_ListBoxPane* self,
	psy_ui_Graphics* g)
{
	uintptr_t i;
	const psy_ui_TextMetric* tm;
	psy_ui_RealSize size;
	psy_ui_RealPoint cp;	
	double line_height;
	psy_ui_Style* style;
	psy_ui_Colour colour;

	assert(self);
	
	tm = psy_ui_component_textmetric(&self->component);
	size = psy_ui_component_scroll_size_px(&self->component);
	line_height = floor(tm->tmHeight * 1.2);
	cp = psy_ui_realpoint_zero();
	style = psy_ui_style(psy_ui_STYLE_LIST_ITEM);
	colour = psy_ui_component_colour(&self->component);
	for (i = 0; i < psy_table_size(&self->items); ++i) {
		psy_ui_ListItem* item;
		
		item = psy_ui_listboxpane_item(self, i);
		if (!item) {
			continue;
		}
		if (item->selected) {
			style = psy_ui_style(psy_ui_STYLE_LIST_ITEM_SELECT);
			psy_ui_graphics_draw_solid_rectangle(g,
				psy_ui_realrectangle_make(cp,
					psy_ui_realsize_make(size.width, line_height)),
				psy_ui_style_background_colour(style));
			psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(style));
			psy_ui_graphics_textout(g, cp, psy_ui_listitem_text(item),
				psy_strlen(psy_ui_listitem_text(item)));
			style = psy_ui_style(psy_ui_STYLE_LIST_ITEM);
			psy_ui_graphics_set_text_colour(g, colour);
		} else {
			psy_ui_graphics_textout(g, cp, psy_ui_listitem_text(item),
				psy_strlen(psy_ui_listitem_text(item)));
		}
		cp.y += line_height;
	}
}

void psy_ui_listboxpane_on_preferred_size(psy_ui_ListBoxPane* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{
	const psy_ui_TextMetric* tm;

	tm = psy_ui_component_textmetric(&self->component);
	rv->width = (self->charnumber == 0)
		? psy_ui_value_make_px(tm->tmAveCharWidth * 40)
		: psy_ui_value_make_px(tm->tmAveCharWidth * self->charnumber);
	rv->height = psy_ui_value_make_px((intptr_t)(tm->tmHeight * 1.2) *
		(double)psy_ui_listboxpane_count(self));
}

void psy_ui_listboxpane_on_mouse_down(psy_ui_ListBoxPane* self,
	psy_ui_MouseEvent* ev)
{
	const psy_ui_TextMetric* tm;
	uintptr_t index;
	double line_height;

	assert(self);
	
	tm = psy_ui_component_textmetric(&self->component);
	line_height = floor(tm->tmHeight * 1.2);	
	if (psy_ui_mouseevent_pt(ev).y / line_height > 0.0) {
		index = (uintptr_t)(psy_ui_mouseevent_pt(ev).y / line_height);
	} else {
		index = psy_INDEX_INVALID;
	}
	if (index < psy_table_size(&self->items)) {
		if (self->multiselect) {
			if ((self->selindex == psy_INDEX_INVALID) ||
					(!psy_ui_mouseevent_ctrl_key(ev))) {
				psy_ui_listboxpane_set_cur_sel(self, psy_INDEX_INVALID);
				psy_ui_listboxpane_set_cur_sel(self, index);
			} else {
				psy_ui_ListItem* item;
															
				item = psy_ui_listboxpane_item(self, index);
				if (item) {					
					psy_ui_listitem_select(item);
				}
				psy_ui_component_invalidate(&self->component);
			}					
		} else {
			psy_ui_listboxpane_set_cur_sel(self, index);		
		}
		psy_signal_emit(&self->signal_selchanged, self, 0);
	}
}

void psy_ui_listboxpane_clear(psy_ui_ListBoxPane* self)
{
	self->selindex = psy_INDEX_INVALID;
	psy_table_dispose_all(&self->items,
		(psy_fp_disposefunc)psy_ui_listitem_dispose);
	psy_table_init(&self->items);
	psy_ui_component_update_overflow(&self->component);
	psy_ui_component_invalidate(&self->component);
}

uintptr_t psy_ui_listboxpane_add_text(psy_ui_ListBoxPane* self,
	const char* text)
{
	assert(self);
	
	psy_table_insert(&self->items, psy_table_size(&self->items),
		psy_ui_listitem_alloc_init(text));		
	psy_ui_component_update_overflow(&self->component);
	psy_ui_component_invalidate(&self->component);
	return psy_table_size(&self->items) - 1;
}

void psy_ui_listboxpane_set_text(psy_ui_ListBoxPane* self, const char* text,
	uintptr_t index)
{
	psy_ui_ListItem* item;
	
	assert(self);
		
	item = psy_ui_listboxpane_item(self, index);
	if (item) {
		psy_ui_listitem_set_text(item, text);
	}
}

void psy_ui_listboxpane_set_cur_sel(psy_ui_ListBoxPane* self, uintptr_t index)
{	
	assert(self);
	
	if (index == psy_INDEX_INVALID) {
		uintptr_t i;		
				
		for (i = 0; i < psy_table_size(&self->items); ++i) {
			psy_ui_ListItem* item;
			
			item = psy_ui_listboxpane_item(self, i);
			if (!item) {
				continue;
			}
			item->selected = FALSE;					
		}
	} else {
		psy_ui_ListItem* item;
		
		item = psy_ui_listboxpane_item(self, self->selindex);
		if (item) {
			psy_ui_listitem_deselect(item);
		}
		self->selindex = index;
		item = psy_ui_listboxpane_item(self, self->selindex);
		if (item) {
			psy_ui_listitem_select(item);
		}
	}
	psy_ui_component_invalidate(&self->component);
}

uintptr_t psy_ui_listboxpane_cur_sel(const psy_ui_ListBoxPane* self)
{
	assert(self);
	
	return self->selindex;
}

void psy_ui_listboxpane_set_char_number(psy_ui_ListBoxPane* self, double num)
{
	assert(self);
	
	self->charnumber = num;
}

uintptr_t psy_ui_listboxpane_count(const psy_ui_ListBoxPane* self)
{
	assert(self);
	
	return psy_table_size(&self->items);
}

psy_ui_ListItem* psy_ui_listboxpane_item(psy_ui_ListBoxPane* self,
	uintptr_t index)
{	
	assert(self);
	
	return (psy_ui_ListItem*)psy_table_at(&self->items, index);
}

const psy_ui_ListItem* psy_ui_listboxpane_item_const(
	const psy_ui_ListBoxPane* self, uintptr_t index)
{	
	assert(self);
	
	return (const psy_ui_ListItem*)psy_table_at_const(&self->items, index);
}

bool psy_ui_listboxpane_on_input(psy_ui_ListBoxPane* self, InputHandler* sender)
{
	assert(self);

	return psy_ui_listboxpane_handle_command(self, inputhandler_cmd(sender));
}

bool psy_ui_listboxpane_handle_command(psy_ui_ListBoxPane* self, psy_EventDriverCmd cmd)
{
	assert(self);

	switch (psy_eventdrivercmd_id(&cmd)) {
	case CMD_NAVTOP:
		
		return TRUE;
	case CMD_NAVBOTTOM:
		
		return TRUE;
	case CMD_NAVUP:
		psy_ui_listboxpane_set_cur_sel(self,
			psy_ui_listboxpane_cur_sel(self) - 1);
		return TRUE;
	case CMD_NAVPAGEUP:		
		return TRUE;
	case CMD_NAVDOWN: {
		psy_ui_listboxpane_set_cur_sel(self,
			psy_ui_listboxpane_cur_sel(self) + 1);
		return TRUE; }
	case CMD_NAVPAGEDOWN:		
		return TRUE;		
	case CMD_SELECTMACHINE:		
		return TRUE;
	default:
		return FALSE;
	}
}



/* psy_ui_ListBox */

/* prototypes */
static void psy_ui_listbox_on_destroyed(psy_ui_ListBox*);
static void psy_ui_listbox_on_preferred_size(psy_ui_ListBox*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static void psy_ui_listbox_on_sel_changed(psy_ui_ListBox*,
	psy_ui_ListBoxPane* sender);

/* vtable */
static psy_ui_ComponentVtable psy_ui_listbox_vtable;
static bool psy_ui_listbox_vtable_initialized = FALSE;

static void psy_ui_listbox_vtable_init(psy_ui_ListBox* self)
{
	assert(self);
	
	if (!psy_ui_listbox_vtable_initialized) {
		psy_ui_listbox_vtable = *(self->component.vtable);
		psy_ui_listbox_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_listbox_on_destroyed;
		psy_ui_listbox_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_listbox_on_preferred_size;		
		psy_ui_listbox_vtable_initialized = TRUE;
	}
	self->component.vtable = &psy_ui_listbox_vtable;
}

/* implementation */
void psy_ui_listbox_init(psy_ui_ListBox* self, psy_ui_Component* parent)
{  
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_listbox_vtable_init(self);
	self->charnumber = 0.0;
	psy_ui_component_set_style_type(psy_ui_listbox_base(self),
		psy_ui_STYLE_LISTBOX);
	psy_signal_init(&self->signal_selchanged);
	psy_ui_listboxpane_init(&self->pane, &self->component);
	psy_ui_scroller_init(&self->scroller, &self->component, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->pane.component);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->pane.signal_selchanged, self,
		psy_ui_listbox_on_sel_changed);
}

void psy_ui_listbox_init_multi_select(psy_ui_ListBox* self, psy_ui_Component*
	parent)
{  
	assert(self);
	
	psy_ui_listbox_init(self, parent);
	self->pane.multiselect = TRUE;
}

psy_ui_ListBox* psy_ui_listbox_alloc(void)
{
	return (psy_ui_ListBox*)malloc(sizeof(psy_ui_ListBox));
}

psy_ui_ListBox* psy_ui_listbox_alloc_init(psy_ui_Component* parent)
{
	psy_ui_ListBox* rv;

	rv = psy_ui_listbox_alloc();
	if (rv) {
		psy_ui_listbox_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_listbox_on_destroyed(psy_ui_ListBox* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_selchanged);
}

uintptr_t psy_ui_listbox_add_text(psy_ui_ListBox* self, const char* text)
{	
	return psy_ui_listboxpane_add_text(&self->pane, text);
}

void psy_ui_listbox_set_text(psy_ui_ListBox* self, const char* text,
	uintptr_t index)
{	
	assert(self);
	
	if (index >= psy_ui_listbox_count(self)) {
		uintptr_t i;

		for (i = psy_ui_listbox_count(self); i <= index; ++i) {
			psy_ui_listbox_add_text(self, "");
		}
	}
	psy_ui_listboxpane_set_text(&self->pane, text, index);
}

void psy_ui_listbox_clear(psy_ui_ListBox* self)
{	
	assert(self);
	
	psy_ui_listboxpane_clear(&self->pane);
}

void psy_ui_listbox_set_cur_sel(psy_ui_ListBox* self, uintptr_t index)
{	
	assert(self);
	
	psy_ui_listboxpane_set_cur_sel(&self->pane, index);
}

uintptr_t psy_ui_listbox_cur_sel(const psy_ui_ListBox* self)
{	
	assert(self);
		
	return psy_ui_listboxpane_cur_sel(&self->pane);
}

void psy_ui_listbox_add_sel(psy_ui_ListBox* self, uintptr_t index)
{		
	psy_ui_ListItem* item;
	
	assert(self);
	
	item = psy_ui_listboxpane_item(&self->pane, index);
	if (item) {
		psy_ui_listitem_select(item);	
	}
}

void psy_ui_listbox_sel_items(const psy_ui_ListBox* self, uintptr_t* items,
	uintptr_t maxitems)
{		
	uintptr_t i;		
	uintptr_t j;
	
	assert(self);
	
	j = 0;
	for (i = 0; j < maxitems && i < psy_table_size(&self->pane.items); ++i) {
		const psy_ui_ListItem* item;
		
		item = psy_ui_listboxpane_item_const(&self->pane, i);
		if (!item) {
			continue;
		}
		if (item->selected) {
			items[j] = i;
			++j;
		}		
	}
}

uintptr_t psy_ui_listbox_sel_count(const psy_ui_ListBox* self)
{		
	uintptr_t i;		
	uintptr_t j;
	
	j = 0;
	for (i = 0; i < psy_table_size(&self->pane.items); ++i) {
		const psy_ui_ListItem* item;
		
		item = psy_ui_listboxpane_item_const(&self->pane, i);
		if (!item) {
			continue;
		}
		if (item->selected) {
			++j;
		}		
	}
	return j;
}

uintptr_t psy_ui_listbox_count(const psy_ui_ListBox* self)
{	
	assert(self);
	
	return psy_ui_listboxpane_count(&self->pane);
}

psy_ui_ListItem* psy_ui_listbox_at(psy_ui_ListBox* self, uintptr_t index)
{
	assert(self);

	return psy_ui_listboxpane_item(&self->pane, index);
}

const psy_ui_ListItem* psy_ui_listbox_at_const(const psy_ui_ListBox* self,
	uintptr_t index)
{
	assert(self);

	return psy_ui_listbox_at((psy_ui_ListBox*)(self), index);
}

void psy_ui_listbox_set_char_number(psy_ui_ListBox* self, double num)
{
	assert(self);
	
	self->charnumber = num;
}

void psy_ui_listbox_on_preferred_size(psy_ui_ListBox* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{
	const psy_ui_TextMetric* tm;
	
	assert(self);
	
	tm = psy_ui_component_textmetric(&self->component);
	rv->width = (self->charnumber == 0)
		? psy_ui_value_make_px(tm->tmAveCharWidth * 40) // tm.tmAveCharWidth
		: psy_ui_value_make_px(tm->tmAveCharWidth * self->charnumber); // 
	rv->height = psy_ui_value_make_px((tm->tmHeight * 1.2) *
		psy_ui_listbox_count(self));	
}

void psy_ui_listbox_on_sel_changed(psy_ui_ListBox* self,
	psy_ui_ListBoxPane* sender)
{	
	assert(self);
		
	psy_signal_emit(&self->signal_selchanged, self, 0);
}

void psy_ui_listbox_text(psy_ui_ListBox* self, char* text, uintptr_t index)
{
	psy_ui_ListItem* item;
	
	assert(self);
	
	item = (psy_ui_ListItem*)psy_table_at(&self->pane.items, index);
	if (!item) {
		psy_snprintf(text, 256, "%s", "");
	} else {
		psy_snprintf(text, 256, "%s", psy_ui_listitem_text(item));
	}
}
