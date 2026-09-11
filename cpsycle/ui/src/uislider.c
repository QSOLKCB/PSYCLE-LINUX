/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uislider.h"
/* local */
#include "uiapp.h"
#include "trackercmds.h"
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* psy_ui_SliderPane */

/* prototypes */
static void psy_ui_sliderpane_on_destroyed(psy_ui_SliderPane*);
static void psy_ui_sliderpane_init_signals(psy_ui_SliderPane*);
static void psy_ui_sliderpane_dispose_signals(psy_ui_SliderPane*);
static void psy_ui_sliderpane_on_draw(psy_ui_SliderPane*, psy_ui_Graphics*);
static void psy_ui_sliderpane_draw_vertical_ruler(psy_ui_SliderPane*,
	psy_ui_Graphics*);
static void psy_ui_sliderpane_on_align(psy_ui_SliderPane*);
static void psy_ui_sliderpane_on_mouse_down(psy_ui_SliderPane*,
	psy_ui_MouseEvent*);
static void psy_ui_sliderpane_on_mouse_up(psy_ui_SliderPane*,
	psy_ui_MouseEvent*);
static void psy_ui_sliderpane_on_mouse_move(psy_ui_SliderPane*,
	psy_ui_MouseEvent*);
static void psy_ui_sliderpane_on_mouse_wheel(psy_ui_SliderPane*,
	psy_ui_MouseEvent*);
static void psy_ui_sliderpane_on_mouse_enter(psy_ui_SliderPane*);
static void psy_ui_sliderpane_on_mouse_leave(psy_ui_SliderPane*);
static void psy_ui_sliderpane_on_timer(psy_ui_SliderPane*, uintptr_t timerid);
static void psy_ui_sliderpane_update_value(psy_ui_SliderPane*);
static void psy_ui_sliderpane_describe_value(psy_ui_SliderPane*);
static void psy_ui_sliderpane_on_preferred_size(psy_ui_SliderPane*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static psy_ui_RealRectangle psy_ui_sliderpane_slider_position(
	const psy_ui_SliderPane*);
static bool psy_ui_sliderpane_on_input(psy_ui_SliderPane*, InputHandler* sender);
static void sliderpane_on_focus_lost(psy_ui_SliderPane*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_SliderPane* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super_vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_sliderpane_on_destroyed;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_sliderpane_on_draw;		
		vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_sliderpane_on_preferred_size;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sliderpane_on_mouse_down;
		vtable.onalign =
			(psy_ui_fp_component)
			psy_ui_sliderpane_on_align;
		vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sliderpane_on_mouse_move;
		vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sliderpane_on_mouse_wheel;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sliderpane_on_mouse_up;
		vtable.onmouseenter =
			(psy_ui_fp_component)
			psy_ui_sliderpane_on_mouse_enter;
		vtable.onmouseleave = 
			(psy_ui_fp_component)
			psy_ui_sliderpane_on_mouse_leave;		
		vtable.on_focus_lost =
			(psy_ui_fp_component)
			sliderpane_on_focus_lost;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			psy_ui_sliderpane_on_timer;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void psy_ui_sliderpane_init(psy_ui_SliderPane* self, psy_ui_Component* parent)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	self->slider = NULL;	
	self->tweakbase = -1;
	self->orientation = psy_ui_HORIZONTAL;
	self->value = 0.0;
	self->defaultvalue = 0.5;
	self->rulerstep = 0.1;
	self->wheelstep = 0.1;
	self->hslidersize = psy_ui_size_make_em(1.0, 1.0);
	self->vslidersize = psy_ui_size_make_em(1.0, 1.0);	
	self->slidersizepx = psy_ui_realsize_make(6.0, 6.0);
	self->poll = FALSE;
	self->hover = FALSE;
	psy_ui_sliderpane_init_signals(self);	
	psy_signal_connect(&self->component.signal_timer, self,
		psy_ui_sliderpane_on_timer);
	psy_ui_component_set_style_types(&self->component,
		psy_ui_STYLE_SLIDERPANE, psy_INDEX_INVALID, psy_INDEX_INVALID,
		psy_INDEX_INVALID);
	/* inputhandler */
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_sliderpane_on_input);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_SLIDERPANE_FOCUS);
}

void psy_ui_sliderpane_init_signals(psy_ui_SliderPane* self)
{
	assert(self);

	psy_signal_init(&self->signal_clicked);
	psy_signal_init(&self->signal_changed);
	psy_signal_init(&self->signal_describevalue);
	psy_signal_init(&self->signal_tweakvalue);
	psy_signal_init(&self->signal_value);
	psy_signal_init(&self->signal_customdraw);
}

