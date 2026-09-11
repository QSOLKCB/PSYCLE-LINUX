/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "noteselect.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* local */
#include "cmddef.h"
#include "cmdsnotes.h"
#include "patternnavigator.h"
/* ui */
#include <trackercmds.h>
/* audio */
#include <sequencecmds.h>
/* platform */
#include "../../detail/portable.h"

#define ISDIGIT TRUE

static void noteselect_connect_input_handler(NoteSelect*, InputHandler*);
static bool noteselect_on_input(NoteSelect*, InputHandler* sender);
static bool noteselect_handle_command(NoteSelect*, psy_EventDriverCmd cmd);

/* NoteSelect */
void noteselect_init(NoteSelect* self, psy_ui_Component* parent,
	psy_ui_Component* pattern, psy_audio_Player* player)
{
	assert(self);	
	assert(player);

	psy_ui_component_init(&self->component, parent, NULL);
	self->cmd = 0;
	self->player = player;
	self->pattern = pattern;
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_BUTTON_FOCUS);
	psy_ui_component_set_tab_index(&self->component, 0);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_app_defaults_const()));
	psy_ui_label_init_text(&self->desc, noteselect_base(self), "Enter");
	psy_ui_label_init(&self->note, noteselect_base(self));	
	noteselect_set_cmd(self, self->cmd);
	noteselect_connect_input_handler(self, psy_ui_app_input_handler(psy_ui_app()));
}

void noteselect_set_cmd(NoteSelect* self, uint8_t cmd)
{	
	uint8_t note;

	assert(self);

	self->cmd = cmd;
	note = (uint8_t)self->cmd + (uint8_t)psy_audio_player_octave(self->player) * 12;	
	psy_ui_label_set_text(&self->note, psy_dsp_notetostr(note, psy_dsp_NOTESTAB_A440));
}

uint8_t noteselect_cmd(const NoteSelect* self)
{
	assert(self);

	return self->cmd;
}

void noteselect_connect_input_handler(NoteSelect* self, InputHandler* input_handler)
{
	assert(self);

	if (!input_handler) {
		return;
	}
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, &self->component,
		(fp_inputhandler_input)noteselect_on_input);
}


bool noteselect_on_input(NoteSelect* self, InputHandler* sender)
{
	assert(self);

	return noteselect_handle_command(self, inputhandler_cmd(sender));
}

bool noteselect_handle_command(NoteSelect* self, psy_EventDriverCmd cmd)
{
	assert(self);

	switch (psy_eventdrivercmd_id(&cmd)) {		
	case CMD_NAVSELECT:
		noteselect_enter(self);
		return TRUE;		
	case CMD_NAVUP:
		if (self->cmd < 12) {
			noteselect_set_cmd(self, noteselect_cmd(self) + 1);
		}
		return TRUE;
	case CMD_NAVDOWN:
		if (self->cmd > 0) {
			noteselect_set_cmd(self, noteselect_cmd(self) - 1);
		}		
		return TRUE;
	default:
		return FALSE;
	}
}

void noteselect_enter(NoteSelect* self)
{	
	psy_ui_Component* restore;

	assert(self);

	restore = psy_ui_app_focus(psy_ui_app());
	if (self->pattern) {
		psy_ui_component_set_focus(self->pattern);
	}
	inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "notes",
		psy_eventdrivercmd_make_cmd(
		noteselect_cmd(self)));
	if (restore) {
		psy_ui_component_set_focus(restore);
	}
}

#endif /* PSYCLE_USE_TRACKERVIEW */
