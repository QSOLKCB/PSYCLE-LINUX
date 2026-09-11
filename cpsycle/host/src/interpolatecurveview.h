/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(INTERPOLATECURVEVIEW_H)
#define INTERPOLATECURVEVIEW_H

/* host */
#include "closebar.h"
#include "patternviewstate.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uicombobox.h>
#include <uinumberedit.h>


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct InterpolateCurveState
*/
typedef struct InterpolateCurveState {	
	psy_Property minval_;
	psy_Property maxval_;
	psy_Property tweak_;
	psy_Property curve_;
	uintptr_t opcount_;
} InterpolateCurveState;

void interpolatecurvestate_init(InterpolateCurveState*);

int interpolatecurvestate_tweak_type(InterpolateCurveState*);
void interpolatecurvestate_update_range(InterpolateCurveState*,
	psy_audio_Machine*, psy_audio_MachineParam*);

/*!
** @struct InterpolateCurveBox
*/
typedef struct InterpolateCurveBox {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_dsp_beatpos_t range_;	
	uintptr_t opcount_;
	psy_ui_Margin spacing_;
	double cx_;
	double cy_;
	/* references */
	InterpolateCurveState* interpolate_state_;
	PatternViewState* pv_state_;
	psy_audio_PatternNode* tweak_node_;
	psy_audio_Pattern* tweak_pattern_;
	Workspace* workspace_;
} InterpolateCurveBox;

void interpolatecurvebox_init(InterpolateCurveBox*, psy_ui_Component* parent,
	InterpolateCurveState*, PatternViewState*, Workspace*);

void interpolatecurvebox_interpolate(InterpolateCurveBox*);

INLINE psy_ui_Component* interpolatecurvebox_base(InterpolateCurveBox* self)
{
	assert(self);

	return &self->component;
}


/*!
** @struct InterpolateCurveBar
*/
typedef struct InterpolateCurveBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_ComboBox note_cmd__;	
	psy_ui_ComboBox curve_type_;
	psy_ui_Label minval_desc_;
	psy_ui_Edit minval_;
	psy_ui_Label maxval_desc_;
	psy_ui_Edit maxval_;
	psy_ui_Button mac_param_range_;
	psy_ui_Button interpolate_;
	/* references */
	InterpolateCurveState* state;
	Workspace* workspace;
} InterpolateCurveBar;

void interpolatecurvebar_init(InterpolateCurveBar*, psy_ui_Component* parent,
	InterpolateCurveState*, Workspace*);

INLINE psy_ui_Component* interpolatecurvebar_base(InterpolateCurveBar* self)
{
	assert(self);

	return &self->component;
}


/*!
** @struct InterpolateCurveView
** @brief Edit automation points and interpolate_ tweaks
**
** @details
** Specifies how the values of the parameters column will change from line
** to line. Clicking the 'Interpolate' Button fills the parameters column
** from the beginning to the end of a selection according to the curve in
** this view.
*/
typedef struct InterpolateCurveView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	CloseBar close_;
	InterpolateCurveBox box_;
	InterpolateCurveBar bar_;
	InterpolateCurveState interpolate_state_;
} InterpolateCurveView;

void interpolatecurveview_init(InterpolateCurveView*, psy_ui_Component* parent,	
	PatternViewState*, Workspace*);

INLINE psy_ui_Component* interpolatecurveview_base(InterpolateCurveView* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* INTERPOLATECURVEVIEW_H */
