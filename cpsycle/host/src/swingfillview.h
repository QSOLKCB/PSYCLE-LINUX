/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SWINGFILLVIEW_H)
#define SWINGFILLVIEW_H

/* host */
#include "patternviewstate.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uilabel.h>
#include <uinumberedit.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
* @struct SwingFillView
*/

typedef struct SwingFillView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component left;
	psy_ui_Button hide;
	psy_ui_Component client;
	psy_ui_NumberEdit tempo;
	psy_ui_NumberEdit width;
	psy_ui_NumberEdit variance;
	psy_ui_NumberEdit phase;
	psy_ui_Component offsetrow;
	psy_ui_Button actual_bpm;
	psy_ui_Button center_bpm;
	psy_ui_Label offsetdesc;	
	psy_ui_Component actions;
	psy_ui_Button apply;	
	bool offset;
	bool fill_track_;
	/* references */	
	PatternViewState* pvstate;
} SwingFillView;

void swingfillview_init(SwingFillView*, psy_ui_Component* parent,
	PatternViewState*);

void swingfillview_reset(SwingFillView* self, int bpm);
void swingfillview_set_values(SwingFillView*, int tempo, int width, double variance,
	double phase, bool offset);

INLINE void swingfillview_use_track(SwingFillView* self)
{
	assert(self);

	self->fill_track_ = TRUE;
}

INLINE void swingfillview_use_block(SwingFillView* self)
{
	assert(self);

	self->fill_track_ = FALSE;
}

INLINE psy_ui_Component* swingfillview_base(SwingFillView* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SWINGFILLVIEW_H */
