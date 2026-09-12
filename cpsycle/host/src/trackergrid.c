/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackergrid.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* local */
#include "cmdsnotes.h"
#include "patternnavigator.h"
/* audio */
#include <sequencecmds.h>
/* std */
#include <stdio.h>
#include <stdlib.h>
/* platform */
#include "../../detail/portable.h"

#define ISDIGIT TRUE


/* prototypes */
static void trackergrid_on_destroyed(TrackerGrid*);
static void trackergrid_connect_input_handler(TrackerGrid*, InputHandler*);
static void trackergrid_on_draw(TrackerGrid*, psy_ui_Graphics*);
static void trackergrid_on_mouse_down(TrackerGrid*, psy_ui_MouseEvent*);
static void trackergrid_on_mouse_move(TrackerGrid*, psy_ui_MouseEvent*);
static void trackergrid_on_mouse_up(TrackerGrid*, psy_ui_MouseEvent*);
static void trackergrid_on_mouse_doubleclick(TrackerGrid*, psy_ui_MouseEvent*);
static psy_ui_RealRectangle trackergrid_selection_bounds(TrackerGrid*);
static void trackergrid_drag_selection(TrackerGrid*, psy_audio_SequenceCursor);
static void trackergrid_on_scroll(TrackerGrid*, psy_ui_Component* sender);
static void trackergrid_clear_midline(TrackerGrid*);
static void trackergrid_input_value(TrackerGrid*, uintptr_t value,
	bool isdigit);
static void trackergrid_prev_track(TrackerGrid*);
static void trackergrid_next_track(TrackerGrid*);
static void trackergrid_prev_line(TrackerGrid*);
static void trackergrid_advance_line(TrackerGrid*);
static void trackergrid_prev_lines(TrackerGrid*, uintptr_t lines, bool wrap);
static void trackergrid_advance_lines(TrackerGrid*, uintptr_t lines, bool wrap);
static void trackergrid_home(TrackerGrid*);
static void trackergrid_end(TrackerGrid*);
static void trackergrid_row_clear(TrackerGrid*);
static void trackergrid_row_clear_note(TrackerGrid*);
static void trackergrid_row_clear_value(TrackerGrid*);
static void trackergrid_prev_col(TrackerGrid*);
static void trackergrid_next_col(TrackerGrid*);
static void trackergrid_set_default_event(TrackerGrid*,
	psy_audio_Pattern* defaultpattern, psy_audio_PatternEvent*);
static void trackergrid_enable_pattern_sync(TrackerGrid*);
static void trackergrid_prevent_pattern_sync(TrackerGrid*);
static void trackergrid_reset_pattern_sync(TrackerGrid*);
static bool trackergrid_on_tracker_cmds(TrackerGrid*, InputHandler*);
static bool trackergrid_on_edt_cmds(TrackerGrid*, InputHandler*);
static bool trackergrid_on_note_cmds(TrackerGrid*, InputHandler* sender);
static bool trackergrid_insert_note(TrackerGrid*, intptr_t note_id);
static bool trackergrid_on_midi_cmds(TrackerGrid*, InputHandler* sender);
static void trackergrid_scroll_left(TrackerGrid*, psy_audio_SequenceCursor);
static void trackergrid_scroll_right(TrackerGrid*, psy_audio_SequenceCursor);
static void trackergrid_scroll_up(TrackerGrid*, psy_audio_SequenceCursor);
static void trackergrid_set_cursor(TrackerGrid*, psy_audio_SequenceCursor);
static void trackergrid_on_timer(TrackerGrid*, uintptr_t id);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(TrackerGrid* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			trackergrid_on_destroyed;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			trackergrid_on_draw;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			trackergrid_on_mouse_down;
		vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			trackergrid_on_mouse_move;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			trackergrid_on_mouse_up;
		vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			trackergrid_on_mouse_doubleclick;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			trackergrid_on_timer;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(trackergrid_base(self), &vtable);
}

/* implementation */
void trackergrid_init(TrackerGrid* self, psy_ui_Component* parent,
	TrackerState* state, psy_DiskOp* disk_op)
{
	assert(self);
	assert(state);

	psy_ui_component_init(trackergrid_base(self), parent, NULL);
	vtable_init(self);
	self->state = state;
	self->player = state->pv->player;	
	assert(self->player);
	psy_table_init(&self->columns);
	patterncmds_init(&self->cmds, state->pv, disk_op);
	psy_audio_sequencecursor_init(&self->old_cursor);
	self->effcursor_always_down = FALSE;
	self->prevent_event_driver = FALSE;
	self->down = FALSE;
	/* setup base component */
	psy_signal_init(&self->signal_col_resize);
	psy_ui_component_set_scroll_redraw(trackergrid_base(self),
		psy_ui_SCROLLREDRAW_NONE);
	psy_ui_component_set_tab_index(trackergrid_base(self), 0);
	psy_ui_component_set_align_expand(trackergrid_base(self), psy_ui_HEXPAND);
	psy_ui_component_set_scroll_step_height(trackergrid_base(self),
		self->state->line_height);
	trackergrid_connect_input_handler(self, psy_ui_app_input_handler(psy_ui_app()));
	/* handle midline invalidation */
	psy_signal_connect(&trackergrid_base(self)->signal_scroll, self,
		trackergrid_on_scroll);
	psy_ui_component_start_timer(trackergrid_base(self), 0, 50);
}

void trackergrid_on_destroyed(TrackerGrid* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_col_resize);
	psy_table_dispose(&self->columns);
}

void trackergrid_connect_input_handler(TrackerGrid* self,
	InputHandler* input_handler)
{
	assert(self);

	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "edit", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)trackergrid_on_edt_cmds);
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)trackergrid_on_tracker_cmds);
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "notes", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)trackergrid_on_note_cmds);
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_MIDI, "", VIEW_ID_PATTERNS,
		self, NULL, (fp_inputhandler_input)trackergrid_on_midi_cmds);
}

