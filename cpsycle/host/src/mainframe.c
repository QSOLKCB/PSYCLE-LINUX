/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "mainframe.h"
/* host */
#include "cmdsgeneral.h"
#include "cmdsnotes.h"
#include "resources/resource.h"
#include "styles.h"
/* ui */
#include <uidefaults.h>
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

/* prototypes */
/* build */
static void mainframe_on_destroyed(MainFrame*);
static void mainframe_init_frame(MainFrame*);
static void mainframe_init_frame_drag(MainFrame*);
static void mainframe_connect_visual_configuration(MainFrame*);
static void mainframe_connect_configuration(MainFrame*);
static void mainframe_init_workspace(MainFrame*);
static void mainframe_init_layout(MainFrame*);
static void mainframe_init_messageview(MainFrame*);
static void mainframe_init_kbd_help(MainFrame*);
static void mainframe_connect_statusbar(MainFrame*);
static void mainframe_init_bars(MainFrame*);
static void mainframe_init_metronome_bar(MainFrame*,
	psy_ui_Component* parent);
static void mainframe_init_help_setting_bar(MainFrame*,
	psy_ui_Component* parent);
static void mainframe_init_gear(MainFrame*);
static void mainframe_init_param_rack(MainFrame*);
static void mainframe_init_cpu_view(MainFrame*);
static void mainframe_init_midi_monitor(MainFrame*);
static void mainframe_init_step_sequencer_view(MainFrame*);
static void mainframe_init_keyboard_view(MainFrame*);
static void mainframe_init_seq_editor(MainFrame*);
static void mainframe_init_sequence_view(MainFrame*);
static void mainframe_init_sequencer_bar(MainFrame*);
#ifdef PSYCLE_USE_PLUGIN_EDITOR
static void mainframe_init_plugin_editor(MainFrame*);
#endif
static void mainframe_connect_song(MainFrame*);
static void mainframe_init_minmaximize(MainFrame*);
static void mainframe_connect_workspace(MainFrame*);
static void mainframe_connect_input_handler(MainFrame*);
static void mainframe_init_interpreter(MainFrame*);
/* events */
static void mainframe_on_key_down(MainFrame*, psy_ui_KeyboardEvent*);
static void mainframe_check_play_start_with_right_ctrl(MainFrame*, psy_ui_KeyboardEvent*);
static void mainframe_on_key_up(MainFrame*, psy_ui_KeyboardEvent*);
static void mainframe_delegate_keyboard(MainFrame*, intptr_t message,
	psy_ui_KeyboardEvent*);
static void mainframe_on_song_changed(MainFrame*, psy_audio_Player* sender);
static void mainframe_on_focus(MainFrame*);
static void mainframe_update_song_title(MainFrame*);
static void mainframe_on_timer(MainFrame*, uintptr_t timerid);
static bool mainframe_on_input(MainFrame*, InputHandler* sender);
static void mainframe_on_piano_kbd(MainFrame*, psy_Property* sender);
static void mainframe_on_seq_editor(MainFrame*, psy_Property* sender);
static void mainframe_on_step_sequencer(MainFrame*, psy_Property* sender);
static void mainframe_connect_step_sequencer_buttons(MainFrame*);
static void mainframe_connect_piano_kbd_buttons(MainFrame*);
static void mainframe_connect_seq_editor_buttons(MainFrame*);
static void mainframe_on_toggle_messageview(MainFrame*, psy_ui_Component* sender);
static void mainframe_on_toggle_kbd_help(MainFrame*, psy_ui_Component* sender);
static void mainframe_on_exit(MainFrame*, psy_ui_Component* sender);
static void mainframe_on_songtracks_changed(MainFrame*,
	psy_audio_Patterns* sender);
static bool mainframe_on_close(MainFrame*);
static void mainframe_seqeditor_on_float(MainFrame*, psy_ui_Button* sender);
static void mainframe_on_metronome_bar(MainFrame*, psy_Property* sender);
static void mainframe_on_trackscope_view(MainFrame*, psy_Property* sender);
#ifdef PSYCLE_USE_PLUGIN_EDITOR
static void mainframe_on_plugin_editor(MainFrame*, psy_Property* sender);
static void mainframe_on_zoom(MainFrame*, psy_Property* sender);
#endif
static void mainframe_on_param_rack(MainFrame*, psy_Property* sender);
static void mainframe_on_gear(MainFrame*, psy_Property* sender);
static void mainframe_on_midi_monitor(MainFrame*, psy_Property* sender);
static void mainframe_on_cpu_view(MainFrame*, psy_Property* sender);
static void mainframe_on_help(MainFrame*, psy_ui_Button* sender);
static void mainframe_on_settings(MainFrame*, psy_ui_Button* sender);
static void mainframe_align(MainFrame*);
static void mainframe_on_app_theme(MainFrame*, psy_Property* sender);
static void mainframe_on_set_default_font(MainFrame*, psy_Property* sender);
static void mainframe_show_hide_sidebar(MainFrame*, psy_ui_Component* view,
	psy_ui_Splitter*, bool show);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MainFrame* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.onclose =
			(psy_ui_fp_component_onclose)
			mainframe_on_close;
		vtable.on_destroyed =
			(psy_ui_fp_component)
			mainframe_on_destroyed;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			mainframe_on_key_down;
		vtable.onkeyup =
			(psy_ui_fp_component_on_key_event)
			mainframe_on_key_up;		
		vtable.on_focus =
			(psy_ui_fp_component)
			mainframe_on_focus;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			mainframe_on_timer;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(mainframe_base(self), &vtable);
}

