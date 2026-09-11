/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMRACK_H)
#define PARAMRACK_H

/* host */
#include "closebar.h"
#include "paramview.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uinumberedit.h>
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ParamRackBox */

struct ParamRackBox;
struct ParamViews;

typedef struct ParamRackBox {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component header;
	psy_ui_Label title;
	psy_ui_Button inserteffect;
	psy_ui_Scroller scroller;
	ParamView parameters;
	/* references */	
	struct ParamRackBox* nextbox;		
	psy_audio_Machine* machine;
	struct ParamViews* param_views;
	Workspace* workspace;
} ParamRackBox;

void paramrackbox_init(ParamRackBox*, psy_ui_Component* parent,
	psy_audio_Machine*, Workspace*);

ParamRackBox* paramrackbox_alloc(void);
ParamRackBox* paramrackbox_alloc_init(psy_ui_Component* parent,
	psy_audio_Machine*, Workspace*);

void paramrackbox_select(ParamRackBox*);
void paramrackbox_deselect(ParamRackBox*);

/* ParamRackMode */
typedef enum ParamRackMode {
	PARAMRACK_NONE,
	PARAMRACK_ALL,
	PARAMRACK_INPUTS,
	PARAMRACK_OUTPUTS,
	PARAMRACK_INCHAIN,
	PARAMRACK_OUTCHAIN,
	PARAMRACK_LEVEL
} ParamRackMode;

typedef struct ParamRackBatchBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button solo;
	psy_ui_Button mute;
	psy_ui_Button remove;
	psy_ui_Button replace;
	psy_ui_Button select;
} ParamRackBatchBar;

void paramrackbatchbar_init(ParamRackBatchBar*, psy_ui_Component* parent);

typedef struct ParamRackModeBar {
	/*! @extends  */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_select;
	/*! @internal */
	psy_ui_Button all;
	psy_ui_Button inputs;
	psy_ui_Button outputs;
	psy_ui_Button inchain;
	psy_ui_Button outchain;
	psy_ui_Button level;
} ParamRackModeBar;

void paramrackmodebar_init(ParamRackModeBar*, psy_ui_Component* parent);
void paramrackmodebar_set_mode(ParamRackModeBar*, ParamRackMode);

/* ParamRackPane */
typedef struct ParamRackPane {
	/* inherit */
	psy_ui_Component component;	
	/*! @internal */
	psy_Table boxes;
	uintptr_t lastselected;
	ParamRackMode mode;
	uintptr_t level;
	/* references */
	Workspace* workspace;
	psy_audio_Machines* machines;	
	ParamRackBox* lastinserted;	
} ParamRackPane;

void paramrackpane_init(ParamRackPane*, psy_ui_Component* parent, Workspace*);

void paramrackpane_set_mode(ParamRackPane*, ParamRackMode);

/*!
** @struct ParamRack
*/
typedef struct ParamRack {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	CloseBar close;	
	ParamRackPane pane;
	psy_ui_Component bottom;	
	ParamRackModeBar modebar;	
	psy_ui_NumberEdit leveledit;
	psy_ui_Scroller scroller;	
} ParamRack;

void paramrack_init(ParamRack*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* paramrack_base(ParamRack* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PARAMRACK_H */