void psy_ui_sliderpane_on_destroyed(psy_ui_SliderPane* self)
{	
	assert(self);

	psy_ui_sliderpane_dispose_signals(self);
	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),
		self, psy_ui_sliderpane_on_input);
}

void psy_ui_sliderpane_dispose_signals(psy_ui_SliderPane* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_clicked);
	psy_signal_dispose(&self->signal_changed);
	psy_signal_dispose(&self->signal_describevalue);
	psy_signal_dispose(&self->signal_tweakvalue);
	psy_signal_dispose(&self->signal_value);
	psy_signal_dispose(&self->signal_customdraw);
}

void psy_ui_sliderpane_set_value(psy_ui_SliderPane* self, double value)
{
	assert(self);

	self->value = value;
}

void psy_ui_sliderpane_set_default_value(psy_ui_SliderPane* self, double value)
{
	assert(self);

	self->defaultvalue = value;
}

double psy_ui_sliderpane_value(psy_ui_SliderPane* self)
{
	assert(self);

	return self->value;
}

void psy_ui_sliderpane_on_draw(psy_ui_SliderPane* self, psy_ui_Graphics* g)
{	
	psy_ui_RealSize size;
	int styletype;

	assert(self);

	psy_signal_emit(&self->signal_customdraw, self, 1, g);

	if (self->orientation == psy_ui_VERTICAL) {												
		psy_ui_sliderpane_draw_vertical_ruler(self, g);		
	}
	if (self->hover) {
		styletype = psy_ui_STYLE_SLIDERTHUMB_HOVER;
	} else {
		styletype = psy_ui_STYLE_SLIDERTHUMB;
	}	
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_sliderpane_slider_position(self),
		psy_ui_style_background_colour(psy_ui_style_const(styletype)));
	size = psy_ui_component_size_px(&self->component);
	psy_ui_graphics_draw_border(g,
		psy_ui_realrectangle_make(psy_ui_realpoint_zero(), size),
		&psy_ui_style_const(psy_ui_STYLE_SLIDERPANE)->border,
		psy_ui_component_textmetric(&self->component));
}

void psy_ui_sliderpane_draw_vertical_ruler(psy_ui_SliderPane* self,
	psy_ui_Graphics* g)
{
	double step = 0;
	double markwidth = 5;
	psy_ui_RealSize size;

	assert(self);

	psy_ui_graphics_set_colour(g, psy_ui_style_const(psy_ui_STYLE_ROOT)->border.top.colour);
	size = psy_ui_component_size_px(&self->component);
	for (step = 0; step <= 1.0; step += self->rulerstep) {
		double cpy;

		cpy = step * size.height;
		psy_ui_drawline(g, psy_ui_realpoint_make(0, cpy),
			psy_ui_realpoint_make(markwidth, cpy));
		psy_ui_drawline(g, psy_ui_realpoint_make(size.width - markwidth, cpy),
			psy_ui_realpoint_make(size.width, cpy));
	}
}

void psy_ui_sliderpane_on_align(psy_ui_SliderPane* self)
{	
	assert(self);

	self->slidersizepx = psy_ui_size_px(
		(self->orientation == psy_ui_HORIZONTAL)
			? &self->hslidersize
			: &self->vslidersize,
		psy_ui_component_textmetric(psy_ui_sliderpane_base(self)), NULL);
}

void psy_ui_sliderpane_on_mouse_down(psy_ui_SliderPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 1) {
		psy_ui_RealSize size;

		size = psy_ui_component_scroll_size_px(&self->component);
		if (self->orientation == psy_ui_HORIZONTAL) {
			self->tweakbase = (psy_ui_mouseevent_pt(ev).x) -
				self->value * (size.width - self->slidersizepx.width);
		} else if (self->orientation == psy_ui_VERTICAL) {
			self->tweakbase = psy_ui_mouseevent_pt(ev).y -
				((1.0 - self->value) * (size.height -
					self->slidersizepx.width));
		}
		if (self->poll) {
			psy_ui_component_stop_timer(&self->component, 0);
		}
		psy_ui_component_capture(&self->component);
	} else {
		double value;
		double* pvalue;

		self->value = self->defaultvalue;
		psy_signal_emit_double(&self->signal_tweakvalue, self->slider, 1,
			self->value);
		value = (double)self->value;
		pvalue = &value;
		if (self->slider) {
			psy_signal_emit(&self->signal_value, self->slider, 1, pvalue);
		}
		if (self->value != value) {
			self->value = value;
			if (self->slider) {
				psy_signal_emit(&self->signal_changed, self->slider, 0);
			}
		}		
		psy_ui_sliderpane_describe_value(self);
		psy_ui_component_invalidate(&self->component);
	}
}