/* implementation */
void mainframe_init(MainFrame* self, const PsycleCmdLine* cmdline)
{
	assert(self);
	assert(cmdline);
		
	mainframe_init_frame(self);	
	mainframe_init_workspace(self);
	mainframe_connect_visual_configuration(self);	
	mainframe_init_layout(self);	
	mainframe_init_messageview(self);
	mainframe_init_kbd_help(self);	
	mainframe_init_bars(self);
	mainframe_init_gear(self);
	mainframe_init_param_rack(self);
	mainframe_init_cpu_view(self);
	mainframe_init_midi_monitor(self);
	mainframe_init_sequence_view(self);
	mainframe_init_sequencer_bar(self);
	mainframe_init_keyboard_view(self);
	mainframe_init_step_sequencer_view(self);
	mainframe_init_seq_editor(self);
#ifdef PSYCLE_USE_PLUGIN_EDITOR	
	mainframe_init_plugin_editor(self);
#endif	
	mainframe_connect_statusbar(self);
	mainframe_init_minmaximize(self);
	mainframe_update_song_title(self);
	mainframe_init_interpreter(self);	
	mainframe_connect_song(self);
	mainframe_connect_workspace(self);	
	mainframe_connect_piano_kbd_buttons(self);	
	mainframe_connect_step_sequencer_buttons(self);	
	mainframe_connect_seq_editor_buttons(self);
	mainframe_connect_configuration(self);	
	if (!psyclecmdline_skip_scan(cmdline) && !workspace_has_plugin_cache(
			&self->workspace_)) {
		workspace_scan_plugins(&self->workspace_);
	}	
	startscript_init(&self->start_script_, self);
	startscript_run(&self->start_script_);
	workspace_select_start_view(&self->workspace_);
	/* fix for machineview master center */
	self->main_views_.machineview.wireview.pane.starting = FALSE;	
	mainframe_init_frame_drag(self);
	if (psy_strlen(psyclecmdline_song_name(cmdline)) > 0) {
		workspace_load_song_from_path(&self->workspace_,
			psyclecmdline_song_name(cmdline));
	}
	workspace_start_audio(&self->workspace_);
	kbdhelp_update(&self->kbd_help_);
	psy_ui_component_start_timer(mainframe_base(self), 0, 50);	
}

void mainframe_on_destroyed(MainFrame* self)
{	
	assert(self);
	
	paramviews_dispose(&self->param_views_);
	startscript_dispose(&self->start_script_);	
	workspace_dispose(&self->workspace_);
	interpreter_dispose(&self->interpreter_);
#ifdef PSYCLE_USE_FRAME_DRAG
	framedrag_dispose(&self->frame_drag_);
#endif
	psy_ui_app_stop(psy_ui_app());
}

MainFrame* mainframe_alloc(void)
{
	return (MainFrame*)malloc(sizeof(MainFrame));
}

MainFrame* mainframe_alloc_init(const PsycleCmdLine* cmdline)
{
	MainFrame* rv;

	rv = mainframe_alloc();
	if (rv) {
		mainframe_init(rv, cmdline);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void mainframe_init_frame(MainFrame* self)
{
	assert(self);

	psy_ui_frame_init_main(mainframe_base(self));
	vtable_init(self);
	self->starting_ = TRUE;
	self->title_modified_ = FALSE;
	self->allow_frame_move_ = FALSE;
	psy_ui_component_doublebuffer(mainframe_base(self));
	psy_ui_app_set_main(psy_ui_app(), mainframe_base(self));
	psy_ui_component_set_icon_ressource(mainframe_base(self), IDI_PSYCLEICON);
	init_host_styles(&psy_ui_app_defaults()->styles,
		psy_ui_defaults()->styles.theme_mode);
	psy_ui_component_init_align(&self->pane_, mainframe_base(self),
		mainframe_base(self), psy_ui_ALIGN_CLIENT);
}

void mainframe_init_frame_drag(MainFrame* self)
{
	assert(self);

#ifdef PSYCLE_USE_FRAME_DRAG
	framedrag_init(&self->frame_drag_, &self->component);
	framedrag_add(&self->frame_drag_, &self->top_);
	framedrag_add(&self->frame_drag_, &self->top_rows_);
	framedrag_add(&self->frame_drag_, &self->top_row_0_client_);
	framedrag_add(&self->frame_drag_, &self->file_bar_.component);
	framedrag_add(&self->frame_drag_, &self->song_bar_.component);
	framedrag_add(&self->frame_drag_, &self->play_bar_.component);
	framedrag_add(&self->frame_drag_, &self->undo_redo_bar_.component);
	framedrag_add(&self->frame_drag_, &self->machine_bar_.component);
	framedrag_add(&self->frame_drag_, &self->vu_.component);
	framedrag_add(&self->frame_drag_, &self->vu_.vu_meter_.component);
#endif
}

void mainframe_connect_visual_configuration(MainFrame* self)
{
	psy_Configuration* cfg;

	assert(self);

	cfg = psycleconfig_visual(workspace_cfg(&self->workspace_));
	psy_configuration_connect(cfg, "apptheme", self, mainframe_on_app_theme);
	psy_configuration_configure(cfg, "apptheme");
	psy_configuration_connect(cfg, "defaultfont", self, mainframe_on_set_default_font);
	psy_configuration_configure(cfg, "defaultfont");
	psy_configuration_connect(cfg, "zoom", self, mainframe_on_zoom);
	psy_configuration_configure(cfg, "zoom");	
}

void mainframe_connect_configuration(MainFrame* self)
{
	psy_Configuration* general;

	assert(self);

	general = psycleconfig_general(workspace_cfg(&self->workspace_));
#ifdef PSYCLE_USE_PLUGIN_EDITOR	
	psy_configuration_connect(general, "bench.showplugineditor",
		self, mainframe_on_plugin_editor);
	psy_configuration_configure(general, "bench.showplugineditor");
#endif		
	psy_configuration_connect(general, "bench.showparamrack",
		self, mainframe_on_param_rack);
	psy_configuration_configure(general, "bench.showparamrack");
	psy_configuration_connect(general, "bench.showgear",
		self, mainframe_on_gear);
	psy_configuration_configure(general, "bench.showgear");
	psy_configuration_connect(general, "bench.showmidi",
		self, mainframe_on_midi_monitor);
	psy_configuration_configure(general, "bench.showmidi");
	psy_configuration_connect(general, "bench.showcpu",
		self, mainframe_on_cpu_view);
	psy_configuration_configure(general, "bench.showcpu");
}

void mainframe_init_workspace(MainFrame* self)
{
	assert(self);
	
	workspace_init(&self->workspace_, mainframe_base(self));	
	self->patview_cfg_ = psycleconfig_patview(workspace_cfg(&self->workspace_));
	self->metronome_cfg_ = psycleconfig_metronome(workspace_cfg(&self->workspace_));
	paramviews_init(&self->param_views_, &self->component, &self->workspace_);
	workspace_set_param_views(&self->workspace_, &self->param_views_);	
}

void mainframe_init_layout(MainFrame* self)
{	
	assert(self);
	
	mainstatusbar_init(&self->status_, &self->pane_, &self->workspace_);
	psy_ui_component_set_logger(&self->component,
		psy_ui_LOG_STATUS, &self->status_.status_label_.logger);
	psy_ui_component_set_logger(&self->component,
		psy_ui_LOG_TOOLTIP, &self->status_.status_label_.logger);
	psy_ui_component_set_logger(&self->component,
		psy_ui_LOG_PROGRESS, &self->status_.status_label_.progress_bar_.logger);
	workspace_set_progress_logger(&self->workspace_,
		&self->status_.status_label_.progress_bar_.logger);
	psy_ui_component_set_align(mainstatusbar_base(&self->status_),
		psy_ui_ALIGN_BOTTOM);	
	psy_ui_component_init_align(&self->client_, &self->pane_, NULL,
		psy_ui_ALIGN_CLIENT);
	closebar_init(&self->close_, &self->pane_, NULL);
	psy_ui_component_enable_app_focus_out(closebar_base(&self->close_));
	psy_ui_component_enable_app_focus_out(psy_ui_button_base(&self->close_.hide));
	closebar_set_custom_mode(&self->close_);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->close_.hide), 0);
	psy_signal_connect(&self->close_.hide.signal_clicked,
		self, mainframe_on_exit);
	psy_ui_component_init_align(&self->top_, &self->pane_, NULL,
		psy_ui_ALIGN_TOP);	
	psy_ui_component_set_default_align(&self->top_, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());
	mainviews_init(&self->main_views_, &self->client_, &self->pane_,
		&self->workspace_);	
	psy_ui_component_set_align(mainviews_base(&self->main_views_),
		psy_ui_ALIGN_CLIENT);
}

