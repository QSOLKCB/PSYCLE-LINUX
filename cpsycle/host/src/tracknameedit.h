/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TRACKNAMEEDIT_H)
#define TRACKNAMEEDIT_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* host */
#include "closebar.h"
#include "patternviewstate.h"
#include "workspace.h"
/* audio */
#include <pattern.h>
/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uilistbox.h>
#include <uinumberedit.h>
#include <uitext.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HeaderStyleSwitch {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label label;
	psy_ui_Button classic;
	psy_ui_Button text;
	psy_ui_Label hint;
	/* references */
	Workspace* workspace;
} HeaderStyleSwitch;

void headerstyleswitch_init(HeaderStyleSwitch*, psy_ui_Component* parent,
	Workspace* workspace);

typedef struct TrackNaming {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button names_share;
	psy_ui_Button names_individual;
	/* references */
	PatternViewState* state;
} TrackNaming;

void tracknaming_init(TrackNaming*, psy_ui_Component* parent,
	PatternViewState*);

typedef struct TrackNameEdit {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	CloseBar close_bar;	
	psy_ui_Component client;
	psy_ui_Label name_label;
	psy_ui_Edit name_edit;
	psy_ui_ListBox names;
	psy_ui_Component left;
	HeaderStyleSwitch style_switch;
	TrackNaming naming;	
	/* references */	
	uintptr_t pattern_index;
	Workspace* workspace;
	PatternViewState* state;
} TrackNameEdit;

void tracknameedit_init(TrackNameEdit*, psy_ui_Component* parent,
	PatternViewState*, Workspace* workspace);

INLINE psy_ui_Component* tracknameedit_base(TrackNameEdit* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_PATTERN_VIEW */

#endif /* TRACKNAMEEDIT_H */