void psy_ui_sliderpane_on_mouse_move(psy_ui_SliderPane* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (self->tweakbase != -1) {
		psy_ui_RealSize size;
		double value;
		double* pvalue;		
		
		size = psy_ui_component_scroll_size_px(&self->component);
		if (self->orientation == psy_ui_HORIZONTAL) {			
			self->value = psy_max(0.0, psy_min(1.0,
				(psy_ui_mouseevent_pt(ev).x - self->tweakbase) /
				(size.width - self->slidersizepx.width)));
		} else {			
			self->value = psy_max(0.0, psy_min(1.0,
				1.0 - (psy_ui_mouseevent_pt(ev).y - self->tweakbase) /
				(size.height - self->slidersizepx.width)));
		}
		if (self->slider) {
			psy_signal_emit_double(&self->signal_tweakvalue, self->slider, 1,
				self->value);
		}
		pvalue = &value;
		value = 0.f;
		if (self->slider) {
			psy_signal_emit(&self->signal_value, self->slider, 1, pvalue);			
		}
		if (self->value != value) {
			self->value = value;
			if (self->slider) {
				psy_signal_emit(&self->signal_changed, self->slider, 0);
			}
		}				
		psy_ui_sliderpane_describe_value(self);
		psy_ui_component_invalidate(&self->component);
	}
}

void psy_ui_sliderpane_on_mouse_up(psy_ui_SliderPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (self->tweakbase != -1) {
		self->tweakbase = -1;
		psy_ui_component_release_capture(&self->component);
		if (self->poll) {
			psy_ui_component_start_timer(&self->component, 0, 50);
		}
	}
}

