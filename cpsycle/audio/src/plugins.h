/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PLUGINS_H
#define psy_audio_PLUGINS_H

/* local */
#include "machinedefs.h"
#include "machineinfo.h"
/* container */
#include <hashtbl.h>
#include <propertiesio.h>
#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif


/* PluginFilter */

typedef struct PluginFilterItem {
	uintptr_t key;
	char* name;
	bool active;	
} PluginFilterItem;

void pluginfilteritem_init(PluginFilterItem*, uintptr_t key, const char* name,
	bool active);
void pluginfilteritem_dispose(PluginFilterItem*);
	
PluginFilterItem* pluginfilteritem_allocinit(uintptr_t key, const char* name,
	bool active);

typedef struct PluginFilterGroup {	
	psy_List* items;
	uintptr_t id;
	char* label;
} PluginFilterGroup;

void pluginfiltergroup_init(PluginFilterGroup*, uintptr_t id,
	const char* label);
void pluginfiltergroup_dispose(PluginFilterGroup*);

PluginFilterGroup* pluginfiltergroup_allocinit(uintptr_t id,
	const char* label);

void pluginfiltergroup_clear(PluginFilterGroup*);
bool pluginfiltergroup_add(PluginFilterGroup*,
	uintptr_t key, const char* name, bool active);
bool pluginfiltergroup_remove(PluginFilterGroup*, uintptr_t key);

void pluginfiltergroup_select(PluginFilterGroup*, 
	uintptr_t key);
void pluginfiltergroup_select_all(PluginFilterGroup*);
void pluginfiltergroup_deselect(PluginFilterGroup*, 
	uintptr_t key);
void pluginfiltergroup_deselect_all(PluginFilterGroup*);	
void pluginfiltergroup_toggle(PluginFilterGroup*, 
	uintptr_t key);
bool pluginfiltergroup_selected(const PluginFilterGroup*, 
	uintptr_t key);
bool pluginfiltergroup_all_active(const PluginFilterGroup*);
bool pluginfiltergroup_exists(const PluginFilterGroup*, 
	uintptr_t key);	

typedef struct PluginFilter {
	psy_Signal signal_changed;
	char_dyn_t* search_text;	
	PluginFilterGroup mode;
	PluginFilterGroup types;
	PluginFilterGroup categories;
	PluginFilterGroup sort;
} PluginFilter;

void pluginfilter_init(PluginFilter*);
void pluginfilter_dispose(PluginFilter*);

void pluginfilter_select_all(PluginFilter*);
void pluginfilter_notify(PluginFilter*);
void pluginfilter_set_search_text(PluginFilter*, const char* text);


/* psy_audio_Plugins */

typedef struct psy_audio_Plugins {	
	psy_List* plugins_;
	char* ini_path_;
} psy_audio_Plugins;

void psy_audio_plugins_init(psy_audio_Plugins*);
void psy_audio_plugins_dispose(psy_audio_Plugins*);

void psy_audio_plugins_copy(psy_audio_Plugins*,
	const psy_audio_Plugins* other);
void psy_audio_plugins_reset(psy_audio_Plugins*);
void psy_audio_plugins_clear(psy_audio_Plugins*);
int psy_audio_plugins_load(psy_audio_Plugins*);
int psy_audio_plugins_save(psy_audio_Plugins*);
void psy_audio_plugins_add(psy_audio_Plugins*, const psy_audio_MachineInfo*);
void psy_audio_plugins_remove(psy_audio_Plugins*, const char* id);
bool psy_audio_plugins_exists(psy_audio_Plugins*, const psy_audio_MachineInfo*);
uintptr_t psy_audio_plugins_size(const psy_audio_Plugins*);
void psy_audio_plugins_filter(psy_audio_Plugins*, PluginFilter*,
	psy_audio_Plugins* rv);
void psy_audio_plugins_sort(psy_audio_Plugins*, PluginFilter*,
	psy_audio_Plugins* rv);
const psy_audio_MachineInfo* psy_audio_plugins_at_const(
	const psy_audio_Plugins*, uintptr_t index);
psy_List* psy_audio_plugins_begin(psy_audio_Plugins*);
psy_audio_MachineInfo* psy_audio_plugins_at_id(psy_audio_Plugins*,
	const char* key);
const psy_audio_MachineInfo* psy_audio_plugins_at_id_const(
	const psy_audio_Plugins*, const char* key);
void psy_audio_plugins_update_filter_categories(psy_audio_Plugins*,
	PluginFilter*);
void psy_audio_plugins_set_native_categories(psy_audio_Plugins*);


/* psy_audio_PluginScanTask */

typedef struct psy_audio_PluginScanTask {
	psy_audio_MachineType type;
	char wildcard[256];
	char label[256];	
	char key[256];
	bool recursive;
} psy_audio_PluginScanTask;

void psy_audio_pluginscantask_init_all(psy_audio_PluginScanTask*,
	psy_audio_MachineType type, const char* wildcard, const char* label,
	const char* key, int recursive);


#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PLUGINS_H */