void mainframe_init_messageview(MainFrame* self)
{
	assert(self);
	
	messageview_init(&self->message_view_, &self->pane_);
	psy_ui_component_set_preferred_height(messageview_base(&self->message_view_),
		psy_ui_value_zero());
	psy_ui_component_set_align(messageview_base(&self->message_view_),
		psy_ui_ALIGN_BOTTOM);	
	psy_ui_splitter_init(&self->splitter_message_view_, &self->pane_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->splitter_message_view_),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_logger(mainframe_base(self), psy_ui_LOG_TERMINAL,
		psy_ui_terminal_logger_base(&self->message_view_.terminal_));
	workspace_set_terminal_output(&self->workspace_,
		psy_ui_terminal_logger_base(&self->message_view_.terminal_));	
}

void mainframe_init_kbd_help(MainFrame* self)
{
	assert(self);
	
	kbdhelp_init(&self->kbd_help_, &self->pane_, &self->workspace_);
	psy_ui_component_set_align(kbdhelp_base(&self->kbd_help_),
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_hide(kbdhelp_base(&self->kbd_help_));
}

void mainframe_init_minmaximize(MainFrame* self)
{
	assert(self);
	
	mainviewbar_add_minmaximze(&self->main_views_.mainviewbar, &self->left_);
	mainviewbar_add_minmaximze(&self->main_views_.mainviewbar, &self->top_row_1_);
	mainviewbar_add_minmaximze(&self->main_views_.mainviewbar,
		machinebar_base(&self->machine_bar_));
	mainviewbar_add_minmaximze(&self->main_views_.mainviewbar,
		trackscopeview_base(&self->track_scopes_));
}

void mainframe_connect_statusbar(MainFrame* self)
{
	assert(self);
	
	psy_signal_connect(&self->status_.terminal_.signal_clicked,
		self, mainframe_on_toggle_messageview);	
	psy_signal_connect(&self->status_.togglekbd.signal_clicked,
		self, mainframe_on_toggle_kbd_help);
}

void mainframe_init_bars(MainFrame* self)
{
	assert(self);
	
	/* rows */
	psy_ui_component_init_align(&self->top_rows_, &self->top_, NULL,
		psy_ui_ALIGN_TOP);	
	psy_ui_component_set_default_align(&self->top_rows_, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());
	/* row0 */
	psy_ui_component_init(&self->top_row_0, &self->top_rows_, NULL);		
	psy_ui_component_init(&self->top_row_0_client_, &self->top_row_0, NULL);
	psy_ui_component_set_align(&self->top_row_0_client_, psy_ui_ALIGN_CLIENT);	
	psy_ui_component_set_default_align(&self->top_row_0_client_, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));	
	/* file_bar_ */
	filebar_init(&self->file_bar_, &self->top_row_0_client_, &self->workspace_);	
	/* undoredo */
	undoredobar_init(&self->undo_redo_bar_, &self->top_row_0_client_, &self->workspace_);	
	/* play_bar_ */
	playbar_init(&self->play_bar_, &self->top_row_0_client_, &self->workspace_);	
	/* metronome_bar_ */
	mainframe_init_metronome_bar(self, playbar_base(&self->play_bar_));
	mainframe_init_help_setting_bar(self, &self->top_row_0);	
	/* playposition */	
	playposbar_init(&self->play_pos_bar_, &self->top_row_0, workspace_player(
		&self->workspace_));
	psy_ui_component_set_align(&self->play_pos_bar_.component, psy_ui_ALIGN_RIGHT);
	/* row1 */
	psy_ui_component_init(&self->top_row_1_, &self->top_rows_, NULL);
	/* song_bar_ */	
	songbar_init(&self->song_bar_, &self->top_row_1_, &self->workspace_);
	psy_ui_component_set_align(&self->song_bar_.component, psy_ui_ALIGN_CLIENT);	
	/* vu_ */
	vubar_init(&self->vu_, &self->top_row_1_, psycleconfig_visual(
		workspace_cfg(&self->workspace_)));
	psy_ui_component_set_align(&self->vu_.component, psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_margin(&self->vu_.component,
		psy_ui_margin_make_em(0.0, 0.75, 0.0, 0.0));
	/* row2 */	
	machinebar_init(&self->machine_bar_, &self->top_rows_, &self->workspace_);
	/* scopebar */
	trackscopeview_init(&self->track_scopes_, &self->top_, &self->workspace_);	
	psy_configuration_connect(self->patview_cfg_, "trackscopes",
		self, mainframe_on_trackscope_view);
	psy_configuration_configure(self->patview_cfg_, "trackscopes");
}

void mainframe_init_metronome_bar(MainFrame* self, psy_ui_Component* parent)
{
	assert(self);

	metronomebar_init(&self->metronome_bar_, parent,
		&self->workspace_);
	psy_configuration_connect(self->metronome_cfg_, "show",
		self, mainframe_on_metronome_bar);
	psy_configuration_configure(self->metronome_cfg_, "show");
	psy_ui_component_set_margin(metronomebar_base(&self->metronome_bar_),
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));
	psy_ui_component_set_align(metronomebar_base(&self->metronome_bar_),
		psy_ui_ALIGN_TOP);
}