void psy_ui_sliderpane_on_mouse_wheel(psy_ui_SliderPane* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_delta(ev) != 0) {
		if (psy_ui_mouseevent_delta(ev) > 0) {
			self->value += self->wheelstep;
		} else {
			self->value -= self->wheelstep;
		}
		self->value = psy_max(0.0, psy_min(1.0, self->value));
		if (self->slider) {
			psy_signal_emit_double(&self->signal_tweakvalue, self->slider, 1,
				self->value);
			psy_signal_emit(&self->signal_changed, self->slider, 0);
		}
		psy_ui_component_invalidate(&self->component);
		psy_ui_sliderpane_describe_value(self);
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_sliderpane_on_mouse_enter(psy_ui_SliderPane* self)
{
	assert(self);

	self->hover = TRUE;
	psy_ui_component_invalidate(&self->component);
}

void psy_ui_sliderpane_on_mouse_leave(psy_ui_SliderPane* self)
{
	assert(self);

	if (self->hover != FALSE) {
		self->hover = FALSE;
		psy_ui_component_invalidate(&self->component);
	}
}

void psy_ui_sliderpane_show_vertical(psy_ui_SliderPane* self)
{
	assert(self);

	self->orientation = psy_ui_VERTICAL;
}

void psy_ui_sliderpane_show_horizontal(psy_ui_SliderPane* self)
{
	assert(self);

	self->orientation = psy_ui_HORIZONTAL;
}

psy_ui_Orientation psy_ui_sliderpane_orientation(psy_ui_SliderPane* self)
{
	assert(self);

	return self->orientation;
}

void psy_ui_sliderpane_on_timer(psy_ui_SliderPane* self, uintptr_t timerid)
{	
	assert(self);

	if (psy_ui_component_draw_visible(psy_ui_sliderpane_base(self))) {
		psy_ui_sliderpane_update_value(self);
		psy_ui_sliderpane_describe_value(self);
	}
}

void psy_ui_sliderpane_update_value(psy_ui_SliderPane* self)
{
	double value = 0;
	double* pvalue;

	assert(self);

	pvalue = &value;
	if (self->slider) {
		psy_signal_emit(&self->signal_value, self->slider, 1, pvalue);
	}
	if (self->value != value) {
		self->value = value;
#ifndef PSYCLE_DEBUG_PREVENT_TIMER_DRAW
		psy_ui_component_invalidate(&self->component);
#endif
	}
}

void psy_ui_sliderpane_connect(psy_ui_SliderPane* self, void* context,
	ui_slider_fpdescribe fp_describe, ui_slider_fptweak fp_tweak,
	ui_slider_fpvalue fp_value)
{
	assert(self);

	if (fp_describe) {
		psy_signal_connect(&self->signal_describevalue, context, fp_describe);
	}
	if (fp_tweak) {
		psy_signal_connect(&self->signal_tweakvalue, context, fp_tweak);
	}
	if (fp_value) {
		psy_signal_connect(&self->signal_value, context, fp_value);
	}
}

void psy_ui_sliderpane_describe_value(psy_ui_SliderPane* self)
{
	assert(self);

	if (self->slider) {
		psy_ui_slider_describe_value(self->slider);
	}
}

void psy_ui_sliderpane_on_preferred_size(psy_ui_SliderPane* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{	
	assert(self);

	if (self->orientation == psy_ui_HORIZONTAL) {
		rv->width = psy_ui_value_make_ew(20);
		rv->height = psy_ui_value_make_eh(1.3);
	} else {
		rv->width = psy_ui_value_make_ew(4.0);
		rv->height = psy_ui_value_make_eh(20.0);
	}
}

psy_ui_RealRectangle psy_ui_sliderpane_slider_position(
	const psy_ui_SliderPane* self)
{
	psy_ui_RealSize size;
	
	assert(self);

	size = psy_ui_component_scroll_size_px(&self->component);
	if (self->orientation == psy_ui_HORIZONTAL) {
		return psy_ui_realrectangle_make(psy_ui_realpoint_make(
			floor((size.width - self->slidersizepx.width) * self->value), 2.0),
			psy_ui_realsize_make(self->slidersizepx.width, size.height - 4));
	}
	return psy_ui_realrectangle_make(psy_ui_realpoint_make(2.0,
		floor(((size.height - self->slidersizepx.height) * (1 - self->value)))),
		psy_ui_realsize_make(size.width - 4, self->slidersizepx.height));
}

bool psy_ui_sliderpane_on_input(psy_ui_SliderPane* self, InputHandler* sender)
{
	psy_ui_Component* restore;

	assert(self);

	restore = psy_ui_app_focus(psy_ui_app());
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVLEFT: {
		self->value -= self->wheelstep;
		self->value = psy_max(0.0, psy_min(1.0, self->value));
		if (self->slider) {
			psy_signal_emit_double(&self->signal_tweakvalue, self->slider, 1,
				self->value);
			psy_signal_emit(&self->signal_changed, self->slider, 0);
		}
		psy_ui_component_invalidate(&self->component);
		psy_ui_sliderpane_describe_value(self);
		return TRUE; }
	case CMD_NAVRIGHT: {
		self->value += self->wheelstep;
		self->value = psy_max(0.0, psy_min(1.0, self->value));
		if (self->slider) {
			psy_signal_emit_double(&self->signal_tweakvalue, self->slider, 1,
				self->value);
			psy_signal_emit(&self->signal_changed, self->slider, 0);
		}
		psy_ui_component_invalidate(&self->component);
		psy_ui_sliderpane_describe_value(self);
		return TRUE; }
	case CMD_NAVSELECT: {		
		psy_ui_component_set_focus(psy_ui_component_parent(
			psy_ui_sliderpane_base(self)));
		return TRUE; }
	default:
		return FALSE;
	}
}

void sliderpane_on_focus_lost(psy_ui_SliderPane* self)
{
	assert(self);
	
	super_vtable.on_focus_lost(&self->component);
	psy_ui_component_set_tab_index(&self->component,
		psy_INDEX_INVALID);
}


/* psy_ui_Slider */

static void psy_ui_slider_on_destroyed(psy_ui_Slider*);
static bool psy_ui_slider_on_input(psy_ui_Slider*, InputHandler* sender);

/* vtable */
static psy_ui_ComponentVtable slider_vtable;
static bool slider_vtable_initialized = FALSE;

static void slider_vtable_init(psy_ui_Slider* self)
{
	assert(self);

	if (!slider_vtable_initialized) {
		slider_vtable = *(self->component.vtable);
		slider_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_slider_on_destroyed;		
		slider_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &slider_vtable);
}

/* implementation */
void psy_ui_slider_init(psy_ui_Slider* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	slider_vtable_init(self);
	psy_ui_label_init(&self->desc, &self->component);	
	psy_ui_sliderpane_init(&self->pane, &self->component);
	self->pane.slider = self;	
	psy_ui_component_set_align(psy_ui_sliderpane_base(&self->pane),
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_px(1.0, 1.0, 1.0, 1.0));
	psy_ui_label_init(&self->value, &self->component);
	psy_ui_label_prevent_translation(&self->value);	
	psy_ui_slider_show_horizontal(self);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_SLIDERPANE_FOCUS);
	/* inputhandler */
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_slider_on_input);
	psy_ui_component_set_tab_index(&self->component, 0);
}

