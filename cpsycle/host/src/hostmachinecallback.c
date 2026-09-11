/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "hostmachinecallback.h"

/* platform */
#include "../../detail/portable.h"
#include <uicomponent.h>
#include "psyclescript.h"
#include "luaconfig.h"

#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif

/* prototypes */
static bool hostmachinecallback_on_machine_file_select_load(
	HostMachineCallback*, char filter[], char inoutName[]);
static void hostmachinecallback_on_load_change(HostMachineCallback*,
	psy_FileSelect* sender);
static bool hostmachinecallback_on_machine_file_select_save(
	HostMachineCallback*, char filter[], char inoutName[]);
static void hostmachinecallback_on_machine_file_select_directory(
	HostMachineCallback*);
static void hostmachinecallback_on_machine_terminal_output(HostMachineCallback*,
	const char* text);
static bool hostmachinecallback_on_machine_edit_resize(HostMachineCallback*,
	psy_audio_Machine* sender, intptr_t w, intptr_t h);
static void hostmachinecallback_on_machine_bus_changed(HostMachineCallback*,
	psy_audio_Machine* sender);
static void hostmachinecallback_on_message(HostMachineCallback*,
	const char* text);
static void hostmachinecallback_on_custom_setup(HostMachineCallback*,
	int msg, void* data);

/* vtable */
static psy_audio_MachineCallbackVtable hostmachinecallback_vtable;
static bool hostmachinecallback_vtable_initialized = FALSE;

static void hostmachinecallbackvtable_init(HostMachineCallback* self)
{
	assert(self);

	if (!hostmachinecallback_vtable_initialized) {
		hostmachinecallback_vtable = *self->machinecallback.vtable;		
		hostmachinecallback_vtable.fileselect_load =
			(fp_mcb_fileselect_load)
			hostmachinecallback_on_machine_file_select_load;
		hostmachinecallback_vtable.fileselect_save =
			(fp_mcb_fileselect_save)
			hostmachinecallback_on_machine_file_select_save;
		hostmachinecallback_vtable.fileselect_directory =
			(fp_mcb_fileselect_directory)
			hostmachinecallback_on_machine_file_select_directory;
		hostmachinecallback_vtable.editresize =
			(fp_mcb_editresize)
			hostmachinecallback_on_machine_edit_resize;
		hostmachinecallback_vtable.buschanged =
			(fp_mcb_buschanged)
			hostmachinecallback_on_machine_bus_changed;
		hostmachinecallback_vtable.output =
			(fp_mcb_output)
			hostmachinecallback_on_machine_terminal_output;
		hostmachinecallback_vtable.message =
			(fp_mcb_message)
			hostmachinecallback_on_message;
                hostmachinecallback_vtable.custom_setup =
			(fp_mcb_custom_setup)
			hostmachinecallback_on_custom_setup;
		hostmachinecallback_vtable_initialized = TRUE;
	}
	self->machinecallback.vtable = &hostmachinecallback_vtable;
}

/* implementation */
void hostmachinecallback_init(HostMachineCallback* self,
	psy_Configuration* dir_config,
	psy_Signal* signal_machine_edit_resize,
	psy_Signal* signal_bus_changed,
	psy_DiskOp* disk_op)
{
	assert(self);
	assert(dir_config);	
	assert(signal_machine_edit_resize);

	psy_audio_machinecallback_init(&self->machinecallback);
	hostmachinecallbackvtable_init(self);	
	self->dir_config = dir_config;
	self->logger = NULL;
	self->signal_machine_edit_resize = signal_machine_edit_resize;
	self->signal_bus_changed = signal_bus_changed;
	self->message = NULL;
	self->disk_op = disk_op;
	self->ui_ = NULL;
}

void hostmachinecallback_dispose(HostMachineCallback* self)
{
	free(self->message);
	self->message = NULL;
}

bool hostmachinecallback_on_machine_file_select_load(HostMachineCallback* self,
	char filter[], char inout_name[])
{
	psy_FileSelect load;
	
	self->inout_name = inout_name;
	self->res = 0;
	psy_fileselect_init(&load);
	psy_fileselect_connect_change(&load, self,
		(psy_fp_fileselect_change)hostmachinecallback_on_load_change);
	// psy_fileselect_connect_cancel(&self->load, self,
	//	(psy_fp_fileselect_change)workspace_on_diskop_song_cancel);
	psy_fileselect_set_title(&load,
		psy_ui_translate("file.loadsong"));	
	// psy_fileselect_set_default_extension(&load,
	//	psy_audio_songfile_standardloadfilter());
	// psy_fileselect_add_filter(&self->load_song, "psy", "*.psy");
	// psy_fileselect_add_filter(&self->load_song, "mod", "*.mod");
	psy_diskop_execute_wait(self->disk_op, &load, NULL, NULL);			
	return self->res;
}

void hostmachinecallback_on_load_change(HostMachineCallback* self,
	psy_FileSelect* sender)
{
	assert(self);
		
	psy_snprintf(self->inout_name, 1024, "%s", psy_fileselect_value(sender));
	self->res = 1;	
}

bool hostmachinecallback_on_machine_file_select_save(HostMachineCallback* self,
	char filter[], char inout_name[])
{
	/*bool success;
	psy_ui_SaveDialog dialog;

	assert(self);

	psy_ui_savedialog_init_all(&dialog, NULL, "file.plugin-filesave", filter,
		"", ""); //dirconfig_vsts32(self->dir_config));
	success = psy_ui_savedialog_execute(&dialog);
	psy_snprintf(inout_name, _MAX_PATH, "%s",
		psy_path_full(psy_ui_savedialog_path(&dialog)));
	psy_ui_savedialog_dispose(&dialog);*/
	return FALSE;
}

void hostmachinecallback_on_machine_file_select_directory(
	HostMachineCallback* self)
{

}

void hostmachinecallback_on_machine_terminal_output(HostMachineCallback* self,
	const char* text)
{
	if (self->logger) {
		// self->terminalstyleid = STYLE_TERM_BUTTON_ERROR;
		psy_logger_error(self->logger, text);
	}
}

bool hostmachinecallback_on_machine_edit_resize(HostMachineCallback* self,
	psy_audio_Machine* sender, intptr_t w, intptr_t h)
{
	psy_signal_emit(self->signal_machine_edit_resize, self, 3, sender, w, h);
	return TRUE;
}

void hostmachinecallback_on_machine_bus_changed(HostMachineCallback* self,
	psy_audio_Machine* sender)
{
	psy_signal_emit(self->signal_bus_changed, self, 1, sender);
}

void hostmachinecallback_on_message(HostMachineCallback* self,
	const char* text)
{
	psy_strreset(&self->message, text);
}

void hostmachinecallback_on_custom_setup(HostMachineCallback* self,
	int msg, void* data)
{
    if (msg == 1) {
        psy_PsycleScript* script;
        psy_LuaUi luaui;
    
        script = (psy_PsycleScript*)data;
        if (script) {
			psy_luaui_init(&luaui, script);
			
			psyclescript_require(script, "psycle.config", psy_lua_config_open);
			{ /* set ui handle */
				lua_getglobal(script->L, "psycle");
				lua_pushlightuserdata(script->L, self->ui_);
				lua_setfield(script->L, -2, "ui");					
				lua_pop(script->L, 1);				
			}			
        }
    }
}