void mainframe_init_help_setting_bar(MainFrame* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init_align(&self->help_setting_bar,
		parent, NULL, psy_ui_ALIGN_RIGHT);
	psy_ui_component_init_align(&self->help_setting_bar_client,
		&self->help_setting_bar, NULL, psy_ui_ALIGN_TOP);
	/* help button */
	psy_ui_button_init_text_connect(&self->help_btn_,
		&self->help_setting_bar_client, "main.help",
		self, mainframe_on_help);
	psy_ui_component_set_id(psy_ui_button_base(&self->help_btn_),
		VIEW_ID_HELPVIEW);
	psy_ui_component_set_align(psy_ui_button_base(&self->help_btn_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_tooltip(psy_ui_button_base(&self->help_btn_),
		"Help");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->help_btn_),
		0);
	/* settings button */
	psy_ui_button_init_text_connect(&self->settings_btn,
		&self->help_setting_bar_client, "main.settings",
		self, mainframe_on_settings);
	psy_ui_button_set_svg(&self->settings_btn, 
		psy_ui_app_svg(psy_ui_app(), "img.settings"));		
	psy_ui_component_set_id(psy_ui_button_base(&self->settings_btn),
		VIEW_ID_SETTINGS);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->settings_btn),
		0);
	psy_ui_component_set_align(psy_ui_button_base(&self->settings_btn),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_tooltip(psy_ui_button_base(&self->settings_btn),
		"Settings");
}

void mainframe_init_gear(MainFrame* self)
{
	assert(self);
	
	gear_init(&self->gear_, &self->client_, &self->param_views_, &self->workspace_);	
	psy_ui_component_set_align(gear_base(&self->gear_), psy_ui_ALIGN_RIGHT);
	psy_ui_splitter_init(&self->gear_splitter, &self->client_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->gear_splitter),
		psy_ui_ALIGN_RIGHT);
	gear_set_splitter(&self->gear_, psy_ui_splitter_base(&self->gear_splitter));
}

void mainframe_init_param_rack(MainFrame* self)
{
	assert(self);
	
	paramrack_init(&self->param_gear_, &self->client_, &self->workspace_);	
	psy_ui_component_set_align(&self->param_gear_.component,
		psy_ui_ALIGN_BOTTOM);
	psy_ui_splitter_init(&self->param_gear_splitter_, &self->client_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->param_gear_splitter_),
		psy_ui_ALIGN_BOTTOM);	
}

void mainframe_init_cpu_view(MainFrame* self)
{
	assert(self);
	
	cpuview_init(&self->cpu_, &self->client_, &self->workspace_);
	psy_ui_component_hide(cpuview_base(&self->cpu_));
	psy_ui_component_set_align(cpuview_base(&self->cpu_), psy_ui_ALIGN_RIGHT);
	psy_ui_splitter_init(&self->cpu_splitter_, &self->client_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->cpu_splitter_),
		psy_ui_ALIGN_RIGHT);
	psy_ui_splitter_set_toggle(&self->cpu_splitter_, cpuview_base(&self->cpu_));
	psy_ui_splitter_set_button(&self->cpu_splitter_, &self->status_.cpu);
	psy_ui_component_hide(psy_ui_splitter_base(&self->cpu_splitter_));
}

void mainframe_init_midi_monitor(MainFrame* self)
{
	assert(self);
	
	midimonitor_init(&self->midi_, &self->client_, &self->workspace_);
	psy_ui_component_hide(midimonitor_base(&self->midi_));
	psy_ui_component_set_align(midimonitor_base(&self->midi_),
		psy_ui_ALIGN_RIGHT);
	psy_ui_splitter_init(&self->midi_splitter_, &self->client_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->midi_splitter_),
		psy_ui_ALIGN_RIGHT);	
	psy_ui_component_hide(psy_ui_splitter_base(&self->midi_splitter_));	
}

