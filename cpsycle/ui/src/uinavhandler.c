/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uinavhandler.h"
/* local */
#include "uiapp.h"
#include "uicomponent.h"
#include "trackercmds.h"

#include "../../detail/trace.h"

#define CS_EDT_START 512

/* prototypes */
static void psy_ui_navhandler_connect_input_handler(psy_ui_NavHandler*,
	InputHandler*);
static bool psy_ui_navhandler_on_input(psy_ui_NavHandler*, InputHandler* sender);
static bool psy_ui_navhandler_handle_command(psy_ui_NavHandler*, psy_EventDriverCmd);
static void psy_ui_navhandler_prev(psy_ui_NavHandler*);
static void psy_ui_navhandler_next(psy_ui_NavHandler*);
static bool psy_ui_navhandler_vertical(psy_ui_NavHandler*);

/* implementation */
void psy_ui_navhandler_init(psy_ui_NavHandler* self)
{
	assert(self);
		
	self->send_cmd = TRUE;	
	psy_signal_init(&self->signal_selected);			
}

void psy_ui_navhandler_dispose(psy_ui_NavHandler* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_selected);
}

void psy_ui_navhandler_connect(psy_ui_NavHandler* self,
	void* context, void* fp)	
{
	assert(self);

	psy_signal_connect(&self->signal_selected, context, fp);	
}

void psy_ui_navhandler_start(psy_ui_NavHandler* self)
{
	assert(self);

	psy_ui_navhandler_connect_input_handler(self, psy_ui_app_input_handler(
		psy_ui_app()));
}

void psy_ui_navhandler_connect_input_handler(psy_ui_NavHandler* self,
	InputHandler* input_handler)
{
	assert(self);
	
	inputhandler_connect(input_handler, INPUTHANDLER_IMM,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_navhandler_on_input);
}


bool psy_ui_navhandler_on_input(psy_ui_NavHandler* self, InputHandler* sender)
{
	assert(self);

	return psy_ui_navhandler_handle_command(self, inputhandler_cmd(sender));
}

bool psy_ui_navhandler_handle_command(psy_ui_NavHandler* self, psy_EventDriverCmd cmd)
{
	psy_ui_Component* focus;	

	assert(self);	

	focus = psy_ui_app_focus(psy_ui_app());	
	if (!focus) {
		return FALSE;
	}		
	if (psy_ui_navhandler_vertical(self)) {
		switch (psy_eventdrivercmd_id(&cmd)) {
		case CMD_NAVUP:
			psy_ui_navhandler_prev(self);
			return TRUE;
		case CMD_NAVDOWN:
			psy_ui_navhandler_next(self);
			return TRUE;
		case CMD_NAVSELECT:
			psy_ui_navhandler_execute(self, psy_ui_component_id(focus));
			psy_signal_emit(&self->signal_selected, self, 0);
			return TRUE;
		default:
			return FALSE;
		}
	} else {
		switch (psy_eventdrivercmd_id(&cmd)) {
		case CMD_NAVLEFT:			
			psy_ui_navhandler_prev(self);			
			return TRUE;
		case CMD_NAVRIGHT:			
			psy_ui_navhandler_next(self);			
			return TRUE;
		case CMD_NAVSELECT:
			psy_ui_navhandler_execute(self, psy_ui_component_id(focus));
			psy_signal_emit(&self->signal_selected, self, 0);
			return TRUE;
		default:
			return FALSE;
		}
	}
}

void psy_ui_navhandler_prev(psy_ui_NavHandler* self)
{
	psy_ui_Component* focus;
	psy_ui_Component* prev;

	assert(self);

	focus = psy_ui_app_focus(psy_ui_app());
	if (!focus) {
		return;
	}
	prev = psy_ui_component_prev_tab_index(focus);
	if (prev) {
		psy_ui_component_set_focus(prev);
	}
}

void psy_ui_navhandler_next(psy_ui_NavHandler* self)
{
	psy_ui_Component* focus;
	psy_ui_Component* next;

	assert(self);

	focus = psy_ui_app_focus(psy_ui_app());
	if (!focus) {
		return;
	}
	next = psy_ui_component_next_tab_index(focus);
	if (next) {
		psy_ui_component_set_focus(next);
	}
}

void psy_ui_navhandler_execute(psy_ui_NavHandler* self, uintptr_t id)
{
	const char* section;

	assert(self);
	
	if (id == psy_INDEX_INVALID) {
		return;
	}
	if (id < CS_EDT_START) {
		section = "general";
	} else {
		section = "edit";
	}	
	psy_ui_component_align_invalidate(psy_ui_app_main(psy_ui_app()));
	if (self->send_cmd) {
		inputhandler_send(psy_ui_app_input_handler(psy_ui_app()),
			section, psy_eventdrivercmd_make_cmd(id));
	}
}

bool psy_ui_navhandler_vertical(psy_ui_NavHandler* self)
{
	psy_ui_Component* focus;
	psy_ui_Component* parent;
	psy_ui_AlignType align;
	psy_List* p;
	psy_List* q;
	bool rv;

	assert(self);

	focus = psy_ui_app_focus(psy_ui_app());
	if (!focus) {
		return FALSE;
	}
	parent = psy_ui_component_parent(focus);	
	if (!parent) {
		return FALSE;
	}
	rv = TRUE;
	q = psy_ui_component_children(parent, psy_ui_NONE_RECURSIVE, FALSE);
	for (p = q; p != NULL; p = p->next) {
		psy_ui_Component* component;

		component = (psy_ui_Component*)p->entry;
		if ((component->align != psy_ui_ALIGN_TOP &&
				component->align != psy_ui_ALIGN_BOTTOM &&
				component->align != psy_ui_ALIGN_CLIENT)) {
			rv = FALSE;
			break;
		}
	}	
	psy_list_free(q);
	return rv;
}
