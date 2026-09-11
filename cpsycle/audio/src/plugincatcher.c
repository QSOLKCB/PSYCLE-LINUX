/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "plugincatcher.h"
/* local */
#include "audiorecorder.h"
#include "dummy.h"
#include "duplicator.h"
#include "duplicator2.h"
#include "luaplugin.h"
#include "master.h"
#include "mixer.h"
#include "plugin.h"
#include "sampler.h"
#include "xmsampler.h"
#ifdef PSYCLE_USE_VST2
#include "vstplugin.h"
#endif
#ifdef PSYCLE_USE_LADSPA
#include "ladspaplugin.h"
#endif
#ifdef PSYCLE_USE_LV2
#include "lv2plugin.h"
#endif
/* container */
#include <qsort.h>
/* file */
#include <dir.h>
/* platform*/
#include "../../detail/cpu.h"
#include "../../detail/portable.h"
#include "../../detail/os.h"


#if defined DIVERSALIS__OS__MICROSOFT
#define SLASH "\\"
#define MODULEEXT ".dll"
#elif defined DIVERSALIS__OS__APPLE
#define SLASH "/"
#define	MODULEEXT ".dylib"
#define _MAX_PATH 4096
#else
#define SLASH "/"
#define MODULEEXT ".so"
#define _MAX_PATH 4096
#endif


/* prototypes */
static void plugincatcher_init_scan_tasks(psy_audio_PluginCatcher*);
static void plugincatcher_add_internals(psy_audio_PluginCatcher*);
static void plugincatcher_makesampler(psy_audio_PluginCatcher*);
static void plugincatcher_makeduplicator(psy_audio_PluginCatcher*);
static int on_enum_dir(psy_audio_PluginCatcher*, const char* path, int flag);
static int pathhasextension(const char* path);
static void plugincatcher_scan_multipath(psy_audio_PluginCatcher*,
	const char* multipath, const char* wildcard, int option,
	bool recursive);

/* implementation */
void psy_audio_plugincatcher_init(psy_audio_PluginCatcher* self,
	psy_Logger* logger)
{
	char inipath[_MAX_PATH];
	
	assert(self);

	self->logger_ = logger;
	psy_audio_plugins_init(&self->plugins_);	
	plugincatcher_add_internals(self);
	strcpy(inipath, psy_dir_config());
	self->dir_cfg_ = NULL;
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)
	self->native_root_ = psy_strdup(PSYCLE_APP_DIR);
#else
	self->native_root_ = psy_strdup(PSYCLE_APP64_DIR);
#endif
	self->save_after_scan_ = TRUE;
	self->has_plugin_cache_ = FALSE;
	self->scanning_ = FALSE;
	self->abort_ = FALSE;
	self->scan_tasks_ = NULL;
	psy_signal_init(&self->signal_changed);
	psy_signal_init(&self->signal_scanstart);
	psy_signal_init(&self->signal_scanend);
	psy_signal_init(&self->signal_scanprogress);
	psy_signal_init(&self->signal_scanfile);
	psy_signal_init(&self->signal_taskstart);
	plugincatcher_init_scan_tasks(self);
}

void psy_audio_plugincatcher_dispose(psy_audio_PluginCatcher* self)
{
	assert(self);
	
	psy_audio_plugins_dispose(&self->plugins_);
	self->dir_cfg_ = NULL;
	free(self->native_root_);
	self->native_root_ = NULL;
	psy_signal_dispose(&self->signal_changed);
	psy_signal_dispose(&self->signal_scanstart);
	psy_signal_dispose(&self->signal_scanend);
	psy_signal_dispose(&self->signal_scanprogress);
	psy_signal_dispose(&self->signal_scanfile);
	psy_signal_dispose(&self->signal_taskstart);	
	psy_list_deallocate(&self->scan_tasks_, (psy_fp_disposefunc)NULL);
}

void psy_audio_plugincatcher_set_directories(psy_audio_PluginCatcher* self,
	psy_Configuration* dir_cfg)
{
	assert(self);
	assert(dir_cfg);
	
	self->dir_cfg_ = dir_cfg;
	psy_strreset(&self->native_root_, psy_configuration_value_str(
		self->dir_cfg_, "app", PSYCLE_APP_DIR));
	psy_list_deallocate(&self->scan_tasks_, (psy_fp_disposefunc)NULL);
	plugincatcher_init_scan_tasks(self);
}

void psy_audio_plugincatcher_clear(psy_audio_PluginCatcher* self)
{
	assert(self);
	
	psy_audio_plugins_clear(&self->plugins_);
	plugincatcher_add_internals(self);
	self->has_plugin_cache_ = FALSE;
}