#ifdef PSYCLE_USE_PLUGIN_EDITOR
void mainframe_on_plugin_editor(MainFrame* self, psy_Property* sender)
{	
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		psy_ui_component_show(plugineditor_base(&self->plugin_editor_));
		psy_ui_component_show(psy_ui_splitter_base(
			&self->plugin_editor_splitter_));
	} else {
		psy_ui_component_hide(plugineditor_base(&self->plugin_editor_));
		psy_ui_component_hide(psy_ui_splitter_base(
			&self->plugin_editor_splitter_));
	}
	mainframe_align(self);
}
#endif

void mainframe_on_param_rack(MainFrame* self, psy_Property* sender)
{	
	assert(self);
	
	mainframe_show_hide_sidebar(self, paramrack_base(&self->param_gear_),
		&self->param_gear_splitter_, psy_property_item_bool(sender));	
}

void mainframe_on_gear(MainFrame* self, psy_Property* sender)
{	
	assert(self);
	
	mainframe_show_hide_sidebar(self, gear_base(&self->gear_),
		&self->gear_splitter, psy_property_item_bool(sender));	
}

void mainframe_on_midi_monitor(MainFrame* self, psy_Property* sender)
{	
	assert(self);
	
	mainframe_show_hide_sidebar(self, midimonitor_base(&self->midi_),
		&self->midi_splitter_, psy_property_item_bool(sender));	
}

void mainframe_on_cpu_view(MainFrame* self, psy_Property* sender)
{	
	assert(self);
	
	mainframe_show_hide_sidebar(self, cpuview_base(&self->cpu_),
		&self->cpu_splitter_, psy_property_item_bool(sender));	
}

void mainframe_show_hide_sidebar(MainFrame* self, psy_ui_Component* view,
	psy_ui_Splitter* splitter, bool show)
{
	assert(self);

	if (show) {
		psy_ui_component_show(view);
		if (splitter) {
			psy_ui_component_show(psy_ui_splitter_base(splitter));									
		}
		psy_ui_component_set_focus(view);
	} else {
		psy_ui_component_hide(view);
		if (splitter) {
			psy_ui_component_hide(psy_ui_splitter_base(splitter));
		}
	}	
	if (view->align == psy_ui_ALIGN_BOTTOM) {
		mainframe_align(self);
	} else if (psy_ui_component_visible(&self->component)) {
		psy_ui_component_align_invalidate(&self->client_);		
	}	
}

void mainframe_init_step_sequencer_view(MainFrame* self)
{
	assert(self);
#ifdef PSYCLE_USE_STEPSEQUENCER
	
	stepsequencerview_init(&self->step_sequencer_, &self->pane_,
		&self->workspace_);
	psy_ui_component_set_align(stepsequencerview_base(&self->step_sequencer_),
		psy_ui_ALIGN_BOTTOM);	
#endif		
}

void mainframe_init_keyboard_view(MainFrame* self)
{
	assert(self);
	
	keyboardview_init(&self->piano_kbd_, &self->pane_, &self->workspace_);
	psy_ui_component_set_align(keyboardview_base(&self->piano_kbd_),
		psy_ui_ALIGN_BOTTOM);	
				
}

void mainframe_init_seq_editor(MainFrame* self)
{
	assert(self);
#ifdef PSYCLE_USE_SEQEDITOR	
	seqeditor_init(&self->seq_edit_, &self->pane_, &self->workspace_);
	psy_ui_component_set_align(seqeditor_base(&self->seq_edit_),
		psy_ui_ALIGN_BOTTOM);	
	psy_ui_component_set_maximum_size(seqeditor_base(&self->seq_edit_),
		psy_ui_size_make(psy_ui_value_zero(), psy_ui_value_make_ph(0.7)));
	psy_ui_splitter_init(&self->seq_edit_splitter_, &self->pane_);	
	psy_ui_component_set_align(psy_ui_splitter_base(&self->seq_edit_splitter_),
		psy_ui_ALIGN_BOTTOM);	
	/* connect float */
	psy_signal_connect(&self->seq_edit_.toolbar.view_float.signal_clicked,
		self, mainframe_seqeditor_on_float);
#endif		
}

void mainframe_init_sequence_view(MainFrame* self)
{
#ifdef PSYCLE_USE_SEQVIEW	
	psy_ui_Size size;
	
	assert(self);
	
	psy_ui_component_init_align(&self->left_, &self->pane_, NULL,
		psy_ui_ALIGN_LEFT);
	psy_ui_splitter_init(&self->splitter_, &self->pane_);
	seqview_init(&self->sequence_view_, &self->left_, &self->workspace_);
	psy_ui_component_set_align(seqview_base(&self->sequence_view_),
		psy_ui_ALIGN_CLIENT);
	size = psy_ui_component_preferred_size(seqview_base(&self->sequence_view_),
		NULL);
	psy_ui_component_set_preferred_width(&self->left_, size.width);
#endif	
}

void mainframe_init_sequencer_bar(MainFrame* self)
{
	assert(self);
	
	sequencerbar_init(&self->sequencer_bar_, &self->left_,
		psycleconfig_misc(workspace_cfg(&self->workspace_)),		
		psycleconfig_general(workspace_cfg(&self->workspace_)));
	psy_ui_component_set_align(sequencerbar_base(&self->sequencer_bar_),
		psy_ui_ALIGN_BOTTOM);
}

