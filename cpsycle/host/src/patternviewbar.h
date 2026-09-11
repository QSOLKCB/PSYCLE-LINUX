/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEWBAR_H)
#define PATTERNVIEWBAR_H

/* host */
#include "stepbox.h"
#include "zoombox.h"
#include "workspace.h"
/* ui */
#include <uicheckbox.h>
#include <uilabel.h>
#include <uisizer.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** PatternViewStatus
**
** Displays the pattern cursor position.
*/

typedef struct PatternViewStatus {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label pat_desc;
	psy_ui_Label pat;
	psy_ui_Label ln_desc;
	psy_ui_Label ln;
	psy_ui_Label trk_desc;
	psy_ui_Label trk;
	psy_ui_Label col_desc;
	psy_ui_Label col;
	psy_ui_Label oct_desc;
	psy_ui_Label oct;
	psy_ui_Label mode;
	psy_ui_Label follow;
	/* references */
	Workspace* workspace;
} PatternViewStatus;

void patternviewstatus_init(PatternViewStatus* self, psy_ui_Component* parent,
	Workspace*);

void patternviewstatus_update(PatternViewStatus*);

/*
** PatternViewBar
**
** The bar displayed in the mainframe status bar, if the patternview is active
*/

typedef struct PatternViewBar {
	/*! @extends  */
	psy_ui_Component component;	
	/*! @internal */
	ZoomBox zoombox;
	PatternCursorStepBox cursorstep;
	psy_ui_CheckBox movecursorwhenpaste;
	psy_ui_CheckBox defaultentries;
	psy_ui_CheckBox displaysinglepattern;
	PatternViewStatus status;
	psy_ui_Sizer sizer_;
	/* references */
	psy_Configuration* patconfig;
	Workspace* workspace;	
} PatternViewBar;

void patternviewbar_init(PatternViewBar*, psy_ui_Component* parent,
	psy_Configuration*, Workspace*);

INLINE double patternviewbar_zoom(const PatternViewBar* self)
{
	return zoombox_rate(&self->zoombox);
}

INLINE psy_ui_Component* patternviewbar_base(PatternViewBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PATTERNVIEWBAR_H */
