/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEFRAME_H)
#define MACHINEFRAME_H

#include "newval.h"
#include "paramview.h"
#include "paramlistbox.h"
#include "parammap.h"
#include "presetsbar.h"
#include "workspace.h"
#include "zoombox.h"
#include <presets.h>
#include <uinotebook.h>
#include <uibutton.h>
#include <uieditor.h>
#include <uitextarea.h>
#include <uilabel.h>
#include <uicombobox.h>
#include <uiimage.h>
#include <presets.h>
#include <machine.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** FrameMachine
*/

typedef struct ParameterBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Component row0;
	psy_ui_Component buttons;
	ZoomBox zoombox;		
	psy_ui_Button power;
	psy_ui_Button parameters;
	psy_ui_Button parammap;	
	psy_ui_Button help;
	psy_ui_Button command;
	psy_ui_Button isbus;
	psy_ui_Button more;
	psy_ui_Button bank;
	ProgramBar programbar;
	/* references */
	psy_audio_Machine* machine;	
} ParameterBar;

void parameterbar_init(ParameterBar*, psy_ui_Component* parent,
	const psy_ui_SVG* power);

struct ParamViews;
struct MachineEditorView;

/* FrameMachine */
typedef struct FrameMachine {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component top;
	psy_ui_Notebook bar_value_notebook;
	ParameterBar parameterbar;	
	psy_ui_Component right;	
	ParamMap parammap;
	ParameterListBox parameterbox;	
	psy_ui_Notebook notebook;
	psy_ui_Component helpview;
	psy_ui_Scroller scroller;
	psy_ui_Component help;
	psy_ui_Label help_text;
	psy_ui_Component help_right;	
	psy_ui_Image image;
	psy_ui_Label image_desc;
	psy_ui_Component* view;
	psy_ui_Component* machineview;	
	psy_audio_Machine* machine;	
	NewValView newval;
	bool showfullmenu;
	uintptr_t macid;	
	/* references */
	ParamView* paramview;
	struct ParamViews* paramviews;
	Workspace* workspace;
	struct MachineEditorView* editorview;
} FrameMachine;

void machineframe_init(FrameMachine*, psy_ui_Component* parent,
	psy_audio_Machine*, struct ParamViews* paramviews, Workspace*);

FrameMachine* machineframe_alloc(void);
FrameMachine* machineframe_alloc_init(psy_ui_Component* parent,
	psy_audio_Machine*, struct ParamViews*, Workspace*);

void machineframe_setview(FrameMachine*, psy_ui_Component* view, psy_audio_Machine*);
void machineframe_set_param_view(FrameMachine*, ParamView*, psy_audio_Machine*);

#ifdef __cplusplus
}
#endif

#endif /* MACHINEFRAME_H */