void trackergrid_on_draw(TrackerGrid* self, psy_ui_Graphics* g)
{
	psy_ui_RealRectangle g_clip;

	assert(self);

	g_clip = psy_ui_graphics_cliprect(g);
	/* prepares patternentry draw done in trackergridcolumn */
	trackerstate_update_clip_events(self->state, &g_clip,
		psy_ui_component_textmetric(trackergrid_base(self)));
	trackereventtable_prepare_selection(&self->state->track_events,
		self->state->pv->sequence, &self->state->pv->selection.block);
}

void trackergrid_prev_track(TrackerGrid* self)
{
	PatternColNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patterncolnavigator_init(&navigator, self->state, TRUE);
	cursor = patterncolnavigator_prev_track(&navigator,
		patternviewstate_cursor(self->state->pv));
	if (patterncolnavigator_wrap(&navigator)) {
		trackergrid_scroll_right(self, cursor);
	}
	else {
		trackergrid_scroll_left(self, cursor);
	}
	trackergrid_set_cursor(self, cursor);
}

void trackergrid_next_track(TrackerGrid* self)
{
	PatternColNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patterncolnavigator_init(&navigator, self->state, TRUE);
	cursor = patterncolnavigator_next_track(&navigator,
		patternviewstate_cursor(self->state->pv));
	if (patterncolnavigator_wrap(&navigator)) {
		trackergrid_scroll_left(self, cursor);
	}
	else {
		trackergrid_scroll_right(self, cursor);
	}
	trackergrid_set_cursor(self, cursor);
}

void trackergrid_scroll_up(TrackerGrid* self, psy_audio_SequenceCursor cursor)
{
	intptr_t line;
	intptr_t topline;
	double top;

	assert(self);

	if (patternviewstate_single_mode(self->state->pv)) {
		line = psy_audio_sequencecursor_line(&cursor);
	}
	else {
		line = psy_audio_sequencecursor_line_abs(&cursor,
			self->state->pv->sequence);
	}
	top = self->state->beat_convert.line_px * line;
	if (self->state->midline) {
		psy_ui_RealSize gridsize;

		gridsize = psy_ui_component_scroll_size_px(trackergrid_base(self));
		topline = (intptr_t)(gridsize.height /
			self->state->beat_convert.line_px / 2.0);
	}
	else {
		topline = 0;
	}
	if (psy_ui_component_scroll_top_px(trackergrid_base(self)) +
		topline * self->state->beat_convert.line_px > top) {
		intptr_t dlines;

		dlines = (intptr_t)((psy_ui_component_scroll_top_px(trackergrid_base(self)) +
			topline * self->state->beat_convert.line_px - top) /
			(self->state->beat_convert.line_px));
		self->state->scroll_cursor = TRUE;
		psy_ui_component_set_scroll_top_px(trackergrid_base(self),
			psy_ui_component_scroll_top_px(trackergrid_base(self)) -
			psy_ui_component_scroll_step_height_px(trackergrid_base(self)) * dlines);
		self->state->scroll_cursor = FALSE;
	}
}

void trackergrid_scroll_down(TrackerGrid* self, psy_audio_SequenceCursor cursor,
	bool set)
{
	intptr_t line;
	intptr_t visilines;
	psy_ui_RealSize size;

	assert(self);

	size = psy_ui_component_clientsize_px(trackergrid_base(self));
	visilines = (intptr_t)(size.height / self->state->beat_convert.line_px);
	if (self->state->midline) {
		visilines /= 2;
	}
	else {
		--visilines;
	}
	if (patternviewstate_single_mode(self->state->pv)) {
		line = psy_audio_sequencecursor_line(&cursor);
	}
	else {
		line = psy_audio_sequencecursor_line_abs(&cursor,
			self->state->pv->sequence);
	}
	if (visilines < line - psy_ui_component_scroll_top_px(trackergrid_base(self)) /
		self->state->beat_convert.line_px) {
		intptr_t dlines;

		dlines = (intptr_t)
			(line - psy_ui_component_scroll_top_px(trackergrid_base(self)) /
				self->state->beat_convert.line_px - visilines);
		if (set) {
			trackergrid_set_cursor(self, cursor);
		}
		self->state->scroll_cursor = TRUE;
		psy_ui_component_set_scroll_top_px(trackergrid_base(self),
			psy_ui_component_scroll_top_px(trackergrid_base(self)) +
			psy_ui_component_scroll_step_height_px(trackergrid_base(self)) * dlines);
		self->state->scroll_cursor = FALSE;
	}
	if (set) {
		trackergrid_set_cursor(self, cursor);
	}
}

void trackergrid_scroll_left(TrackerGrid* self, psy_audio_SequenceCursor cursor)
{
	uintptr_t index;

	assert(self);

	psy_ui_component_intersect(trackergrid_base(self), psy_ui_realpoint_make(
		psy_ui_component_scroll_left_px(trackergrid_base(self)), 0.0),
		&index);
	if ((index != psy_INDEX_INVALID) && (index > cursor.channel_)) {
		psy_ui_Component* column;
		psy_ui_RealRectangle position;

		column = (psy_ui_Component*)psy_table_at(&self->columns,
			cursor.channel_);
		position = psy_ui_component_position(column);
		psy_ui_component_set_scroll_left(trackergrid_base(self),
			psy_ui_value_make_px(position.left));
	}
}

