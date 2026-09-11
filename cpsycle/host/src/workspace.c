/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "workspace.h"
/* local */
#include "cmdproperties.h"
#include "cmdsgeneral.h"
#include "cmdsnotes.h"
#include "sequencehostcmds.h"
#include "styles.h"
#include "paramviews.h"
/* dsp */
#include <operations.h>
/* audio */
#include <exclusivelock.h>
#include <kbddriver.h>
#include <luaplayer.h>
#include <songio.h>
/* ui */
#include <trackercmds.h>
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"


/* MachinesInsert */

/* implementation */
void machinesinsert_init(MachinesInsert* self)
{	
	assert(self);
	
	machineinsert_reset(self);	
}

void machineinsert_reset(MachinesInsert* self)
{
	self->replace_mac = psy_INDEX_INVALID;
	self->wire = psy_audio_wire_zero();
	self->random_position = TRUE;
	self->count = 0;
}

void machineinsert_append(MachinesInsert* self, psy_audio_Wire wire)
{
	assert(self);
	
	self->replace_mac = psy_INDEX_INVALID;
	self->wire = wire;
	self->count = 0;
}

void machineinsert_replace(MachinesInsert* self, uintptr_t replace_mac)
{
	assert(self);
	
	self->replace_mac = replace_mac;
	self->wire = psy_audio_wire_zero();
	self->count = 0;
}

/* Workspace */

/* prototypes */
static void workspace_init_views(Workspace*);
static void workspace_init_player(Workspace*);
static void workspace_init_config(Workspace*);
static void workspace_init_song(Workspace*);
static void workspace_init_signals(Workspace*);
static void workspace_connect_configuration(Workspace*);
static void workspace_dispose_signals(Workspace*);
static void workspace_update_play_status(Workspace*, bool follow_song);
static void workspace_reset_views(Workspace*);
/* config */
static void workspace_set_song(Workspace*, psy_audio_Song*);	
static void workspace_update_save_point(Workspace*);
/* song */
static void workspace_reset_undo_save_point(Workspace*);
static void workspace_new_song_internal(Workspace*);
static void workspace_save_song_internal(Workspace*, const char*);
static void workspace_load_sample_internal(Workspace*, const char* path);
/* file */
static void workspace_on_confirm_accept_new(Workspace*);
static void workspace_on_confirm_reject_new(Workspace*);
static void workspace_on_confirm_accept_load(Workspace*);
static void workspace_on_confirm_reject_load(Workspace*);
static void workspace_confirm_accept_close(Workspace*);
static void workspace_confirm_reject_close(Workspace*);
static void workspace_on_confirm_continue(Workspace*);
static void workspace_confirm_accept_seqclear(Workspace*);
static void workspace_confirm_reject_seqclear(Workspace*);
static void workspace_init_disk_ops(Workspace*);
static void workspace_init_song_load(Workspace*);
static void workspace_init_song_save(Workspace*);
static void workspace_on_load_song_file_name_change(Workspace*,
	psy_FileSelect* sender);
static void workspace_init_sample_load(Workspace*);
static void workspace_on_load_song_change(Workspace*, psy_FileSelect* sender);
static void workspace_on_save_song_change(Workspace*, psy_FileSelect* sender);
static void workspace_on_load_dir_song_change(Workspace*, psy_FileSelect* sender);
static void workspace_on_save_dir_song_change(Workspace*, psy_FileSelect* sender);
static void workspace_on_diskop_song_cancel(Workspace*, psy_FileSelect* sender);
static void workspace_on_load_sample_change(Workspace*, psy_FileSelect* sender);
static void workspace_on_load_sample_cancel(Workspace*, psy_FileSelect* sender);
static void workspace_load_sample_confirm_overwrite_accept(Workspace*);
static void workspace_load_sample_confirm_overwrite_reject(Workspace*);
static void workspace_load_sample_confirm_overwrite_continue(Workspace*);
static void workspace_on_load_sample_file_name_change(Workspace*,
	psy_FileSelect* sender);
static void workspace_on_song_directory_changed(Workspace*,
	psy_Property* sender);
static void workspace_on_confirm_new_save_changed(Workspace*, psy_FileSelect*
	sender);
static void workspace_on_confirm_new_save_cancel(Workspace*, psy_FileSelect*
	sender);
static void workspace_on_confirm_close_save_changed(Workspace*, psy_FileSelect*
	sender);
static void workspace_on_confirm_close_save_cancel(Workspace*, psy_FileSelect*
	sender);
static void workspace_on_play_status_changed(Workspace*, psy_audio_Sequencer*);
static void workspace_on_follow_song_changed(Workspace*, psy_Property* sender);
static void workspace_on_plugin_scan(Workspace*, psy_Property* sender);
static void workspace_on_ft2_file_explorer(Workspace*, psy_Property* sender);
static void workspace_on_use_jbridge(Workspace*, psy_Property* sender);
static void workspace_on_edit_mode(Workspace*, psy_Property* sender);
static void workspace_on_pattern_default_lines(Workspace*,
	psy_Property* sender);
static void workspace_load_confirm(Workspace*, const char* path);
static void workspace_save_confirm(Workspace*, const char* path);
static bool workspace_on_notes(Workspace*, InputHandler* sender);
static bool workspace_on_input_handler_callback(Workspace*, int message, void* param1);
static void workspace_add_recent(Workspace*, const psy_audio_Song*, const char* filename);

/* implementation */
void workspace_init(Workspace* self, psy_ui_Component* main)
{
	assert(self);
		
	self->main = main;
	self->update_title = FALSE;
	self->song_has_file_ = FALSE;
	self->song_file_name_ = psy_strdup(PSYCLE_UNTITLED);
	machinesinsert_init(&self->insert);	
	filechooser_init(&self->file_chooser, self);
	workspace_init_views(self);
	workspace_init_config(self);
	workspace_init_disk_ops(self);
	workspace_init_player(self);	
	workspace_init_song(self);	
	workspace_init_signals(self);
	workspace_connect_configuration(self);
}

void workspace_init_views(Workspace* self)
{
	assert(self);
	
	self->undo_save_point_ = 0;	
	self->machines_undo_save_point_ = 0;		
	self->restoreview = viewindex_make_section(VIEW_ID_MACHINES,
		SECTION_ID_MACHINEVIEW_WIRES);
	self->modified_without_undo = FALSE;
	self->param_views_ = NULL;	
	self->terminal_output = NULL;
	self->progress = NULL;
	self->confirm = NULL;	
	psy_property_init_type(&self->edit_mode, "edtmode",
		PSY_PROPERTY_TYPE_BOOL);
	psy_property_set_item_bool(&self->edit_mode, TRUE);
	psy_property_connect(&self->edit_mode, self,
		workspace_on_edit_mode);
	psy_audio_sequencepaste_init(&self->sequencepaste);
	viewhistory_init(&self->view_history);
	psy_undoredo_init(&self->undo_redo_);	
}

