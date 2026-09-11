/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQEDITRULER_H)
#define SEQEDITRULER_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "seqeditorstate.h"
#include "workspace.h"
/* ui */
#include <uitextarea.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SeqEditRuler */
typedef struct SeqEditRuler {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Colour overlaycolour;
	psy_ui_Colour linecolour;
	psy_ui_Colour linebeatcolour;
	psy_ui_Colour linebeat4colour;
	double baseline;
	/* references */
	SeqEditState* state;	
} SeqEditRuler;

void seqeditruler_init(SeqEditRuler*, psy_ui_Component* parent,
	SeqEditState*);
	
void seqeditruler_update_cursor_position(SeqEditRuler*);
void seqeditruler_update_edit_position(SeqEditRuler*);
void seqeditruler_update_play_position(SeqEditRuler*);

INLINE psy_ui_Component* seqeditruler_base(SeqEditRuler* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_SEQEDITOR */

#endif /* SEQEDITRULER_H */