void trackergrid_scroll_right(TrackerGrid* self, psy_audio_SequenceCursor
	cursor)
{
	uintptr_t visitracks;
	uintptr_t tracks;
	psy_ui_RealSize size;
	intptr_t trackright;
	intptr_t trackleft;

	assert(self);

	size = psy_ui_component_clientsize_px(trackergrid_base(self));
	trackleft = trackerstate_px_to_track(self->state,
		psy_ui_component_scroll_left_px(trackergrid_base(self)),
		psy_ui_component_textmetric(trackergrid_base(self)));
	trackright = trackerstate_px_to_track(self->state,
		size.width +
		psy_ui_component_scroll_left_px(trackergrid_base(self)),
		psy_ui_component_textmetric(trackergrid_base(self)));
	visitracks = trackright - trackleft;
	tracks = cursor.channel_ + 1;
	if (tracks > trackleft + visitracks) {
		psy_ui_Component* column;

		column = (psy_ui_Component*)psy_table_at(&self->columns,
			tracks - visitracks);
		if (column) {
			psy_ui_RealRectangle position;

			position = psy_ui_component_position(column);
			psy_ui_component_set_scroll_left(trackergrid_base(self),
				psy_ui_value_make_px(position.left));
		}
	}
}

void trackergrid_prev_line(TrackerGrid* self)
{
	assert(self);

	trackergrid_prev_lines(self,
		patternviewstate_cursor_step(self->state->pv),
		psy_configuration_value_bool(self->state->pv->patconfig,
			"wraparound", TRUE));
}

void trackergrid_advance_line(TrackerGrid* self)
{
	assert(self);

	trackergrid_advance_lines(self,
		patternviewstate_cursor_step(self->state->pv),
		psy_configuration_value_bool(self->state->pv->patconfig,
			"wraparound", TRUE));
}

void trackergrid_advance_lines(TrackerGrid* self, uintptr_t lines, bool wrap)
{
	PatternLineNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patternlinenavigator_init(&navigator, self->state->pv, wrap);
	cursor = patternlinenavigator_down(&navigator, lines,
		patternviewstate_cursor(self->state->pv));
	if (self->state->midline) {
		trackergrid_set_cursor(self, cursor);
		trackergrid_center_on_cursor(self);
	}
	else if (patternlinennavigator_wrap(&navigator)) {
		trackergrid_set_cursor(self, cursor);
		trackergrid_scroll_up(self, cursor);
	}
	else {
		trackergrid_scroll_down(self, cursor, TRUE);
	}
}

void trackergrid_prev_lines(TrackerGrid* self, uintptr_t lines, bool wrap)
{
	assert(self);

	PatternLineNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patternlinenavigator_init(&navigator, self->state->pv, wrap);
	cursor = patternlinenavigator_up(&navigator, lines,
		patternviewstate_cursor(self->state->pv));
	if (self->state->midline) {
		trackergrid_set_cursor(self, cursor);
		trackergrid_center_on_cursor(self);
	}
	else if (!patternlinennavigator_wrap(&navigator)) {
		trackergrid_set_cursor(self, cursor);
		trackergrid_scroll_up(self, cursor);
	}
	else {
		trackergrid_scroll_down(self, cursor, TRUE);
	}
}

void trackergrid_home(TrackerGrid* self)
{
	assert(self);

	if (patternviewstate_ft2home(self->state->pv)) {
		PatternLineNavigator navigator;
		psy_audio_SequenceCursor cursor;

		patternlinenavigator_init(&navigator, self->state->pv, FALSE);
		cursor = patternlinenavigator_home(&navigator,
			patternviewstate_cursor(self->state->pv));
		trackergrid_set_cursor(self, cursor);
		if (self->state->midline) {
			trackergrid_center_on_cursor(self);
		}
		else {
			trackergrid_scroll_up(self, cursor);
		}
	}
	else {
		PatternColNavigator navigator;
		psy_audio_SequenceCursor cursor;

		patterncolnavigator_init(&navigator, self->state, FALSE);
		cursor = patterncolnavigator_home(&navigator,
			patternviewstate_cursor(self->state->pv));
		trackergrid_set_cursor(self, cursor);
		trackergrid_scroll_left(self, cursor);
	}
}

void trackergrid_end(TrackerGrid* self)
{
	assert(self);

	if (patternviewstate_ft2home(self->state->pv)) {
		PatternLineNavigator navigator;
		psy_audio_SequenceCursor cursor;

		patternlinenavigator_init(&navigator, self->state->pv, FALSE);
		cursor = patternlinenavigator_end(&navigator,
			patternviewstate_cursor(self->state->pv));
		if (self->state->midline) {
			trackergrid_set_cursor(self, cursor);
			trackergrid_center_on_cursor(self);
		}
		else {
			trackergrid_scroll_down(self, cursor, TRUE);
		}
	}
	else {
		PatternColNavigator navigator;
		psy_audio_SequenceCursor cursor;

		patterncolnavigator_init(&navigator, self->state, FALSE);
		cursor = patterncolnavigator_end(&navigator,
			patternviewstate_cursor(self->state->pv));
		trackergrid_set_cursor(self, cursor);
		trackergrid_scroll_right(self, cursor);
	}
}

void trackergrid_prev_col(TrackerGrid* self)
{
	PatternColNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patterncolnavigator_init(&navigator, self->state, TRUE);
	cursor = patterncolnavigator_prev_col(&navigator,
		patternviewstate_cursor(self->state->pv));
	if (patterncolnavigator_wrap(&navigator)) {
		trackergrid_scroll_right(self, cursor);
	}
	else {
		trackergrid_scroll_left(self, cursor);
	}
	trackergrid_set_cursor(self, cursor);
}