void plugincatcher_init_scan_tasks(psy_audio_PluginCatcher* self)
{
	psy_audio_PluginScanTask* task;

	assert(self);
	
	/* natives */
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)
	psy_audio_pluginscantask_init_all(task, psy_audio_PLUGIN,
		"*"MODULEEXT, "Natives 32bit", "plugins.plugins32", TRUE);
#else
	psy_audio_pluginscantask_init_all(task, psy_audio_PLUGIN,
		"*"MODULEEXT, "Natives 64bit", "plugins.plugins64", TRUE);
#endif
	psy_list_append(&self->scan_tasks_, task);

	/* lua */
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
	psy_audio_pluginscantask_init_all(task, psy_audio_LUA,
		"*.lua", "Luas", "plugins.luascripts", FALSE);
	psy_list_append(&self->scan_tasks_, task);

	/* vst2 */
#ifdef PSYCLE_USE_VST2
	
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
	psy_audio_pluginscantask_init_all(task, psy_audio_VST,
		"*"MODULEEXT, "Vsts 32bit", "plugins.vsts32", TRUE);
	psy_list_append(&self->scan_tasks_, task);
	if (self->dir_cfg_ && psy_configuration_value_bool(self->dir_cfg_,
			"plugins.usejbridge", FALSE)) {
		psy_audio_PluginScanTask* task;

		task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
		psy_audio_pluginscantask_init_all(task, psy_audio_VST,
			"*"MODULEEXT, "Vsts 64bit", "plugins.vsts64", TRUE);
		psy_list_append(&self->scan_tasks_, task);
	}
#else		
	if (self->dir_cfg_ && psy_configuration_value_bool(self->dir_cfg_,
			"plugins.usejbridge", FALSE)) {
		psy_audio_PluginScanTask* task;

		task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
		psy_audio_pluginscantask_init_all(task, psy_audio_VST,
			"*"MODULEEXT, "Vsts 32bit", "plugins.vsts32", TRUE);
		psy_list_append(&self->scan_tasks_, task);
	}
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
	psy_audio_pluginscantask_init_all(task, psy_audio_VST,
		"*"MODULEEXT, "Vsts 64bit", "plugins.vsts64", TRUE);
	psy_list_append(&self->scan_tasks_, task);
#endif
	
#endif	

	/* ladspas */
#ifdef PSYCLE_USE_LADSPA
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
	psy_audio_pluginscantask_init_all(task, psy_audio_LADSPA,
		"*"MODULEEXT, "Ladspas", "plugins.ladspas", TRUE);
	psy_list_append(&self->scan_tasks_, task);
#endif	
	
#ifdef PSYCLE_USE_LV2	
	task = (psy_audio_PluginScanTask*)malloc(sizeof(psy_audio_PluginScanTask));
	psy_audio_pluginscantask_init_all(task, psy_audio_LV2,
		"*.lv2", "LV2s", "plugins.lv2s", FALSE);
	psy_list_append(&self->scan_tasks_, task);
#endif	
}

void plugincatcher_add_internals(psy_audio_PluginCatcher* self)
{
	assert(self);
	
	psy_audio_plugins_add(&self->plugins_, psy_audio_xmsampler_info());		
	psy_audio_plugins_add(&self->plugins_, psy_audio_sampler_info());	
	psy_audio_plugins_add(&self->plugins_, psy_audio_dummymachine_info());		
	psy_audio_plugins_add(&self->plugins_, psy_audio_mixer_info());
	psy_audio_plugins_add(&self->plugins_, psy_audio_duplicator_info());
	psy_audio_plugins_add(&self->plugins_, psy_audio_duplicator2_info());
	psy_audio_plugins_add(&self->plugins_, psy_audio_audiorecorder_info());	
}


void plugincatcher_scan_multipath(psy_audio_PluginCatcher* self,
	const char* multi_path, const char* wildcard, int option,
	bool recursive)
{
	char text[4096];
	char seps[] = ";,";
	char *token;

	assert(self);
	
	psy_snprintf(text, 4096, "%s", multi_path);	
	token = strtok(text, seps);	
	while (token != NULL && !self->abort_) {
		char norm[4096];

		psy_dir_normalize(token, norm);
		if (recursive) {			
			psy_dir_enumerate_recursive(self, norm, wildcard, option,
				(psy_fp_findfile)on_enum_dir);
		} else {
			psy_dir_enumerate(self, norm, wildcard, option,
				(psy_fp_findfile)on_enum_dir);
		}
		token = strtok(0, seps);
	}	
}

