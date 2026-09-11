/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqeditor.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "styles.h"


/* prototypes */
static void seqeditor_on_destroyed(SeqEditor*);
static void seqeditor_on_song_changed(SeqEditor*, psy_audio_Player* sender);
static void seqeditor_update_song(SeqEditor*);
static void seqeditor_build(SeqEditor*);
static void seqeditor_on_tracks_scroll(SeqEditor*, psy_ui_Component* sender,
	double dx, double dy);
static void seqeditor_on_hzoom(SeqEditor*, ZoomBox* sender);
static void seqeditor_update_scroll_step(SeqEditor*);
static void seqeditor_on_clear(SeqEditor*, psy_audio_Sequence*);
static void seqeditor_on_track_insert(SeqEditor*, psy_audio_Sequence*,
	uintptr_t trackidx);
static void seqeditor_on_track_remove(SeqEditor*, psy_audio_Sequence*,
	uintptr_t trackidx);
static void seqeditor_on_track_swap(SeqEditor*,
	psy_audio_Sequence* sender, uintptr_t first, uintptr_t second);
static void seqeditor_on_mouse_move(SeqEditor*, psy_ui_MouseEvent*);
static void seqeditor_on_mouse_up(SeqEditor*, psy_ui_MouseEvent*);
static void seqeditor_on_toggle_expand(SeqEditor*, psy_ui_Button* sender);
static void seqeditor_on_track_resize(SeqEditor*, psy_ui_Component* sender,
	uintptr_t trackid, double* height);
static void seqeditor_on_toggle_timesig(SeqEditor*, psy_ui_Button* sender);
static void seqeditor_on_toggle_loop(SeqEditor*, psy_ui_Button* sender);
static void seqeditor_on_cursor_changed(SeqEditor*, psy_audio_Sequence*
	sender);
static void seqeditor_on_show_pattern_names(SeqEditor*, psy_Property* sender);
static void seqeditor_update_cursor_line_position(SeqEditor*);
static void seqeditor_on_timer(SeqEditor*, uintptr_t timerid);
static void seqeditor_update_seq_edit_line_position(SeqEditor*);
static void seqeditor_on_tracks_align(SeqEditor*, psy_ui_Component* sender);
static void seqeditor_on_scroll(SeqEditor*, psy_ui_Component*,
	double dx, double dy);
static void seqeditor_update_line_positions(SeqEditor*);
static void seqeditor_on_play_line_changed(SeqEditor*,
	psy_audio_Sequencer* sender);
static void seqeditor_on_play_status_changed(SeqEditor*,
	psy_audio_Sequencer* sender);
static void seqeditor_on_follow(SeqEditor*);

/* vtable */
static psy_ui_ComponentVtable seqeditor_vtable;
static bool seqeditor_vtable_initialized = FALSE;

