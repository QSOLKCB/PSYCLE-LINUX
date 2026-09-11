/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pianoroll.h"

#ifdef PSYCLE_USE_PIANOROLL

/* local */
#include "cmddef.h"
#include "cmdsnotes.h"
#include "trackergridstate.h"
/* audio */
#include <sequencecmds.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


#define PIANOROLL_REFRESHRATE 50
#define CMD_ENTER 1100


/* PianoBar */

/* implenentation */
void pianobar_init(PianoBar* self, psy_ui_Component* parent,
	PianoGridState* gridstate)
{
	assert(self);
		
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_default_align(pianobar_base(self), psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));	
	psy_ui_label_init_text(&self->beats, pianobar_base(self), "pv.beats");
	psy_ui_component_set_margin(pianobar_base(self),
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 1.0));
	zoombox_init(&self->zoombox_beatwidth, pianobar_base(self));
	psy_ui_label_init_text(&self->keys, pianobar_base(self), "pv.keyboard");
	zoombox_init(&self->zoombox_keyheight, pianobar_base(self));
	psy_ui_label_init_text(&self->tracks, pianobar_base(self),
		"pv.showtracks");
	psy_ui_button_init(&self->tracks_all, pianobar_base(self));
	psy_ui_button_exchange(&self->tracks_all, gridstate->track_all);	
	psy_ui_button_init(&self->track_curr, pianobar_base(self));
	psy_ui_button_exchange(&self->track_curr,
		gridstate->track_current);
	psy_ui_button_init(&self->tracks_active, pianobar_base(self));
	psy_ui_button_exchange(&self->tracks_active,
		gridstate->track_active);	
}


/* Pianoroll */

/* protoypes */
static void pianoroll_on_destroyed(Pianoroll*);
static void pianoroll_on_timer(Pianoroll*, uintptr_t timer_id);
static void pianoroll_on_lpb_changed(Pianoroll*, psy_audio_Player* sender,
	uintptr_t lpb);
static void pianoroll_on_align(Pianoroll*);
static void pianoroll_on_mouse_down(Pianoroll*, psy_ui_MouseEvent*);
static void pianoroll_on_grid_scroll(Pianoroll*, psy_ui_Component* sender,
	double dx, double dy);
static void pianoroll_on_beat_width(Pianoroll*, ZoomBox* sender);
static void pianoroll_on_key_height(Pianoroll*, ZoomBox* sender);
static void pianoroll_on_key_type(Pianoroll*, psy_ui_ComboBox* sender);
static void pianoroll_on_cursor_changed(Pianoroll*, psy_audio_Sequence* sender);
static void pianoroll_on_play_status_changed(Pianoroll*,
	psy_audio_Sequencer* sender);
static void pianoroll_on_play_line_changed(Pianoroll*,
	psy_audio_Sequencer* sender);
static void pianoroll_on_song_changed(Pianoroll*, psy_audio_Player* sender);
static void pianoroll_connect_song(Pianoroll*, psy_audio_Song*);
static bool pianoroll_playing_following_song(const Pianoroll*);
static void pianoroll_sync_scroll_left(Pianoroll*);
static void pianoroll_sync_scroll_top(Pianoroll*);
static void pianoroll_update_scroll_step(Pianoroll*);
static void pianoroll_on_single_display(Pianoroll*, psy_Property* sender);
static void pianoroll_on_sequence_track_reposition(Pianoroll*,
	psy_audio_Sequence* sender, uintptr_t trackidx);
static void pianoroll_on_follow(Pianoroll*);
	
/* vtable */
static psy_ui_ComponentVtable pianoroll_vtable;
static bool pianoroll_vtable_initialized = FALSE;

static void pianoroll_vtable_init(Pianoroll* self)
{
	assert(self);

	if (!pianoroll_vtable_initialized) {
		pianoroll_vtable = *(self->component.vtable);
		pianoroll_vtable.on_destroyed =
			(psy_ui_fp_component)
			pianoroll_on_destroyed;
		pianoroll_vtable.onalign =
			(psy_ui_fp_component)
			pianoroll_on_align;
		pianoroll_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			pianoroll_on_mouse_down;		
		pianoroll_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			pianoroll_on_timer;		
		pianoroll_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &pianoroll_vtable);
}

