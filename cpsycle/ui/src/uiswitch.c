/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiswitch.h"
/* std */
#include <stdlib.h>

/* prototypes */
static void psy_ui_switch_on_destroyed(psy_ui_Switch*);
static void psy_ui_switch_init_svg(psy_ui_Switch* self);
static void psy_ui_switch_on_draw(psy_ui_Switch*, psy_ui_Graphics*);
static void psy_ui_switch_on_mouse_down(psy_ui_Switch*, psy_ui_MouseEvent*);
static void psy_ui_switch_on_property_changed(psy_ui_Switch*,
	psy_Property* sender);
static void psy_ui_switch_before_property_destroyed(psy_ui_Switch*,
	psy_Property* sender);

/* vtable */
static psy_ui_ComponentVtable psy_ui_switch_vtable;
static int psy_ui_switch_vtable_initialized = FALSE;

static void psy_ui_switch_vtable_init(psy_ui_Switch* self)
{
	assert(self);

	if (!psy_ui_switch_vtable_initialized) {
		psy_ui_switch_vtable = *(self->component.vtable);
		psy_ui_switch_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_switch_on_destroyed;
		psy_ui_switch_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_switch_on_draw;
		psy_ui_switch_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_switch_on_mouse_down;
		psy_ui_switch_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_switch_vtable);
}

/* implementation */
void psy_ui_switch_init(psy_ui_Switch* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_switch_vtable_init(self);
	psy_signal_init(&self->signal_clicked);
	self->state_ = FALSE;
	self->property_ = NULL;
	psy_ui_component_set_aligner(psy_ui_switch_base(self), NULL);
	psy_ui_component_set_style_types(psy_ui_switch_base(self),
		psy_ui_STYLE_SWITCH, psy_ui_STYLE_SWITCH_HOVER,
		psy_ui_STYLE_SWITCH_SELECT, psy_INDEX_INVALID);
	psy_ui_component_set_preferred_size(psy_ui_switch_base(self),
		psy_ui_size_make_em(5.0, 1.5));
	psy_ui_switch_init_svg(self);	
}

void psy_ui_switch_init_exchange(psy_ui_Switch* self, psy_ui_Component* parent,
	psy_Property* property)
{
	assert(self);

	psy_ui_switch_init(self, parent);
	psy_ui_switch_exchange(self, property);
}

void psy_ui_switch_on_destroyed(psy_ui_Switch* self)
{	
	assert(self);

	if (self->property_) {
		psy_property_disconnect(self->property_, self);
	}
	psy_signal_dispose(&self->signal_clicked);
	psy_ui_svg_dispose(&self->svg_on);
	psy_ui_svg_dispose(&self->svg_off);
}

psy_ui_Switch* psy_ui_switch_alloc(void)
{
	return (psy_ui_Switch*)malloc(sizeof(psy_ui_Switch));
}

psy_ui_Switch* psy_ui_switch_alloc_init(psy_ui_Component* parent)
{
	psy_ui_Switch* rv;

	rv = psy_ui_switch_alloc();
	if (rv) {
		psy_ui_switch_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);		
	}
	return rv;
}

psy_ui_Switch* psy_ui_switch_alloc_init_exchange(psy_ui_Component* parent,
	psy_Property* property)
{
	psy_ui_Switch* rv;

	rv = psy_ui_switch_alloc();
	if (rv) {
		psy_ui_switch_init_exchange(rv, parent, property);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_switch_init_svg(psy_ui_Switch* self)
{
	psy_ui_SVGRect r;
	psy_ui_RealSize knob_size;
	psy_ui_RealSize corner;
	psy_ui_RealPoint ident;
	double switch_width;
	psy_ui_RealRectangle viewbox;

	assert(self);

	knob_size = psy_ui_realsize_make(20.0, 10.0);
	viewbox = psy_ui_realrectangle_make(psy_ui_realpoint_zero(), knob_size);
	switch_width = 8.0;
	corner = psy_ui_realsize_make(6.0, 6.0);
	ident = psy_ui_realpoint_make(3.0, 2.0);
	psy_ui_svg_init(&self->svg_off);
	self->svg_off.size = knob_size;
	self->svg_off.viewbox = viewbox;	
	psy_ui_svgrect_init_all(&r, viewbox, corner, FALSE);
	psy_ui_svg_add_rect(&self->svg_off, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(ident,
			psy_ui_realsize_make(switch_width, knob_size.height - ident.y * 2)),
			corner, TRUE);	
	psy_ui_svg_add_rect(&self->svg_off, &r);
	psy_ui_svg_init(&self->svg_on);
	self->svg_on.size = knob_size;
	self->svg_on.viewbox = viewbox;
	psy_ui_svgrect_init_all(&r, viewbox, corner, FALSE);		
	psy_ui_svg_add_rect(&self->svg_on, &r);
	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(knob_size.width - switch_width - ident.x, ident.y),
		psy_ui_realsize_make(switch_width, knob_size.height - ident.y * 2)),
		corner, TRUE);	
	psy_ui_svg_add_rect(&self->svg_on, &r);
}