void psy_audio_plugincatcher_scan(psy_audio_PluginCatcher* self)
{	
	assert(self);
	
	self->abort_ = FALSE;
	self->scanning_ = TRUE;
	self->plugin_count_ = self->file_count_ = 0;	
	if (!self->dir_cfg_) {
		return;
	}	
	psy_signal_emit(&self->signal_scanstart, self, 0);	
	psy_audio_plugincatcher_clear(self);
	if (self->dir_cfg_) {
		psy_List* p;

		for (p = self->scan_tasks_; p != NULL; p = p->next) {
			psy_audio_PluginScanTask* task;
			const char* path;

			task = (psy_audio_PluginScanTask*)p->entry;
			psy_signal_emit(&self->signal_taskstart, self, 1, task);			
			path = psy_configuration_value_str(self->dir_cfg_, task->key,
				NULL);			
			if (psy_strlen(path) > 0) {							
				plugincatcher_scan_multipath(self, path, task->wildcard,
					task->type, task->recursive);
			}
			if (self->abort_) {
				break;
			}
		}
	}
	psy_audio_plugins_set_native_categories(&self->plugins_);
	if (self->save_after_scan_) {
		psy_audio_plugincatcher_save(self);
	}
	psy_signal_emit(&self->signal_changed, self, 0);
	psy_signal_emit(&self->signal_scanprogress, self, 1, 0);
	self->scanning_ = FALSE;
	psy_signal_emit(&self->signal_scanend, self, 0);	
}

void psy_audio_plugincatcher_abort(psy_audio_PluginCatcher* self)
{
	assert(self);
	
	self->abort_ = TRUE;
}

int on_enum_dir(psy_audio_PluginCatcher* self, const char* path, int type)
{
	psy_audio_MachineInfo macinfo;	
	
	assert(self);
	
	++self->file_count_;
	psy_signal_emit(&self->signal_scanfile, self, 2, path, type);
	machineinfo_init(&macinfo);	
	switch (type) {
		case psy_audio_PLUGIN:
			if (psy_audio_plugin_psycle_test(path, self->native_root_,
					&macinfo)) {
				psy_audio_plugins_add(&self->plugins_, &macinfo);				
				++self->plugin_count_;
				psy_signal_emit(&self->signal_scanprogress, self, 1, 1);
			}
			break;
		case psy_audio_LUA:			
			if (psy_audio_luaplugin_test(path, &macinfo, self->logger_)) {	
				psy_audio_plugins_add(&self->plugins_, &macinfo);				
				++self->plugin_count_;
				psy_signal_emit(&self->signal_scanprogress, self, 1, 1);
			}
			break;
#ifdef PSYCLE_USE_VST2			
		case psy_audio_VST:						
			if (psy_audio_vstplugin_test(path, &macinfo, psy_configuration_value_bool(self->dir_cfg_,
					"plugins.usejbridge", FALSE))) {
				psy_audio_plugins_add(&self->plugins_, &macinfo);				
				++self->plugin_count_;
				psy_signal_emit(&self->signal_scanprogress, self, 1, 1);
			}
			break;
#endif			
#ifdef PSYCLE_USE_LADSPA
		case psy_audio_LADSPA: {
			uintptr_t shellidx;			

			shellidx = 0;
			for (; psy_audio_ladspaplugin_test(path, &macinfo, shellidx) != 0;
					++shellidx) {
				psy_audio_plugins_add(&self->plugins_, &macinfo);				
				++self->plugin_count_;
				psy_signal_emit(&self->signal_scanprogress, self, 1, 1);
			}
			break; }
#endif			
#ifdef PSYCLE_USE_LV2
		case psy_audio_LV2: {
			uintptr_t shellidx;			
			
			shellidx = 0;										
			for (; psy_audio_lv2plugin_test(path, &macinfo,
					shellidx) != 0; ++shellidx) {
				psy_audio_plugins_add(&self->plugins_, &macinfo);				
				++self->plugin_count_;
				psy_signal_emit(&self->signal_scanprogress, self, 1, 1);
			}
			break; }
#endif						
		default:
			break;
	}
	machineinfo_dispose(&macinfo);
	if (self->abort_) {
		return 0;
	}
	return 1;
}

int psy_audio_plugincatcher_load(psy_audio_PluginCatcher* self)
{
	int rv;

	assert(self);
	
	rv = psy_audio_plugins_load(&self->plugins_);
	self->has_plugin_cache_ = (rv == PSY_OK);
	psy_signal_emit(&self->signal_changed, self, 0);
	return rv;
}

