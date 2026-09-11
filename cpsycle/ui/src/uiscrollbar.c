/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiscrollbar.h"
/* local */
#include "uiapp.h"
/* std */
#include <math.h>

#define REPEAT_TIMEINTERVAL 50
#define REPEAT_DELAYCOUNT 15
#define REPEAT_DELAYFIRSTCOUNT 5

/* psy_ui_ScrollBarPane */
/* prototypes */
static void psy_ui_scrollbarpane_on_destroyed(psy_ui_ScrollBarPane*);
static void psy_ui_scrollbarpane_on_mouse_down(psy_ui_ScrollBarPane*,
	psy_ui_MouseEvent*);
static void psy_ui_scrollbarpane_on_mouse_up(psy_ui_ScrollBarPane*,
	psy_ui_MouseEvent*);
static void psy_ui_scrollbarpane_on_mouse_move(psy_ui_ScrollBarPane*,
	psy_ui_MouseEvent*);
static double psy_ui_scrollbarpane_step(psy_ui_ScrollBarPane*);
static double psy_ui_scrollbarpane_position_in_scroll_range(
	const psy_ui_ScrollBarPane*, double pos);
static void psy_ui_scrollbarpane_update_thumb_position(psy_ui_ScrollBarPane*);
static void psy_ui_scrollbarpane_on_timer(psy_ui_ScrollBarPane*, uintptr_t id);

/* vtable */
static psy_ui_ComponentVtable psy_ui_scrollbarpane_vtable;
static bool psy_ui_scrollbarpane_vtable_initialized = FALSE;

static void psy_ui_scrollbarpane_vtable_init(psy_ui_ScrollBarPane* self)
{
	assert(self);

	if (!psy_ui_scrollbarpane_vtable_initialized) {
		psy_ui_scrollbarpane_vtable = *(self->component.vtable);
		psy_ui_scrollbarpane_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_scrollbarpane_on_destroyed;
		psy_ui_scrollbarpane_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_scrollbarpane_on_mouse_down;
		psy_ui_scrollbarpane_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_scrollbarpane_on_mouse_down;
		psy_ui_scrollbarpane_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_scrollbarpane_on_mouse_move;
		psy_ui_scrollbarpane_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_scrollbarpane_on_mouse_up;

		psy_ui_scrollbarpane_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			psy_ui_scrollbarpane_on_timer;
		psy_ui_scrollbarpane_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_scrollbarpane_vtable);
}

/* implementation */
void psy_ui_scrollbarpane_init(psy_ui_ScrollBarPane* self,
	psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_scrollbarpane_vtable_init(self);
	psy_ui_component_init(&self->thumb, &self->component, NULL);
	psy_signal_init(&self->signal_changed);
	psy_ui_intpoint_init(&self->scrollrange);
	self->position = 0.0;
	self->screenpos = 0.0;
	self->orientation = psy_ui_VERTICAL;
	self->repeat = 0;
	self->repeat_delay_counter = 0;
	psy_ui_scrollbarpane_set_orientation(self, psy_ui_VERTICAL);
}

void psy_ui_scrollbarpane_on_destroyed(psy_ui_ScrollBarPane* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_changed);
}

void psy_ui_scrollbarpane_set_orientation(psy_ui_ScrollBarPane* self,
	psy_ui_Orientation orientation)
{
	assert(self);

	self->orientation = orientation;
	if (self->orientation == psy_ui_HORIZONTAL) {
		psy_ui_component_set_style_type(&self->component, psy_ui_STYLE_HSCROLLPANE);
		psy_ui_component_set_style_type(&self->thumb, psy_ui_STYLE_HSCROLLTHUMB);
		psy_ui_component_set_style_type_hover(&self->thumb,
			psy_ui_STYLE_HSCROLLTHUMB_HOVER);
		psy_ui_component_set_style_type_active(&self->thumb,
			psy_ui_STYLE_HSCROLLTHUMB_HOVER);
		psy_ui_component_set_preferred_size(&self->thumb,
			psy_ui_size_make_em(1.0, 1.0));
	}
	else {
		psy_ui_component_set_style_type(&self->component, psy_ui_STYLE_VSCROLLPANE);
		psy_ui_component_set_style_type(&self->thumb, psy_ui_STYLE_VSCROLLTHUMB);
		psy_ui_component_set_style_type_hover(&self->thumb,
			psy_ui_STYLE_VSCROLLTHUMB_HOVER);
		psy_ui_component_set_style_type_active(&self->thumb,
			psy_ui_STYLE_VSCROLLTHUMB_HOVER);
		psy_ui_component_set_preferred_size(&self->thumb,
			psy_ui_size_make_em(1.0, 1.0));
	}
	psy_ui_scrollbarpane_update_thumb_position(self);
}