void trackergrid_next_col(TrackerGrid* self)
{
	PatternColNavigator navigator;
	psy_audio_SequenceCursor cursor;

	assert(self);

	patterncolnavigator_init(&navigator, self->state, TRUE);
	cursor = patterncolnavigator_next_col(&navigator,
		patternviewstate_cursor(self->state->pv));
	if (patterncolnavigator_wrap(&navigator)) {
		trackergrid_scroll_left(self, cursor);
	}
	else {
		trackergrid_scroll_right(self, cursor);
	}
	trackergrid_set_cursor(self, cursor);
}

void trackergrid_set_default_event(TrackerGrid* self,
	psy_audio_Pattern* defaults, psy_audio_PatternEvent* ev)
{
	psy_audio_PatternNode* node;
	psy_audio_PatternNode* prev;

	assert(self);

	node = psy_audio_pattern_findnode_cursor(defaults, self->state->pv->cursor,
		&prev);
	if (node) {
		psy_audio_PatternEvent* defaultevent;

		defaultevent = psy_audio_patternentry_front(
			psy_audio_patternnode_entry(node));
		if (defaultevent->inst != psy_audio_NOTECOMMANDS_INST_EMPTY) {
			ev->inst = defaultevent->inst;
		}
		if (defaultevent->mach != psy_audio_NOTECOMMANDS_EMPTY) {
			ev->mach = defaultevent->mach;
		}
		if (defaultevent->vol != psy_audio_NOTECOMMANDS_VOL_EMPTY) {
			ev->vol = defaultevent->vol;
		}
	}
}

void trackergrid_row_clear(TrackerGrid* self)
{
	assert(self);

	if (self->state->pv->cursor.column == PATTERNEVENT_COLUMN_NOTE) {
		trackergrid_row_clear_note(self);
	}
	else {
		trackergrid_row_clear_value(self);
	}
}

void trackergrid_row_clear_note(TrackerGrid* self)
{
	psy_audio_Pattern* pattern;

	assert(self);

	pattern = patternviewstate_pattern(self->state->pv);
	if (!pattern || !self->state->pv->undo_redo) {
		return;
	}
	self->prevent_cursor = TRUE;
	psy_undoredo_execute(self->state->pv->undo_redo,
		&removecommand_allocinit(pattern, self->state->pv->cursor,
			self->state->pv->sequence)->command);
	self->prevent_cursor = FALSE;
	trackergrid_advance_line(self);
}

void trackergrid_row_clear_value(TrackerGrid* self)
{
	TrackDef* trackdef;
	TrackColumnDef* coldef;

	assert(self);

	trackdef = trackerconfig_trackdef(self->state->track_config,
		self->state->pv->cursor.channel_);
	coldef = trackdef_column_def(trackdef, self->state->pv->cursor.column);
	trackergrid_input_value(self, coldef->emptyvalue, !ISDIGIT);
}

bool trackergrid_on_note_cmds(TrackerGrid* self, InputHandler* sender)
{
	assert(self);

	if (psy_audio_player_edit_mode(self->state->pv->player)) {
		psy_EventDriverCmd cmd;

		assert(self);

		cmd = inputhandler_cmd(sender);
		if (cmd.id != -1 && trackergrid_insert_note(self, cmd.id)) {
			if (getenv("PSYCLE_RUNTIME_SMOKE")) {
				fprintf(stderr, "psycle: runtime smoke tracker note inserted\n");
				fflush(stderr);
			}
		}
	}
	return FALSE;
}

bool trackergrid_insert_note(TrackerGrid* self, intptr_t note_id)
{
	psy_audio_Pattern* pattern;

	assert(self);

	pattern = patternviewstate_pattern(self->state->pv);
	if (!pattern || !self->state->pv->undo_redo) {
		return 0;
	}
	if (note_id != -1) {
		psy_audio_PatternEvent ev;
		bool insert;

		trackergrid_prevent_pattern_sync(self);
		insert = FALSE;
		if (note_id >= CMD_NOTE_OFF_C_0 && note_id < 255) {
			ev = psy_audio_player_pattern_event(self->player, (uint8_t)note_id);
			ev.note = CMD_NOTE_STOP;
			psy_audio_player_play_event(self->player, &ev, note_id - CMD_NOTE_OFF_C_0);
		}
		else if (note_id == CMD_NOTE_CHORD_END) {
			if (self->state->pv->chord) {
				psy_audio_SequenceCursor cursor;

				cursor = self->state->pv->cursor;
				cursor.channel_ = self->state->pv->chord_begin;
				trackergrid_scroll_left(self, cursor);
				trackergrid_set_cursor(self, cursor);
				trackergrid_advance_line(self);
			}
			self->state->pv->chord = FALSE;
			self->state->pv->chord_begin = 0;
		}
		else if (note_id >= CMD_NOTE_CHORD_C_0 && note_id < CMD_NOTE_STOP) {
			if (!self->state->pv->chord) {
				self->state->pv->chord_begin = self->state->pv->cursor.channel_;
				self->state->pv->chord = TRUE;
			}
			ev = psy_audio_player_pattern_event(self->player,
				(uint8_t)note_id - (uint8_t)CMD_NOTE_CHORD_C_0);
			insert = TRUE;
		}
		else if (note_id < 256) {
			if (note_id == CMD_NOTE_DEFAULT) {
				ev = psy_audio_patterndefaults_event(&self->player->patterndefaults,
					psy_audio_sequencecursor_channel(patternviewstate_cursor(self->state->pv)));
			} else {
				ev = psy_audio_player_pattern_event(self->player, (uint8_t)note_id);
			}
			psy_audio_player_play_event(self->state->pv->player, &ev,
				psy_audio_NOTECOMMANDS_EMPTY);
			insert = TRUE;
		}
		if (insert) {
			psy_undoredo_execute(self->state->pv->undo_redo,
				&insertcommand_allocinit(pattern,
					*patternviewstate_cursor(self->state->pv), ev,
					psy_dsp_beatpos_zero(),
					patternviewstate_sequence(self->state->pv))->command);
			if (self->state->pv->chord != FALSE) {
				trackergrid_next_track(self);
			}
			else {
				trackergrid_advance_line(self);
			}
			if (ev.note < psy_audio_NOTECOMMANDS_RELEASE) {
				self->state->pv->cursor.key = ev.note;
				trackergrid_set_cursor(self, self->state->pv->cursor);
			}
		}
		trackergrid_enable_pattern_sync(self);
		return 1;
	}
	return 0;
}

