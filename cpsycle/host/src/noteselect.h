/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(NOTESELECT)
#define NOTESELECT

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* ui */
#include <uilabel.h>
#include <uiscroller.h>
/* audio */
#include <player.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct NoteSelect
*/
typedef struct NoteSelect {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label desc;
	psy_ui_Label note;	
	uint8_t cmd;
	/* references */
	psy_audio_Player* player;
	psy_ui_Component* pattern;
} NoteSelect;

void noteselect_init(NoteSelect*, psy_ui_Component* parent,
	psy_ui_Component* pattern, psy_audio_Player*);

void noteselect_set_cmd(NoteSelect*, uint8_t note);
uint8_t noteselect_cmd(const NoteSelect*);
void noteselect_enter(NoteSelect*);

INLINE psy_ui_Component* noteselect_base(NoteSelect* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* NOTESELECT */