/* implenentation */
void pianoroll_init(Pianoroll* self, psy_ui_Component* parent,
	PatternViewState* pvstate, psy_DiskOp* disk_op, Workspace* workspace)
{
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	pianoroll_vtable_init(self);	
	self->workspace = workspace;
	self->player = workspace_player(workspace);
	assert(self->player);
	self->opcount = 0;
	self->center_key = TRUE;
	/* shared states */
	keyboardstate_init(&self->keyboardstate, psy_ui_VERTICAL, FALSE);
	pianogridstate_init(&self->gridstate, pvstate);	
	/* bar */
	pianobar_init(&self->bar, pianoroll_base(self), &self->gridstate);
	psy_ui_component_set_align(&self->bar.component, psy_ui_ALIGN_BOTTOM);
	psy_signal_connect(&self->bar.zoombox_beatwidth.signal_changed, self,
		pianoroll_on_beat_width);
	psy_signal_connect(&self->bar.zoombox_keyheight.signal_changed, self,
		pianoroll_on_key_height);
	psy_ui_component_set_align(&self->bar.zoombox_keyheight.component,
		psy_ui_ALIGN_LEFT);	
	/* hscroll */
	psy_ui_scrollbar_init(&self->hscroll, pianoroll_base(self));
	psy_ui_component_set_align(&self->hscroll.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_padding(&self->hscroll.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 10.0));	
	/* paramroll */
	paramroll_init(&self->param_roll, pianoroll_base(self), &self->gridstate);
	psy_ui_component_set_align(paramroll_base(&self->param_roll),
		psy_ui_ALIGN_BOTTOM);	
	/* client */	
	psy_ui_component_init(&self->client, pianoroll_base(self), NULL);
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_CLIENT);
	/* editbar */
	patterneditbar_init(&self->editbar, &self->client, &self->gridstate,
		pvstate, workspace);
	psy_ui_component_set_align(&self->editbar.component, psy_ui_ALIGN_BOTTOM);	
	/* left top area (keyboardheader, keyboard) */
	psy_ui_component_init(&self->left_top, &self->client, NULL);
	psy_ui_component_set_align(&self->left_top, psy_ui_ALIGN_LEFT);	
	/* beat ruler */
	psy_ui_component_init_align(&self->ruler_pane, &self->client, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_preferred_height(&self->ruler_pane,
		psy_ui_value_make_eh(2.0));
	pianoruler_init(&self->ruler, &self->ruler_pane, &self->gridstate);
	psy_ui_component_set_align(pianoruler_base(&self->ruler),
		psy_ui_ALIGN_FIXED);
	/* client area (event grid) */
	pianogrid_init(&self->grid, &self->client, &self->keyboardstate,
		&self->gridstate, disk_op);
	psy_ui_scroller_init(&self->scroller, &self->client,
		&self->hscroll, NULL);	
	psy_ui_scroller_set_client(&self->scroller, pianogrid_base(&self->grid));
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);	
	psy_ui_component_set_align(&self->grid.component, psy_ui_ALIGN_FIXED);
	/* Keyboard */
	psy_ui_component_init(&self->keyboardpane, &self->left_top, NULL);
	psy_ui_component_set_align(&self->keyboardpane, psy_ui_ALIGN_CLIENT);
	pianokeyboard_init(&self->keyboard, &self->keyboardpane,
		&self->keyboardstate, self->player);
	psy_ui_combobox_init(&self->keytype, &self->left_top);	
	psy_ui_combobox_add_text(&self->keytype, "Keys");
	psy_ui_combobox_add_text(&self->keytype, "Notes");
	psy_ui_combobox_add_text(&self->keytype, "Drums");
	psy_ui_combobox_select(&self->keytype, 0);
	psy_ui_component_set_preferred_size(psy_ui_combobox_base(&self->keytype),
		psy_ui_size_make_em(10.0, 2.0));	
	psy_ui_component_set_align(psy_ui_combobox_base(&self->keytype),
		psy_ui_ALIGN_TOP);
	psy_signal_connect(&self->keytype.signal_selchanged, self,
		pianoroll_on_key_type);
	psy_ui_component_set_align(&self->keyboard.component,
		psy_ui_ALIGN_HCLIENT);
	/* splitter */
	psy_ui_splitter_init(&self->splitter, pianoroll_base(self));
	psy_ui_component_set_align(psy_ui_splitter_base(&self->splitter),
		psy_ui_ALIGN_BOTTOM);
	/* connect signals */
	psy_signal_connect(&pianogrid_base(&self->grid)->signal_scrolled, self,
		pianoroll_on_grid_scroll);
	pianoroll_connect_song(self, workspace_song(workspace));
	if (self->workspace->song) {
		psy_signal_connect(&self->workspace->song->sequence_.signal_cursor_changed,
			self, pianoroll_on_cursor_changed);
		psy_signal_connect(
			&self->workspace->song->sequence_.signal_track_reposition,
			self, pianoroll_on_sequence_track_reposition);
	}
	psy_signal_connect(
		&self->player->sequencer.signal_play_line_changed, self,
		pianoroll_on_play_line_changed);
	psy_signal_connect(
		&self->player->sequencer.signal_play_status_changed, self,
		pianoroll_on_play_status_changed);		
	psy_signal_connect(&self->player->signal_song_changed, self,
		pianoroll_on_song_changed);	
	psy_signal_connect(&self->player->signal_lpbchanged, self,
		pianoroll_on_lpb_changed);
	psy_configuration_connect(psycleconfig_patview(workspace_cfg(self->workspace)),
		"displaysinglepattern", self, pianoroll_on_single_display);
	psy_ui_component_start_timer(pianoroll_base(self), 0,
		PIANOROLL_REFRESHRATE);
	keyboardstate_update_metrics(&self->keyboardstate,
		psy_ui_component_textmetric(pianoroll_base(self)));
}