void workspace_init_player(Workspace* self)
{
	psy_Property* cmddef;

	assert(self);
		
	psy_audio_init();
	hostmachinecallback_init(&self->hostmachinecallback,
		psycleconfig_directories(&self->cfg_),
		&self->signal_machine_edit_resize,
		&self->signal_bus_changed,
		filechooser_base(&self->file_chooser));
	cmddef = cmdproperties_create();
	psy_audio_player_init(&self->player_,
		hostmachinecallback_base(&self->hostmachinecallback),		
		/* mainwindow platform handle for directx driver */
		(self->main)
		? psy_ui_component_platform(self->main)
		: NULL,
		psycleconfig_audio(&self->cfg_),
		psycleconfig_input(&self->cfg_),
		psycleconfig_misc(&self->cfg_),
		psycleconfig_metronome(&self->cfg_),		
		psycleconfig_seqedit(&self->cfg_),		
		psycleconfig_compat(&self->cfg_),		
		psycleconfig_global(&self->cfg_),
		psycleconfig_midi(&self->cfg_),
		cmddef, NULL);
	psy_property_deallocate(cmddef);	
	psy_audio_luabind_set_player(&self->player_);	
	pluginscanthread_init(&self->plugin_scan_thread_,		
		&self->player_.plugincatcher, NULL);
	psy_audio_plugincatcher_set_directories(&self->player_.plugincatcher,
		psycleconfig_directories(&self->cfg_));
	psy_audio_plugincatcher_load(&self->player_.plugincatcher);
	psy_audio_eventdrivers_connect(&self->player_.eventdrivers,
		psy_ui_app_input_handler(psy_ui_app()), (fp_eventdriver_input)
		inputhandler_event_driver_input);
	self->player_.machinefactory.use_jbridge = psy_configuration_value_bool(
		psycleconfig_directories(workspace_cfg(self)),
		"plugins.usejbridge", FALSE);
	psy_signal_connect(&self->player_.sequencer.signal_play_status_changed,
		self, workspace_on_play_status_changed);
}

void workspace_init_song(Workspace* self)
{
	assert(self);
	
	self->song = psy_audio_song_alloc_init(&self->player_.machinefactory);
	psy_audio_player_set_song(&self->player_, self->song);	
}

void workspace_init_config(Workspace* self)
{
	assert(self);
	
	psycleconfig_init(&self->cfg_, filechooser_base(&self->file_chooser));
	psycleconfig_load(&self->cfg_);
	if (psy_configuration_value_bool(psycleconfig_misc(&self->cfg_),
			"ft2fileexplorer", TRUE)) {
		filechooser_use_file_view(&self->file_chooser);
	} else {
		filechooser_use_native(&self->file_chooser);
	}
}

void workspace_init_signals(Workspace* self)
{
	assert(self);
			
	psy_signal_init(&self->signal_view_selected);	
	psy_signal_init(&self->signal_machine_edit_resize);
	psy_signal_init(&self->signal_bus_changed);
}

void workspace_init_disk_ops(Workspace* self)
{	
	assert(self);
			
	workspace_init_song_load(self);
	workspace_init_song_save(self);
	workspace_init_sample_load(self);
}

void workspace_init_song_load(Workspace* self)
{				
	assert(self);
			
	psy_fileselect_init(&self->load_song);
	psy_fileselect_set_value(&self->load_song, PSYCLE_UNTITLED);
	psy_fileselect_connect_change(&self->load_song, self,
		(psy_fp_fileselect_change)workspace_on_load_song_change);
	psy_fileselect_connect_cancel(&self->load_song, self,
		(psy_fp_fileselect_change)workspace_on_diskop_song_cancel);
	psy_fileselect_connect_change_dir(&self->load_song, self,
		(psy_fp_fileselect_change)workspace_on_load_dir_song_change);
	psy_fileselect_set_title(&self->load_song,
		psy_ui_translate("file.loadsong"));	
	psy_fileselect_set_default_extension(&self->load_song,
		psy_audio_songfile_standardloadfilter());
	psy_fileselect_add_filter(&self->load_song, "psy", "*.psy");
	psy_fileselect_add_filter(&self->load_song, "mod", "*.mod");
#ifdef PSYCLE_USE_MIDI_FILE
	psy_fileselect_add_filter(&self->load_song, "midi", "*.mid");
#endif
	/* psy_fileselect_connect_file_name_change(&self->load_song, self,
		(psy_fp_fileselect_change)workspace_on_load_song_file_name_change);	
	psy_fileselect_enable_preview(&self->load_song); */
}

void workspace_on_load_song_file_name_change(Workspace* self,
	psy_FileSelect* sender)
{
	psy_audio_SongFile songfile;
	const char* path;

	assert(self);

	path = psy_fileselect_value(sender);			
	psy_audio_songfile_init(&songfile);
	psy_audio_songfile_set_logger(&songfile, self->terminal_output);	
	songfile.file = 0;		
	if (psy_audio_songfile_song_info(&songfile, path) != PSY_OK) {			
	}
	psy_audio_songfile_dispose(&songfile);
}

void workspace_init_song_save(Workspace* self)
{			
	assert(self);
				
	psy_fileselect_init(&self->save_song);
	psy_fileselect_connect_change(&self->save_song, self,
		(psy_fp_fileselect_change)workspace_on_save_song_change);
	psy_fileselect_connect_cancel(&self->save_song, self,
		(psy_fp_fileselect_change)workspace_on_diskop_song_cancel);
	psy_fileselect_connect_change_dir(&self->save_song, self,
		(psy_fp_fileselect_change)workspace_on_save_dir_song_change);
	psy_fileselect_set_title(&self->save_song,
		psy_ui_translate("file.savesong"));
	psy_fileselect_set_default_extension(&self->save_song,
		psy_audio_songfile_standardloadfilter());
	psy_fileselect_add_filter(&self->save_song, "psy", "*.psy");
	psy_fileselect_add_filter(&self->save_song, "xm", "*.xm");	
}

void workspace_init_sample_load(Workspace* self)
{	
	assert(self);

	psy_fileselect_init(&self->load_sample);
	psy_fileselect_connect_change(&self->load_sample, self,
		(psy_fp_fileselect_change)workspace_on_load_sample_change);
	psy_fileselect_connect_cancel(&self->load_sample, self,
		(psy_fp_fileselect_change)workspace_on_load_sample_cancel);
	psy_fileselect_connect_file_name_change(&self->load_sample, self,
		(psy_fp_fileselect_change)workspace_on_load_sample_file_name_change);
	psy_fileselect_set_title(&self->load_sample, "Load Sample");
	psy_fileselect_set_default_extension(&self->load_sample, "wav");
	psy_fileselect_add_filter(&self->load_sample, "Wave", "*.wav");
	psy_fileselect_add_filter(&self->load_sample, "IFF", "*.iff");	
	psy_fileselect_enable_preview(&self->load_sample);
}

