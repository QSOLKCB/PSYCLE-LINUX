/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PLUGINCATCHER_H
#define psy_audio_PLUGINCATCHER_H

/* local */
#include "plugins.h"
#include "machinedefs.h"
#include "machineinfo.h"
/* container */
#include <configuration.h>
#include <hashtbl.h>
#include <logger.h>
#include <propertiesio.h>
#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_PluginCatcher
*/

typedef struct psy_audio_PluginCatcher {
	/* signals */
	psy_Signal signal_changed;
	psy_Signal signal_scanstart;
	psy_Signal signal_scanend;
	psy_Signal signal_scanprogress;
	psy_Signal signal_scanfile;
	psy_Signal signal_taskstart;
	/*! @internal */
	psy_audio_Plugins plugins_;
	char* native_root_;
	psy_Configuration* dir_cfg_;
	bool save_after_scan_;
	bool has_plugin_cache_;
	bool scanning_;
	bool abort_;
	psy_List* scan_tasks_;
	uintptr_t file_count_;
	uintptr_t plugin_count_;	
	psy_Logger* logger_;	
} psy_audio_PluginCatcher;

void psy_audio_plugincatcher_init(psy_audio_PluginCatcher*, psy_Logger*);
void psy_audio_plugincatcher_dispose(psy_audio_PluginCatcher*);

void psy_audio_plugincatcher_scan(psy_audio_PluginCatcher*);
void psy_audio_plugincatcher_clear(psy_audio_PluginCatcher*);
void psy_audio_plugincatcher_abort(psy_audio_PluginCatcher*);
int psy_audio_plugincatcher_load(psy_audio_PluginCatcher*);
int psy_audio_plugincatcher_save(psy_audio_PluginCatcher*);
void psy_audio_plugincatcher_set_directories(psy_audio_PluginCatcher*,
	psy_Configuration* dir_cfg);
char* psy_audio_plugincatcher_module_path(psy_audio_PluginCatcher*,
	psy_audio_MachineType, int newgamefxblitzifversionunknown,
	const char* path,
	char* fullpath);
uintptr_t psy_audio_plugincatcher_extract_shell_idx(const char* path);
const char* psy_audio_plugincatcher_search_path(psy_audio_PluginCatcher*,
	const char* name, int type);
bool psy_audio_plugincatcher_scanning(const psy_audio_PluginCatcher*);

INLINE bool psy_audio_plugincatcher_has_cache(const psy_audio_PluginCatcher*
	self)
{
	assert(self);

	return self->has_plugin_cache_;
}

INLINE void psy_audio_plugincatcher_notifychange(psy_audio_PluginCatcher* self)
{
	assert(self);
	
	psy_signal_emit(&self->signal_changed, self, 0);
}

INLINE uintptr_t psy_audio_plugincatcher_plugin_count(const
	psy_audio_PluginCatcher* self)
{
	assert(self);
	
	return self->plugin_count_;
}

INLINE uintptr_t psy_audio_plugincatcher_file_count(const
	psy_audio_PluginCatcher* self)
{
	assert(self);
	
	return self->file_count_;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PLUGINCATCHER_H */
