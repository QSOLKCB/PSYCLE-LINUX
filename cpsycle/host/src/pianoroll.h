/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PIANOROLL_H)
#define PIANOROLL_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PIANOROLL

/* host */
#include "patterneditbar.h"
#include "patternhostcmds.h"
#include "pianokeyboard.h"
#include "paramroll.h"
#include "pianoruler.h"
#include "pianogrid.h"
#include "workspace.h"
#include "zoombox.h"
/* ui */
#include <uicombobox.h>
#include <uilabel.h>
#include <uiscroller.h>
#include <uisplitbar.h>
/* audio */
#include <pattern.h>

#ifdef __cplusplus
extern "C" {
#endif

/* PianoBar */
typedef struct PianoBar {
	/*! @extends  */
	psy_ui_Component component;
	ZoomBox zoombox_beatwidth;
	ZoomBox zoombox_keyheight;
	psy_ui_Label beats;
	psy_ui_Label keys;	
	psy_ui_Label tracks;
	psy_ui_Button tracks_all;
	psy_ui_Button track_curr;
	psy_ui_Button tracks_active;	
} PianoBar;

void pianobar_init(PianoBar*, psy_ui_Component* parent, PianoGridState*);

INLINE psy_ui_Component* pianobar_base(PianoBar* self)
{
	assert(self);

	return &self->component;
}

/*!
** @struct Pianoroll
** @brief The Pianoroll is another way than the normal tracker view to enter
** notes.
**
** @details
** It displays a Pattern selected by the SeqView in a roll with a piano
** keyboard at the left and the time line to the right.
*/
typedef struct Pianoroll {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component ruler_pane;
	PianoRuler ruler;
	psy_ui_Component left_top;	
	psy_ui_ComboBox keytype;
	psy_ui_Component client;	
	Pianogrid grid;
	psy_ui_Scroller scroller;	
	PianoBar bar;
	PatternEditBar editbar;
	KeyboardState keyboardstate;
	psy_ui_Component keyboardpane;
	PianoKeyboard keyboard;	
	PianoGridState gridstate;
	ParamRoll param_roll;	
	psy_ui_Splitter splitter;	
	psy_ui_ScrollBar hscroll;
	uintptr_t opcount;
	bool center_key;		
	/* references */
	psy_audio_Player* player;
	Workspace* workspace;
} Pianoroll;

void pianoroll_init(Pianoroll*, psy_ui_Component* parent, PatternViewState*,
	psy_DiskOp*, Workspace*);
void pianoroll_scroll_to_key(Pianoroll*, uint8_t key);
void pianoroll_align(Pianoroll*, bool keys, bool grid);
void pianoroll_make_cmds(psy_Property* parent);

INLINE psy_ui_Component* pianoroll_base(Pianoroll* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_PIANOROLL */

#endif /* PIANOROLL_H */