bool trackergrid_on_midi_cmds(TrackerGrid* self, InputHandler* sender)
{
	psy_audio_Pattern* pattern;

	assert(self);

	if (self->prevent_event_driver) {
		return FALSE;
	}
	pattern = patternviewstate_pattern(self->state->pv);
	if (!pattern || !self->state->pv->undo_redo) {
		return FALSE;
	}
	if (!psy_audio_player_playing(self->player) &&
		psy_audio_player_edit_mode(self->player)) {
		psy_EventDriverCmd cmd;

		assert(self);

		cmd = inputhandler_cmd(sender);
		if (cmd.type == psy_EVENTDRIVER_MIDI) {
			psy_audio_PatternEvent ev;

			psy_audio_patternevent_clear(&ev);
			psy_audio_midiinput_work_input(&self->player->midiinput,
				cmd.midi, &self->state->pv->player->song->machines_, &ev);
			if (ev.note != psy_audio_NOTECOMMANDS_RELEASE ||
				psy_audio_player_recording_noteoff(self->player)) {
				trackergrid_prevent_pattern_sync(self);
				psy_undoredo_execute(self->state->pv->undo_redo,
					&insertcommand_allocinit(pattern,
						self->state->pv->cursor, ev, psy_dsp_beatpos_zero(),
						self->state->pv->sequence)->command);
				trackergrid_advance_line(self);
				if (ev.note < psy_audio_NOTECOMMANDS_RELEASE) {
					self->state->pv->cursor.key = ev.note;
					trackergrid_set_cursor(self, self->state->pv->cursor);
				}
				trackergrid_enable_pattern_sync(self);
			}
			return TRUE;
		}
	}
	return FALSE;
}

void trackergrid_input_value(TrackerGrid* self, uintptr_t newvalue,
	bool isdigit)
{
	psy_audio_PatternEvent ev;
	psy_audio_Pattern* pattern;

	assert(self);

	pattern = patternviewstate_pattern(self->state->pv);
	if (!pattern || !self->state->pv->undo_redo) {
		return;
	}
	ev = psy_audio_pattern_event_at_cursor(pattern,
		*patternviewstate_cursor(self->state->pv));
	if (isdigit) {
		TrackDef* trackdef;

		trackdef = trackerconfig_trackdef(self->state->track_config,
			self->state->pv->cursor.channel_);
		ev = trackdef_setevent_digit(trackdef, self->state->pv->cursor.column,
			self->state->pv->cursor.digit, &ev, newvalue);
	}
	else {
		psy_audio_patternevent_setvalue(&ev, self->state->pv->cursor.column,
			newvalue);
	}
	trackergrid_prevent_pattern_sync(self);
	psy_undoredo_execute(self->state->pv->undo_redo,
		&insertcommand_allocinit(pattern,
			self->state->pv->cursor, ev, psy_dsp_beatpos_zero(),
			self->state->pv->sequence)->command);
	if (self->effcursor_always_down) {
		trackergrid_advance_line(self);
	}
	else {
		TrackDef* trackdef;
		TrackColumnDef* columndef;

		trackdef = trackerconfig_trackdef(self->state->track_config,
			self->state->pv->cursor.channel_);
		columndef = trackdef_column_def(trackdef,
			self->state->pv->cursor.column);
		if (!isdigit) {
			if (columndef->wrapclearcolumn == PATTERNEVENT_COLUMN_NONE) {
				trackergrid_next_col(self);
			}
			else {
				self->state->pv->cursor.digit = 0;
				self->state->pv->cursor.column = columndef->wrapclearcolumn;
				trackergrid_advance_line(self);
			}
		}
		else if (self->state->pv->cursor.digit + 1 >= columndef->numdigits) {
			if (columndef->wrapeditcolumn == PATTERNEVENT_COLUMN_NONE) {
				trackergrid_next_col(self);
			}
			else {
				self->state->pv->cursor.digit = 0;
				self->state->pv->cursor.column = columndef->wrapeditcolumn;
				trackergrid_advance_line(self);
			}
		}
		else {
			trackergrid_next_col(self);
		}
	}
	trackergrid_invalidate_cursor(self);
	trackergrid_enable_pattern_sync(self);
}

void trackergrid_invalidate_cursor(TrackerGrid* self)
{
	assert(self);

	if (psy_ui_component_draw_visible(trackergrid_base(self))) {
		trackergrid_invalidate_internal_cursor(self, self->old_cursor);
		trackergrid_invalidate_internal_cursor(self, self->state->pv->cursor);
	}
	self->old_cursor = self->state->pv->cursor;
}

void trackergrid_invalidate_internal_cursor(TrackerGrid* self,
	psy_audio_SequenceCursor cursor)
{
	psy_ui_Component* column;

	assert(self);

	column = psy_ui_component_at(trackergrid_base(self), cursor.channel_);
	if (column) {
		psy_ui_component_invalidate_rect(column,
			psy_ui_realrectangle_make(
				psy_ui_realpoint_make(
					0.0, trackerstate_beat_to_px(self->state,
						(patternviewstate_single_mode(self->state->pv))
						? psy_audio_sequencecursor_offset(&cursor)
						: psy_audio_sequencecursor_offset_abs(&cursor,
							self->state->pv->sequence))),
				psy_ui_realsize_make(
					trackerstate_trackwidth(self->state, cursor.channel_,
						psy_ui_component_textmetric(trackergrid_base(self))),
					self->state->beat_convert.line_px)));
	}
}