void workspace_connect_configuration(Workspace* self)
{
	assert(self);

	psy_configuration_connect(psy_dirconfig_base(&self->cfg_.directories),
		"songs", self, workspace_on_song_directory_changed);
	psy_configuration_configure(psy_dirconfig_base(&self->cfg_.directories),
		"songs");
	psy_configuration_connect(keyboardmiscconfig_base(&self->cfg_.misc),
		"followsong", self, workspace_on_follow_song_changed);
	psy_configuration_configure(keyboardmiscconfig_base(&self->cfg_.misc),
		"followsong");
	psy_configuration_connect(globalconfig_base(&self->cfg_.global),
		"regenerateplugincache", self, workspace_on_plugin_scan);
	psy_configuration_connect(psycleconfig_misc(workspace_cfg(
			self)), "ft2fileexplorer",
		self, workspace_on_ft2_file_explorer);
	psy_configuration_connect(psycleconfig_misc(workspace_cfg(
			self)), "numdefaultlines",
		self, workspace_on_pattern_default_lines);	
	psy_configuration_connect(psycleconfig_directories(workspace_cfg(
		self)), "plugins.usejbridge",
		self, workspace_on_use_jbridge);
}

void workspace_dispose(Workspace* self)
{
	assert(self);
	
	psy_audio_midiconfig_write_controllers(&self->player_.midiinput.midiconfig);
	psycleconfig_save(&self->cfg_);	
	psy_audio_player_dispose(&self->player_);
	if (self->song) {
		psy_audio_song_deallocate(self->song);
		self->song = NULL;
	}	
	psycleconfig_dispose(&self->cfg_);		
	psy_undoredo_dispose(&self->undo_redo_);
	viewhistory_dispose(&self->view_history);
	workspace_dispose_signals(self);	
	psy_audio_sequencepaste_dispose(&self->sequencepaste);	
	psy_audio_dispose();	
	psy_fileselect_dispose(&self->load_song);
	psy_fileselect_dispose(&self->save_song);
	psy_fileselect_dispose(&self->load_sample);
	filechooser_dispose(&self->file_chooser);	
	pluginscanthread_dispose(&self->plugin_scan_thread_);
	hostmachinecallback_dispose(&self->hostmachinecallback);
	psy_property_dispose(&self->edit_mode);
	free(self->song_file_name_);
	self->song_file_name_ = NULL;
}

void workspace_dispose_signals(Workspace* self)
{
	assert(self);
						
	psy_signal_dispose(&self->signal_view_selected);	
	psy_signal_dispose(&self->signal_machine_edit_resize);
	psy_signal_dispose(&self->signal_bus_changed);	
}

void workspace_connect_input_handler(Workspace* self)
{	
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()), INPUTHANDLER_IMM,
		psy_EVENTDRIVER_CMD, "notes", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)workspace_on_notes);
	inputhandler_connect_host(psy_ui_app_input_handler(psy_ui_app()),
		self, (fp_inputhandler_hostcallback)
		workspace_on_input_handler_callback);
}

void workspace_clear_sequence_paste(Workspace* self)
{
	assert(self);

	psy_audio_sequencepaste_clear(&self->sequencepaste);
}

void workspace_new_song(Workspace* self)
{
	assert(self);	
	
	if (self->confirm && workspace_save_reminder(self) &&
			workspace_song_modified(self)) {	
		confirmbox_set_labels(self->confirm, "msg.newsong", "msg.savenew",
			"msg.nosavenew");	
		confirmbox_set_callbacks(self->confirm,
			psy_slot_make(self, workspace_on_confirm_accept_new),
			psy_slot_make(self, workspace_on_confirm_reject_new),
			psy_slot_make(self, workspace_on_confirm_continue));
		workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
	} else {
		workspace_new_song_internal(self);
	}
}

void workspace_on_confirm_accept_new(Workspace* self)
{
	psy_FileSelect save_new;
	
	assert(self);
	
	psy_fileselect_init(&save_new);
	psy_fileselect_copy(&save_new, &self->save_song);
	psy_fileselect_connect_change(&save_new, self, (psy_fp_fileselect_change)
		workspace_on_confirm_new_save_changed);
	psy_fileselect_connect_cancel(&save_new, self, (psy_fp_fileselect_change)
		workspace_on_confirm_new_save_cancel);
	workspace_disk_op(self, NULL, &save_new, NULL);
	psy_fileselect_dispose(&save_new);
}

void workspace_on_confirm_new_save_changed(Workspace* self, psy_FileSelect*
	sender)
{
	assert(self);
	
	workspace_save_song_internal(self, psy_fileselect_value(sender));
	workspace_reset_undo_save_point(self);	
	workspace_new_song_internal(self);
}

void workspace_on_confirm_new_save_cancel(Workspace* self, psy_FileSelect*
	sender)
{
	assert(self);
	
	workspace_select_view(self, viewindex_make_all(VIEW_ID_MACHINES, 0,
		psy_INDEX_INVALID, 0));
}

void workspace_on_confirm_reject_new(Workspace* self)
{
	assert(self);

	workspace_reset_undo_save_point(self);
	workspace_new_song_internal(self);
}

void workspace_new_song_internal(Workspace* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	workspace_set_song(self, psy_audio_song_alloc_init(
		&self->player_.machinefactory));	
	psy_audio_player_set_sampler_index(&self->player_,
		psy_configuration_value_int(psycleconfig_seqedit(&self->cfg_),
			"machine", 0x3E));
	psy_audio_exclusivelock_leave();
	psy_fileselect_set_value(&self->load_song, PSYCLE_UNTITLED);	
	workspace_select_view(self, viewindex_make_all(VIEW_ID_MACHINES, 0,
		psy_INDEX_INVALID, 0));
}

void workspace_confirm_load(Workspace* self)
{
	assert(self);
	assert(self->confirm);
	
	confirmbox_set_labels(self->confirm, "msg.loadsong", "msg.saveload",
		"msg.nosaveload");	
	confirmbox_set_callbacks(self->confirm,
		psy_slot_make(self, workspace_on_confirm_accept_load),
		psy_slot_make(self, workspace_on_confirm_reject_load),
		psy_slot_make(self, workspace_on_confirm_continue));
	workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
}

void workspace_on_confirm_accept_load(Workspace* self)
{
	assert(self);
	
	if (psy_configuration_value_bool(keyboardmiscconfig_base(
			&self->cfg_.misc),
			"ft2fileexplorer", TRUE)) {	
		if (self->file_chooser.file_view_) {
			char path[4096];
		
			fileview_filename(self->file_chooser.file_view_, path, 4096);
			workspace_load_song_from_path(self, path);
		}
	} else {
		// workspace_load_song_fileselect(self);
	}
}

void workspace_on_confirm_reject_load(Workspace* self)
{		
	assert(self);

	if (self->file_chooser.file_view_) {
		char path[4096];
		
		fileview_filename(self->file_chooser.file_view_, path, 4096);
		workspace_load_song_from_path(self, path);
	}
}

