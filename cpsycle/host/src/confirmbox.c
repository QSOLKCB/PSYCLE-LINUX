/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "confirmbox.h"


/* prototypes */
static void confirmbox_on_accept(ConfirmBox*, psy_ui_Component* sender);
static void confirmbox_on_reject(ConfirmBox*, psy_ui_Component* sender);
static void confirmbox_on_continue(ConfirmBox*, psy_ui_Component* sender);
static void confirmbox_on_focus(ConfirmBox*);

/* vtable */
static psy_ui_ComponentVtable confirmbox_vtable;
static bool confirmbox_vtable_initialized = FALSE;

static void confirmbox_vtable_init(ConfirmBox* self)
{
	if (!confirmbox_vtable_initialized) {
		confirmbox_vtable = *(self->component.vtable);		
		confirmbox_vtable.on_focus =
			(psy_ui_fp_component)
			confirmbox_on_focus;
		confirmbox_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(confirmbox_base(self), &confirmbox_vtable);
}

/* implementation */
void confirmbox_init(ConfirmBox* self, psy_ui_Component* parent)
{	
	psy_ui_Margin padding;
	
	assert(self);
	
	psy_ui_component_init(confirmbox_base(self), parent, NULL);
	confirmbox_vtable_init(self);
	self->signal_accept = psy_slot_make(NULL, NULL);
	self->signal_reject = psy_slot_make(NULL, NULL);
	self->signal_continue = psy_slot_make(NULL, NULL);
	psy_ui_component_set_tab_index(confirmbox_base(self), 0);
	psy_ui_component_init_align(&self->view_, confirmbox_base(self), NULL,
		psy_ui_ALIGN_CENTER);
	psy_ui_component_set_tab_index(&self->view_, 0);
	psy_ui_component_set_default_align(&self->view_,
		psy_ui_ALIGN_TOP, psy_ui_margin_zero());
	psy_ui_margin_init_em(&padding, 0.5, 0.0, 0.5, 0.0);
	psy_ui_label_init_text(&self->title_, &self->view_, "msg.psyreq");
	psy_ui_component_set_padding(&self->title_.component, padding);
	psy_ui_label_init_text(&self->header_, &self->view_, "");
	psy_ui_component_set_padding(&self->header_.component, padding);
	psy_ui_component_init_align(&self->buttons_, &self->view_, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_tab_index(&self->buttons_, 0);
	psy_ui_component_set_default_align(&self->buttons_, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());
	psy_ui_button_init_text_connect(&self->accept_, &self->buttons_, "msg.yes",
		self, confirmbox_on_accept);
	psy_ui_component_set_id(psy_ui_button_base(&self->accept_), CONFIRM_YES);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->accept_), 0);
	psy_ui_component_set_padding(&self->accept_.component, padding);
	psy_ui_button_init_text_connect(&self->reject_, &self->buttons_, "msg.no",
		self, confirmbox_on_reject);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->reject_), 0);
	psy_ui_component_set_id(psy_ui_button_base(&self->reject_), CONFIRM_NO);
	psy_ui_component_set_padding(&self->reject_.component, padding);
	psy_ui_button_init_text_connect(&self->continue_, &self->buttons_, "msg.cont",
		self, confirmbox_on_continue);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->continue_), 0);
	psy_ui_component_set_id(psy_ui_button_base(&self->continue_), CONFIRM_CONTINUE);
	psy_ui_component_set_padding(&self->continue_.component, padding);
}

void confirmbox_set_labels(ConfirmBox* self, const char* title,
	const char* yesstr, const char* nostr)
{		
	assert(self);
	
	psy_ui_label_set_text(&self->title_, title); 
	psy_ui_label_set_text(&self->header_, "");
	psy_ui_button_set_text(&self->accept_, yesstr);
	psy_ui_button_set_text(&self->reject_, nostr);
	psy_ui_button_set_text(&self->continue_, "msg.cont");
	psy_ui_component_align(&self->component);
}

void confirmbox_on_accept(ConfirmBox* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_slot_emit(&self->signal_accept);	
}

void confirmbox_on_reject(ConfirmBox* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_slot_emit(&self->signal_reject);	
}

void confirmbox_on_continue(ConfirmBox* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_slot_emit(&self->signal_continue);
}

void confirmbox_set_callbacks(ConfirmBox* self, psy_Slot accept,
	psy_Slot reject, psy_Slot cont)
{
	assert(self);
	
	self->signal_accept = accept;
	if (self->signal_accept.fp) {
		psy_ui_component_show(&self->accept_.component);
	}
	else {
		psy_ui_component_hide(&self->accept_.component);
	}
	self->signal_reject = reject;
	if (self->signal_reject.fp) {
		psy_ui_component_show(&self->reject_.component);
	} else {
		psy_ui_component_hide(&self->reject_.component);
	}
	self->signal_continue = cont;
	if (self->signal_continue.fp) {
		psy_ui_component_show(&self->continue_.component);
	} else {
		psy_ui_component_hide(&self->continue_.component);
	}
}

void confirmbox_on_focus(ConfirmBox* self)
{
	assert(self);

	psy_ui_component_set_focus(psy_ui_button_base(&self->accept_));
}