void trackergrid_invalidate_playbar(TrackerGrid* self)
{
	double minval;
	double maxval;
	psy_ui_RealSize size;

	assert(self);

	trackerstate_playbar_update_range(self->state,
		&self->state->pv->player->sequencer.hostseqtime,
		&minval, &maxval);
	if (maxval == 0.0) {
		return;
	}
	size = psy_ui_component_scroll_size_px(trackergrid_base(self));
	psy_ui_component_invalidate_rect(trackergrid_base(self),
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(0.0, minval),
			psy_ui_realsize_make(
				size.width,
				maxval - minval + self->state->beat_convert.line_px)));
}

void trackergrid_on_scroll(TrackerGrid* self, psy_ui_Component* sender)
{
	assert(self);

	if (self->state->midline) {
		trackergrid_clear_midline(self);
	}
}

void trackergrid_clear_midline(TrackerGrid* self)
{
	psy_ui_RealSize size;
	intptr_t visilines;

	assert(self);

	size = psy_ui_component_clientsize_px(trackergrid_base(self));
	self->state->midline = FALSE;
	visilines = (intptr_t)(size.height / self->state->beat_convert.line_px);
	psy_ui_component_invalidate_rect(trackergrid_base(self),
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(
				psy_ui_component_scroll_left_px(trackergrid_base(self)),
				((visilines) / 2 - 1) * self->state->beat_convert.line_px +
				psy_ui_component_scroll_top_px(trackergrid_base(self))),
			psy_ui_realsize_make(size.width,
				self->state->beat_convert.line_px)));
	psy_ui_component_update(trackergrid_base(self));
	self->state->midline = TRUE;
	psy_ui_component_invalidate_rect(trackergrid_base(self),
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(
				psy_ui_component_scroll_left_px(trackergrid_base(self)),
				(visilines / 2 - 2) * self->state->beat_convert.line_px +
				psy_ui_component_scroll_top_px(trackergrid_base(self))),
			psy_ui_realsize_make(size.width,
				self->state->beat_convert.line_px * 4)));
}

void trackergrid_center_on_cursor(TrackerGrid* self)
{
	intptr_t line;
	intptr_t visilines;
	psy_ui_RealSize size;

	assert(self);

	if (patternviewstate_single_mode(self->state->pv)) {
		line = beatline_beat_to_line(&self->state->pv->beat_line,
			psy_audio_sequencecursor_offset(&self->state->pv->cursor));
	}
	else {
		line = beatline_beat_to_line(&self->state->pv->beat_line,
			self->state->pv->cursor.offset);
	}
	size = psy_ui_component_scroll_size_px(psy_ui_component_parent(
		trackergrid_base(self)));
	visilines = (intptr_t)(size.height / self->state->beat_convert.line_px);
	psy_ui_component_set_scroll_top_px(trackergrid_base(self),
		-(visilines / 2 - line) * self->state->beat_convert.line_px);
}

void trackergrid_set_center_mode(TrackerGrid* self, intptr_t mode)
{
	assert(self);

	self->state->midline = (mode != FALSE);
	if (mode) {
		psy_ui_component_set_overflow(trackergrid_base(self), (psy_ui_Overflow)
			(psy_ui_OVERFLOW_SCROLL | psy_ui_OVERFLOW_VSCROLLCENTER));
		trackergrid_center_on_cursor(self);
	}
	else {
		psy_ui_component_set_overflow(trackergrid_base(self), psy_ui_OVERFLOW_SCROLL);
		psy_ui_component_set_scroll_top_px(trackergrid_base(self), 0.0);
	}
}

void trackergrid_on_mouse_down(TrackerGrid* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	self->down = TRUE;
	if (trackdrag_active(&self->state->track_config->resize)) {
		psy_signal_emit(&self->signal_col_resize, self, 0);
	}
	else if (patternviewstate_sequence(self->state->pv) &&
		psy_ui_mouseevent_button(ev) == 1) {
		self->last_drag_cursor = self->state->pv->selection.block.drag_base_;
		patternselection_disable(&self->state->pv->selection);
		patternselection_set(&self->state->pv->selection,
			self->last_drag_cursor, self->last_drag_cursor);
		if (!psy_ui_component_has_focus(trackergrid_base(self))) {
			psy_ui_component_set_focus(trackergrid_base(self));
		}
		psy_ui_component_capture(trackergrid_base(self));
	}
}

void trackergrid_on_mouse_move(TrackerGrid* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) != 1) {
		return;
	}
	if (trackdrag_active(&self->state->track_config->resize)) {
		psy_signal_emit(&self->signal_col_resize, self, 0);
		psy_ui_mouseevent_stop_propagation(ev);
	}
	else {
		psy_audio_SequenceCursor cursor;
		TrackerColumn* column;
		uintptr_t index;

		column = (TrackerColumn*)psy_ui_component_intersect(
			trackergrid_base(self), psy_ui_mouseevent_pt(ev), &index);
		if (column) {
			cursor = trackerstate_make_cursor(self->state,
				psy_ui_mouseevent_pt(ev), column->track,
				psy_ui_component_textmetric(trackergrid_base(self)));
			if (!psy_audio_sequencecursor_equal(&cursor,
				&self->last_drag_cursor)) {
				psy_ui_RealRectangle rc;
				psy_ui_RealRectangle rc_curr;

				rc = trackergrid_selection_bounds(self);
				if (!psy_audio_blockselection_valid(
					&self->state->pv->selection.block)) {
					patternselection_start_drag(&self->state->pv->selection,
						cursor);
				}
				else {
					trackergrid_drag_selection(self, cursor);
				}
				rc_curr = trackergrid_selection_bounds(self);
				psy_ui_realrectangle_union(&rc, &rc_curr);
				psy_ui_component_invalidate_rect(trackergrid_base(self), rc);
				self->last_drag_cursor = cursor;
			}
		}
	}
}