int psy_audio_plugincatcher_save(psy_audio_PluginCatcher* self)
{
	return psy_audio_plugins_save(&self->plugins_);
}

uintptr_t psy_audio_plugincatcher_extract_shell_idx(const char* path)
{
	if (path) {
		char* str;

		str = strrchr(path, ':');
		if (str) {
			return atoi(str + 1);
		}
	}
	return 0;
}

char* psy_audio_plugincatcher_module_path(psy_audio_PluginCatcher* self,
	psy_audio_MachineType machtype,
	int newgamefxblitzifversionunknown,
	const char* path, char* fullpath)
{
	char* str;
	char search_path[psy_MAX_PATH];
	const psy_audio_MachineInfo* plugin;
	
	assert(self);
	assert(fullpath);
		
	if (!path) {
		fullpath[0] = '\0';
		return fullpath;
	}
	if (pathhasextension(path)) {		
		strcpy(fullpath, path);
		return fullpath;
	}		
	str = strrchr(path, ':');
	if (str) {
		psy_snprintf(search_path, psy_MAX_PATH, "%s", path);
	} else {
		psy_snprintf(search_path, psy_MAX_PATH, "%s:0", path);
	}	
	plugin = psy_audio_plugins_at_id_const(&self->plugins_, search_path);
	if (plugin && plugin->type != machtype) {
		plugin = NULL;			
	}		
	if (!plugin) {
		const char* searchname;			
	
		searchname = NULL;			
		if (strstr(path, "blitz")) {
			if (newgamefxblitzifversionunknown) {
				searchname = "blitz16:0";
			} else {
				searchname = "blitz12:0";
			}
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
			if (plugin && plugin->type != machtype) {
				plugin = NULL;			
			}				
			if (!plugin) {					
				plugin = psy_audio_plugins_at_id_const(&self->plugins_,
					"blitzn:0");
				if (plugin && plugin->type != machtype) {
					plugin = NULL;			
				}				
			}
			if (!plugin) {
				if (newgamefxblitzifversionunknown) {
					searchname = "blitz12:0";
				} else {
					searchname = "blitz16:0";
				}
				plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
				if (plugin && plugin->type != machtype) {
					plugin = NULL;			
				}
			}
		} else if (strstr(path, "gamefx")) {
			if (newgamefxblitzifversionunknown) {
				searchname = "gamefx16:0";
			} else {
				searchname = "gamefx13:0";
			}
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
			if (plugin && plugin->type != machtype) {
				plugin = NULL;			
			}
			if (!plugin) {
				searchname = "gamefxn:0";
				plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
				if (plugin && plugin->type != machtype) {
					plugin = NULL;			
				}
			}
			if (!plugin) {
				if (newgamefxblitzifversionunknown) {
					searchname = "gamefx13:0";
				} else {
					searchname = "gamefx16:0";
				}
				plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
				if (plugin && plugin->type != machtype) {
					plugin = NULL;			
				}
			}
		} else if (strstr(path, "synth1-vst")) {			
			searchname = "synth1-vst64:0";			
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, searchname);
			if (plugin && plugin->type != machtype) {
				plugin = NULL;
			}
		}
	}
	if (!plugin) {
		if (strstr(path, "blitzn")) {				
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, "blitz:0");
			if (plugin && plugin->type != machtype) {
				plugin = NULL;			
			}
		}
	}
	if (!plugin) {
		if (strstr(path, "blitz16")) {				
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, "blitzn:0");
			if (plugin && plugin->type != machtype) {
				plugin = NULL;			
			}
		}
	}
	if (!plugin) {
		if (strstr(path, "blitzn")) {				
			plugin = psy_audio_plugins_at_id_const(&self->plugins_, "blitz16:0");
			if (plugin && plugin->type != machtype) {
				plugin = NULL;			
			}
		}
	}
	if (plugin) {
		strcpy(fullpath, plugin->modulepath);
	} else {
		strcpy(fullpath, "");
	}
	return fullpath;
}

int pathhasextension(const char* path)
{
	return (strrchr(path, '.') != 0);
}

const char* psy_audio_plugincatcher_search_path(psy_audio_PluginCatcher* self,
	const char* name, int machtype)
{
	const psy_audio_MachineInfo* plugin;
	
	assert(self);
	
	plugin = psy_audio_plugins_at_id_const(&self->plugins_, name);
	if (plugin && plugin->type == machtype) {
		return plugin->modulepath;
	}	
	return "";
}

bool psy_audio_plugincatcher_scanning(const psy_audio_PluginCatcher* self)
{
	assert(self);
	
	return self->scanning_;
}