void workspace_on_confirm_continue(Workspace* self)
{
	assert(self);
	
	workspace_restore_view(self);
}

void workspace_confirm_close(Workspace* self)
{
	assert(self);
	assert(self->confirm);
	
	confirmbox_set_labels(self->confirm, "msg.psyexit", "msg.saveexit",
		"msg.nosaveexit");
	confirmbox_set_callbacks(self->confirm,
		psy_slot_make(self, workspace_confirm_accept_close),
		psy_slot_make(self, workspace_confirm_reject_close),
		psy_slot_make(self, workspace_on_confirm_continue));
	workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
}

void workspace_confirm_accept_close(Workspace* self)
{
	psy_FileSelect save;
	
	assert(self);
	
	psy_fileselect_init(&save);
	psy_fileselect_copy(&save, &self->save_song);
	psy_fileselect_connect_change(&save, self, (psy_fp_fileselect_change)
		workspace_on_confirm_close_save_changed);
	psy_fileselect_connect_cancel(&save, self, (psy_fp_fileselect_change)
		workspace_on_confirm_close_save_cancel);
	workspace_disk_op(self, NULL, &save, NULL);
	psy_fileselect_dispose(&save);	
}

void workspace_on_confirm_close_save_changed(Workspace* self, psy_FileSelect*
	sender)
{
	assert(self);
		
	workspace_save_song_internal(self, psy_fileselect_value(sender));
	workspace_reset_undo_save_point(self);	
	psy_ui_app_close(psy_ui_app());
}

void workspace_on_confirm_close_save_cancel(Workspace* self, psy_FileSelect*
	sender)
{
	assert(self);
	
	workspace_select_view(self, viewindex_make_all(VIEW_ID_MACHINES, 0,
		psy_INDEX_INVALID, 0));
}

void workspace_confirm_reject_close(Workspace* self)
{
	assert(self);
		
	workspace_reset_undo_save_point(self);
	psy_ui_app_close(psy_ui_app());
}

void workspace_reset_undo_save_point(Workspace* self)
{
	assert(self);

	self->modified_without_undo = FALSE;
	self->undo_save_point_ = psy_undoredo_undo_size(&self->undo_redo_);
	if (self->song) {
		self->machines_undo_save_point_ = psy_undoredo_undo_size(
			&self->song->machines_.undoredo);
	} else {
		self->machines_undo_save_point_ = 0;
	}
}

void workspace_confirm_seq_clear(Workspace* self)
{
	assert(self);
	assert(self->confirm);
	
	if (!workspace_song(self)) {
		return;
	}
	confirmbox_set_labels(self->confirm, "msg.seqclear", "msg.yes", "msg.no");	
	confirmbox_set_callbacks(self->confirm,
		psy_slot_make(self, workspace_confirm_accept_seqclear),
		psy_slot_make(self, workspace_confirm_reject_seqclear),
		psy_slot_make(self, workspace_on_confirm_continue));
	workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
}

void workspace_confirm_accept_seqclear(Workspace* self)
{
	SequenceCmds cmds;
	
	assert(self);
		
	workspace_restore_view(self);
	sequencecmds_init(&cmds, self);
	sequencecmds_clear(&cmds);	
}

void workspace_confirm_reject_seqclear(Workspace* self)
{
	assert(self);
	
	workspace_restore_view(self);
}

void workspace_load_song(Workspace* self)
{
	assert(self);

	if (workspace_save_reminder(self) &&
			workspace_song_modified(self)) {		
		confirmbox_set_labels(self->confirm, "msg.loadsong", "msg.saveload",
			"msg.nosaveload");	
		confirmbox_set_callbacks(self->confirm,
			psy_slot_make(self, workspace_on_confirm_accept_load),
			psy_slot_make(self, workspace_on_confirm_reject_load),
			psy_slot_make(self, workspace_on_confirm_continue));
		workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
	} else {
		workspace_disk_op(self, &self->load_song, NULL, NULL);			
	}	
}

void workspace_load_song_from_path(Workspace* self, const char* filename)
{	
	psy_audio_Song* song;	
	psy_audio_SongReader song_reader;
	psy_Loggers loggers;
	
	assert(self);

	song = psy_audio_song_alloc_init(psy_audio_player_machine_factory(
		&self->player_));
	if (!song) {
		return;
	}	
	psy_audio_player_fade_out(&self->player_);	
	psy_loggers_init(&loggers);
	psy_loggers_add(&loggers, self->terminal_output);
	psy_loggers_add(&loggers, self->progress);		
	psy_audio_songreader_init(&song_reader, song, psy_loggers_base(&loggers),
		psy_configuration_value_bool(psycleconfig_directories(workspace_cfg(self)),
		"plugins.usejbridge", FALSE));
	psy_audio_exclusivelock_enter();
	psy_audio_player_set_empty_song(&self->player_);
	psy_audio_machinecallback_set_song(workspace_base(self), song);
	if (psy_audio_songreader_load(&song_reader, filename) != PSY_OK) {
		psy_audio_player_set_song(&self->player_, self->song);
		psy_audio_exclusivelock_leave();
		psy_audio_song_deallocate(song);
		psy_audio_songreader_dispose(&song_reader);
		psy_loggers_dispose(&loggers);
		return;
	}	
	psy_audio_songreader_dispose(&song_reader);
	psy_loggers_dispose(&loggers);
	psy_audio_player_set_position(&self->player_, 0.0);
	workspace_set_song_filename(self, filename);
	workspace_set_song(self, song);	
	if (psy_configuration_value_bool(psycleconfig_general(&self->cfg_),
			"saverecentsongs", FALSE)) {
		workspace_add_recent(self, song, filename);
	}
	workspace_clear_undo(self);		
	if (psy_configuration_value_bool(psycleconfig_general(&self->cfg_),
			"playsongafterload", FALSE)) {
		psy_audio_player_start(&self->player_);
	}
	psy_audio_exclusivelock_leave();
}

void workspace_set_song_filename(Workspace* self, const char* filename)
{
	assert(self);

	psy_strreset(&self->song_file_name_, filename);
	self->song_has_file_ = TRUE;
}

void workspace_add_recent(Workspace* self, const psy_audio_Song* song,
	const char* filename)
{
	PlayList playlist;
	PlayListItemInfo item_info;
	psy_Path recent;

	assert(self);

	psy_path_init(&recent, NULL);
	psy_path_set_prefix(&recent, psy_dir_config());
	psy_path_set_name(&recent, PSYCLE_RECENT_SONG_INI);
	playlist_init(&playlist, psy_path_full(&recent));
	psy_path_dispose(&recent);
	if (!playlist_load(&playlist)) {
		playlist_set_title(&playlist, "Recent");
	}
	playlistiteminfo_init(&item_info, 0, psy_audio_song_title(song), "");
	if (!playlist_exist(&playlist, filename)) {
		PlayListItem item;

		playlistitem_init(&item, filename, &item_info);
		playlist_append(&playlist, &item);
		playlistitem_dispose(&item);
	}
	playlistiteminfo_dispose(&item_info);
	playlist_save(&playlist);
}