#ifdef PSYCLE_USE_PLUGIN_EDITOR
void mainframe_init_plugin_editor(MainFrame* self)
{
	assert(self);
	
	plugineditor_init(&self->plugin_editor_, &self->pane_, &self->workspace_);	
	psy_ui_component_set_align(plugineditor_base(&self->plugin_editor_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_hide(plugineditor_base(&self->plugin_editor_));	
	psy_ui_splitter_init(&self->plugin_editor_splitter_, &self->pane_);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->plugin_editor_splitter_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_hide(psy_ui_splitter_base(&self->plugin_editor_splitter_));
	plugineditor_set_splitter(&self->plugin_editor_, psy_ui_splitter_base(
		&self->plugin_editor_splitter_));
}
#endif

void mainframe_connect_workspace(MainFrame* self)
{
	assert(self);
		
	mainframe_connect_input_handler(self);
	psy_signal_connect(&self->workspace_.player_.signal_song_changed, self,
		mainframe_on_song_changed);	
}

void mainframe_connect_input_handler(MainFrame* self)
{
	assert(self);
	
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_IMM, psy_EVENTDRIVER_CMD, "general",
		psy_INDEX_INVALID, self, NULL, (fp_inputhandler_input)mainframe_on_input);
	workspace_connect_input_handler(&self->workspace_);
}

void mainframe_init_interpreter(MainFrame* self)
{
	assert(self);
	
	interpreter_init(&self->interpreter_, &self->workspace_);
	interpreter_start(&self->interpreter_);
}

bool mainframe_on_input(MainFrame* self, InputHandler* sender)
{
	psy_EventDriverCmd cmd;

	assert(self);
	
	cmd = inputhandler_cmd(sender);
	switch (cmd.id) {	
	case CMD_IMM_FILE_MENU:
		psy_ui_component_set_focus(&self->file_bar_.song_new_.component);
		return TRUE;	
	case CMD_IMM_HELPSHORTCUT:
		mainframe_on_toggle_kbd_help(self, mainframe_base(self));
		return 1;
	case CMD_IMM_INFOMACHINE:
		if (self->workspace_.song) {
			paramviews_show(&self->param_views_,
				psy_audio_machines_selected(&self->workspace_.song->machines_));
		}
		break;
	case CMD_IMM_INFOPATTERN:
#ifdef PSYCLE_USE_PATTERN_VIEW
		if (workspace_current_view(&self->workspace_).id != VIEW_ID_PATTERNS) {
			workspace_select_view(&self->workspace_,
				viewindex_make_all(VIEW_ID_PATTERNS, 0, 0, psy_INDEX_INVALID));
		}
		if (!psy_ui_component_visible(&self->main_views_.patternview.properties.component)) {
			psy_ui_Tab* tab;
			psy_ui_component_toggle_visibility(&self->main_views_.patternview.properties.component);

			tab = psy_ui_tabbar_tab(&self->main_views_.patternview.tabbar.tabbar, 5);
			if (tab) {
				tab->checkstate = TABCHECKSTATE_ON;
				psy_ui_component_invalidate(psy_ui_tabbar_base(
					&self->main_views_.patternview.tabbar.tabbar));
			}
		}
		psy_ui_component_set_focus(&self->main_views_.patternview.properties.component);
#endif		
		return 1;
	case CMD_IMM_INFOTRACKNAMES:
#ifdef PSYCLE_USE_PATTERN_VIEW
		if (workspace_current_view(&self->workspace_).id != VIEW_ID_PATTERNS) {
			workspace_select_view(&self->workspace_,
				viewindex_make_all(VIEW_ID_PATTERNS, 0, 0, psy_INDEX_INVALID));
		}				
		psy_ui_component_toggle_visibility(&self->main_views_.patternview.tracknames.component);		
		psy_ui_component_set_focus(&self->main_views_.patternview.tracknames.component);
#endif
		return 1;
	case CMD_IMM_MAXPATTERN:
		mainviewbar_toggle_minmaximze(&self->main_views_.mainviewbar);
		return 1;
	case CMD_IMM_TERMINAL:
		mainframe_on_toggle_messageview(self, mainframe_base(self));
		return 1;
	case CMD_IMM_STYLES:
		workspace_select_view(&self->workspace_,	viewindex_make_all(
			VIEW_ID_STYLEVIEW, psy_INDEX_INVALID, psy_INDEX_INVALID,
			psy_INDEX_INVALID));
		return 1;
	case CMD_IMM_FULLSCREEN:
		psy_ui_component_togglefullscreen(mainframe_base(self));		
		return 1;		
	case CMD_IMM_QUIT:
		psy_ui_app_close(psy_ui_app());
		return 1;
	case CMD_IMM_EDTPLAY:
		psy_ui_component_set_focus(&self->play_bar_.buttons);
		return 1;
	case CMD_EDT_EDITQUANTIZEDEC:
		// workspace_edit_quantize_change(&self->workspace_, -1);
		// patterncursorstepbox_update(&self->patternviewbar.cursorstep);
		/*const int total = 17;
		const int nextsel = (total + keyboardmiscconfig_cursor_step(
			&self->config.misc) + diff) % total;

		assert(self);

		keyboardmiscconfig_setcursorstep(&self->config.misc, nextsel);*/
		return 1;
	case CMD_EDT_EDITQUANTIZEINC:
		// workspace_edit_quantize_change(&self->workspace_, 1);
		// patterncursorstepbox_update(&self->patternviewbar.cursorstep);
		return 1;	
	}
	workspace_on_input(&self->workspace_, cmd.id);
	return 0;
}

void mainframe_on_song_changed(MainFrame* self, psy_audio_Player* sender)
{
	assert(self);
	
	if (workspace_song_has_file(&self->workspace_) &&
			psy_configuration_value_bool(
				psycleconfig_general(workspace_cfg(&self->workspace_)),
				"showsonginfoonload", TRUE)) {	
		workspace_select_view(&self->workspace_, viewindex_make_section(
			VIEW_ID_SONGPROPERTIES, SECTION_ID_SONGPROPERTIES_VIEW));
	}
	mainframe_update_song_title(self);
	mainframe_connect_song(self);
	vubar_reset(&self->vu_);
	mainstatusbar_update_song(&self->status_);	
	mainframe_align(self);
}

void mainframe_on_songtracks_changed(MainFrame* self,
	psy_audio_Patterns* sender)
{
	assert(self);

	mainframe_align(self);
}

void mainframe_connect_song(MainFrame* self)
{
	assert(self);
	
	if (workspace_song(&self->workspace_)) {		
		psy_signal_connect(
			&workspace_song(&self->workspace_)->patterns_.signal_numsongtrackschanged,
			self, mainframe_on_songtracks_changed);
		vubar_set_machine(&self->vu_, psy_audio_machines_master(
			psy_audio_song_machines(workspace_song(&self->workspace_))));
	} else {
		vubar_set_machine(&self->vu_, NULL);
	}
}

void mainframe_update_song_title(MainFrame* self)
{
	char title[512];
	
	assert(self);

	workspace_app_title(&self->workspace_, title, 512);
	psy_ui_component_set_title(mainframe_base(self), title);
	mainstatusbar_set_default_status_text(&self->status_,
		workspace_song_title(&self->workspace_));
	self->workspace_.update_title = FALSE;
}

void mainframe_on_timer(MainFrame* self, uintptr_t timerid)
{	
	if (self->starting_ && !psy_ui_component_draw_visible(&self->component)) {
		return;
	}
	self->starting_ = FALSE;	
	vubar_idle(&self->vu_);
	workspace_idle(&self->workspace_);
	mainstatusbar_idle(&self->status_);
	trackscopeview_idle(&self->track_scopes_);
#ifdef PSYCLE_USE_SEQVIEW	
	seqview_idle(&self->sequence_view_);
#endif	
#ifdef PSYCLE_USE_PLUGIN_EDITOR
	plugineditorbar_idle(&self->plugin_editor_.status);
#endif	
 	playposbar_idle(&self->play_pos_bar_);	
	machineview_idle(&self->main_views_.machineview);	
	if (self->workspace_.update_title ||
		self->title_modified_ != workspace_song_modified(&self->workspace_)) {
		self->title_modified_ = workspace_song_modified(&self->workspace_);
		mainframe_update_song_title(self);
	}
	if (self->sequencer_bar_.doresize) {
		psy_ui_component_align(&self->left_);
		psy_ui_component_invalidate(&self->left_);
		self->sequencer_bar_.doresize = FALSE;
	}
}

void mainframe_on_focus(MainFrame* self)
{
	psy_ui_Component* currview;
	
	assert(self);

	currview = psy_ui_notebook_active_page(&self->main_views_.notebook);
	if (currview) {
		psy_ui_component_set_focus(currview);
	}
}

void mainframe_on_exit(MainFrame* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_ui_app_close(psy_ui_app());
}

bool mainframe_on_close(MainFrame* self)
{	
	assert(self);
		
	if (workspace_save_reminder(&self->workspace_) &&
			workspace_song_modified(&self->workspace_)) {
		workspace_confirm_close(&self->workspace_);
		return FALSE;
	}
	psy_ui_component_stop_timer(mainframe_base(self), 0);
	return TRUE;
}

void mainframe_on_seq_editor(MainFrame* self, psy_Property* sender)
{
	assert(self);
	
#ifdef PSYCLE_USE_SEQEDITOR	
	mainframe_show_hide_sidebar(self, seqeditor_base(&self->seq_edit_),
		&self->seq_edit_splitter_, psy_property_item_bool(sender));	
#endif	
}

void mainframe_on_piano_kbd(MainFrame* self, psy_Property* sender)
{
	assert(self);
	
	mainframe_show_hide_sidebar(self, keyboardview_base(&self->piano_kbd_),
		NULL, psy_property_item_bool(sender));
}

void mainframe_on_step_sequencer(MainFrame* self, psy_Property* sender)
{
#ifdef PSYCLE_USE_STEPSEQUENCER
	mainframe_show_hide_sidebar(self, stepsequencerview_base(
		&self->step_sequencer_), NULL, psy_property_item_bool(sender));	
#endif	
}

void mainframe_connect_seq_editor_buttons(MainFrame* self)
{	
	psy_Configuration* general;

	assert(self);

	general = psycleconfig_general(workspace_cfg(&self->workspace_));	
	psy_configuration_connect(general, "bench.showsequenceedit",
		self, mainframe_on_seq_editor);	
	psy_configuration_configure(general, "bench.showsequenceedit");	
	psy_ui_button_exchange(&self->sequencer_bar_.toggle_seq_edit_,
		psy_configuration_at(general, "bench.showsequenceedit"));
}

void mainframe_connect_step_sequencer_buttons(MainFrame* self)
{
	psy_Configuration* general;

	assert(self);

	general = psycleconfig_general(workspace_cfg(&self->workspace_));	
	psy_configuration_connect(general, "bench.showstepsequencer",
		self, mainframe_on_step_sequencer);
	psy_configuration_configure(general, "bench.showstepsequencer");
	psy_ui_button_exchange(&self->sequencer_bar_.toggle_step_seq_,
		psy_configuration_at(general, "bench.showstepsequencer"));
}

void mainframe_connect_piano_kbd_buttons(MainFrame* self)
{
	assert(self);
	
	psy_configuration_connect(
		psycleconfig_general(workspace_cfg(&self->workspace_)),
		"bench.showpianokbd", self, mainframe_on_piano_kbd);
	psy_configuration_configure(
		psycleconfig_general(workspace_cfg(&self->workspace_)),
		"bench.showpianokbd");
	psy_ui_button_exchange(&self->sequencer_bar_.toggle_kbd_,
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(&self->workspace_)),
			"bench.showpianokbd"));	
}

