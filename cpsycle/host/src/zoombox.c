/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "zoombox.h"
/* host */
#include "styles.h"
/* ui */
#include <trackercmds.h>
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void zoombox_on_destroyed(ZoomBox*);
static void zoombox_on_zoom_in(ZoomBox*, psy_ui_Component* sender);
static void zoombox_on_zoom_out(ZoomBox*, psy_ui_Component* sender);
static void zoombox_on_mouse_wheel(ZoomBox*, psy_ui_MouseEvent*);
static void zoombox_on_edit_accept(ZoomBox*, psy_ui_Edit* sender);
static void zoombox_on_edit_reject(ZoomBox*, psy_ui_Edit* sender);
static void zoombox_update(ZoomBox*);
static void zoombox_on_property_changed(ZoomBox*, psy_Property* sender);
static void zoombox_before_property_destroyed(ZoomBox*, psy_Property* sender);
static bool zoombox_on_input(ZoomBox*, InputHandler* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(ZoomBox* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			zoombox_on_destroyed;
		vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			zoombox_on_mouse_wheel;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(zoombox_base(self), &vtable);
}

/* implementation */
void zoombox_init(ZoomBox* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	vtable_init(self);
	psy_signal_init(&self->signal_changed);
	self->zoom_rate_ = 1.0;
	self->zoom_step_ = 0.1;	
	self->range_ = psy_realpair_make(0.10, 10.0);	
	self->property_ = NULL;
	self->prevent_wheel_select_ = FALSE;
	psy_ui_component_set_style_type(&self->component, STYLE_ZOOMBOX);
	psy_ui_component_set_style_type_focus(&self->component, psy_ui_STYLE_FOCUS);
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_zero());
	psy_ui_button_init_connect(&self->zoom_out_, zoombox_base(self),
		self, zoombox_on_zoom_out);
	/* zoom out */
	psy_ui_button_prevent_translation(&self->zoom_out_);
	psy_ui_button_set_text(&self->zoom_out_, "-");	
	psy_ui_button_set_char_number(&self->zoom_out_, 2.0);
	/* zoom */
	psy_ui_edit_init(&self->zoom_, zoombox_base(self));	
	psy_ui_edit_set_char_number(&self->zoom_, 7);
	psy_ui_edit_enable_input_field(&self->zoom_);
	psy_ui_component_set_preferred_height(&self->zoom_.component,
		psy_ui_value_make_eh(1.0));
	psy_signal_connect(&self->zoom_.signal_accept, self,
		zoombox_on_edit_accept);
	psy_signal_connect(&self->zoom_.signal_reject, self,
		zoombox_on_edit_reject);
	zoombox_update(self);
	/* zoom in */
	psy_ui_button_init_connect(&self->zoom_in_, zoombox_base(self),
		self, zoombox_on_zoom_in);
	psy_ui_button_prevent_translation(&self->zoom_in_);
	psy_ui_button_set_text(&self->zoom_in_, "+");	
	psy_ui_button_set_char_number(&self->zoom_in_, 2.0);
	/* inputhandler */
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)zoombox_on_input);
}

void zoombox_init_connect(ZoomBox* self, psy_ui_Component* parent,
	void* context, void* fp)
{
	assert(self);

	zoombox_init(self, parent);
	psy_signal_connect(&self->signal_changed, context, fp);
}

void zoombox_init_exchange(ZoomBox* self, psy_ui_Component* parent,
	psy_Property* property)
{
	assert(self);

	zoombox_init(self, parent);
	self->property_ = property;
	if (self->property_) {
		zoombox_exchange(self, self->property_);
	}
}

ZoomBox* zoombox_alloc(void)
{
	return (ZoomBox*)malloc(sizeof(ZoomBox));
}