void workspace_set_song(Workspace* self, psy_audio_Song* song)
{
	psy_audio_Song* old;
	
	assert(self);

	if (self->song == song) {
		return;
	}
	old = self->song;
	self->song = song;		
	psy_audio_player_set_song(&self->player_, self->song);		
	if (old) {
		psy_audio_song_deallocate(old);
	}
	workspace_reset_views(self);	
}

void workspace_load_sample(Workspace* self, psy_audio_SampleIndex
	sample_index)
{
	assert(self);
	
	self->load_sample_index = sample_index;	
	workspace_disk_op(self, &self->load_sample, NULL, NULL);	
}

void workspace_on_load_sample_change(Workspace* self, psy_FileSelect* sender)
{	
	assert(self);
	
	if (psy_audio_samples_at_const(psy_audio_song_samples(self->song),
			self->load_sample_index)) {
		ViewIndex restore;

		confirmbox_set_labels(self->confirm,
			"msg.loadsample", "msg.cont", "msg.cancel");
		confirmbox_set_callbacks(self->confirm,
			psy_slot_make(self, workspace_load_sample_confirm_overwrite_accept),
			psy_slot_make(self, workspace_load_sample_confirm_overwrite_reject),
			psy_slot_make(NULL, NULL));
		psy_fileselect_copy(&self->load_sample, sender);
		restore = self->restoreview;
		workspace_select_view(self, viewindex_make(VIEW_ID_CONFIRM));
		self->restoreview = restore;
	} else {
		workspace_load_sample_internal(self, psy_fileselect_value(sender));
		workspace_restore_view(self);
	}
}

void workspace_on_load_sample_cancel(Workspace* self, psy_FileSelect* sender)
{
	assert(self);

	psy_audio_player_wave_prev_stop(&self->player_);
	workspace_restore_view(self);
}

void workspace_on_load_sample_file_name_change(Workspace* self,
	psy_FileSelect* sender)
{
	const char* path;

	assert(self);

	path = psy_fileselect_value(sender);
	psy_audio_player_wave_prev_play(&self->player_, path);
}

void workspace_load_sample_confirm_overwrite_accept(Workspace* self)
{
	workspace_load_sample_internal(self, psy_fileselect_value(
		&self->load_sample));
	workspace_restore_view(self);
}

void workspace_load_sample_confirm_overwrite_reject(Workspace* self)
{
	workspace_restore_view(self);
}

void workspace_load_sample_confirm_overwrite_continue(Workspace* self)
{
	workspace_restore_view(self);
}

void workspace_load_sample_internal(Workspace * self, const char* path)
{
	assert(self);

	if (workspace_song(self)) {
		psy_audio_Sample* sample;		
		psy_audio_Instrument* instrument;
		psy_audio_InstrumentEntry entry;
		psy_audio_InstrumentIndex inst;
		bool inst_created;

		/* create sample */
		sample = psy_audio_sample_alloc_init(0);
		psy_audio_sample_load(sample, path);		
		psy_audio_samples_insert(&workspace_song(self)->samples_,
			sample, self->load_sample_index);
		/* add sample to instrument */
		inst = psy_audio_instruments_selected(&self->song->instruments_);		
		instrument = psy_audio_instruments_at(psy_audio_song_instruments(
			workspace_song(self)), inst);
		if (!instrument) {
			instrument = psy_audio_instrument_allocinit();
			psy_audio_instrument_set_name(instrument, psy_audio_sample_name(
				sample));
			inst_created = TRUE;
		} else {
			inst_created = FALSE;
		}
		psy_audio_instrumententry_init(&entry);
		entry.sampleindex = self->load_sample_index;
		psy_audio_instrument_add_entry(instrument, &entry);
		if (inst_created) {
			psy_audio_instruments_insert(psy_audio_song_instruments(
				workspace_song(self)), instrument, inst);
		}
		psy_audio_instruments_select(psy_audio_song_instruments(
			workspace_song(self)), inst);
		psy_audio_machines_notify_aux_change(psy_audio_song_machines(
			self->song));		
	}
}

void workspace_reset_views(Workspace* self)
{
	ViewIndex view;	
	psy_audio_SequenceCursor cursor;
		
	assert(self);
	
	workspace_clear_sequence_paste(self);
	workspace_clear_undo(self);
	self->modified_without_undo = FALSE;		
	view = viewhistory_current(&self->view_history);
	viewhistory_clear(&self->view_history);
	viewhistory_add(&self->view_history, view);
	psy_audio_sequenceselection_clear(&self->song->sequence_.selection);
	psy_audio_sequenceselection_select_first(&self->song->sequence_.selection,
		psy_audio_orderindex_make(0, 0));		
	psy_audio_sequencecursor_init(&cursor);
	psy_audio_sequence_set_cursor(&self->song->sequence_, cursor);
}

void workspace_save_song_as(Workspace* self)
{
	assert(self);
				
	workspace_disk_op(self, NULL, &self->save_song, NULL);
}

void workspace_save_song_internal(Workspace* self, const char* path)
{	
	int status;
	psy_audio_SongFile songfile;
	psy_Path extract;
	int format;

	assert(self);
	
	if (!self->song || (path == NULL)) {
		return;
	}
	psy_audio_songfile_init_song(&songfile, self->song);
	psy_audio_songfile_set_logger(&songfile, self->terminal_output);	
	psy_path_init(&extract, path);	
	format = 0;
	if (psy_path_has_ext(&extract)) {
		if (strcmp(psy_path_ext(&extract), "psy") == 0) {
			format = 0;
		} else if (strcmp(psy_path_ext(&extract), "xm") == 0) {
			format = 1;
		}
	}
	psy_path_dispose(&extract);
	switch (format) {
	case 1:
		status = psy_audio_songfile_exportmodule(&songfile, path);
	default:
		status = psy_audio_songfile_save(&songfile, path);
		break;
	}
	if (status == PSY_OK) {
		psy_strreset(&self->song_file_name_, path);
		self->song_has_file_ = TRUE;
		if (self->terminal_output) {
			psy_logger_output(self->terminal_output, "ready\n");
		}
	}
	psy_audio_songfile_dispose(&songfile);
	if (status == PSY_OK) {	
		workspace_update_save_point(self);		
		self->update_title = TRUE;		
	}
}

void workspace_update_save_point(Workspace* self)
{
	assert(self);
	
	self->undo_save_point_ = psy_undoredo_undo_size(&self->undo_redo_);
	if (self->song) {
		self->machines_undo_save_point_ = psy_undoredo_undo_size(
			&self->song->machines_.undoredo);
	} else {
		self->machines_undo_save_point_ = 0;
	}
	self->modified_without_undo = FALSE;
}