void mainframe_on_toggle_messageview(MainFrame* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (!psy_ui_is_value_zero(psy_ui_component_scroll_size(messageview_base(
			&self->message_view_)).height)) {
		psy_ui_component_set_preferred_size(messageview_base(
			&self->message_view_), psy_ui_size_zero());
		self->message_view_.terminal_.last_status_ = PSY_OK;
	} else {
		psy_ui_component_set_preferred_size(messageview_base(
			&self->message_view_), psy_ui_size_make(psy_ui_value_zero(),
				psy_ui_value_make_eh(10.0)));
	}
	mainframe_align(self);
}

void mainframe_on_toggle_kbd_help(MainFrame* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_ui_component_toggle_visibility(kbdhelp_base(&self->kbd_help_));
}

void mainframe_show_nav_bar(MainFrame* self, psy_ui_Component* sender)
{
	assert(self);

	psy_ui_component_toggle_visibility(kbdhelp_base(&self->kbd_help_));
}

void mainframe_on_key_down(MainFrame* self, psy_ui_KeyboardEvent* ev)
{	
	assert(self);
		
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		return;
	}
	mainframe_check_play_start_with_right_ctrl(self, ev);
	mainframe_delegate_keyboard(self, psy_EVENTDRIVER_PRESS, ev);	
}