void psy_ui_scrollbarpane_update_thumb_position(psy_ui_ScrollBarPane* self)
{
	psy_ui_RealRectangle new_position;
	psy_ui_RealRectangle update_position;
	psy_ui_RealSize size;
	psy_ui_RealSize thumb_size;
	const psy_ui_TextMetric* tm;

	assert(self);

	size = psy_ui_component_scroll_size_px(&self->component);
	tm = psy_ui_component_textmetric(&self->component);
	if (self->orientation == psy_ui_HORIZONTAL) {
		thumb_size = psy_ui_realsize_make(tm->tmAveCharWidth * 2.5,
			size.height);
		new_position = psy_ui_realrectangle_make(
			psy_ui_realpoint_make(self->screenpos, 0.0),
			thumb_size);
	}
	else {
		thumb_size = psy_ui_realsize_make(size.width, tm->tmHeight);
		new_position = psy_ui_realrectangle_make(
			psy_ui_realpoint_make(0.0, self->screenpos),
			thumb_size);
	}
	update_position = psy_ui_component_position(&self->thumb);
	psy_ui_component_set_position(&self->thumb,
		psy_ui_rectangle_make_px(&new_position));
	psy_ui_realrectangle_union(&update_position, &new_position);
	psy_ui_component_invalidate_rect(&self->component, update_position);
}

void psy_ui_scrollbarpane_on_mouse_down(psy_ui_ScrollBarPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_set_focus(&self->component);
	if (psy_ui_mouseevent_target(ev) == &self->thumb) {
		psy_ui_component_capture(&self->thumb);
		if (self->orientation == psy_ui_HORIZONTAL) {
			self->dragoffset = psy_ui_mouseevent_pt(ev).x - self->screenpos;
		}
		else if (self->orientation == psy_ui_VERTICAL) {
			self->dragoffset = psy_ui_mouseevent_pt(ev).y - self->screenpos;
		}
	}
	else {
		psy_ui_RealRectangle thumb_position;

		thumb_position = psy_ui_component_position(&self->thumb);
		psy_ui_component_capture(&self->component);
		if (self->orientation == psy_ui_HORIZONTAL) {
			if (psy_ui_mouseevent_pt(ev).x > thumb_position.right) {
				self->repeat = 1;
			}
			else {
				self->repeat = -1;
			}
		}
		else if (self->orientation == psy_ui_VERTICAL) {
			if (psy_ui_mouseevent_pt(ev).y > thumb_position.bottom) {
				self->repeat = 1;
			}
			else {
				self->repeat = -1;
			}
		}
		psy_ui_scrollbarpane_set_thumb_position(self,
			self->position + self->repeat);
		psy_signal_emit(&self->signal_changed, self, 0);
		self->repeat_delay_counter = REPEAT_DELAYCOUNT;
		psy_ui_component_start_timer(&self->component, 0,
			REPEAT_TIMEINTERVAL);
	}
}