void workspace_disk_op_song(Workspace* self)
{
	assert(self);
	
	workspace_disk_op(self, &self->load_song, &self->save_song, NULL);
}

void workspace_start_audio(Workspace* self)
{	
	assert(self);
	
	psy_audio_player_start_audio(&self->player_, self->terminal_output);	
}

void workspace_undo(Workspace* self)
{
	assert(self);

	if (workspace_current_view(self).id == VIEW_ID_MACHINES) {
		psy_undoredo_undo(&self->song->machines_.undoredo);
	} else {
		psy_undoredo_undo(&self->undo_redo_);
	}
}

void workspace_redo(Workspace* self)
{
	assert(self);

	if (workspace_current_view(self).id == VIEW_ID_MACHINES) {
		psy_undoredo_redo(&self->song->machines_.undoredo);
	} else {
		psy_undoredo_redo(&self->undo_redo_);
	}
}

bool workspace_currview_has_undo(Workspace* self)
{
	assert(self);

	if (!self->song) {
		return FALSE;
	}
	if (workspace_current_view(self).id == VIEW_ID_MACHINES) {
		return psy_undoredo_undo_size(&self->song->machines_.undoredo) != 0;
	}
	return (psy_undoredo_undo_size(&self->undo_redo_) != 0);
}

bool workspace_currview_has_redo(Workspace* self)
{
	assert(self);

	if (!self->song) {
		return FALSE;
	}
	if (workspace_current_view(self).id == VIEW_ID_MACHINES) {
		return psy_undoredo_redo_size(&self->song->machines_.undoredo) !=
			self->machines_undo_save_point_;
	}
	return (psy_undoredo_redo_size(&self->undo_redo_) !=
		self->undo_save_point_);
}

void workspace_clear_undo(Workspace* self)
{
	assert(self);

	psy_undoredo_dispose(&self->undo_redo_);
	psy_undoredo_init(&self->undo_redo_);
	self->undo_save_point_ = 0;
	if (self->song) {
		psy_undoredo_dispose(&self->song->machines_.undoredo);
		psy_undoredo_init(&self->song->machines_.undoredo);
	}
	self->machines_undo_save_point_ = 0;
}

ViewIndex workspace_current_view(Workspace* self)
{
	assert(self);

	return viewhistory_current(&self->view_history);
}

int workspace_has_plugin_cache(const Workspace* self)
{
	assert(self);

	return psy_audio_plugincatcher_has_cache(&self->player_.plugincatcher);
}

psy_EventDriver* workspace_kbd_driver(Workspace* self)
{
 	assert(self);

	return psy_audio_player_kbd_driver(&self->player_);
}

void workspace_idle(Workspace* self)
{
	assert(self);

	workspace_update_play_status(self, self->player_.follow_song);		
	pluginscanthread_idle(&self->plugin_scan_thread_);
	psy_audio_player_idle(&self->player_);	
}

void workspace_update_play_status(Workspace* self, bool follow_song)
{	
	bool restore_prevented;

	assert(self);	

	restore_prevented = viewhistory_prevented(&self->view_history);
	viewhistory_prevent(&self->view_history);
	psy_audio_sequencer_update_host_seq_time(&self->player_.sequencer,
		follow_song);	
	if (!restore_prevented) {
		viewhistory_enable(&self->view_history);
	}
}

void workspace_on_play_status_changed(Workspace* self,
	psy_audio_Sequencer* sequencer)
{
	assert(self);
	
	if (sequencer->hostseqtime.playend && self->player_.sequencer.mode ==
		psy_audio_SEQUENCERPLAYMODE_PLAYFILES) {
		psy_fileselect_next(&self->load_song);
	}
}

void workspace_save_view(Workspace* self)
{
	assert(self);

	self->restoreview = workspace_current_view(self);
}

void workspace_restore_view(Workspace* self)
{
	assert(self);

	workspace_select_view(self, self->restoreview);	
}

void workspace_update_view_focus(Workspace* self)
{
	ViewIndex view;

	assert(self);

	view = workspace_current_view(self);
	workspace_select_view(self, view);
}

void workspace_select_view(Workspace* self, ViewIndex view_index)
{
	assert(self);

	if (view_index.id == VIEW_ID_CONFIRM &&
			workspace_current_view(self).id != VIEW_ID_CONFIRM) {
		workspace_save_view(self);		
	}	
	psy_signal_emit(&self->signal_view_selected, self, 3, view_index.id,
		view_index.section, view_index.option);
}

void workspace_add_view(Workspace* self, ViewIndex view)
{
	assert(self);
	
	viewhistory_add(&self->view_history, view);
}

void workspace_back(Workspace* self)
{
	ViewIndex view;

	assert(self);

	view = viewhistory_current(&self->view_history);
	if (viewhistory_back(&self->view_history)) {
		if (!viewhistory_equal(&self->view_history, view)) {
			workspace_update_currview(self);
		}
	}
}

void workspace_forward(Workspace* self)
{
	ViewIndex view;

	assert(self);

	view = viewhistory_current(&self->view_history);
	if (viewhistory_forward(&self->view_history)) {
		if (!viewhistory_equal(&self->view_history, view)) {
			workspace_update_currview(self);
		}
	}
}

void workspace_update_currview(Workspace* self)
{
	ViewIndex view;
	int prevented;

	assert(self);

	view = viewhistory_current(&self->view_history);
	prevented = viewhistory_prevented(&self->view_history);
	viewhistory_prevent(&self->view_history);
	workspace_select_view(self, view);
	if (!prevented) {
		viewhistory_enable(&self->view_history);
	}
}

bool workspace_song_modified(const Workspace* self)
{
	assert(self);

	if (!self->song) {
		return FALSE;
	}
	if (self->modified_without_undo) {
		return TRUE;
	}	
	return ((psy_undoredo_undo_size(&self->undo_redo_) !=
			self->undo_save_point_) ||
		(psy_undoredo_undo_size(&self->song->machines_.undoredo) !=
			self->machines_undo_save_point_));
}

void workspace_scan_plugins(Workspace* self)
{
	pluginscanthread_start(&self->plugin_scan_thread_);
}

void workspace_mark_song_modified(Workspace* self)
{
	assert(self);

	self->modified_without_undo = TRUE;
}

void workspace_set_progress_logger(Workspace* self, psy_Logger* progress)
{
	assert(self);

	self->progress = progress;
	self->plugin_scan_thread_.progress = progress;
}

