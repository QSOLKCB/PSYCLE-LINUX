/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uinumberedit.h"
/* local */
#include "uiapp.h"
/* std */
#include <ctype.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static bool psy_ui_numberedit_isedit(char c);
static void psy_ui_numberedit_on_destroyed(psy_ui_NumberEdit*);
static void psy_ui_numberedit_on_less(psy_ui_NumberEdit*,
	psy_ui_Component* sender);
static void psy_ui_numberedit_on_more(psy_ui_NumberEdit*,
	psy_ui_Component* sender);
static void psy_ui_numberedit_on_edit_key_down(psy_ui_NumberEdit*,
	psy_ui_Component* sender, psy_ui_KeyboardEvent*);
static void psy_ui_numberedit_on_edit_key_up(psy_ui_NumberEdit*,
	psy_ui_Component* sender, psy_ui_KeyboardEvent*);
static void psy_ui_numberedit_on_edit_accept(psy_ui_NumberEdit*,
	psy_ui_Component* sender);
static bool psy_ui_numberedit_has_range(const psy_ui_NumberEdit*);
static void psy_ui_numberedit_update_display(psy_ui_NumberEdit*,
	double value);
static double psy_ui_numberedit_display_to_value(psy_ui_NumberEdit*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_NumberEdit* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_numberedit_on_destroyed;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void psy_ui_numberedit_init(psy_ui_NumberEdit* self, psy_ui_Component* parent,
	const char* desc, double value, double minval, double maxval)
{
	assert(self);

	psy_ui_component_init(psy_ui_numberedit_base(self), parent, NULL);
	vtable_init(self);
	self->value = value;
	self->minval = minval;
	self->maxval = maxval;
	if (psy_ui_numberedit_has_range(self)) {
		value = psy_min(psy_max(value, self->minval), self->maxval);
	}
	self->restore = value;
	self->type = psy_ui_NUMBEREDIT_TYPE_INT;
	self->format = psy_ui_NUMBEREDIT_FORMAT_DEC;
	psy_ui_component_set_align_expand(psy_ui_numberedit_base(self),
		psy_ui_HEXPAND);
	psy_ui_component_set_default_align(psy_ui_numberedit_base(self),
		psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	psy_ui_label_init(&self->desc, psy_ui_numberedit_base(self));
	psy_ui_label_set_text(&self->desc, desc);
	psy_ui_edit_init(&self->edit, psy_ui_numberedit_base(self));	
	psy_ui_edit_set_char_number(&self->edit, 5);
	psy_ui_numberedit_update_display(self, self->value);
	psy_ui_edit_enable_input_field(&self->edit);
	psy_ui_button_init_resource_connect(&self->less,
		psy_ui_numberedit_base(self), "img.icon-less",
		self, psy_ui_numberedit_on_less);		
	psy_ui_button_init_resource_connect(&self->more,
		psy_ui_numberedit_base(self), "img.icon-more",
		self, psy_ui_numberedit_on_more);	
	psy_signal_init(&self->signal_changed);	
	psy_signal_connect(&self->edit.pane.component.signal_key_down, self,
		psy_ui_numberedit_on_edit_key_down);	
	psy_signal_connect(&self->edit.pane.component.signal_key_up, self,
		psy_ui_numberedit_on_edit_key_up);
	psy_signal_connect(&self->edit.signal_accept, self,
		psy_ui_numberedit_on_edit_accept);	
}

void psy_ui_numberedit_init_connect(psy_ui_NumberEdit* self,
	psy_ui_Component* parent, const char* desc, double value,
	double minval, double maxval, void* context, void* fp)
{
	assert(self);

	psy_ui_numberedit_init(self, parent, desc, value, minval, maxval);
	psy_signal_connect(&self->signal_changed, context, fp);
}

void psy_ui_numberedit_on_destroyed(psy_ui_NumberEdit* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_changed);
}

void psy_ui_numberedit_set_type(psy_ui_NumberEdit* self,
	psy_ui_NumberEditType type)
{
	assert(self);
	
	self->type = type;
	psy_ui_numberedit_update_display(self, psy_ui_numberedit_value(self));
}

void psy_ui_numberedit_set_format(psy_ui_NumberEdit* self,
	psy_ui_NumberEditFormat format)
{
	assert(self);
	
	self->format = format;
	psy_ui_numberedit_update_display(self, psy_ui_numberedit_value(self));
}

double psy_ui_numberedit_value(psy_ui_NumberEdit* self)
{
	assert(self);

	return self->value;	
}

void psy_ui_numberedit_set_value(psy_ui_NumberEdit* self, double value)
{
	assert(self);

	if (value == psy_ui_numberedit_value(self)) {
		return;
	}
	if (psy_ui_numberedit_has_range(self)) {
		value = psy_min(psy_max(value, self->minval), self->maxval);
	}	
	psy_ui_numberedit_update_display(self, value);	
	self->value = value;
	self->restore = value;
}

void psy_ui_numberedit_update_display(psy_ui_NumberEdit* self,
	double value)
{
	char text[128];	

	assert(self);
	
	if (self->type == psy_ui_NUMBEREDIT_TYPE_INT) {
		if (self->format == psy_ui_NUMBEREDIT_FORMAT_DEC) {
			psy_snprintf(text, 128, "%d", (int)value);
		} else {
			psy_snprintf(text, 128, "%x", (int)value);		
		}
	} else {
		psy_snprintf(text, 128, "%.2f", value);
	}
	psy_ui_edit_set_text(&self->edit, text);		
}

double psy_ui_numberedit_display_to_value(psy_ui_NumberEdit* self)
{
	if (self->type == psy_ui_NUMBEREDIT_TYPE_INT) {
		if (self->format == psy_ui_NUMBEREDIT_FORMAT_DEC) {
			return (double)floor(atof(psy_ui_edit_text(&self->edit)));
		} else {
			return (double)strtoll(psy_ui_edit_text(&self->edit), NULL, 16);
		}
	}
	return (double)atof(psy_ui_edit_text(&self->edit));
}

void psy_ui_numberedit_enable(psy_ui_NumberEdit* self)
{
	assert(self);

	psy_ui_edit_enable(&self->edit);
}

void psy_ui_numberedit_prevent(psy_ui_NumberEdit* self)
{
	assert(self);

	psy_ui_edit_prevent(&self->edit);
}

void psy_ui_numberedit_set_edit_char_number(psy_ui_NumberEdit* self,
	int charnumber)
{
	assert(self);

	psy_ui_edit_set_char_number(&self->edit, charnumber);
}

void psy_ui_numberedit_set_desc_char_number(psy_ui_NumberEdit* self,
	int charnumber)
{
	assert(self);

	psy_ui_label_set_char_number(&self->desc, charnumber);
}

void psy_ui_numberedit_on_less(psy_ui_NumberEdit* self,
	psy_ui_Component* sender)
{
	assert(self);

	psy_ui_numberedit_set_value(self, psy_ui_numberedit_value(self) - 1.0);
	psy_signal_emit(&self->signal_changed, self, 0);
}

void psy_ui_numberedit_on_more(psy_ui_NumberEdit* self,
	psy_ui_Component* sender)
{
	assert(self);

	psy_ui_numberedit_set_value(self, psy_ui_numberedit_value(self) + 1.0);
	psy_signal_emit(&self->signal_changed, self, 0);
}

void psy_ui_numberedit_on_edit_key_down(psy_ui_NumberEdit* self,
	psy_ui_Component* sender, psy_ui_KeyboardEvent* ev)
{	
	assert(self);
	
	if (!((self->format == psy_ui_NUMBEREDIT_FORMAT_HEX) &&
		(isxdigit(psy_ui_keyboardevent_keycode(ev)))) &&
		!psy_ui_numberedit_isedit(psy_ui_keyboardevent_keycode(ev)) &&
		(isalpha(psy_ui_keyboardevent_keycode(ev)) ||
			(psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE))) {
		psy_ui_numberedit_set_value(self, self->restore);
		psy_ui_keyboardevent_prevent_default(ev);
		return;
	}
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN) {
		double value;

		psy_ui_component_set_focus(&self->component);
		psy_ui_keyboardevent_prevent_default(ev);
		value = psy_ui_numberedit_display_to_value(self);
		if (self->maxval != 0 && self->minval != 0) {
			value = psy_min(psy_max(value, self->minval), self->maxval);
		}
		psy_ui_numberedit_set_value(self, value);
		psy_signal_emit(&self->signal_changed, self, 0);
	}
	psy_ui_keyboardevent_stop_propagation(ev);
}