void pianoroll_on_destroyed(Pianoroll* self)
{
	assert(self);
	
	pianogridstate_dispose(&self->gridstate);
}

void pianoroll_scroll_to_key(Pianoroll* self, uint8_t key)
{
	assert(self);
	
	psy_ui_component_set_scroll_top(pianogrid_base(&self->grid),
		psy_ui_value_make_px(
			self->keyboardstate.key_extent_px *
			(self->keyboardstate.keymax - key)));
}

void pianoroll_on_timer(Pianoroll* self, uintptr_t timer_id)
{
	psy_audio_Pattern* pattern;
	
	assert(self);
	
	if (!psy_ui_component_draw_visible(&self->component)) {
		return;
	}
	pattern = patternviewstate_pattern(self->gridstate.pv);
	if (pattern) {
		if (psy_audio_pattern_opcount(pattern) != self->opcount) {
			psy_ui_component_invalidate(pianogrid_base(&self->grid));
			psy_ui_component_invalidate(paramroll_base(&self->param_roll));
			self->opcount = psy_audio_pattern_opcount(pattern);
		}		
	} else {
		self->opcount = 0;
	}	
}

void pianoroll_on_align(Pianoroll* self)
{
	assert(self);	
	
	pianoroll_update_scroll_step(self);
	if (self->center_key && psy_ui_component_draw_visible(
			pianoroll_base(self))) {
		self->center_key = FALSE;
		pianoroll_scroll_to_key(self, 84);		
	}
}

void pianoroll_on_mouse_down(Pianoroll* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_set_focus(pianoroll_base(self));
}

void pianoroll_update_scroll_step(Pianoroll* self)
{
	assert(self);
	
	psy_ui_component_set_scroll_step(pianogrid_base(&self->grid),
		psy_ui_size_make_px(
			pianogridstate_steppx(&self->gridstate),
			self->keyboardstate.key_extent_px));
	psy_ui_component_set_scroll_step_height(pianokeyboard_base(&self->keyboard),
		psy_ui_value_make_px(self->keyboardstate.key_extent_px));
	psy_ui_component_set_scroll_step_width(pianoruler_base(&self->ruler),
		psy_ui_value_make_px(pianogridstate_steppx(&self->gridstate)));
}

void pianoroll_on_lpb_changed(Pianoroll* self, psy_audio_Player* sender,
	uintptr_t lpb)
{
	assert(self);
	
	psy_ui_component_set_scroll_step_width(pianogrid_base(&self->grid),
		psy_ui_value_make_px(pianogridstate_steppx(&self->gridstate)));
	pianoroll_align(self, FALSE, TRUE);
}

void pianoroll_on_cursor_changed(Pianoroll* self, psy_audio_Sequence* sender)
{	
	assert(self);
	
	bool invalidate_cursor;
	
	if (!psy_ui_component_draw_visible(pianoroll_base(self))) {
		return;
	}		
	if (psy_audio_sequence_lpb_changed(sender)) {		
		psy_ui_component_align(&self->component);
		psy_ui_component_invalidate(&self->component);
	}
	if ((pianogridstate_track_display(&self->gridstate) ==
				PIANOROLL_TRACK_DISPLAY_CURRENT) &&
			(sender->cursor.channel_ != sender->lastcursor.channel_)) {
		psy_ui_component_invalidate(&self->component);
		pianogrid_invalidate_cursor(&self->grid);
		return;
	}
	invalidate_cursor = TRUE;
	if (patternviewstate_single_mode(self->gridstate.pv)) {		
		if (!psy_audio_orderindex_equal(
				&sender->cursor.order_index,
				sender->lastcursor.order_index)) {
			if (pianoroll_playing_following_song(self)) {						
				psy_ui_component_set_scroll_left_px(&self->grid.component, 0.0);
			}
			pianoroll_align(self, FALSE, TRUE);			
			invalidate_cursor = FALSE;
		}			
	}
	if (invalidate_cursor && !(pianoroll_playing_following_song(self))) {
		pianogrid_invalidate_cursor(&self->grid);
	}
}