void workspace_on_input(Workspace* self, uintptr_t cmdid)
{
	assert(self);

	switch (cmdid) {
	case CMD_IMM_EDTMODE:
		psy_property_set_item_bool(&self->edit_mode,
			!psy_property_item_bool(&self->edit_mode));
		break;
	case CMD_IMM_ADDMACHINE:
		workspace_select_view(self, viewindex_make_section(VIEW_ID_MACHINES,
			SECTION_ID_MACHINEVIEW_NEWMACHINE));
		break;
	case CMD_IMM_EDITMACHINE:
		if (workspace_current_view(self).id != VIEW_ID_MACHINES) {
			workspace_select_view(self, viewindex_make(VIEW_ID_MACHINES));
		} else {
			if (workspace_current_view(self).section == SECTION_ID_MACHINEVIEW_WIRES) {
				workspace_select_view(self, viewindex_make_section(
					VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_STACK));
			} else {
				workspace_select_view(self, viewindex_make_section(
					VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_WIRES));
			}
		}
		break;
	case CMD_IMM_HELP:
		workspace_select_view(self, viewindex_make_section(
			VIEW_ID_HELPVIEW, SECTION_ID_HELPVIEW_HELP));
		break;
	case CMD_IMM_EDITPATTERN: {
		ViewIndex view;
		
		view = workspace_current_view(self);					
		if (view.id != VIEW_ID_PATTERNS) {			
			workspace_select_view(self, viewindex_make(VIEW_ID_PATTERNS));
		} else if (view.section == SECTION_ID_PATTERNVIEW_TRACKER) {			
			workspace_select_view(self, viewindex_make_section(
				VIEW_ID_PATTERNS, SECTION_ID_PATTERNVIEW_PIANO));
		} else {				
			workspace_select_view(self, viewindex_make_section(
				VIEW_ID_PATTERNS, SECTION_ID_PATTERNVIEW_TRACKER));				
		}				
		break; }
	case CMD_IMM_EDITINSTR:		
		workspace_select_view(self, viewindex_make(VIEW_ID_INSTRUMENTS));
		break;
	case CMD_IMM_EDITSAMPLE:
		workspace_select_view(self, viewindex_make(VIEW_ID_SAMPLES));
		break;
	case CMD_IMM_EDITWAVE:
		workspace_select_view(self, viewindex_make_section(
			VIEW_ID_SAMPLES, 2));				
		break;
	case CMD_IMM_INSTRDEC:
		if (self->song) {
			psy_audio_instruments_dec(&self->song->instruments_);
		}
		break;
	case CMD_IMM_INSTRINC:
		if (self->song) {
			psy_audio_instruments_inc(&self->song->instruments_);
		}
		break;
	case CMD_IMM_ENABLEAUDIO: {
		psy_Property* p;
			
		p = psy_configuration_at(psycleconfig_global(&self->cfg_),
			"enableaudio");
		if (p) {			
			psy_property_set_item_bool(p, !psy_property_item_bool(p));			
		}		
		break; }
	case CMD_IMM_SETTINGS:
		workspace_select_view(self, viewindex_make(VIEW_ID_SETTINGS));
		break;
	case CMD_IMM_LOADSONG:
		workspace_load_song(self);
		break;
	case CMD_IMM_LOAD_SAMPLE:
		//workspace_load_sample(self);
		break;
	case CMD_IMM_INFOMACHINE:
		if (self->song && self->param_views_) {
			paramviews_show(self->param_views_, psy_audio_machines_selected(
				&self->song->machines_));
		}
		break;
	case CMD_IMM_PATTERNINC: {
		SequenceCmds cmds;

		sequencecmds_init(&cmds, self);
		sequencecmds_incpattern(&cmds);
		break; }
	case CMD_IMM_PATTERNDEC: {
		SequenceCmds cmds;

		sequencecmds_init(&cmds, self);
		sequencecmds_decpattern(&cmds);
		break; }
	case CMD_IMM_SONGPOSDEC:
		if (self->song) {
			psy_audio_sequence_dec_seqpos(&self->song->sequence_);
		}
		break;
	case CMD_IMM_SONGPOSINC:
		if (self->song) {
			psy_audio_sequence_inc_seqpos(&self->song->sequence_);
		}
		break;
	case CMD_IMM_PLAYSONG:
		psy_audio_player_start_begin(&self->player_);		
		break;
	case CMD_IMM_PLAYSTART:
		psy_audio_player_start_curr_seq_pos(&self->player_);
		break;
	case CMD_IMM_PLAYSTOP:
		psy_audio_player_stop(&self->player_);
		break;
	case CMD_IMM_PLAYROWTRACK:
		/* todo */
		break;
	case CMD_IMM_PLAYROWPATTERN:
		/* todo */
		break;
	case CMD_IMM_PLAYFROMPOS:
		/* todo */
		break;
	case CMD_IMM_SAVESONG:
		if (self->song) {
			if (self->song_has_file_) {				
				workspace_save_song_internal(self, self->song_file_name_);
			} else {
				workspace_disk_op(self, NULL, &self->save_song, NULL);
			}
		}
		break;
	case CMD_IMM_SAVESONGAS:
		workspace_disk_op(self, NULL, &self->save_song, NULL);
		break;
	case CMD_IMM_FOLLOWSONG: {		
		psy_Property* p;
			
		p = psy_configuration_at(keyboardmiscconfig_base(&self->cfg_.misc),
				"followsong");
		if (p) {
			if (self->player_.follow_song) {
				psy_property_set_item_bool(p, FALSE);
			} else {
				psy_property_set_item_bool(p, TRUE);
			}			
		}
		break; }
	case CMD_COLUMN_0:
	case CMD_COLUMN_1:
	case CMD_COLUMN_2:
	case CMD_COLUMN_3:
	case CMD_COLUMN_4:
	case CMD_COLUMN_5:
	case CMD_COLUMN_6:
	case CMD_COLUMN_7:
	case CMD_COLUMN_8:
	case CMD_COLUMN_9:
	case CMD_COLUMN_A:
	case CMD_COLUMN_B:
	case CMD_COLUMN_C:
	case CMD_COLUMN_D:
	case CMD_COLUMN_E:
	case CMD_COLUMN_F:
		if (self->song && psy_audio_song_num_song_tracks(self->song) >=
				(uintptr_t)(cmdid - CMD_COLUMN_0)) {
			self->song->sequence_.cursor.channel_ = (cmdid - CMD_COLUMN_0);
			psy_audio_sequence_set_cursor(psy_audio_song_sequence(self->song),
				self->song->sequence_.cursor);
		}
		break;
	case CMD_IMM_TAB1:
		workspace_select_view(self, viewindex_make(0));
		break;
	case CMD_IMM_TAB2:
		workspace_select_view(self, viewindex_make(1));
		break;
	case CMD_IMM_TAB3:
		workspace_select_view(self, viewindex_make(2));
		break;
	case CMD_IMM_TAB4:
		workspace_select_view(self, viewindex_make(3));
		break;
	case CMD_IMM_TAB5:
		workspace_select_view(self, viewindex_make(4));
		break;
	case CMD_IMM_TAB6:
		workspace_select_view(self, viewindex_make(5));
		break;
	case CMD_IMM_TAB7:
		workspace_select_view(self, viewindex_make(6));
		break;
	case CMD_IMM_TAB8:
		workspace_select_view(self, viewindex_make(7));
		break;
	case CMD_IMM_TAB9:
		workspace_select_view(self, viewindex_make(8));
		break;
	default:
		break;
	}
}

