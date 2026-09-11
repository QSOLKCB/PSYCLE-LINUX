/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PLUGINSVIEW_H)
#define PLUGINSVIEW_H

/* ui */
#include <uicomponent.h>
/* audio */
#include <machine.h>
#include <plugincatcher.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** PluginsView
**
** Displays plugin properties in a list. Used by NewMachine.
*/


/* NewMachineSelection */

typedef struct NewMachineSelection {
	psy_List* items;
} NewMachineSelection;

void newmachineselection_init(NewMachineSelection*);
void newmachineselection_dispose(NewMachineSelection*);

INLINE bool newmachineselection_empty(const NewMachineSelection* self)
{
	return self->items == NULL;
}

INLINE psy_List* newmachineselection_begin(NewMachineSelection* self)
{
	return self->items;
}

INLINE const psy_List* newmachineselection_begin_const(
	NewMachineSelection* self)
{
	return self->items;
}

void newmachineselection_clear(NewMachineSelection*);
void newmachineselection_select(NewMachineSelection*, uintptr_t index);
void newmachineselection_single_select(NewMachineSelection*, uintptr_t index);
void newmachineselection_select_all(NewMachineSelection*, uintptr_t size);
void newmachineselection_deselect(NewMachineSelection*, uintptr_t index);
void newmachineselection_toggle(NewMachineSelection*, uintptr_t index);
bool newmachineselection_is_selected(const NewMachineSelection*,
	uintptr_t index);
uintptr_t newmachineselection_first(const NewMachineSelection*);
bool newmachineselection_valid(const NewMachineSelection*);


/* PluginsView */

typedef struct PluginsView {
	/*! @extends  */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_selected;	
	psy_Signal signal_changed;
	/*! @internal */
	psy_ui_Size item_size_;
	psy_ui_RealSize item_size_px_;	
	uintptr_t num_cols_;	
	/* plugins */
	psy_audio_Plugins plugins_;
	/* filtered  and sorted plugins */
	psy_audio_Plugins curr_plugins_;	
	psy_ui_RealPoint dragpt;
	bool multidrag;
	uintptr_t dragindex;
	NewMachineSelection selection;
	intptr_t scroll_dir;
	/* References */
	PluginFilter* filter;		
} PluginsView;

void pluginsview_init(PluginsView*, psy_ui_Component* parent);

void pluginsview_clear(PluginsView*);
void pluginsview_reset_current(PluginsView*);
void pluginsview_set_plugins(PluginsView*, const psy_audio_Plugins*);
void pluginsview_set_filter(PluginsView*, PluginFilter*);
const psy_audio_MachineInfo* pluginsview_selected(PluginsView*);
const psy_audio_MachineInfo* pluginsview_plugin(const PluginsView*,
	uintptr_t index);
double pluginsview_index_to_px(PluginsView*, uintptr_t index);
	
INLINE psy_ui_Component* pluginsview_base(PluginsView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PLUGINSVIEW_H */