void mainframe_check_play_start_with_right_ctrl(MainFrame* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	if (psy_configuration_value_bool(psycleconfig_misc(workspace_cfg(&self->workspace_)),
			"playstartwithrctrl", TRUE)) {
		if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_CONTROL) {
			/* todo: this win32 detection only */
			int extended = (psy_ui_keyboardevent_keydata(ev) & 0x01000000) != 0;
			if (extended) {
				/* right ctrl */
				psy_audio_player_start_curr_seq_pos(workspace_player(
					&self->workspace_));				
			}
		} else if (psy_audio_player_playing(workspace_player(&self->workspace_)) &&
				psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_SPACE) {
			psy_audio_player_stop(workspace_player(&self->workspace_));			
		}
	}
}

void mainframe_on_key_up(MainFrame* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		return;
	}	
	mainframe_delegate_keyboard(self, psy_EVENTDRIVER_RELEASE, ev);
}

/* delegate keyboard events to the keyboard driver */
void mainframe_delegate_keyboard(MainFrame* self, intptr_t message,
	psy_ui_KeyboardEvent* ev)
{
	assert(self);
			
	if (!workspace_kbd_driver(&self->workspace_)) {
		return;
	}
	psy_eventdriver_write(workspace_kbd_driver(&self->workspace_),
		psy_eventdriverinput_make(message, psy_ui_keyboardevent_encode(ev,
			message == psy_EVENTDRIVER_RELEASE),
			psy_ui_keyboardevent_repeat(ev)));
}

void mainframe_seqeditor_on_float(MainFrame* self, psy_ui_Button* sender)
{	
	assert(self);

#ifdef PSYCLE_USE_SEQEDITOR	
	seqeditor_base(&self->seq_edit_)->splitter_ = psy_ui_splitter_base(&self->seq_edit_splitter_);
	seqeditor_base(&self->seq_edit_)->frame_icons_ = &self->seq_edit_.toolbar.alignbar;
	seqeditor_base(&self->seq_edit_)->sizer_ = NULL;
	seqeditor_base(&self->seq_edit_)->kbd_ = workspace_kbd_driver(&self->workspace_);
	seqeditor_base(&self->seq_edit_)->frame_size_ = psy_ui_size_make_em(140.0, 40.0);
	psy_ui_component_float(seqeditor_base(&self->seq_edit_));	
#endif		
}

void mainframe_on_metronome_bar(MainFrame* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		psy_ui_component_show(metronomebar_base(&self->metronome_bar_));
	} else {
		psy_ui_component_hide(metronomebar_base(&self->metronome_bar_));
	}	
	mainframe_align(self);	
}

void mainframe_on_trackscope_view(MainFrame* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		psy_ui_component_show(trackscopeview_base(&self->track_scopes_));
		trackscopes_start(&self->track_scopes_.scopes);
	} else {
		psy_ui_component_hide(trackscopeview_base(&self->track_scopes_));
		trackscopes_stop(&self->track_scopes_.scopes);
	}
	mainframe_align(self);
}

void mainframe_on_help(MainFrame* self, psy_ui_Button* sender)
{
	assert(self);
	
	inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "general",
		psy_eventdrivercmd_make_cmd(CMD_IMM_HELP));	
}

void mainframe_on_settings(MainFrame* self, psy_ui_Button* sender)
{
	assert(self);
	
	inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "general",
		psy_eventdrivercmd_make_cmd(CMD_IMM_SETTINGS));	
}

void mainframe_align(MainFrame* self)
{
	assert(self);

	if (psy_ui_component_draw_visible(&self->pane_)) {
		psy_ui_component_align_invalidate(&self->pane_);		
	}
}

void mainframe_on_zoom(MainFrame* self, psy_Property* sender)
{
	assert(self);
	
	psy_ui_app_set_zoom_rate(psy_ui_app(), psy_property_item_double(
                sender));
}

bool mainframe_show_maximized_at_start(const MainFrame* self)
{	
	assert(self);
        
	return psy_configuration_value_bool(
		psycleconfig_general_const(workspace_cfg_const(
                    &self->workspace_)),
		"bench.showmaximizedatstart", TRUE);		
}

void mainframe_on_app_theme(MainFrame* self, psy_Property* sender)
{
	psy_ui_ThemeMode theme;

	assert(self);

	theme = (psy_ui_ThemeMode)(psy_property_item_int(sender));
	/* reset styles */
	psy_ui_defaults_init_theme(psy_ui_app_defaults(), theme, TRUE);
	init_host_styles(&psy_ui_app_defaults()->styles, theme);
	psy_ui_app_notify_theme_change(psy_ui_app());	
	mainframe_align(self);
}

void mainframe_on_set_default_font(MainFrame* self, psy_Property* sender)
{
	assert(self);

	if (psy_property_type(sender) == PSY_PROPERTY_TYPE_FONT) {
		psy_ui_Font font;
		
		psy_ui_font_init_string(&font, psy_property_item_str(sender));
		psy_ui_app_set_default_font(psy_ui_app(), &font);
		psy_ui_font_dispose(&font);
	}
}