static void seqeditor_vtable_init(SeqEditor* self)
{
	assert(self);

	if (!seqeditor_vtable_initialized) {
		seqeditor_vtable = *(self->component.vtable);
		seqeditor_vtable.on_destroyed =
			(psy_ui_fp_component)
			seqeditor_on_destroyed;
		seqeditor_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			seqeditor_on_mouse_move;
		seqeditor_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			seqeditor_on_mouse_up;
		seqeditor_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			seqeditor_on_timer;
		seqeditor_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(seqeditor_base(self), &seqeditor_vtable);
}

/* implementation */
void seqeditor_init(SeqEditor* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	seqeditor_vtable_init(self);
	self->workspace = workspace;
	self->expanded = FALSE;
	psy_ui_component_set_title(&self->component, "seqedit.editor");
	psy_ui_component_set_style_type(&self->component, STYLE_SIDE_VIEW);
	psy_ui_component_set_preferred_height(&self->component,
		psy_ui_value_make_ph(0.20));
	closebar_init(&self->close_bar, seqeditor_base(self),
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showsequenceedit"));
	sequencecmds_init(&self->cmds, workspace);
	seqeditstate_init(&self->state, &self->cmds, &self->component);
	/* toolbar */
	seqedittoolbar_init(&self->toolbar, &self->component, &self->state);
	psy_ui_component_set_align(&self->toolbar.component, psy_ui_ALIGN_TOP);
	/* tracks */
	psy_ui_component_init(&self->tracks, &self->component, NULL);
	psy_ui_component_set_align(&self->tracks, psy_ui_ALIGN_CLIENT);
	/* track_header */
	psy_ui_component_init(&self->tracks_header, &self->tracks, NULL);
	psy_ui_component_set_align(&self->tracks_header, psy_ui_ALIGN_TOP);
	/* header descbar */
	seqeditorheaderdescbar_init(&self->headerdescbar, &self->tracks_header,
		&self->state);
	psy_ui_component_set_align(&self->headerdescbar.component,
		psy_ui_ALIGN_LEFT);
	psy_signal_connect(&self->headerdescbar.hzoom.signal_changed, self,
		seqeditor_on_hzoom);
	/* items header */
	seqeditheader_init(&self->header, &self->tracks_header, &self->state);
	psy_ui_component_set_align(&self->header.component, psy_ui_ALIGN_CLIENT);
	/* tracks pane */
	psy_ui_component_init(&self->tracks_pane, &self->tracks, NULL);
	psy_ui_component_set_style_type(&self->tracks_pane, STYLE_SEQEDT_TRACKS);
	psy_ui_component_set_align(&self->tracks_pane, psy_ui_ALIGN_CLIENT);
	/* track description */
	psy_ui_component_init(&self->trackdescpane, &self->tracks_pane, NULL);
	psy_ui_component_set_align(&self->trackdescpane, psy_ui_ALIGN_LEFT);
	psy_ui_component_set_preferred_width(&self->trackdescpane,
		psy_ui_value_make_ew(40.0));
	seqedittrackdesc_init(&self->trackdesc, &self->trackdescpane,
		&self->state, workspace);
	psy_signal_connect(&self->trackdesc.signal_resize, self,
		seqeditor_on_track_resize);
	psy_ui_component_set_align(&self->trackdesc.component,
		psy_ui_ALIGN_HCLIENT);
	/* track items */
	seqeditortracks_init(&self->track_items, &self->tracks_pane, &self->state,
		workspace);
	psy_ui_scroller_init(&self->scroller, &self->tracks_pane, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->track_items.component);
	psy_ui_component_set_align(&self->track_items.component,
		psy_ui_ALIGN_FIXED);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->track_items.component.signal_scrolled, self,
		seqeditor_on_scroll);
	psy_signal_connect(&self->scroller.pane.signal_align, self,
		seqeditor_on_tracks_align);
	/* cursor line */
	seqeditorline_init(&self->cursorline, &self->scroller.pane, &self->state);
	psy_ui_component_set_align(seqeditorline_base(&self->cursorline),
		psy_ui_ALIGN_NONE);
	psy_ui_component_set_margin(seqeditorline_base(&self->cursorline),
		psy_ui_margin_zero());
	psy_ui_component_set_style_type(seqeditorline_base(&self->cursorline),
		STYLE_SEQEDT_LINE_CURSOR);
	/* play line */
	seqeditorplayline_init(&self->playline, &self->scroller.pane, &self->state);
	psy_ui_component_set_align(seqeditorplayline_base(&self->playline),
		psy_ui_ALIGN_NONE);
	psy_ui_component_set_margin(seqeditorplayline_base(&self->playline),
		psy_ui_margin_zero());
	psy_ui_component_hide(seqeditorplayline_base(&self->playline));
	/* seqeditpos line */
	seqeditorline_init(&self->seqeditposline, &self->scroller.pane,
		&self->state);
	psy_ui_component_set_align(seqeditorline_base(&self->seqeditposline),
		psy_ui_ALIGN_NONE);
	psy_ui_component_set_margin(seqeditorline_base(&self->seqeditposline),
		psy_ui_margin_zero());
	psy_ui_component_set_style_type(seqeditorline_base(&self->seqeditposline),
		STYLE_SEQEDT_LINE_SEQPOS);
	/* properties */
	seqeditproperties_init(&self->properties, &self->component, &self->state);
	psy_ui_component_set_align(&self->properties.component, psy_ui_ALIGN_RIGHT);
	psy_ui_splitter_init(&self->splitbar_properties, &self->component);
	psy_ui_component_set_align(psy_ui_splitter_base(&self->splitbar_properties),
		psy_ui_ALIGN_RIGHT);
	/* update */
	seqeditor_update_song(self);
	/* connect signals */
	psy_signal_connect(&self->toolbar.expand.signal_clicked, self,
		seqeditor_on_toggle_expand);
	psy_signal_connect(&self->state.workspace->player_.signal_song_changed, self,
		seqeditor_on_song_changed);
	psy_signal_connect(&self->track_items.component.signal_scrolled, self,
		seqeditor_on_tracks_scroll);
	psy_configuration_connect(psycleconfig_general(workspace_cfg(self->workspace)),
		"showpatternnames", self, seqeditor_on_show_pattern_names);
	psy_configuration_configure(psycleconfig_general(workspace_cfg(self->workspace)),
		"showpatternnames");
	psy_signal_connect(&self->toolbar.timesig.signal_clicked, self,
		seqeditor_on_toggle_timesig);
	psy_signal_connect(&self->toolbar.loop.signal_clicked, self,
		seqeditor_on_toggle_loop);
	psy_signal_connect(
		&workspace->player_.sequencer.signal_play_status_changed, self,
		seqeditor_on_play_status_changed);
	psy_signal_connect(
		&workspace->player_.sequencer.signal_play_line_changed, self,
		seqeditor_on_play_line_changed);
	seqeditor_update_scroll_step(self);
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void seqeditor_on_destroyed(SeqEditor* self)
{
	assert(self);

	seqeditstate_dispose(&self->state);
}

void seqeditor_on_song_changed(SeqEditor* self, psy_audio_Player* sender)
{
	assert(self);

	seqeditor_update_song(self);
}

void seqeditor_update_song(SeqEditor* self)
{
	psy_audio_Sequence* sequence;

	assert(self);

	if (!self->state.workspace->song) {
		return;
	}
	sequence = &self->state.workspace->song->sequence_;
	psy_ui_component_move(&self->trackdesc.component, psy_ui_point_zero());
	psy_ui_component_move(&self->track_items.component,
		psy_ui_point_zero());
	seqeditor_build(self);
	seqeditortoolbar_update_track_name(&self->toolbar);
	psy_ui_component_invalidate(&self->trackdesc.component);
	psy_ui_component_invalidate(&self->header.component);
	psy_ui_component_align_invalidate(&self->track_items.component);
	psy_signal_connect(&sequence->signal_clear, self,
		seqeditor_on_clear);
	psy_signal_connect(&sequence->signal_track_insert, self,
		seqeditor_on_track_insert);
	psy_signal_connect(&sequence->signal_track_swap, self,
		seqeditor_on_track_swap);
	psy_signal_connect(&sequence->signal_track_remove, self,
		seqeditor_on_track_remove);
	psy_signal_connect(&sequence->signal_cursor_changed, self,
		seqeditor_on_cursor_changed);
}

void seqeditor_update_scroll_step(SeqEditor* self)
{
	assert(self);

	psy_ui_component_set_scroll_step_height(&self->track_items.component,
		seqeditstate_line_height(&self->state));
	psy_ui_component_set_scroll_step_height(&self->trackdesc.component,
		seqeditstate_line_height(&self->state));
}

void seqeditor_on_clear(SeqEditor* self, psy_audio_Sequence* sender)
{
	assert(self);

	psy_ui_component_move(&self->trackdesc.component, psy_ui_point_zero());
	psy_ui_component_move(&self->track_items.component, psy_ui_point_zero());
	seqeditor_build(self);
	psy_ui_component_invalidate(&self->trackdesc.component);
	psy_ui_component_align(&self->track_items.component);
	psy_ui_component_invalidate(&self->track_items.component);
}

void seqeditor_on_track_insert(SeqEditor* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	assert(self);

	seqeditor_build(self);
}

void seqeditor_on_track_swap(SeqEditor* self, psy_audio_Sequence* sender,
	uintptr_t first, uintptr_t second)
{
	assert(self);

	seqeditor_build(self);
	psy_ui_component_invalidate(&self->trackdesc.component);
	psy_ui_component_align(&self->track_items.component);
	psy_ui_component_invalidate(&self->track_items.component);
}

void seqeditor_on_track_remove(SeqEditor* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	assert(self);

	seqeditor_build(self);
}

void seqeditor_build(SeqEditor* self)
{
	assert(self);

	seqeditortracks_build(&self->track_items);
	seqedittrackdesc_build(&self->trackdesc);
	psy_ui_component_align(&self->scroller.pane);
	psy_ui_component_align(&self->trackdescpane);
	psy_ui_component_invalidate(&self->component);
}

void seqeditor_on_tracks_scroll(SeqEditor* self, psy_ui_Component* sender,
	double dx, double dy)
{
	psy_ui_component_set_scroll_top(&self->trackdesc.component,
		psy_ui_component_scroll_top(&self->track_items.component));
	psy_ui_component_set_scroll_left(&self->header.pane,
		psy_ui_component_scroll_left(&self->track_items.component));
}

void seqeditor_on_show_pattern_names(SeqEditor* self, psy_Property* sender)
{
	assert(self);

	self->state.showpatternnames = psy_property_item_bool(sender);
	psy_ui_component_invalidate(&self->track_items.component);
}

void seqeditor_on_hzoom(SeqEditor* self, ZoomBox* sender)
{
	assert(self);

	self->state.pxperbeat = zoombox_rate(sender) * self->state.defaultpxperbeat;
	psy_ui_component_align(&self->tracks);
	psy_ui_component_invalidate(&self->tracks);
}

void seqeditor_on_scroll(SeqEditor* self, psy_ui_Component* sender,
	double dx, double dy)
{
	assert(self);

	if (dx != 0) {
		seqeditor_update_line_positions(self);
	}
}

void seqeditor_on_mouse_move(SeqEditor* self, psy_ui_MouseEvent* ev)
{
	double l;

	assert(self);

	l = psy_ui_component_position(&self->trackdescpane).right +
		psy_ui_component_position(&self->close_bar.component).right;
	seqeditstate_set_cursor(&self->state, seqeditstate_quantize(&self->state,
		seqeditstate_pxtobeat(&self->state, psy_ui_mouseevent_pt(ev).x - l +
			psy_ui_component_scroll_left_px(&self->track_items.component))));
	seqeditor_update_cursor_line_position(self);
	if (self->state.updatecursorposition) {
		psy_ui_component_invalidate(&self->header.component);
		self->state.updatecursorposition = FALSE;
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void seqeditor_on_mouse_up(SeqEditor* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (self->state.cmd == SEQEDTCMD_NEWTRACK) {
		sequencecmds_append_track(self->state.cmds);
	}
	else if (self->state.cmd == SEQEDTCMD_DELTRACK) {
		sequencecmds_remove_track(self->state.cmds,
			self->state.dragseqpos.track);
	}
	self->state.cmd = SEQEDTCMD_NONE;
	psy_ui_mouseevent_stop_propagation(ev);
}

void seqeditor_on_toggle_expand(SeqEditor* self, psy_ui_Button* sender)
{
	assert(self);

	self->expanded = !self->expanded;
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make(psy_ui_value_zero(), psy_ui_value_make_ph(
			(self->expanded) ? 0.75 : 0.2)));
	psy_ui_component_align(psy_ui_component_parent(&self->component));
	psy_ui_component_invalidate(psy_ui_component_parent(&self->component));
}

void seqeditor_on_track_resize(SeqEditor* self, psy_ui_Component* sender,
	uintptr_t track_id, double* height)
{
	psy_ui_Component* track;

	assert(self);
	assert(height);

	track = psy_ui_component_at(&self->track_items.component, track_id);
	if (track) {
		psy_ui_component_set_preferred_height(track,
			psy_ui_value_make_eh(*height));
		psy_ui_component_align(&self->tracks_pane);
		psy_ui_component_invalidate(&self->tracks_pane);
	}
}

void seqeditor_on_toggle_timesig(SeqEditor* self, psy_ui_Button* sender)
{
	psy_ui_Size size;

	assert(self);

	if (psy_ui_component_visible(&self->header.timesig.component)) {
		seqeditheader_hide_timesig(&self->header);
	}
	else {
		seqeditheader_show_timesig(&self->header);
	}
	size = psy_ui_component_preferred_size(&self->header.component, NULL);
	if (psy_ui_component_visible(&self->header.timesig.component)) {
		psy_ui_button_highlight(sender);		
		psy_ui_button_set_svg(sender, psy_ui_app_svg(psy_ui_app(), "img.icon-less"));
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_button_base(sender)));
	}
	else {
		psy_ui_button_disable_highlight(sender);
		psy_ui_button_set_svg(sender, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_button_base(sender)));
	}
	psy_ui_component_set_preferred_height(&self->headerdescbar.component,
		size.height);
	psy_ui_component_align(&self->tracks_pane);
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void seqeditor_on_toggle_loop(SeqEditor* self, psy_ui_Button* sender)
{
	psy_ui_Size size;

	if (psy_ui_component_visible(&self->header.loops.component)) {
		seqeditheader_hide_loops(&self->header);
	}
	else {
		seqeditheader_show_loops(&self->header);
	}
	size = psy_ui_component_preferred_size(&self->header.component, NULL);
	if (psy_ui_component_visible(&self->header.loops.component)) {
		psy_ui_button_highlight(sender);
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_button_base(sender)));
	}
	else {
		psy_ui_button_disable_highlight(sender);
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_button_base(sender)));
	}
	psy_ui_component_set_preferred_height(&self->headerdescbar.component,
		size.height);
	psy_ui_component_align(&self->tracks_pane);
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void seqeditor_update_cursor_line_position(SeqEditor* self)
{
	assert(self);

	self->state.tracks_scroll_left = psy_ui_component_scroll_left_px(
		&self->track_items.component);
	seqeditorline_updateposition(&self->cursorline,
		self->state.cursorposition);
	seqeditruler_update_cursor_position(&self->header.ruler);
}