bool psy_ui_numberedit_isedit(char c)
{
	switch (c) {
	case psy_ui_KEY_C:		    
	case psy_ui_KEY_V:
	case psy_ui_KEY_ESCAPE:
	case psy_ui_KEY_LEFT:
	case psy_ui_KEY_RIGHT:
	case psy_ui_KEY_UP:		
	case psy_ui_KEY_DOWN:		
	case psy_ui_KEY_PRIOR:		
	case psy_ui_KEY_NEXT:		
	case psy_ui_KEY_HOME:
	case psy_ui_KEY_END:
	case psy_ui_KEY_BACK:
	case psy_ui_KEY_DELETE:	
	case psy_ui_KEY_SPACE:
	case psy_ui_KEY_TAB:
	case psy_ui_KEY_SHIFT:
	case psy_ui_KEY_CONTROL:
	case psy_ui_KEY_MENU:
	case psy_ui_KEY_RETURN:
		return TRUE;
	default:
		return FALSE;
	}
}

void psy_ui_numberedit_on_edit_key_up(psy_ui_NumberEdit* self,
	psy_ui_Component* sender, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	psy_ui_keyboardevent_stop_propagation(ev);
}

void psy_ui_numberedit_on_edit_accept(psy_ui_NumberEdit* self,
	psy_ui_Component* sender)
{
	double value;
	
	assert(self);

	value = psy_ui_numberedit_display_to_value(self);
	if (self->maxval != 0 && self->minval != 0) {
		value = psy_min(psy_max(value, self->minval), self->maxval);
	}
	psy_ui_numberedit_set_value(self, value);
	psy_signal_emit(&self->signal_changed, self, 0);
}

bool psy_ui_numberedit_has_range(const psy_ui_NumberEdit* self)
{
	return ((self->maxval != 0) || (self->minval != 0));
}