void psy_ui_scrollbarpane_on_mouse_up(psy_ui_ScrollBarPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_target(ev) == &self->thumb) {
		psy_ui_component_release_capture(&self->thumb);
		psy_ui_scrollbarpane_set_thumb_position(self, self->position);
	}
	else {
		psy_ui_component_release_capture(&self->component);
	}
	if (self->repeat != 0) {
		psy_ui_component_stop_timer(&self->component, 0);
		self->repeat = 0;
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_scrollbarpane_on_mouse_move(psy_ui_ScrollBarPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if ((ev->button_ == 1) && (psy_ui_mouseevent_target(ev) == &self->thumb)) {
		double step;
		double position;
		psy_ui_RealSize size;
		psy_ui_RealSize thumb_size;

		size = psy_ui_component_scroll_size_px(&self->component);
		thumb_size = psy_ui_component_scroll_size_px(&self->thumb);
		if (self->orientation == psy_ui_HORIZONTAL) {
			self->screenpos = psy_max(0, psy_min(
				psy_ui_mouseevent_pt(ev).x - self->dragoffset,
				size.width - thumb_size.width));
		}
		else {
			self->screenpos = psy_max(0, psy_min(
				psy_ui_mouseevent_pt(ev).y - self->dragoffset,
				size.height - thumb_size.height));
		}
		psy_ui_scrollbarpane_update_thumb_position(self);
		step = psy_ui_scrollbarpane_step(self);
		position = psy_ui_scrollbarpane_position_in_scroll_range(self,
			((self->screenpos) * step) + self->scrollrange.x);
		if (position != self->position) {
			self->position = position;
			psy_signal_emit(&self->signal_changed, self, 0);
		}
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_scrollbarpane_set_thumb_position(psy_ui_ScrollBarPane* self,
	double position)
{
	double step;

	assert(self);

	step = psy_ui_scrollbarpane_step(self);
	if (step != 0.0) {
		self->position = psy_ui_scrollbarpane_position_in_scroll_range(self, position);
		self->screenpos = (1.0 / step) * floor(self->position - self->scrollrange.x);
		psy_ui_scrollbarpane_update_thumb_position(self);
	}
}

double psy_ui_scrollbarpane_position_in_scroll_range(
	const psy_ui_ScrollBarPane* self, double pos)
{
	assert(self);

	if (pos < self->scrollrange.x) {
		pos = (double)self->scrollrange.x;
	}
	if (pos > self->scrollrange.y) {
		pos = (double)self->scrollrange.y;
	}
	return pos;
}

double psy_ui_scrollbarpane_step(psy_ui_ScrollBarPane* self)
{
	double rv;
	psy_ui_RealSize size;
	psy_ui_RealSize panesize;

	assert(self);

	panesize = psy_ui_component_scroll_size_px(&self->component);
	size = psy_ui_component_scroll_size_px(&self->thumb);
	if (self->orientation == psy_ui_HORIZONTAL) {
		rv = (self->scrollrange.y - self->scrollrange.x) /
			(panesize.width - size.width);
	}
	else if (self->orientation == psy_ui_VERTICAL) {
		rv = (self->scrollrange.y - self->scrollrange.x) /
			(panesize.height - size.height);
	}
	else {
		rv = 0.0;
	}
	return rv;
}

void psy_ui_scrollbarpane_on_timer(psy_ui_ScrollBarPane* self, uintptr_t id)
{
	assert(self);

	if (!psy_ui_component_draw_visible(psy_ui_scrollbarpane_base(self))) {
		self->repeat_delay_counter = 0;
		self->repeat = 0;
		psy_ui_component_stop_timer(psy_ui_scrollbarpane_base(self), 0);
		return;
	}
	if (self->repeat_delay_counter == 0 ||
		self->repeat_delay_counter == REPEAT_DELAYFIRSTCOUNT) {
		psy_ui_scrollbarpane_set_thumb_position(self,
			self->position + self->repeat);
		psy_signal_emit(&self->signal_changed, self, 0);
	}
	if (self->repeat_delay_counter > 0) {
		--self->repeat_delay_counter;
	}
}


/* psy_ui_ScrollBar */

/* prototypes */
static void psy_ui_scrollbar_on_destroyed(psy_ui_ScrollBar*);
static void psy_ui_scrollbar_on_less(psy_ui_ScrollBar*,
	psy_ui_Component* sender);
static void psy_ui_scrollbar_on_more(psy_ui_ScrollBar*,
	psy_ui_Component* sender);
static void psy_ui_scrollbar_on_scroll_pane_changed(psy_ui_ScrollBar*,
	psy_ui_ScrollBarPane* sender);
static void psy_ui_scrollbar_on_wheel(psy_ui_ScrollBar*, psy_ui_MouseEvent*);

/* vtable */
static psy_ui_ComponentVtable psy_ui_scrollbar_vtable;
static bool psy_ui_scrollbar_vtable_initialized = FALSE;

static void psy_ui_scrollbar_vtable_init(psy_ui_ScrollBar* self)
{
	assert(self);

	if (!psy_ui_scrollbar_vtable_initialized) {
		psy_ui_scrollbar_vtable = *(self->component.vtable);
		psy_ui_scrollbar_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_scrollbarpane_on_destroyed;
		psy_ui_scrollbar_vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_scrollbar_on_wheel;
		psy_ui_scrollbar_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_scrollbar_vtable);
}

/* implementation */
void psy_ui_scrollbar_init(psy_ui_ScrollBar* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_scrollbar_vtable_init(self);
	self->visible_state_change = FALSE;
	/* more button */
	psy_ui_button_init_resource_connect(&self->more, &self->component,
		"img.icon-down", self, psy_ui_scrollbar_on_more);	
	psy_ui_button_set_char_number(&self->more, 2.0);
	psy_ui_component_set_align(psy_ui_button_base(&self->more),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_button_set_repeat(&self->more, 100, 300);
	/* less button */
	psy_ui_button_init_resource_connect(&self->less, &self->component,
		"img.icon-up", self, psy_ui_scrollbar_on_less);	
	psy_ui_button_set_char_number(&self->less, 2);
	psy_ui_component_set_align(psy_ui_button_base(&self->less),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_button_set_repeat(&self->less, 100, 300);
	/* scrollpane */
	psy_ui_scrollbarpane_init(&self->pane, &self->component);
	psy_ui_component_set_align(psy_ui_scrollbarpane_base(&self->pane),
		psy_ui_ALIGN_CLIENT);
	/* orientation */
	psy_ui_scrollbar_set_orientation(self, psy_ui_VERTICAL);
	/* signals */
	psy_signal_init(&self->signal_changed);
	/* connect pane */
	psy_signal_connect(&self->pane.signal_changed, self,
		psy_ui_scrollbar_on_scroll_pane_changed);
}

void psy_ui_scrollbar_set_orientation(psy_ui_ScrollBar* self,
	psy_ui_Orientation orientation)
{
	assert(self);

	if (orientation == psy_ui_HORIZONTAL) {		
		psy_ui_button_set_svg(&self->more, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));			
		psy_ui_component_set_align(psy_ui_button_base(&self->more),
			psy_ui_ALIGN_RIGHT);
		psy_ui_component_set_style_types(psy_ui_button_base(&self->more),
			psy_ui_STYLE_HSCROLLBUTTON, psy_ui_STYLE_HSCROLLBUTTON_HOVER,
			psy_INDEX_INVALID, psy_INDEX_INVALID);
		psy_ui_component_set_style_type_active(psy_ui_button_base(&self->more),
			psy_ui_STYLE_HSCROLLBUTTON_ACTIVE);		
		psy_ui_button_set_svg(&self->less, psy_ui_app_svg(psy_ui_app(), "img.icon-less"));			
		psy_ui_component_set_align(psy_ui_button_base(&self->less),
			psy_ui_ALIGN_RIGHT);
		psy_ui_component_set_style_types(psy_ui_button_base(&self->less),
			psy_ui_STYLE_HSCROLLBUTTON, psy_ui_STYLE_HSCROLLBUTTON_HOVER,
			psy_INDEX_INVALID, psy_INDEX_INVALID);
		psy_ui_component_set_style_type_active(psy_ui_button_base(&self->less),
			psy_ui_STYLE_HSCROLLBUTTON_ACTIVE);
		psy_ui_scrollbarpane_update_thumb_position(&self->pane);
	} else if (orientation == psy_ui_VERTICAL) {		
		psy_ui_button_set_svg(&self->more, psy_ui_app_svg(psy_ui_app(), "img.icon-down"));
		psy_ui_component_set_align(psy_ui_button_base(&self->more),
			psy_ui_ALIGN_BOTTOM);
		psy_ui_component_set_style_types(psy_ui_button_base(&self->more),
			psy_ui_STYLE_VSCROLLBUTTON, psy_ui_STYLE_VSCROLLBUTTON_HOVER,
			psy_INDEX_INVALID, psy_INDEX_INVALID);
		psy_ui_component_set_style_type_active(psy_ui_button_base(&self->more),
			psy_ui_STYLE_VSCROLLBUTTON_ACTIVE);		
		psy_ui_button_set_svg(&self->less, psy_ui_app_svg(psy_ui_app(), "img.icon-up"));
		psy_ui_component_set_align(psy_ui_button_base(&self->less),
			psy_ui_ALIGN_BOTTOM);
		psy_ui_component_set_style_types(psy_ui_button_base(&self->less),
			psy_ui_STYLE_VSCROLLBUTTON, psy_ui_STYLE_VSCROLLBUTTON_HOVER,
			psy_INDEX_INVALID, psy_INDEX_INVALID);
		psy_ui_component_set_style_type_active(psy_ui_button_base(&self->less),
			psy_ui_STYLE_VSCROLLBUTTON_ACTIVE);
		psy_ui_scrollbarpane_update_thumb_position(&self->pane);
	}
	psy_ui_scrollbarpane_set_orientation(&self->pane, orientation);
}

void psy_ui_scrollbar_on_destroyed(psy_ui_ScrollBar* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_changed);
}

void psy_ui_scrollbar_on_wheel(psy_ui_ScrollBar* self, psy_ui_MouseEvent* ev)
{
	if (psy_ui_mouseevent_delta(ev) > 0) {
		psy_ui_scrollbar_on_less(self, psy_ui_mouseevent_target(ev));
	}
	else {
		psy_ui_scrollbar_on_more(self, psy_ui_mouseevent_target(ev));
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_scrollbar_on_less(psy_ui_ScrollBar* self, psy_ui_Component* sender)
{
	assert(self);

	psy_ui_scrollbar_set_thumb_position(self,
		psy_ui_scrollbar_position(self) - 1);
	psy_signal_emit(&self->signal_changed, self, 0);
}

void psy_ui_scrollbar_on_more(psy_ui_ScrollBar* self, psy_ui_Component* sender)
{
	assert(self);

	psy_ui_scrollbar_set_thumb_position(self,
		psy_ui_scrollbar_position(self) + 1);
	psy_signal_emit(&self->signal_changed, self, 0);
}

void psy_ui_scrollbar_on_scroll_pane_changed(psy_ui_ScrollBar* self,
	psy_ui_ScrollBarPane* sender)
{
	assert(self);

	psy_signal_emit(&self->signal_changed, self, 0);
}