void seqeditor_on_timer(SeqEditor* self, uintptr_t timerid)
{
	assert(self);

	if (psy_audio_player_playing(workspace_player(self->workspace))) {
		seqeditorplayline_update(&self->playline);
	}
}

void seqeditor_on_cursor_changed(SeqEditor* self, psy_audio_Sequence* sender)
{
	assert(self);

	if (!psy_audio_orderindex_equal(&sender->cursor.order_index,
		sender->lastcursor.order_index)) {
		seqeditor_update_seq_edit_line_position(self);
	}
}

void seqeditor_update_seq_edit_line_position(SeqEditor* self)
{
	psy_audio_SequenceEntry* seqentry;

	assert(self);

	if (!seqeditstate_sequence(&self->state)) {
		return;
	}
	seqentry = psy_audio_sequence_entry(seqeditstate_sequence(&self->state),
		seqeditstate_sequence(&self->state)->cursor.order_index);
	if (seqentry) {
		seqeditorline_updateposition(&self->seqeditposline, seqentry->offset);
		seqeditruler_update_edit_position(&self->header.ruler);
	}
}

void seqeditor_on_tracks_align(SeqEditor* self, psy_ui_Component* sender)
{
	assert(self);

	seqeditor_update_line_positions(self);
}

void seqeditor_update_line_positions(SeqEditor* self)
{
	assert(self);

	self->state.tracks_scroll_left = psy_ui_component_scroll_left_px(
		&self->track_items.component);
	if (psy_audio_player_playing(workspace_player(self->workspace))) {
		seqeditorplayline_update(&self->playline);
	}
	seqeditor_update_seq_edit_line_position(self);
	seqeditorline_updateposition(&self->cursorline,
		self->state.cursorposition);
}