void trackergrid_on_mouse_up(TrackerGrid* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_release_capture(trackergrid_base(self));
	if (!self->down || psy_ui_mouseevent_button(ev) != 1) {
		self->down = FALSE;
		return;
	}
	self->down = FALSE;
	if (trackdrag_active(&self->state->track_config->resize)) {
		trackdrag_stop(&self->state->track_config->resize);
		psy_signal_emit(&self->signal_col_resize, self, 0);
	}
	else if (!psy_audio_blockselection_valid(&self->state->pv->selection.block)) {
		trackergrid_set_cursor(self, self->state->pv->selection.block.drag_base_);
	}
	patternselection_stop_drag(&self->state->pv->selection);
}

psy_ui_RealRectangle trackergrid_selection_bounds(TrackerGrid* self)
{
	psy_ui_RealRectangle rv;
	psy_audio_Sequence* sequence;

	assert(self);

	psy_ui_realrectangle_init(&rv);
	sequence = patternviewstate_sequence(self->state->pv);
	if (!sequence) {
		return rv;
	}
	if (psy_audio_blockselection_valid(&self->state->pv->selection.block)) {
		psy_audio_SequenceEntry* top_entry;
		psy_audio_SequenceEntry* bottom_entry;

		psy_dsp_beatpos_t top_abs;
		psy_dsp_beatpos_t bottom_abs;

		top_entry = psy_audio_sequence_entry(sequence,
			self->state->pv->selection.block.topleft.order_index);
		if (!top_entry) {
			return rv;
		}
		bottom_entry = psy_audio_sequence_entry(sequence,
			self->state->pv->selection.block.bottomright.order_index);
		if (!bottom_entry) {
			return rv;
		}
		top_abs = psy_dsp_beatpos_add(
			psy_audio_sequenceentry_offset(top_entry),
			psy_audio_sequencecursor_offset(
				&self->state->pv->selection.block.topleft));
		bottom_abs = psy_dsp_beatpos_add(
			psy_audio_sequenceentry_offset(bottom_entry),
			psy_audio_sequencecursor_offset(
				&self->state->pv->selection.block.bottomright));
		if (patternviewstate_single_mode(self->state->pv)) {
			psy_audio_SequenceEntry* cursor_entry;

			cursor_entry = psy_audio_sequence_entry(sequence,
				self->state->pv->cursor.order_index);
			if (!cursor_entry) {
				return rv;
			}
			rv.top = trackerstate_beat_to_px(self->state,
				psy_dsp_beatpos_sub(top_abs,
					psy_audio_sequenceentry_offset(cursor_entry)));
			rv.bottom = trackerstate_beat_to_px(self->state,
				psy_dsp_beatpos_sub(bottom_abs,
					psy_audio_sequenceentry_offset(cursor_entry)));
		}
		else {
			rv.top = trackerstate_beat_to_px(self->state, top_abs);
			rv.bottom = trackerstate_beat_to_px(self->state, bottom_abs);
		}
		rv.top = psy_max(0.0, rv.top);
		rv.bottom = psy_max(0.0, rv.bottom);
		if (rv.bottom < rv.top) {
			rv.bottom = rv.top;
		}
		if (rv.bottom > rv.top) {
			TrackerColumn* column;

			/* left */
			column = (TrackerColumn*)psy_table_at(&self->columns,
				self->state->pv->selection.block.topleft.channel_);
			if (column) {
				psy_ui_RealRectangle position;

				position = psy_ui_component_position(trackercolumn_base(
					column));
				rv.left = position.left;
			}
			/* right */
			column = (TrackerColumn*)psy_table_at(&self->columns,
				self->state->pv->selection.block.bottomright.channel_);
			if (column) {
				psy_ui_RealRectangle position;

				position = psy_ui_component_position(trackercolumn_base(
					column));
				rv.right = position.left;
			}
			else if (self->state->pv->selection.block.bottomright.channel_ > 0) {
				column = (TrackerColumn*)psy_table_at(&self->columns,
					self->state->pv->selection.block.bottomright.channel_ - 1);
				if (column) {
					psy_ui_RealRectangle position;

					position = psy_ui_component_position(trackercolumn_base(
						column));
					rv.right = position.right;
				}
			}
		}
	}
	return rv;
}

void trackergrid_on_mouse_doubleclick(TrackerGrid* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 1) {
		inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "edit",
			psy_eventdrivercmd_make_cmd(CMD_SELECTCOL));
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void trackergrid_drag_selection(TrackerGrid* self,
	psy_audio_SequenceCursor cursor)
{
	bool restore_midline;

	assert(self);

	restore_midline = self->state->midline;
	self->state->midline = FALSE;
	patternselection_drag(&self->state->pv->selection, cursor,
		self->state->pv->sequence);
	if (psy_dsp_beatpos_less(psy_audio_sequencecursor_offset(&cursor),
		psy_audio_sequencecursor_offset(&self->last_drag_cursor))) {
		trackergrid_scroll_up(self, cursor);
	}
	else {
		trackergrid_scroll_down(self, cursor, FALSE);
	}
	if (psy_audio_sequencecursor_channel(&cursor) <
		psy_audio_sequencecursor_channel(&self->last_drag_cursor)) {
		trackergrid_scroll_left(self, cursor);
	}
	else {
		trackergrid_scroll_right(self, cursor);
	}
	self->state->midline = restore_midline;
}

