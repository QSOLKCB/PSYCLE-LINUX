/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/ 

#if !defined(PATTERNEDITBAR_H)
#define PATTERNEDITBAR_H

/* host */
#include "patternviewstate.h"
#include "pianogridstate.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PatternEditorNoteMode {
	PATTERN_EDT_NOTE_MODE_ENDLESS,
	PATTERN_EDT_NOTE_MODE_SEMI_BREVE,
	PATTERN_EDT_NOTE_MODE_MINIM,
	PATTERN_EDT_NOTE_MODE_MINIM_DOT,
	PATTERN_EDT_NOTE_MODE_CROTCHET,
	PATTERN_EDT_NOTE_MODE_CROTCHET_DOT,
	PATTERN_EDT_NOTE_MODE_QUAVER,
	PATTERN_EDT_NOTE_MODE_QUAVER_DOT,
	PATTERN_EDT_NOTE_MODE_SEMI_QUAVER,
	PATTERN_EDT_NOTE_MODE_SELECT
} PatternEditorNoteMode;

/*!
** @struct PatternEditBar
** @brief Note duration selection bar
*/
typedef struct PatternEditBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button endless;
	psy_ui_Button semibreve;
	psy_ui_Button minim;
	psy_ui_Button minim_dot;
	psy_ui_Button crotchet;
	psy_ui_Button crotchet_dot;
	psy_ui_Button icon_quaver;
	psy_ui_Button quaver_dot;
	psy_ui_Button semiquaver;
	psy_ui_Button select;
	/* references */
	Workspace* workspace_;
	PatternViewState* state;
	PianoGridState* gridstate;
} PatternEditBar;

void patterneditbar_init(PatternEditBar*, psy_ui_Component* parent,
	PianoGridState*, PatternViewState*, Workspace*);
	
void patterneditbar_update_selection(PatternEditBar*);

INLINE psy_ui_Component* patterneditbar_base(PatternEditBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PATTERNEDITBAR_H */