void seqeditor_on_play_status_changed(SeqEditor* self,
	psy_audio_Sequencer* sender)
{
	assert(self);

	if (sender->hostseqtime.playing) {
		seqeditorplayline_update(&self->playline);
		psy_ui_component_show_align(seqeditorplayline_base(&self->playline));
		psy_ui_component_invalidate(&self->scroller.pane);
	}
	else {
		psy_ui_component_hide(seqeditorplayline_base(&self->playline));
		psy_ui_component_invalidate(&self->scroller.pane);
	}
}

void seqeditor_on_play_line_changed(SeqEditor* self,
	psy_audio_Sequencer* sender)
{
	if (self->workspace->player_.follow_song) {
		seqeditor_on_follow(self);
	}
}

void seqeditor_on_follow(SeqEditor* self)
{
	psy_ui_RealSize size;
	double visi2;
	double playpos;
	double center;

	size = psy_ui_component_scroll_size_px(&self->scroller.pane);
	visi2 = size.width / self->state.pxperbeat / 2.0 - 2.0;
	playpos =
		floor(psy_dsp_beatpos_real(self->workspace->player_.sequencer.hostseqtime.currplaycursor.abs_offset)
			* 0.25) * 4;
	center = psy_max(0.0, playpos - visi2);
	psy_ui_component_set_scroll_left_px(&self->track_items.component,
		center * self->state.pxperbeat);
}

#endif /* PSYCLE_USE_SEQEDITOR */