void trackergrid_enable_pattern_sync(TrackerGrid* self)
{
	assert(self);

	trackergrid_reset_pattern_sync(self);
}

void trackergrid_prevent_pattern_sync(TrackerGrid* self)
{
	assert(self);

	trackergrid_reset_pattern_sync(self);
}

void trackergrid_reset_pattern_sync(TrackerGrid* self)
{
	psy_audio_Pattern* pattern;

	assert(self);

	pattern = patternviewstate_pattern(self->state->pv);
	if (pattern) {
		self->component.opcount = psy_audio_pattern_opcount(pattern);
	}
	else {
		self->component.opcount = 0;
	}
}

bool trackergrid_on_edt_cmds(TrackerGrid* self, InputHandler* sender)
{
	assert(self);

	psy_EventDriverCmd cmd;

	cmd = inputhandler_cmd(sender);
	if (cmd.id == CMD_ROWCLEAR) {
		trackergrid_row_clear(self);
		return TRUE;
	}
	if (patterncmds_handle_edt_command(&self->cmds, cmd)) {
		if (cmd.id == CMD_ROWDELETE) {
			if (patternviewstate_ft2delete(self->state->pv)) {
				trackergrid_prev_line(self);
			}
		}
		else {
			psy_ui_component_invalidate(trackergrid_base(self));
		}
		return TRUE;
	}
	return FALSE;
}

bool trackergrid_on_tracker_cmds(TrackerGrid* self, InputHandler* sender)
{
	psy_EventDriverCmd cmd;

	assert(self);

	cmd = inputhandler_cmd(sender);
	switch (cmd.id) {
	case CMD_NAVUP:
		if (patternviewstate_move_cursor_one_step(self->state->pv)) {
			trackergrid_prev_lines(self, 1, 0);
		}
		else {
			trackergrid_prev_line(self);
		}
		return TRUE;
	case CMD_NAVPAGEUP:
		trackergrid_prev_lines(self, patternviewstate_curr_pgup_down_step(
			self->state->pv), FALSE);
		return TRUE;
	case CMD_NAVDOWN:
		if (patternviewstate_move_cursor_one_step(self->state->pv)) {
			trackergrid_advance_lines(self, 1, FALSE);
		}
		else {
			trackergrid_advance_line(self);
		}
		return TRUE;
	case CMD_NAVPAGEDOWN:
		trackergrid_advance_lines(self, patternviewstate_curr_pgup_down_step(
			self->state->pv), FALSE);
		return TRUE;
	case CMD_NAVLEFT:
		trackergrid_prev_col(self);
		return TRUE;
	case CMD_NAVRIGHT:
		trackergrid_next_col(self);
		return TRUE;
	case CMD_NAVTOP:
		trackergrid_home(self);
		return TRUE;
	case CMD_NAVBOTTOM:
		trackergrid_end(self);
		return TRUE;
	case CMD_COLUMNPREV:
		trackergrid_prev_track(self);
		return TRUE;
	case CMD_COLUMNNEXT:
		trackergrid_next_track(self);
		if (getenv("PSYCLE_RUNTIME_SMOKE")) {
			fprintf(stderr, "psycle: runtime smoke tracker column next\n");
			fflush(stderr);
		}
		return TRUE;
	case CMD_DIGIT0:
	case CMD_DIGIT1:
	case CMD_DIGIT2:
	case CMD_DIGIT3:
	case CMD_DIGIT4:
	case CMD_DIGIT5:
	case CMD_DIGIT6:
	case CMD_DIGIT7:
	case CMD_DIGIT8:
	case CMD_DIGIT9:
	case CMD_DIGITA:
	case CMD_DIGITB:
	case CMD_DIGITC:
	case CMD_DIGITD:
	case CMD_DIGITE:
	case CMD_DIGITF:
		if (psy_audio_player_edit_mode(self->state->pv->player) &&
			(self->state->pv->cursor.column != PATTERNEVENT_COLUMN_NOTE)) {
			trackergrid_input_value(self, (uint8_t)(cmd.id - CMD_DIGIT0),
				ISDIGIT);
			return TRUE;
		}
		break;
	default:
		break;
	}
	return FALSE;
}

void trackergrid_show_empty_data(TrackerGrid* self, int showstate)
{
	assert(self);

	self->state->show_empty_data = showstate;
	psy_ui_component_align(trackergrid_base(self));
}

void trackergrid_build(TrackerGrid* self)
{
	uintptr_t t;
	uintptr_t num_tracks;

	assert(self);

	psy_ui_component_clear(trackergrid_base(self));
	psy_table_clear(&self->columns);
	num_tracks = patternviewstate_num_song_tracks(self->state->pv);
	for (t = 0; t < num_tracks; ++t) {
		TrackerColumn* column;

		column = trackercolumn_alloc_init(trackergrid_base(self), t,
			self->state, psy_audio_player_sequencer(self->player));
		if (column) {
			psy_table_insert(&self->columns, t, column);
		}
	}
	psy_ui_component_align(trackergrid_base(self));
}

void trackergrid_set_cursor(TrackerGrid* self, psy_audio_SequenceCursor cursor)
{
	assert(self);

	if (patternviewstate_sequence(self->state->pv)) {
		psy_audio_sequence_set_cursor(patternviewstate_sequence(
			self->state->pv), cursor);
	}
}

void trackergrid_on_timer(TrackerGrid* self, uintptr_t id)
{
	assert(self);

	if (trackergrid_check_update(self)) {
		psy_ui_component_align(trackergrid_base(self));
		psy_ui_component_invalidate(trackergrid_base(self));
	}
}

#endif /* PSYCLE_USE_TRACKERVIEW */