void workspace_app_title(Workspace* self, char* rv_title, uintptr_t max_len)
{	
	assert(self);

	rv_title[0] = '\n';
	if (self->song) {
		psy_Path path;

		psy_path_init(&path, self->song_file_name_);
		psy_snprintf(rv_title, max_len,
			(workspace_song_modified(self))
			? "[%s.%s *] %s %s"
			: "[%s.%s] %s %s",
			psy_path_name(&path), psy_path_ext(&path),
			PSYCLE__TITLE, " (" PSYCLE__VERSION ")");
		psy_path_dispose(&path);
	} else {
		psy_snprintf(rv_title, max_len,	"%s %s", PSYCLE__TITLE,
			" (" PSYCLE__VERSION ")");
	}
}

const char* workspace_song_title(const Workspace* self)
{
	assert(self);

	if (self->song) {
		return psy_audio_song_title(self->song);
	}
	return "";		
}

void workspace_select_start_view(Workspace* self)
{	
	ViewIndex view;
	
	assert(self);
		
	if (psy_configuration_value_bool(psycleconfig_general(&self->cfg_),
			"bench.showaboutatstart", TRUE)) {
		view = viewindex_make_section(VIEW_ID_HELPVIEW,
			SECTION_ID_HELPVIEW_ABOUT);
	} else {
		view = viewindex_make_section(VIEW_ID_MACHINES,
			SECTION_ID_MACHINEVIEW_WIRES);
	}	
	workspace_select_view(self, view);
}

void workspace_disk_op(Workspace* self, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir)
{
	assert(self);
		
	psy_diskop_execute(filechooser_base(&self->file_chooser), load, save, dir);	
}

void workspace_on_load_song_change(Workspace* self, psy_FileSelect* sender)
{
	assert(self);
	
	psy_fileselect_copy(&self->load_song, sender);
	workspace_load_confirm(self, psy_fileselect_value(sender));
}

void workspace_on_save_song_change(Workspace* self, psy_FileSelect* sender)
{
	assert(self);
	
	workspace_save_confirm(self, psy_fileselect_value(sender));	
}  

void workspace_on_load_dir_song_change(Workspace* self, psy_FileSelect* sender)
{
	assert(self);

	psy_strreset(&self->load_song.directory, sender->directory);
	psy_strreset(&self->save_song.directory, sender->directory);
}

void workspace_on_save_dir_song_change(Workspace* self, psy_FileSelect* sender)
{
	assert(self);

	psy_strreset(&self->load_song.directory, sender->directory);
	psy_strreset(&self->save_song.directory, sender->directory);	
}

void workspace_on_diskop_song_cancel(Workspace* self, psy_FileSelect* sender)
{
	assert(self);

	workspace_restore_view(self);
}

void workspace_load_confirm(Workspace* self, const char* path)
{	
	assert(self);
	
	if (psy_strlen(path) == 0) {
		workspace_restore_view(self);
		return;
	}	
	if (workspace_save_reminder(self) && workspace_song_modified(self)) {
		workspace_confirm_load(self);
	} else {		
		workspace_load_song_from_path(self, path);
	}
}

void workspace_save_confirm(Workspace* self, const char* path)
{	
	assert(self);
			
	workspace_save_song_internal(self, path);	
	workspace_restore_view(self);
}

void workspace_on_song_directory_changed(Workspace* self, psy_Property* sender)
{
	assert(self);
	
	psy_fileselect_set_directory(&self->load_song, psy_property_item_str(
		sender));
	psy_fileselect_set_directory(&self->save_song, psy_property_item_str(
		sender));
	if (self->file_chooser.file_view_ && self->file_chooser.file_view_->song_link) {
		fileviewlink_set_path(self->file_chooser.file_view_->song_link,
			psy_property_item_str(sender));
	}
}

void workspace_on_follow_song_changed(Workspace* self, psy_Property* sender)
{
	assert(self);
	
	self->player_.follow_song = psy_property_item_bool(sender);
}

void workspace_on_plugin_scan(Workspace* self, psy_Property* sender)
{	
	assert(self);
	
	pluginscanthread_start(&self->plugin_scan_thread_);
}

void workspace_on_ft2_file_explorer(Workspace* self, psy_Property* sender)
{	
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		filechooser_use_file_view(&self->file_chooser);
	} else {
		filechooser_use_native(&self->file_chooser);
	}	
}

void workspace_on_use_jbridge(Workspace* self, psy_Property* sender)
{
	assert(self);

	if (psy_property_item_bool(sender)) {
		self->player_.machinefactory.use_jbridge = (psy_property_item_bool(sender));
	}	
}

void workspace_on_edit_mode(Workspace* self, psy_Property* sender)
{
	assert(self);

	if (psy_property_item_bool(sender)) {
		psy_audio_player_start_edit_mode(&self->player_);		
	} else {
		psy_audio_player_stop_edit_mode(&self->player_);		
	}	
}

void workspace_on_pattern_default_lines(Workspace* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_int(sender) > 0) {
		psy_audio_pattern_set_num_default_lines((uintptr_t)
			psy_property_item_int(sender));
	}
}

bool workspace_save_reminder(const Workspace* self)
{
	assert(self);

	return psy_configuration_value_bool(psycleconfig_misc(&((Workspace*)self)->cfg_),
		"savereminder", TRUE);
}

bool workspace_on_notes(Workspace* self, InputHandler* sender)
{
	assert(self);

	psy_audio_PatternEvent ev;
	psy_EventDriverCmd cmd;

	cmd = inputhandler_cmd(sender);
	if (cmd.id >= 0 && cmd.id < 256) {
		psy_EventDriverInput input;

		input = inputhandler_input(sender);
		if ((input.param2 == FALSE) /* no autorepeat */ || (cmd.id >= CMD_NOTE_OFF_C_0)) {
			uint8_t note;
			uint8_t note_off;
			uintptr_t track;

			if (cmd.id >= CMD_NOTE_OFF_C_0) {
				note = psy_audio_NOTECOMMANDS_RELEASE;
				note_off = (uint8_t)cmd.id - (uint8_t)CMD_NOTE_OFF_C_0 +
					(uint8_t)self->player_.octave * 12;;
			} else {
				note = (uint8_t)cmd.id;
				note_off = psy_audio_NOTECOMMANDS_EMPTY;
			}
			ev = psy_audio_player_pattern_event(&self->player_, note);
			track = psy_audio_player_multi_channel_audition(
				&self->player_, &ev, note_off);
			psy_audio_player_play_event(&self->player_, &ev, track);
		}
		return 1;
	}
	return 0;
}

bool workspace_on_input_handler_callback(Workspace* self, int message, void* param1)
{
	assert(self);

	return ((message == INPUTHANDLER_HASFOCUS) && psy_ui_component_has_focus(
		(psy_ui_Component*)param1));
}
