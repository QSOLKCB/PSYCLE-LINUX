/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(WORKSPACE_H)
#define WORKSPACE_H

/* host */
#include "config.h"
#include "confirmbox.h"
#include "filechooser.h"
#include "fileview.h"
#include "hostmachinecallback.h"
#include "pluginscanthread.h"
#include "undoredo.h"
#include "viewhistory.h"
#include "svgs.h"
/* audio */
#include <machinefactory.h>
#include <plugincatcher.h>
#include <song.h>
#include <sequence.h>
#include <signal.h>
/* dsp */
#include <notestab.h>
/* ui */
#include <uicomponent.h>
#include <uiapp.h>
/* file */
#include <logger.h>
#include "playlist.h"


#ifdef __cplusplus
extern "C" {
#endif

/* MachinesInsert */

typedef struct MachinesInsert {
	uintptr_t replace_mac;
	psy_audio_Wire wire;
	bool random_position;
	uintptr_t count;
} MachinesInsert;

void machinesinsert_init(MachinesInsert*);

void machineinsert_reset(MachinesInsert*);
void machineinsert_append(MachinesInsert*, psy_audio_Wire);
void machineinsert_replace(MachinesInsert*, uintptr_t replace_mac);


struct ParamViews;

/*
** @struct Workspace
** @brief connects the player with the psycle host ui and configures both
**
** @detail
** Workspace
**           <>---- HostMachineCallback
**           <>---- PsycleConfig;
**           <>---- ViewHistory
**           <>---- psy_audio_Player;
**           <>---- psy_audio_MachineFactory
**           <>---- psy_audio_PluginCatcher
**           <>---- psy_audio_Song
*/

typedef struct Workspace {
	/* @extends */
	HostMachineCallback hostmachinecallback;	
	/* signals */			
	psy_Signal signal_view_selected;
	psy_Signal signal_machine_edit_resize;
	psy_Signal signal_bus_changed;
	/* @internal */
	psy_audio_Player player_;
	psy_audio_Song* song;
	PsycleConfig cfg_;	
	psy_ui_Component* main;	
	ViewHistory view_history;
	ViewIndex restoreview;
	psy_audio_SequencePaste sequencepaste;	
	bool modified_without_undo;	
	psy_UndoRedo undo_redo_;
	uintptr_t undo_save_point_;
	uintptr_t machines_undo_save_point_;
	PluginScanThread plugin_scan_thread_;
	struct ParamViews* param_views_;
	psy_Logger* terminal_output;
	psy_Logger* progress;
	ConfirmBox* confirm;
	MachinesInsert insert;		
	psy_Property edit_mode;
	FileChooser file_chooser;
	psy_FileSelect load_song;
	psy_FileSelect save_song;	
	psy_FileSelect load_sample;
	psy_audio_SampleIndex load_sample_index;	
	bool update_title;	
	bool song_has_file_;
	char* song_file_name_;
} Workspace;

void workspace_init(Workspace*, psy_ui_Component* handle);
void workspace_dispose(Workspace*);

void workspace_start_audio(Workspace*);
void workspace_clear_sequence_paste(Workspace*);
void workspace_new_song(Workspace*);
void workspace_load_song(Workspace*);
void workspace_load_song_from_path(Workspace*, const char*);
void workspace_save_song_as(Workspace*);
void workspace_load_sample(Workspace*, psy_audio_SampleIndex);
void workspace_disk_op_song(Workspace*);
void workspace_disk_op(Workspace*, psy_FileSelect* load, psy_FileSelect* save,
	psy_FileSelect* dir);
void workspace_scan_plugins(Workspace*);
void workspace_mark_song_modified(Workspace*);
void workspace_set_progress_logger(Workspace*, psy_Logger*);


INLINE void workspace_set_param_views(Workspace* self,
	struct ParamViews* param_views)
{
	assert(self);

	self->param_views_ = param_views;
}

INLINE struct ParamViews* workspace_param_views(Workspace* self)	
{
	assert(self);

	return self->param_views_;
}

INLINE PsycleConfig* workspace_cfg(Workspace* self) { return &self->cfg_; }

INLINE const PsycleConfig* workspace_cfg_const(const Workspace* self)
{
    return &self->cfg_;
}

INLINE psy_audio_Song* workspace_song(Workspace* self) { return self->song; }

INLINE const psy_audio_Song* workspace_song_const(const Workspace* self)
{
	return self->song;
}

INLINE psy_audio_Player* workspace_player(Workspace* self)
{
	return &self->player_;
}

void workspace_undo(Workspace*);
void workspace_redo(Workspace*);
int workspace_has_plugin_cache(const Workspace*);
psy_EventDriver* workspace_kbd_driver(Workspace*);
void workspace_idle(Workspace*);
void workspace_select_view(Workspace*, ViewIndex);
void workspace_save_view(Workspace*);
void workspace_restore_view(Workspace*);
void workspace_update_view_focus(Workspace*);
void workspace_add_view(Workspace*, ViewIndex);
void workspace_back(Workspace*);
void workspace_forward(Workspace*);
void workspace_update_currview(Workspace*);
ViewIndex workspace_current_view(Workspace*);
bool workspace_song_modified(const Workspace*);
bool workspace_currview_has_undo(Workspace*);
bool workspace_currview_has_redo(Workspace*);
void workspace_clear_undo(Workspace*);
void workspace_app_title(Workspace*, char* rv_title, uintptr_t max_len);
const char* workspace_song_title(const Workspace*);
void workspace_select_start_view(Workspace*);
void workspace_on_input(Workspace*, uintptr_t cmd);
void workspace_confirm_load(Workspace*);
void workspace_confirm_close(Workspace*);
void workspace_confirm_seq_clear(Workspace*);
void workspace_connect_input_handler(Workspace*);

INLINE bool workspace_song_has_file(const Workspace* self)
{
	assert(self);

	return (self->song_has_file_);
}

INLINE const psy_audio_HostSequencerTime* workspace_host_seq_time(
	const Workspace* self)
{
	return &self->player_.sequencer.hostseqtime;
}

INLINE void workspace_set_terminal_output(Workspace* self,
	psy_Logger* terminal_output)
{
	self->terminal_output = terminal_output;
	self->hostmachinecallback.logger = terminal_output;
	self->player_.machinefactory.logger = terminal_output;
}

bool workspace_save_reminder(const Workspace*);

INLINE bool workspace_update_title(const Workspace* self)
{
	assert(self);

	return self->update_title;
}

INLINE psy_audio_MachineCallback* workspace_base(Workspace* self)
{
	assert(self);

	return hostmachinecallback_base(&self->hostmachinecallback);
}

void workspace_set_song_filename(Workspace*, const char* filename);

#ifdef __cplusplus
}
#endif

#endif /* WORKSPACE_H */
