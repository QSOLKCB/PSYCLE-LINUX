/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEVIEW_H)
#define MACHINEVIEW_H

/* host */
#include "machineframe.h"
#include "machineeditorview.h" /* vst view */
#include "machineui.h"
#include "machinestackview.h"
#include "machineviewbar.h"
#include "machineviewstyles.h"
#include "machinewireview.h"
#include "newmachine.h"
#include "paramview.h"
#include "paramviews.h"
#include "paramviewstyles.h"
#include <uitabbar.h>
#include "workspace.h"
/* ui */
#include <uinotebook.h>
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MachineView
** @brief Editor/Viewer for the machines.
*/
typedef struct MachineView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	MachineViewBar machineviewbar;
	psy_ui_TabBar tab_bar_;	
	psy_ui_Notebook notebook;	
	MachineWireView wireview;	
	MachineStackView stackview;
	NewMachine newmachine;
	MachineMenu machine_menu;
	bool shownewmachine;
	MachineViewStyles machine_styles;
	MachineStyleConfigurator machine_style_configurator;
	ParamViewStyles param_styles;
	ParamStyleConfigurator param_style_configurator;
	/* references */
	Workspace* workspace;
	psy_Configuration* macview_config;	
} MachineView;

void machineview_init(MachineView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace*);

void machineview_idle(MachineView*);

INLINE psy_ui_Component* machineview_base(MachineView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEVIEW_H */