void pianoroll_on_play_status_changed(Pianoroll* self, 
	psy_audio_Sequencer* sender)
{
	assert(self);

	if (!psy_ui_component_draw_visible(pianoroll_base(self))) {
		return;
	}
	pianogrid_invalidate_cursor(&self->grid);
}

void pianoroll_on_play_line_changed(Pianoroll* self,
	psy_audio_Sequencer* sender)
{
	assert(self);

	if (!psy_ui_component_draw_visible(pianoroll_base(self))) {
		return;
	}
	if (pianoroll_playing_following_song(self)) {
		pianoroll_on_follow(self);
	}		
	pianogrid_invalidate_playbar(&self->grid);	
}


bool pianoroll_playing_following_song(const Pianoroll* self)
{
	assert(self);
	
	return (psy_audio_hostsequencertime_playing(
			&self->player->sequencer.hostseqtime) &&
		self->player->follow_song);
}

void pianoroll_on_song_changed(Pianoroll* self, psy_audio_Player* sender)
{	
	assert(self);
	
	pianoroll_connect_song(self, psy_audio_player_song(sender));	
	psy_audio_sequencecursor_init(&self->grid.old_cursor);
}

void pianoroll_connect_song(Pianoroll* self, psy_audio_Song* song)
{
	psy_audio_Sequence* sequence;
	
	assert(self);
	
	if (!song) {
		return;
	}
	sequence = psy_audio_song_sequence(song);
	psy_signal_connect(&sequence->signal_cursor_changed,
		self, pianoroll_on_cursor_changed);
	psy_signal_connect(&sequence->signal_track_reposition,
		self, pianoroll_on_sequence_track_reposition);		
}

void pianoroll_on_grid_scroll(Pianoroll* self, psy_ui_Component* sender,
	double dx, double dy)
{
	assert(self);

	pianoroll_sync_scroll_left(self);
	pianoroll_sync_scroll_top(self);	
}

void pianoroll_sync_scroll_left(Pianoroll* self)
{
	assert(self);
	
	psy_ui_component_set_scroll_left(pianoruler_base(&self->ruler),
		psy_ui_component_scroll_left(pianogrid_base(&self->grid)));
	psy_ui_component_set_scroll_left(&self->param_roll.draw_.component,
		psy_ui_component_scroll_left(pianogrid_base(&self->grid)));
}

void pianoroll_sync_scroll_top(Pianoroll* self)
{
	assert(self);
		
	psy_ui_component_set_scroll_top(&self->keyboard.component,
		psy_ui_component_scroll_top(pianogrid_base(&self->grid)));
}

void pianoroll_on_beat_width(Pianoroll* self, ZoomBox* sender)
{
	assert(self);

	pianogridstate_set_zoom(&self->gridstate, zoombox_rate(sender));
	psy_ui_component_set_scroll_step_width(pianogrid_base(&self->grid),
		psy_ui_value_make_px(pianogridstate_steppx(&self->gridstate)));	
	pianoroll_align(self, FALSE, TRUE);
}

void pianoroll_on_key_height(Pianoroll* self, ZoomBox* sender)
{
	assert(self);

	self->keyboardstate.key_extent = psy_ui_mul_value_real(
		self->keyboardstate.default_key_extent, zoombox_rate(sender));
	keyboardstate_update_metrics(&self->keyboardstate,
		psy_ui_component_textmetric(&self->component));
	psy_ui_component_set_scroll_step_height(pianogrid_base(&self->grid),
		psy_ui_value_make_px(self->keyboardstate.key_extent_px));
	psy_ui_component_set_scroll_step_height(pianokeyboard_base(&self->keyboard),
		psy_ui_value_make_px(self->keyboardstate.key_extent_px));	
	psy_ui_component_set_scroll_top(&self->keyboard.component,
		psy_ui_component_scroll_top(pianogrid_base(&self->grid)));
	pianoroll_align(self, TRUE, TRUE);	
}