void psy_ui_slider_init_text(psy_ui_Slider* self, psy_ui_Component* parent,
	const char* text)
{
	assert(self);
	
	psy_ui_slider_init(self, parent);
	psy_ui_slider_set_text(self, text);
}

void psy_ui_slider_on_destroyed(psy_ui_Slider* self)
{
	assert(self);

	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),
		self, psy_ui_slider_on_input);
}

void psy_ui_slider_connect(psy_ui_Slider* self, void* context,
	ui_slider_fpdescribe fp_describe, ui_slider_fptweak fp_tweak,
	ui_slider_fpvalue fp_value)
{
	assert(self);
	
	psy_ui_sliderpane_connect(&self->pane, context,
		fp_describe, fp_tweak, fp_value);	
}

void psy_ui_slider_set_text(psy_ui_Slider* self, const char* text)
{
	assert(self);
	
	psy_ui_label_set_text(&self->desc, text);
}

void psy_ui_slider_set_char_number(psy_ui_Slider* self, double number)
{
	assert(self);
	
	psy_ui_label_set_char_number(&self->desc, number);
}

void psy_ui_slider_set_value_char_number(psy_ui_Slider* self, double number)
{
	assert(self);
	
	psy_ui_label_set_char_number(&self->value, number);
}

void psy_ui_slider_show_vertical(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_sliderpane_show_vertical(&self->pane);
	psy_ui_component_set_align(psy_ui_label_base(&self->desc),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_align(psy_ui_label_base(&self->value),
		psy_ui_ALIGN_BOTTOM);	
	psy_ui_component_set_margin(psy_ui_sliderpane_base(&self->pane),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 2.0));		
}

void psy_ui_slider_show_horizontal(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_sliderpane_show_horizontal(&self->pane);
	psy_ui_component_set_align(psy_ui_label_base(&self->desc),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_align(psy_ui_label_base(&self->value),
		psy_ui_ALIGN_RIGHT);	
	psy_ui_component_set_margin(psy_ui_sliderpane_base(&self->pane),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 2.0));		
}

void psy_ui_slider_show_label(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_component_show(&self->desc.component);
}

void psy_ui_slider_hide_label(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_component_hide(&self->desc.component);
}

void psy_ui_slider_show_value_label(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_component_show(&self->value.component);
}

void psy_ui_slider_hide_value_label(psy_ui_Slider* self)
{
	assert(self);
	
	psy_ui_component_hide(&self->value.component);
}

void psy_ui_slider_describe_value(psy_ui_Slider* self)
{
	char text[256];

	assert(self);
	
	text[0] = '\0';
	psy_signal_emit(&self->pane.signal_describevalue, self, 1, text);
	if (text[0] == '\0') {
		psy_snprintf(text, 256, "%f", self->pane.value);
	}
	psy_ui_label_set_text(&self->value, text);	
}

void psy_ui_slider_start_poll(psy_ui_Slider* self)
{
	assert(self);
	
	self->pane.poll = TRUE;	
	psy_ui_component_start_timer(&self->pane.component, 0, 50);
}

void psy_ui_slider_stop_poll(psy_ui_Slider* self)
{
	assert(self);
	
	self->pane.poll = FALSE;
	psy_ui_component_stop_timer(&self->pane.component, 0);
}

void psy_ui_slider_update(psy_ui_Slider* self)
{	
	assert(self);
	
	psy_ui_sliderpane_update_value(&self->pane);
	psy_ui_sliderpane_describe_value(&self->pane);	
}

void psy_ui_slider_set_default_value(psy_ui_Slider* self, double value)
{
	assert(self);
	
	psy_ui_sliderpane_set_default_value(&self->pane, value);
}

void psy_ui_slider_set_ruler_step(psy_ui_Slider* self, double value)
{
	assert(self);

	self->pane.rulerstep = value;
}

void psy_ui_slider_set_wheel_step(psy_ui_Slider* self, double value)
{
	assert(self);

	self->pane.wheelstep = value;
}

bool psy_ui_slider_on_input(psy_ui_Slider* self, InputHandler* sender)
{	
	assert(self);
	
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVSELECT: {
		psy_ui_component_set_tab_index(psy_ui_sliderpane_base(&self->pane), 0);
		psy_ui_component_set_focus(psy_ui_sliderpane_base(&self->pane));		
		return TRUE; }
	default:
		return FALSE;
	}
}
