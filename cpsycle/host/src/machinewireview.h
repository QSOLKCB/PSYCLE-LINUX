/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEWIREVIEW_H)
#define MACHINEWIREVIEW_H

/* host */
#include "machineframe.h"
#include "machineui.h"
#include "machineviewmenu.h"
#include "paramviews.h"
#include "wireframes.h"
#include "workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MachineWirePaneUis
*/
typedef struct MachineWirePaneUis {	
	psy_Table machine_uis_;
	/*! @internal */	
	/* references */
	psy_ui_Component* view;
	psy_audio_Machines* machines;	
	ParamViews* paramviews;
	psy_Configuration* cfg_;
} MachineWirePaneUis;

void machinewireviewuis_init(MachineWirePaneUis*, psy_ui_Component*,
	ParamViews*, psy_Configuration*);
void machinewireviewuis_dispose(MachineWirePaneUis*);

void machinewireviewuis_remove(MachineWirePaneUis*, uintptr_t slot);

/*!
** @struct MachineWirePane
*/
typedef struct MachineWirePane {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	MachineWirePaneUis machine_uis_;
	WireFrames wire_frames_;
	psy_ui_RealPoint drag_pt_;
	uintptr_t drag_slot_;
	psy_ui_Component* drag_machine_ui_;
	MachineViewDragMode drag_mode_;
	uintptr_t selected_slot_;
	psy_audio_Wire drag_wire_;
	psy_audio_Wire hover_wire_;
	bool mouse_moved_;
	bool showwirehover;	
	bool drawvirtualgenerators;	
	bool centermaster;
	bool starting;
	/* references */
	psy_audio_Machines* machines;	
	Workspace* workspace;		
	ParamViews* paramviews;
	MachineMenu* machine_menu;
} MachineWirePane;

void machinewirepane_init(MachineWirePane*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, ParamViews*, MachineMenu*,
	Workspace*);

void machinewirepane_center_master(MachineWirePane*);
void machinewirepane_showvirtualgenerators(MachineWirePane*);
void machinewirepane_hidevirtualgenerators(MachineWirePane*);
void machinewirepane_idle(MachineWirePane*);
void machinewirepane_update_styles(MachineWirePane*);

INLINE psy_ui_Component* machinewirepane_base(MachineWirePane* self)
{
	return &self->component;
}

/*!
** @struct MachineWireView
** @brief displays and edits machines with their wires
*/
typedef struct MachineWireView {
	/*! extends */	
	psy_ui_Scroller scroller;	
	/*! @internal */
	MachineWirePane pane;		
} MachineWireView;

void machinewireview_init(MachineWireView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, ParamViews*, MachineMenu*, Workspace*);

INLINE void machinewireview_center_master(MachineWireView* self)
{
	machinewirepane_center_master(&self->pane);
}

INLINE void machinewireview_showvirtualgenerators(MachineWireView* self)
{
	machinewirepane_showvirtualgenerators(&self->pane);
}

INLINE void machinewireview_hidevirtualgenerators(MachineWireView* self)
{
	machinewirepane_hidevirtualgenerators(&self->pane);
}

INLINE void machinewireview_idle(MachineWireView* self)
{
	machinewirepane_idle(&self->pane);
}

INLINE void machinewireview_update_styles(MachineWireView* self)
{	
	machinewirepane_update_styles(&self->pane);
}

INLINE psy_ui_Component* machinewireview_base(MachineWireView* self)
{
	return psy_ui_scroller_base(&self->scroller);
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEWIREVIEW_H */
