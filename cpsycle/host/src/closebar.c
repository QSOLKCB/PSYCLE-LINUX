/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "closebar.h"
/* ui */
#include <uiapp.h>
#include <uiicons.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void closebar_on_destroyed(CloseBar*);
static void closebar_on_hide(CloseBar*, psy_ui_Button* sender);

/* vtable */
static psy_ui_ComponentVtable closebar_vtable;
static bool closebar_vtable_initialized = FALSE;

static void closebar_vtable_init(CloseBar* self)
{
	assert(self);

	if (!closebar_vtable_initialized) {
		closebar_vtable = *(self->component.vtable);
		closebar_vtable.on_destroyed =
			(psy_ui_fp_component)
			closebar_on_destroyed;
		closebar_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &closebar_vtable);
}

/* implementation */
void closebar_init(CloseBar* self, psy_ui_Component* parent,
	psy_Property* property)
{	
	assert(self);	
	
	psy_ui_component_init_align(&self->component, parent, NULL,
		psy_ui_ALIGN_LEFT);
	closebar_vtable_init(self);
	self->property = property;
	self->cmd = psy_eventdrivercmd_make_cmd(0);
	self->cmd_section = NULL;	
	self->custom = FALSE;
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_ui_button_init_resource_connect(&self->hide, &self->component, "img.icon-close",
		self, closebar_on_hide);
	psy_ui_component_set_align(psy_ui_button_base(&self->hide),
		psy_ui_ALIGN_TOP);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->hide));	
	psy_ui_component_set_preferred_size(&self->hide.component,
		psy_ui_size_make_em(1.0, 1.0));
}

void closebar_init_cmd(CloseBar* self, psy_ui_Component* parent,
	const char* section, psy_EventDriverCmd cmd)
{
	assert(self);

	closebar_init(self, parent, NULL);
	closebar_set_cmd(self, section, cmd);
}

void closebar_on_destroyed(CloseBar* self)
{
	assert(self);

	free(self->cmd_section);
	self->cmd_section = NULL;
}

void closebar_set_cmd(CloseBar* self, const char* section,
	psy_EventDriverCmd cmd)
{
	assert(self);
	
	self->cmd = cmd;
	psy_strreset(&self->cmd_section, section);	
}

void closebar_set_property(CloseBar* self, psy_Property* property)
{
	assert(self);
	
	self->property = property;
}

void closebar_set_custom_mode(CloseBar* self)
{
	self->custom = TRUE;
}

void closebar_on_hide(CloseBar* self, psy_ui_Button* sender)
{
	assert(self);

	if (self->custom) {
		return;
	}
	if (self->cmd.id != 0) {
		inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), self->cmd_section, self->cmd);
	} else if (self->property) {
		psy_property_set_item_bool(self->property, !psy_property_item_bool(
			self->property));
	} else {
		psy_ui_Component* p;
		
		p = psy_ui_component_parent(&self->component);
		psy_ui_component_hide_align(p);
		psy_ui_component_invalidate(psy_ui_component_parent(p));	
	}	
}