ZoomBox* zoombox_alloc_init(psy_ui_Component* parent)
{
	ZoomBox* rv;

	rv = zoombox_alloc();
	if (rv) {
		zoombox_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

ZoomBox* zoombox_alloc_init_exchange(psy_ui_Component* parent,
	psy_Property* property)
{
	ZoomBox* rv;

	rv = zoombox_alloc();
	if (rv) {
		zoombox_init_exchange(rv, parent, property);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void zoombox_on_destroyed(ZoomBox* self)
{
	assert(self);

	if (self->property_) {
		psy_property_disconnect(self->property_, self);
	}
	psy_signal_dispose(&self->signal_changed);
	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),
		self, zoombox_on_input);
}


void zoombox_exchange(ZoomBox* self, psy_Property* property)
{	
	assert(self);
	assert(property);

	self->property_ = property;
	if (property) {
		zoombox_on_property_changed(self, property);		
		psy_property_connect(property,
			self, zoombox_on_property_changed);
		psy_signal_connect(&self->property_->before_destroyed, self,
			zoombox_before_property_destroyed);
	}
}

void zoombox_on_property_changed(ZoomBox* self,
	psy_Property* sender)
{
	assert(self);
	
	if (self->zoom_rate_ != psy_property_item_double(sender)) {
		self->zoom_rate_ = psy_property_item_double(sender);
		zoombox_update(self);
	}
}

void zoombox_before_property_destroyed(ZoomBox* self, psy_Property* sender)
{
	assert(self);

	self->property_ = NULL;
}

void zoombox_on_zoom_in(ZoomBox* self, psy_ui_Component* sender)
{
	assert(self);

	zoombox_set_rate(self,  self->zoom_rate_ + self->zoom_step_);
}

void zoombox_on_zoom_out(ZoomBox* self, psy_ui_Component* sender)
{
	assert(self);

	zoombox_set_rate(self, self->zoom_rate_ - self->zoom_step_);
}

void zoombox_set_rate(ZoomBox* self, double rate)
{	
	assert(self);
	
	rate = psy_max(self->range_.first, rate);
	rate = psy_min(self->range_.second, rate);
	if (rate != self->zoom_rate_) {
		self->zoom_rate_ = rate;
		zoombox_update(self);
		if (self->property_) {
			psy_property_set_item_double(self->property_, self->zoom_rate_);
		}
		psy_signal_emit(&self->signal_changed, self, 0, self->zoom_rate_);		
	}	
}

void zoombox_update(ZoomBox* self)
{
	char text[64];

	assert(self);

	psy_snprintf(text, 64, "%d%%", (int)(self->zoom_rate_ * 100 + 0.5));
	psy_ui_edit_set_text(&self->zoom_, text);
}

void zoombox_set_step(ZoomBox* self, double step)
{
	assert(self);

	self->zoom_step_ = step;
}

void zoombox_on_mouse_wheel(ZoomBox* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (self->prevent_wheel_select_) {		
		return;
	}
	if (psy_ui_mouseevent_delta(ev) != 0) {
		zoombox_set_rate(self, self->zoom_rate_ +
			psy_sgn(psy_ui_mouseevent_delta(ev)) * self->zoom_step_);
	}
	psy_ui_mouseevent_prevent_default(ev);
	psy_ui_mouseevent_stop_propagation(ev);
}

void zoombox_on_edit_accept(ZoomBox* self, psy_ui_Edit* sender)
{
	const char* text;		

	assert(self);

	text = psy_ui_edit_text(&self->zoom_);
	if (text) {				
		char temp[64];
		char* p;
		int rate;

		psy_snprintf(temp, 64, "%s", text);
		if ((p = strchr(temp, '%'))) {
			*p = '\0';
		}
		rate = atoi(temp);
		zoombox_set_rate(self,  psy_min(self->range_.second,
			psy_max(self->range_.first, (double)(rate / 100.0))));
		psy_ui_component_set_focus(&self->component);
	}
}

void zoombox_on_edit_reject(ZoomBox* self, psy_ui_Edit* sender)
{	
	assert(self);

	psy_ui_component_set_focus(&self->component);
}

bool zoombox_on_input(ZoomBox* self, InputHandler* sender)
{
	psy_ui_Component* restore;

	assert(self);

	restore = psy_ui_app_focus(psy_ui_app());
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVUP: {
		zoombox_on_zoom_in(self, NULL);		
		return TRUE; }
	case CMD_NAVDOWN: {
		zoombox_on_zoom_out(self, NULL);		
		return TRUE; }	
	default:
		return FALSE;
	}
}