void psy_ui_switch_on_property_changed(psy_ui_Switch* self, psy_Property* sender)
{
	bool checked;

	assert(self);
	
	if (psy_property_is_choice_item(sender)) {
		checked = (psy_property_at_choice(psy_property_parent(sender))
			== sender);
	} else {
		checked = psy_property_item_bool(sender);
	}
	if (checked) {
		psy_ui_switch_check(self);
	} else {
		psy_ui_switch_uncheck(self);
	}
}

void psy_ui_switch_before_property_destroyed(psy_ui_Switch* self, psy_Property* sender)
{
	assert(self);

	self->property_ = NULL;
}

void psy_ui_switch_on_draw(psy_ui_Switch* self, psy_ui_Graphics* g)
{
	psy_ui_RealSize size;
	const psy_ui_TextMetric* tm;	
	psy_ui_RealSize viewport;	
	psy_ui_SVGDraw svg_draw;

	assert(self);
	
	tm = psy_ui_component_textmetric(psy_ui_switch_base(self));			
	viewport = psy_ui_realsize_make(5.0 * tm->tmAveCharWidth, tm->tmHeight);	
	psy_ui_svgdraw_init(&svg_draw,
		(self->state_ == FALSE) 
		? &self->svg_off 
		: &self->svg_on,
		viewport);
	size = psy_ui_component_scroll_size_px(psy_ui_switch_base(self));
	psy_ui_svgdraw_draw(&svg_draw, g, psy_ui_realpoint_make(0.0,
		floor((size.height - viewport.height) / 2.0)));
}

void psy_ui_switch_on_mouse_down(psy_ui_Switch* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	self->state_ = !self->state_;
	if (self->property_) {
		if (psy_property_is_choice_item(self->property_)) {
			if (!psy_property_parent(self->property_)) {
				return;
			}
			psy_property_set_item_int(psy_property_parent(self->property_),
				psy_property_index(self->property_));
		} else {
			psy_property_set_item_bool(self->property_, self->state_);
		}
	}
	psy_signal_emit(&self->signal_clicked, self, psy_SIGNAL_NOPARAMS);
}

void psy_ui_switch_exchange(psy_ui_Switch* self, psy_Property* property)
{
	assert(self);

	self->property_ = property;
	if (self->property_) {
		psy_ui_switch_on_property_changed(self, self->property_);
		psy_property_connect(self->property_, self, 
			psy_ui_switch_on_property_changed);
		psy_signal_connect(&self->property_->before_destroyed, self,
			psy_ui_switch_before_property_destroyed);
	}
}

void psy_ui_switch_check(psy_ui_Switch* self)
{
	assert(self);

	self->state_ = TRUE;
	psy_ui_component_add_style_state(psy_ui_switch_base(self),
		psy_ui_STYLESTATE_SELECT);	
}

void psy_ui_switch_uncheck(psy_ui_Switch* self)
{
	assert(self);

	self->state_ = FALSE;
	psy_ui_component_remove_style_state(psy_ui_switch_base(self),
		psy_ui_STYLESTATE_SELECT);	
}

bool psy_ui_switch_checked(const psy_ui_Switch* self)
{
	assert(self);

	return (self->state_ != FALSE);
}
