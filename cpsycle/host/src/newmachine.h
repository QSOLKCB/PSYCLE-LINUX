/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(NEWMACHINE_H)
#define NEWMACHINE_H

/* host */
#include "labelpair.h"
#include "newmachinedetail.h"
#include "pluginsview.h"
#include "pluginscanview.h"
#include "searchfield.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uilabel.h>
#include <uitabbar.h>
#include <uinotebook.h>
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif
		
struct NewMachine;

typedef struct NewMachineSearch {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	SearchField search;
	/* references */
	PluginFilter* filter;
} NewMachineSearch;

void newmachinesearch_init(NewMachineSearch*, psy_ui_Component* parent,
	PluginFilter* filter, const psy_ui_SVG*);

void newmachinesearch_set_restore_focus(NewMachineSearch*,
	psy_ui_Component* restore_focus);
void newmachinesearch_set_filter(NewMachineSearch*, PluginFilter*);

INLINE psy_ui_Component* newmachinesearch_base(NewMachineSearch* self)
{
	return &self->component;
}


/* NewMachineBar */

typedef struct NewMachineBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button rescan;
	psy_ui_Label desc;
	psy_ui_Button directories;
	psy_ui_Button add;
	psy_ui_Button cancel;
	/* references */
	Workspace* workspace;
} NewMachineBar;

void newmachinebar_init(NewMachineBar*, psy_ui_Component* parent,
	Workspace*);

INLINE psy_ui_Component* newmachinebar_base(NewMachineBar* self)
{
	return &self->component;
}


/* NewMachineFilterGroup */

typedef struct NewMachineFilterGroup {
	/*! @extends */
	psy_ui_Component component;	
	/*! @internal */
	psy_ui_Component header;
	psy_ui_Button desc;
	psy_ui_Component types;
	psy_ui_Component client;
	/* references */
	PluginFilter* filter;
	PluginFilterGroup* filter_group;
} NewMachineFilterGroup;

void newmachinefiltergroup_init(NewMachineFilterGroup*,
	psy_ui_Component* parent, PluginFilter* filter,
	PluginFilterGroup*);
	
void newmachinefiltergroup_set_filter(NewMachineFilterGroup*, PluginFilter*,
	PluginFilterGroup*);
void newmachinefiltergroup_add(NewMachineFilterGroup*, const char* label,
	uintptr_t id, bool active);
void newmachinefiltergroup_set(NewMachineFilterGroup*, uintptr_t id, bool);
void newmachinefiltergroup_mark(NewMachineFilterGroup*, uintptr_t id);	
void newmachinefiltergroup_unmark(NewMachineFilterGroup*, uintptr_t id);
	
INLINE psy_ui_Component* newmachinefiltergroup_base(NewMachineFilterGroup* self)
{
	return &self->component;
}

/* NewMachineFilters */

typedef struct NewMachineFilters {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Component expand;
	psy_ui_Button toggle;
	psy_ui_Component filters;	
	NewMachineFilterGroup mode;
	NewMachineFilterGroup types;
	NewMachineFilterGroup sort;
	NewMachineFilterGroup categories;	
} NewMachineFilters;

void newmachinefilters_init(NewMachineFilters*, psy_ui_Component* parent,
	PluginFilter*);

void newmachinefilters_setfilter(NewMachineFilters*, PluginFilter*);
void newmachinefilters_build(NewMachineFilters*);


INLINE psy_ui_Component* newmachinefilters_base(NewMachineFilters* self)
{
	return &self->component;
}


typedef struct NewMachinePlugins {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component pane;	
	psy_ui_Scroller scroller;
	NewMachineFilters filters;	
	PluginsView pluginview;	
	/* references */
	PluginScanThread* plugin_scan_thread;
} NewMachinePlugins;
    
void newmachineplugins_init(NewMachinePlugins*, psy_ui_Component* parent,
	PluginFilter*, PluginScanThread*);

void newmachineplugins_build(NewMachinePlugins*);

INLINE psy_ui_Component* newmachineplugins_base(NewMachinePlugins* self)
{
	return &self->component;
}

/*!
** @struct NewMachine
** @brief Chooser for new machines
*/
typedef struct NewMachine {
	/*! @extends */
	psy_ui_Component component;		
	/*! @internal */
	PluginFilter filter;	
	psy_ui_Notebook notebook;
	NewMachineSearch search;
	psy_ui_Component client;	
	NewMachinePlugins plugins;
	NewMachineDetail detail;
	PluginScanView scanview;	
	NewMachineBar bar;	
	ViewIndex restore_view;
	/* references */
	Workspace* workspace;	
} NewMachine;

void newmachine_init(NewMachine*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* newmachine_base(NewMachine* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* NEWMACHINE_H */