void pianoroll_align(Pianoroll* self, bool keys, bool grid)
{
	assert(self);
	
	if (grid) {		
		psy_ui_component_align(&self->scroller.pane);
		psy_ui_component_align(&self->ruler.component);
		psy_ui_component_align(&self->ruler_pane);		
		psy_ui_component_align(&self->param_roll.pane_);
		psy_ui_component_align(&self->component);
	}
	if (keys) {
		psy_ui_component_align(&self->keyboardpane);
	}
	if (grid) {		
		psy_ui_component_invalidate(&self->component);
		psy_ui_component_invalidate(&self->ruler_pane);		
	} else if (keys) {
		psy_ui_component_invalidate(&self->keyboardpane);
	}
}

void pianoroll_on_key_type(Pianoroll* self, psy_ui_ComboBox* sender)
{
	uintptr_t index;

	assert(self);

	index = psy_ui_combobox_cur_sel(sender);
	if (index < KEYBOARDTYPE_NUM) {
		pianokeyboard_set_keyboard_type(&self->keyboard, (KeyboardType)index);
		psy_ui_component_align(pianoroll_base(self));
	}	
}

void pianoroll_on_single_display(Pianoroll* self, psy_Property* sender)
{
	assert(self);
	
	pianoroll_align(self, TRUE, TRUE);	
}

void pianoroll_on_sequence_track_reposition(Pianoroll* self,
	psy_audio_Sequence* sender, uintptr_t trackidx)
{
	psy_audio_OrderIndex order_index;
	
	assert(self);
	
	order_index =psy_audio_sequencecursor_order_index(
		&self->gridstate.pv->cursor);	
	if (trackidx == order_index.track) {
		pianoroll_align(self, FALSE, TRUE);
	}
}

void pianoroll_on_follow(Pianoroll* self)
{		
	psy_ui_RealSize size;
	double visi2;
	double playpos;
	double center;
	double lpb;	
	const psy_audio_HostSequencerTime* hostseqtime;
			
	lpb = (double)psy_audio_sequencecursor_lpb(
		&self->gridstate.pv->sequence->cursor);
	size = psy_ui_component_scroll_size_px(&self->scroller.pane);
	visi2 = size.width / self->gridstate.beat_convert.line_px / 2.0 /
		lpb - 2.0;
	hostseqtime = &self->player->sequencer.hostseqtime;	
	if (patternviewstate_single_mode(self->gridstate.pv)) {
		playpos = psy_dsp_beatpos_real(hostseqtime->currplaycursor.offset);
	} else {
		playpos = psy_dsp_beatpos_real(hostseqtime->currplaycursor.abs_offset);
	}	
	playpos = floor(playpos * 0.25) * lpb;
	center = psy_max(0.0, playpos - visi2);	
	psy_ui_component_set_scroll_left_px(pianogrid_base(&self->grid),
		center * self->gridstate.beat_convert.line_px * lpb);
}

void pianoroll_make_cmds(psy_Property* parent)
{
	psy_Property* cmds;

	assert(parent);

	cmds = psy_property_set_text(psy_property_append_section(parent,
		"pianoroll"), "Pianoroll");
	set_cmd_all(cmds, CMD_NAVUP,
		psy_ui_KEY_UP, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navup", "up");	
	set_cmd_all(cmds, CMD_NAVDOWN,
		psy_ui_KEY_DOWN, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navdown", "down");
	set_cmd_all(cmds, CMD_NAVLEFT,
		psy_ui_KEY_LEFT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navleft", "left");
	set_cmd_all(cmds, CMD_NAVRIGHT,
		psy_ui_KEY_RIGHT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navright", "right");
	set_cmd_all(cmds, CMD_NAVPAGEUP,
		psy_ui_KEY_PRIOR, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navpageup", "pageup");
	set_cmd_all(cmds, CMD_NAVPAGEDOWN,
		psy_ui_KEY_NEXT, psy_SHIFT_OFF, psy_CTRL_OFF,
		"navpagedown", "pagedown");
	set_cmd_all(cmds, CMD_NAVPAGEUPKEYBOARD,
		psy_ui_KEY_PRIOR, psy_SHIFT_ON, psy_CTRL_OFF,
		"navpageupkbd", "pageup kbd");
	set_cmd_all(cmds, CMD_NAVPAGEDOWNKEYBOARD,
		psy_ui_KEY_NEXT, psy_SHIFT_ON, psy_CTRL_OFF,
		"navpagedownkbd", "pagedown kbd");
	set_cmd_all(cmds, CMD_ENTER,
		psy_ui_KEY_SPACE, psy_SHIFT_ON, psy_CTRL_OFF,
		"enter", "enter");	
}

#endif /* PSYCLE_USE_PIANOROLL */
