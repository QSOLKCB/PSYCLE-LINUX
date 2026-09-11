/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "stepsequencerview.h"

#ifdef PSYCLE_USE_STEPSEQUENCER

/* host */
#include "patternhostcmds.h"
#include "styles.h"
/* audio */
#include <exclusivelock.h>
#include <plugin_interface.h>
#include <sequencecmds.h>
#include <songio.h>
/* platform */
#include "../../detail/portable.h"


#define NUMSTEPS 16

/* LastStepMachineParam */

/* prototypes */
static int laststepmachineparam_type(LastStepMachineParam* self)
{
	return MPF_STATE | MPF_SMALL;
}
static void laststepmachineparam_tweak(LastStepMachineParam* self, double val);
static double laststepmachineparam_norm_value(LastStepMachineParam*);
static void laststepmachineparam_range(LastStepMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int laststepmachineparam_name(LastStepMachineParam*, char* text);
static int laststepmachineparam_describe(LastStepMachineParam*, char* text);

/* vtable */
static MachineParamVtable laststepmachineparam_vtable;
static bool laststepmachineparam_vtable_initialized = FALSE;

static void laststepmachineparam_vtable_init(LastStepMachineParam* self)
{
	if (!laststepmachineparam_vtable_initialized) {
		laststepmachineparam_vtable = *(self->machineparam.vtable);
		laststepmachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			laststepmachineparam_norm_value;
		laststepmachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			laststepmachineparam_tweak;
		laststepmachineparam_vtable.range =
			(fp_machineparam_range)
			laststepmachineparam_range;
		laststepmachineparam_vtable.type =
			(fp_machineparam_type)
			laststepmachineparam_type;
		laststepmachineparam_vtable.name =
			(fp_machineparam_name)
			laststepmachineparam_name;
		laststepmachineparam_vtable.describe =
			(fp_machineparam_describe)
			laststepmachineparam_describe;
		laststepmachineparam_vtable_initialized = TRUE;
	}
	self->machineparam.vtable = &laststepmachineparam_vtable;
}

/* implementation */
void laststepmachineparam_init(LastStepMachineParam* self,
	StepSequencerLastStepState* state)
{
	psy_audio_machineparam_init(&self->machineparam);
	laststepmachineparam_vtable_init(self);	
	self->state = state;	
}

void laststepmachineparam_dispose(LastStepMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
}

void laststepmachineparam_tweak(LastStepMachineParam* self, double val)
{		
	assert(self);

	if (stepsequencerlaststepstate_pattern(self->state)) {
		psy_dsp_beatpos_t length;		
				
		length = psy_dsp_beatpos_quantize(psy_dsp_beatpos_make_real(
				val * 64.0, psy_dsp_DEFAULT_PPQ),
			self->state->workspace->song->sequence_.cursor.lpb);
		psy_audio_pattern_set_length(stepsequencerlaststepstate_pattern(
			self->state), length);
	}	
}

double laststepmachineparam_norm_value(LastStepMachineParam* self)
{		
	if (stepsequencerlaststepstate_pattern(self->state)) {
		return psy_dsp_beatpos_real(psy_audio_pattern_length(
			stepsequencerlaststepstate_pattern(self->state))) *
			self->state->workspace->song->sequence_.cursor.lpb /
			64;
	}
	return 0;
}

int laststepmachineparam_name(LastStepMachineParam* self, char* text)
{		
	psy_snprintf(text, 128, "%s", "Last Step");	
	return 1;
}

void laststepmachineparam_range(LastStepMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{			
	*minval = 0;
	*maxval = 64;	
}

int laststepmachineparam_describe(LastStepMachineParam* self, char* text)
{	
	psy_snprintf(text, 128, "L %d", 
		(int)psy_audio_machineparam_scaled_value(&self->machineparam));	
	return 1;
}

/* StepSequencerState */

static void stepsequencerstate__dispose(StepSequencerState* self) { }
static void stepsequencerstate__enter(StepSequencerState* self) { }
static StepSequencerState* stepsequencerstate__tile_press(
	StepSequencerState* self, uintptr_t step) { return NULL; }
static StepSequencerState* stepsequencerstate__shift_press(
	StepSequencerState* self) { return NULL; }
static void stepsequencerstate__update(StepSequencerState* self) { }
static void stepsequencerstate__update_playline(StepSequencerState* self) { }

static psy_audio_MachineParam* stepsequencerstate__param(StepSequencerState*
	self, uintptr_t index)
{
	return NULL;
}

static StepSequencerStateVtable stepsequencerstate_vtable;
static bool stepsequencerstate_vtable_initialized = FALSE;

static void stepsequencerstate_vtable_init(StepSequencerState* self)
{
	if (!stepsequencerstate_vtable_initialized) {
		stepsequencerstate_vtable.dispose = stepsequencerstate__dispose;
		stepsequencerstate_vtable.enter = stepsequencerstate__enter;
		stepsequencerstate_vtable.tile_press = stepsequencerstate__tile_press;
		stepsequencerstate_vtable.shift_press = stepsequencerstate__shift_press;
		stepsequencerstate_vtable.update = stepsequencerstate__update;
		stepsequencerstate_vtable.update_playline =
			stepsequencerstate__update_playline;
		stepsequencerstate_vtable.param = stepsequencerstate__param;		
		stepsequencerstate_vtable_initialized = TRUE;
	}
	self->vtable = &stepsequencerstate_vtable;
}

void stepsequencerstate_init(StepSequencerState* self)
{
	assert(self);
	
	stepsequencerstate_vtable_init(self);	
}


/* StepSequencerNoteState */

/* prototypes */
static void stepsequencernotestate_dispose(StepSequencerNoteState*);
static void stepsequencernotestate_enter(StepSequencerNoteState*);
StepSequencerState* stepsequencernotestate_tile_press(StepSequencerNoteState*,
	uintptr_t step);
static StepSequencerState* stepsequencernotestate_shift_press(
	StepSequencerNoteState*);
static void stepsequencernotestate_update(StepSequencerNoteState*);
static void stepsequencernotestate_update_playline(StepSequencerNoteState*);
static psy_audio_MachineParam* stepsequencernotestate_param(
	StepSequencerNoteState*, uintptr_t index);
static void stepsequencernotestate_init_step_groups(StepSequencerNoteState*);
static void stepsequencernotestate_stepgroup_tweak(StepSequencerNoteState*,
	psy_audio_ChoiceMachineParam* sender, double value);

/* vtable */
static StepSequencerStateVtable stepsequencernotestate_vtable;
static bool stepsequencernotestate_vtable_initialized = FALSE;

static void stepsequencernotestate_vtable_init(StepSequencerNoteState* self)
{
	if (!stepsequencernotestate_vtable_initialized) {		
		stepsequencernotestate_vtable = *(self->state.vtable);
		stepsequencernotestate_vtable.dispose =
			(fp_stepsequencerstate)
			stepsequencernotestate_dispose;
		stepsequencernotestate_vtable.enter =
			(fp_stepsequencerstate)
			stepsequencernotestate_enter;
		stepsequencernotestate_vtable.tile_press =
			(fp_stepsequencerstate_tile_press)
			stepsequencernotestate_tile_press;
		stepsequencernotestate_vtable.shift_press =
			(fp_stepsequencerstate_shift_press)
			stepsequencernotestate_shift_press;
		stepsequencernotestate_vtable.update =
			(fp_stepsequencerstate)
			stepsequencernotestate_update;		
		stepsequencernotestate_vtable.update_playline =
			(fp_stepsequencerstate)
			stepsequencernotestate_update_playline;
		stepsequencernotestate_vtable.param =
			(fp_stepsequencerstate_param)
			stepsequencernotestate_param;		
		stepsequencernotestate_vtable_initialized = TRUE;
	}
	self->state.vtable = &stepsequencernotestate_vtable;
}

/* implementation */
void stepsequencernotestate_init(StepSequencerNoteState* self,
	StepsequencerStates* states, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	stepsequencerstate_init(&self->state);
	stepsequencernotestate_vtable_init(self);	
	self->tiles = NULL;	
	self->workspace = workspace;
	self->stepgroup = 0;
	self->states = states;
	stepsequencernotestate_init_step_groups(self);	
	self->note = 48;
	psy_audio_intmachineparam_init(&self->param_note,
		"Notes", "Notes", MPF_STATE | MPF_SMALL,
		&self->note, 0, psy_audio_NOTECOMMANDS_B9);	
	psy_signal_connect(&self->param_stepgroup.machineparam.signal_tweak,
		self, stepsequencernotestate_stepgroup_tweak);	
}

void stepsequencernotestate_init_step_groups(StepSequencerNoteState* self)
{
	uintptr_t group;
	uintptr_t num_groups;
	
	assert(self);
	
	num_groups = 64;
	psy_audio_choicemachineparam_init(&self->param_stepgroup,
		"Step Group", "group", MPF_STATE | MPF_SMALL,
		(int32_t*)&self->stepgroup, 0, num_groups - 1);
	for (group = 0; group < num_groups; ++group) {
		char str[64];
		
		psy_snprintf(str, 64, "%d - %d", group * NUMSTEPS,
			(group + 1) * NUMSTEPS - 1);
		psy_audio_choicemachineparam_set_description(&self->param_stepgroup,
			group, str);
	}	
}

void stepsequencernotestate_dispose(StepSequencerNoteState* self)
{
	assert(self);
	
	psy_audio_choicemachineparam_dispose(&self->param_stepgroup);
	psy_audio_intmachineparam_dispose(&self->param_note);
	stepsequencerstate__dispose(&self->state);
}

void stepsequencernotestate_enter(StepSequencerNoteState* self)
{	
	if (self->tiles) {
		stepsequencertiles_disable_shift(self->tiles);
		stepsequencernotestate_update(self);	
	}
}

psy_audio_MachineParam* stepsequencernotestate_param(StepSequencerNoteState*
	self, uintptr_t index)
{
	switch (index) {
	case 0:
		return &self->param_note.machineparam;
	case 1:
		return &self->param_stepgroup.machineparam;
	default:
		return NULL;
	}		
}

psy_audio_Pattern* stepsequencernotestate_pattern(StepSequencerNoteState* self)
{
	assert(self);

	if (!self->workspace->song) {
		return NULL;
	}
	return psy_audio_sequence_pattern(&self->workspace->song->sequence_,
		self->workspace->song->sequence_.cursor.order_index);
}

void stepsequencernotestate_update(StepSequencerNoteState* self)
{		
	psy_audio_Song* song;

	assert(self);

	if (!self->tiles) {
		return;
	}
	song = workspace_song(self->workspace);
	if (song) {	
		psy_audio_Pattern* pattern;
		psy_audio_SequenceCursor cursor;
		psy_audio_PatternNode* curr;		
		uintptr_t i;
		uintptr_t line;
		uintptr_t step_row;
		psy_audio_HostSequencerTime host_time;
		uintptr_t pattern_line;		
		uintptr_t play_line;

		host_time = self->workspace->player_.sequencer.hostseqtime;
		cursor = song->sequence_.cursor;
		pattern_line = (uintptr_t)(psy_dsp_beatpos_real(cursor.offset) *
			cursor.lpb);
		play_line = (uintptr_t)(psy_dsp_beatpos_real(
			host_time.currplaycursor.offset) * cursor.lpb);
		step_row = pattern_line / NUMSTEPS;
		line = step_row * NUMSTEPS;
		for (i = 0; i < NUMSTEPS; ++i, ++line) {
			StepSequencerTile* tile;

			tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles, i);
			if (tile) {
				if (psy_audio_hostsequencertime_playing(&host_time) &&
						line == play_line) {
					stepsequencertile_play(tile);
				} else {
					stepsequencertile_reset_play(tile);
				}
				stepsequencertile_turn_off(tile);
			}
		}		
		pattern = psy_audio_sequence_pattern(&song->sequence_,
			cursor.order_index);
		if (!pattern) {
			return;
		}
		curr = psy_audio_pattern_begin(pattern);
		while (curr) {						
			StepSequencerTile* tile;
			psy_audio_PatternEntry* entry;			
			uintptr_t curr_step;		
			
			entry = psy_audio_patternnode_entry(curr);			
			line = psy_dsp_beatpos_line(psy_audio_patternentry_offset(entry),
				psy_audio_sequencecursor_lpb(&cursor));
			curr_step = line % NUMSTEPS;
			tile = NULL;
			if (psy_audio_patternentry_track(entry) == psy_audio_sequencecursor_channel(&cursor)) {
				uintptr_t start;

				start = step_row * NUMSTEPS;
				if (line >= start && line < start + NUMSTEPS) {
					tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles,
						curr_step);
					if (tile) {
						stepsequencertile_turn_on(tile);
					}
				}
			}
			psy_audio_patternnode_next(&curr);
		}
	}	
	if (self->workspace->song) {
		uintptr_t group;
		psy_audio_SequenceCursor cursor;
		
		cursor = self->workspace->song->sequence_.cursor;
		group = (uintptr_t)(psy_dsp_beatpos_real(psy_audio_sequencecursor_offset(&cursor)) /
			NUMSTEPS * cursor.lpb);		
		self->stepgroup = psy_min(group, 63);		
	}
}

void stepsequencernotestate_update_playline(StepSequencerNoteState* self)
{
	psy_audio_Song* song;

	assert(self);

	song = workspace_song(self->workspace);
	if (song) {		
		psy_audio_SequenceCursor cursor;		
		uintptr_t linestart;
		uintptr_t steprow;
		psy_audio_HostSequencerTime host_time;
		StepSequencerTile* tile;

		host_time = self->workspace->player_.sequencer.hostseqtime;
		cursor = song->sequence_.cursor;
		steprow = psy_audio_sequencecursor_line(&cursor) / NUMSTEPS;
		linestart = steprow * NUMSTEPS;
		if (psy_audio_hostsequencertime_playing(&host_time)) {
			tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles,
				psy_audio_sequencecursor_line(&host_time.currplaycursor) -			
				linestart);
			if (tile) {
				stepsequencertile_play(tile);
			}
		}
		tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles,
			psy_audio_sequencecursor_line(&host_time.lastplaycursor) -			
			linestart);
		if (tile) {
			stepsequencertile_reset_play(tile);
		}
	}
}

/*
** Checks and updates the current pattern.
** Called by stepsequencernotestate_update_positions
** return true if current pattern changed
*/
bool stepsequencernotestate_update_pattern(StepSequencerNoteState* self)
{
	psy_audio_Song* song;
	
	assert(self);

	song = self->workspace->song;
	if (!song) {
		return FALSE;
	}
	return !psy_audio_orderindex_equal(
		&song->sequence_.cursor.order_index,
		song->sequence_.lastcursor.order_index);
}

StepSequencerState* stepsequencernotestate_tile_press(
	StepSequencerNoteState* self, uintptr_t step)
{
	psy_audio_Song* song;	
		
	assert(self);
			
	if (step == psy_INDEX_INVALID) {
		return NULL;
	}
	song = workspace_song(self->workspace);
	if (song) {		
		psy_audio_Pattern* pattern;		
		psy_audio_SequenceCursor cursor;
		psy_audio_PatternEvent patternevent;		
		uintptr_t steprow;
				
		cursor = song->sequence_.cursor;
		steprow = psy_audio_sequencecursor_line(&cursor) / NUMSTEPS;
		step += steprow * NUMSTEPS;
		cursor.column = 0;
		cursor.offset = psy_dsp_beatpos_make_real(
			step / (double)cursor.lpb,
			psy_dsp_DEFAULT_PPQ);
		pattern = psy_audio_sequence_pattern(&song->sequence_,
			cursor.order_index);
		if (!pattern) {
			return NULL;
		}
		patternevent = psy_audio_pattern_event_at_cursor(pattern, cursor);		
		if (psy_audio_patternevent_empty(&patternevent)) {						
			psy_audio_PatternEvent event;

			psy_audio_patternevent_clear(&event);
			event.note = (uint8_t)psy_audio_machineparam_scaled_value(
				&self->param_note.machineparam);			
			event.inst = (uint16_t)psy_audio_instruments_selected(
				&song->instruments_).subslot;
			event.mach = (uint8_t)psy_audio_machines_selected(&song->machines_);
			event = psy_audio_patterndefaults_fill_event(
				&workspace_player(self->workspace)->patterndefaults,
				cursor.channel_, event);
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&insertcommand_allocinit(pattern, cursor, event,
					psy_dsp_beatpos_zero(),
					&song->sequence_)->command);
		} else {
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&removecommand_allocinit(pattern, cursor,
				&song->sequence_)->command);
		}		
	}	
	return NULL;
}

StepSequencerState* stepsequencernotestate_shift_press(
	StepSequencerNoteState* self)
{
	assert(self);
			
	return &self->states->shift_state.state;	
}

void stepsequencernotestate_stepgroup_tweak(StepSequencerNoteState* self,
	psy_audio_ChoiceMachineParam* sender, double value)
{
	assert(self);
	
	if (self->workspace->song) {
		uintptr_t group;	
		psy_audio_SequenceCursor cursor;
	
		group = (uintptr_t)((value * (sender->maxval - sender->minval) + 0.5f) +
			sender->minval);
		cursor = self->workspace->song->sequence_.cursor;
		psy_audio_sequencecursor_set_offset(&cursor,
			psy_dsp_beatpos_make_real(
				(double)group * NUMSTEPS * 1.0/(double)cursor.lpb,
				psy_dsp_DEFAULT_PPQ));
		psy_audio_sequence_set_cursor(&self->workspace->song->sequence_,
			cursor);
	}
}

/* StepSequencerLastStepState */

/* prototypes */
static void stepsequencerlaststepstate_dispose(StepSequencerLastStepState*);
static void stepsequencerlaststepstate_enter(StepSequencerLastStepState*);
static StepSequencerState* stepsequencerlaststepstate_tile_press(
	StepSequencerLastStepState*, uintptr_t step);
static StepSequencerState* stepsequencerlaststepstate_shift_press(
	StepSequencerLastStepState*);
static void stepsequencerlaststepstate_update(StepSequencerLastStepState*);
static psy_audio_MachineParam* stepsequencerlaststepstate_param(
	StepSequencerLastStepState*, uintptr_t index);

static void stepsequencerlaststepstate_init_step_groups(
	StepSequencerLastStepState*);
static void stepsequencerlaststepstate_stepgroup_tweak(
	StepSequencerLastStepState*, psy_audio_ChoiceMachineParam* sender,
	double value);
static uintptr_t stepsequencerlaststepstate_bar(
	const StepSequencerLastStepState*);

/* vtable */
static StepSequencerStateVtable stepsequencerlaststepstate_vtable;
static bool stepsequencerlaststepstate_vtable_initialized = FALSE;

static void stepsequencerlaststepstate_vtable_init(
	StepSequencerLastStepState* self)
{
	if (!stepsequencerlaststepstate_vtable_initialized) {
		stepsequencerlaststepstate_vtable = *(self->state.vtable);
		stepsequencerlaststepstate_vtable.dispose =
			(fp_stepsequencerstate)
			stepsequencerlaststepstate_dispose;
		stepsequencerlaststepstate_vtable.enter =
			(fp_stepsequencerstate)
			stepsequencerlaststepstate_enter;
		stepsequencerlaststepstate_vtable.tile_press =
			(fp_stepsequencerstate_tile_press)
			stepsequencerlaststepstate_tile_press;
		stepsequencerlaststepstate_vtable.shift_press =
			(fp_stepsequencerstate_shift_press)
			stepsequencerlaststepstate_shift_press;
		stepsequencerlaststepstate_vtable.update =
			(fp_stepsequencerstate)
			stepsequencerlaststepstate_update;		
		stepsequencerlaststepstate_vtable.param =
			(fp_stepsequencerstate_param)
			stepsequencerlaststepstate_param;
		stepsequencerlaststepstate_vtable_initialized = TRUE;
	}
	self->state.vtable = &stepsequencerlaststepstate_vtable;
}

/* implementation */
void stepsequencerlaststepstate_init(StepSequencerLastStepState* self,
	StepsequencerStates* states, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	stepsequencerstate_init(&self->state);
	stepsequencerlaststepstate_vtable_init(self);
	self->tiles = NULL;	
	self->workspace = workspace;
	self->stepgroup = 0;
	self->states = states;
	stepsequencerlaststepstate_init_step_groups(self);	
	laststepmachineparam_init(&self->param_note, self);	
	psy_signal_connect(&self->param_stepgroup.machineparam.signal_tweak,
		self, stepsequencerlaststepstate_stepgroup_tweak);	
}

void stepsequencerlaststepstate_init_step_groups(
	StepSequencerLastStepState* self)
{
	uintptr_t group;
	uintptr_t num_groups;
	
	assert(self);
	
	num_groups = 64;
	psy_audio_choicemachineparam_init(&self->param_stepgroup,
		"Step Group", "group", MPF_STATE | MPF_SMALL,
		(int32_t*)&self->stepgroup, 0, num_groups - 1);
	for (group = 0; group < num_groups; ++group) {
		char str[64];
		
		psy_snprintf(str, 64, "%d - %d", group * NUMSTEPS,
			(group + 1) * NUMSTEPS - 1);
		psy_audio_choicemachineparam_set_description(&self->param_stepgroup,
			group, str);
	}	
}

void stepsequencerlaststepstate_dispose(StepSequencerLastStepState* self)
{
	assert(self);
		
	psy_audio_choicemachineparam_dispose(&self->param_stepgroup);
	laststepmachineparam_dispose(&self->param_note);	
	stepsequencerstate__dispose(&self->state);
}

void stepsequencerlaststepstate_enter(StepSequencerLastStepState* self)
{
	assert(self);
		
	stepsequencerlaststepstate_update(self);
	if (self->tiles) {
		psy_ui_component_invalidate(&self->tiles->component);
	}
}

psy_audio_MachineParam* stepsequencerlaststepstate_param(
	StepSequencerLastStepState* self, uintptr_t index)
{
	switch (index) {
	case 0:
		return &self->param_note.machineparam;
	case 1:
		return &self->param_stepgroup.machineparam;
	default:
		return NULL;
	}	
}

psy_audio_Pattern* stepsequencerlaststepstate_pattern(
	StepSequencerLastStepState* self)
{
	assert(self);

	if (!self->workspace->song) {
		return NULL;
	}
	return psy_audio_sequence_pattern(&self->workspace->song->sequence_,
		self->workspace->song->sequence_.cursor.order_index);
}

uintptr_t stepsequencerlaststepstate_bar(const StepSequencerLastStepState*
	self)
{
	assert(self);

	if (workspace_song(self->workspace)) {
		psy_audio_SequenceCursor cursor;		

		cursor = psy_audio_sequence_cursor(psy_audio_song_sequence(
			self->workspace->song));
		return psy_audio_sequencecursor_line(&cursor) / NUMSTEPS;		
	}
	return 0;
}

uintptr_t stepsequencerlaststepstate_last_step(const
	StepSequencerLastStepState* self)
{
	assert(self);
				
	return (uintptr_t)(
		psy_dsp_beatpos_real(psy_audio_pattern_length(
			stepsequencerlaststepstate_pattern(
				((StepSequencerLastStepState*)self)))) *
		self->workspace->song->sequence_.cursor.lpb);
}

void stepsequencerlaststepstate_update(StepSequencerLastStepState* self)
{	
	uintptr_t i;
	uintptr_t bar;
	uintptr_t last;

	assert(self);

	if (!self->tiles) {
		return;
	}					
	bar = stepsequencerlaststepstate_bar(self);
	last = stepsequencerlaststepstate_last_step(self);
	for (i = bar * NUMSTEPS; i < (bar + 1) * NUMSTEPS; ++i) {					
		StepSequencerTile* tile;
		
		tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles,
			i - (bar * NUMSTEPS));
		if (tile) {						
			if (i < last) {
				stepsequencertile_turn_on(tile);
			} else {
				stepsequencertile_turn_off(tile);
			}				
		}
	}
}

StepSequencerState* stepsequencerlaststepstate_tile_press(
	StepSequencerLastStepState* self, uintptr_t step)
{	
	psy_dsp_beatpos_t length;
	uintptr_t lpb;
		
	assert(self);
			
	if (step == psy_INDEX_INVALID) {
		return NULL;
	}	
	lpb = self->workspace->song->sequence_.cursor.lpb;
	length = psy_dsp_beatpos_make_real(
		(stepsequencerlaststepstate_bar(self) * NUMSTEPS +
			step + 1) / (double)lpb,
		psy_dsp_DEFAULT_PPQ);	
	psy_audio_pattern_set_length(stepsequencerlaststepstate_pattern(self),
		length);
	return NULL;
}

StepSequencerState* stepsequencerlaststepstate_shift_press(
	StepSequencerLastStepState* self)
{
	return &self->states->note_state.state;
}

void stepsequencerlaststepstate_stepgroup_tweak( StepSequencerLastStepState*
	self, psy_audio_ChoiceMachineParam* sender, double value)
{
	assert(self);
	
	if (self->workspace->song) {
		uintptr_t group;	
		psy_audio_SequenceCursor cursor;
	
		group = (uintptr_t)((value * (sender->maxval - sender->minval) + 0.5f) +
			sender->minval);
		cursor = self->workspace->song->sequence_.cursor;
		psy_audio_sequencecursor_set_offset(&cursor,
			psy_dsp_beatpos_make_real(
				(double)group * NUMSTEPS * 1.0/(double)cursor.lpb,
				psy_dsp_DEFAULT_PPQ));
		psy_audio_sequence_set_cursor(&self->workspace->song->sequence_,
			cursor);
	}
}

/* StepSequencerClearState */

/* prototypes */
static void stepsequencerclearstate_dispose(StepSequencerClearState*);
static void stepsequencerclearstate_enter(StepSequencerClearState*);
static StepSequencerState* stepsequencerclearstate_tile_press(
	StepSequencerClearState*, uintptr_t step);
static StepSequencerState* stepsequencerclearstate_shift_press(
	StepSequencerClearState*);
static void stepsequencerclearstate_update(StepSequencerClearState*);
static psy_audio_MachineParam* stepsequencerclearstate_param(
	StepSequencerClearState*, uintptr_t index);

static void stepsequencerclearstate_init_step_groups(
	StepSequencerClearState*);
static void stepsequencerclearstate_stepgroup_tweak(
	StepSequencerClearState*, psy_audio_ChoiceMachineParam* sender,
	double value);
static uintptr_t stepsequencerclearstate_bar(
	const StepSequencerClearState*);

/* vtable */
static StepSequencerStateVtable stepsequencerclearstate_vtable;
static bool stepsequencerclearstate_vtable_initialized = FALSE;

static void stepsequencerclearstate_vtable_init(
	StepSequencerClearState* self)
{
	if (!stepsequencerclearstate_vtable_initialized) {
		stepsequencerclearstate_vtable = *(self->state.vtable);
		stepsequencerclearstate_vtable.dispose =
			(fp_stepsequencerstate)
			stepsequencerclearstate_dispose;
		stepsequencerclearstate_vtable.enter =
			(fp_stepsequencerstate)
			stepsequencerclearstate_enter;
		stepsequencerclearstate_vtable.tile_press =
			(fp_stepsequencerstate_tile_press)
			stepsequencerclearstate_tile_press;
		stepsequencerclearstate_vtable.shift_press =
			(fp_stepsequencerstate_shift_press)
			stepsequencerclearstate_shift_press;
		stepsequencerclearstate_vtable.update =
			(fp_stepsequencerstate)
			stepsequencerclearstate_update;		
		stepsequencerclearstate_vtable.param =
			(fp_stepsequencerstate_param)
			stepsequencerclearstate_param;
		stepsequencerclearstate_vtable_initialized = TRUE;
	}
	self->state.vtable = &stepsequencerclearstate_vtable;
}

/* implementation */
void stepsequencerclearstate_init(StepSequencerClearState* self,
	StepsequencerStates* states, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	stepsequencerstate_init(&self->state);
	stepsequencerclearstate_vtable_init(self);
	self->tiles = NULL;
	self->workspace = workspace;
	self->stepgroup = 0;
	self->start = psy_INDEX_INVALID;	
	self->states = states;
	stepsequencerclearstate_init_step_groups(self);	
	// clearmachineparam_init(&self->param_note, self);	
	psy_signal_connect(&self->param_stepgroup.machineparam.signal_tweak,
		self, stepsequencerclearstate_stepgroup_tweak);	
}

void stepsequencerclearstate_init_step_groups(
	StepSequencerClearState* self)
{
	uintptr_t group;
	uintptr_t num_groups;
	
	assert(self);
	
	num_groups = 64;
	psy_audio_choicemachineparam_init(&self->param_stepgroup,
		"Step Group", "group", MPF_STATE | MPF_SMALL,
		(int32_t*)&self->stepgroup, 0, num_groups - 1);
	for (group = 0; group < num_groups; ++group) {
		char str[64];
		
		psy_snprintf(str, 64, "%d - %d", group * NUMSTEPS,
			(group + 1) * NUMSTEPS - 1);
		psy_audio_choicemachineparam_set_description(&self->param_stepgroup,
			group, str);
	}	
}

void stepsequencerclearstate_dispose(StepSequencerClearState* self)
{
	assert(self);
		
	psy_audio_choicemachineparam_dispose(&self->param_stepgroup);
	// clearmachineparam_dispose(&self->param_note);	
	stepsequencerstate__dispose(&self->state);
}

void stepsequencerclearstate_enter(StepSequencerClearState* self)
{
	assert(self);
		
	self->start = psy_INDEX_INVALID;
	stepsequencertiles_disable_shift(self->tiles);
	stepsequencerclearstate_update(self);
	if (self->tiles) {
		psy_ui_component_invalidate(&self->tiles->component);
	}
}

psy_audio_MachineParam* stepsequencerclearstate_param(
	StepSequencerClearState* self, uintptr_t index)
{
	switch (index) {
	case 0:
		return NULL; //&self->param_note.machineparam;
	case 1:
		return &self->param_stepgroup.machineparam;
	default:
		return NULL;
	}	
}

psy_audio_Pattern* stepsequencerclearstate_pattern(
	StepSequencerClearState* self)
{
	assert(self);

	if (!self->workspace->song) {
		return NULL;
	}
	return psy_audio_sequence_pattern(&self->workspace->song->sequence_,
		self->workspace->song->sequence_.cursor.order_index);
}

uintptr_t stepsequencerclearstate_bar(const StepSequencerClearState*
	self)
{
	psy_audio_SequenceCursor cursor;
	uintptr_t line;	
	
	assert(self);
			
	cursor = self->workspace->song->sequence_.cursor;
	line = (uintptr_t)(psy_dsp_beatpos_real(cursor.offset) * cursor.lpb);	
	return (line / NUMSTEPS);
}

uintptr_t stepsequencerclearstate_last_step(const
	StepSequencerClearState* self)
{
	assert(self);
				
	return (uintptr_t)(
		psy_dsp_beatpos_real(psy_audio_pattern_length(
			stepsequencerclearstate_pattern(
				((StepSequencerClearState*)self)))) *
		self->workspace->song->sequence_.cursor.lpb);
}

void stepsequencerclearstate_update(StepSequencerClearState* self)
{		
	psy_audio_Song* song;

	assert(self);

	if (!self->tiles) {
		return;
	}
	song = workspace_song(self->workspace);
	if (song) {	
		psy_audio_Pattern* pattern;
		psy_audio_SequenceCursor cursor;
		psy_audio_PatternNode* curr;		
		uintptr_t i;
		uintptr_t line;
		uintptr_t step_row;
		psy_audio_HostSequencerTime host_time;
		uintptr_t pattern_line;		
		uintptr_t play_line;

		host_time = self->workspace->player_.sequencer.hostseqtime;
		cursor = song->sequence_.cursor;
		pattern_line = (uintptr_t)(psy_dsp_beatpos_real(cursor.offset) *
			cursor.lpb);
		play_line = (uintptr_t)(psy_dsp_beatpos_real(
			host_time.currplaycursor.offset) * cursor.lpb);
		step_row = pattern_line / NUMSTEPS;
		line = step_row * NUMSTEPS;
		for (i = 0; i < NUMSTEPS; ++i, ++line) {
			StepSequencerTile* tile;

			tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles, i);
			if (tile) {
				if (psy_audio_hostsequencertime_playing(&host_time) &&
						line == play_line) {
					stepsequencertile_play(tile);
				} else {
					stepsequencertile_reset_play(tile);
				}
				stepsequencertile_turn_off(tile);
			}
		}		
		pattern = psy_audio_sequence_pattern(&song->sequence_,
			cursor.order_index);
		if (!pattern) {
			return;
		}
		curr = psy_audio_pattern_begin(pattern);
		while (curr) {						
			StepSequencerTile* tile;
			psy_audio_PatternEntry* entry;			
			uintptr_t curr_step;		
			
			entry = psy_audio_patternnode_entry(curr);			
			line = psy_dsp_beatpos_line(psy_audio_patternentry_offset(entry),
				psy_audio_sequencecursor_lpb(&cursor));
			curr_step = line % NUMSTEPS;
			tile = NULL;
			if (psy_audio_patternentry_track(entry) ==
					psy_audio_sequencecursor_channel(&cursor)) {
				uintptr_t start;

				start = step_row * NUMSTEPS;
				if (line >= start && line < start + NUMSTEPS) {
					tile = (StepSequencerTile*)psy_table_at(&self->tiles->tiles,
						curr_step);
					if (tile) {
						stepsequencertile_turn_on(tile);
					}
				}
			}
			psy_audio_patternnode_next(&curr);
		}
	}	
	if (workspace_song(self->workspace)) {
		uintptr_t group;
		psy_audio_SequenceCursor cursor;
		
		cursor = self->workspace->song->sequence_.cursor;
		group = (uintptr_t)(psy_dsp_beatpos_real(psy_audio_sequencecursor_offset(&cursor)) /
			NUMSTEPS * cursor.lpb);
		self->stepgroup = psy_min(group, 63);		
	}
}

StepSequencerState* stepsequencerclearstate_tile_press(
	StepSequencerClearState* self, uintptr_t step)
{
	psy_audio_Song* song;	
	uintptr_t steprow;
	psy_audio_SequenceCursor end_cursor;
	psy_audio_BlockSelection selection;
	// PatternCmds cmds;
		
	assert(self);
		
	song = workspace_song(self->workspace);	
	
	if (!song) {
		return NULL;
	}
	if (step == psy_INDEX_INVALID) {
		return NULL;
	}
	if (self->start == psy_INDEX_INVALID) {
		self->start = step;
		self->start_cursor = song->sequence_.cursor;
		steprow = psy_audio_sequencecursor_line(&self->start_cursor) / NUMSTEPS;
		step += steprow * NUMSTEPS;
		self->start_cursor.column = 0;
		self->start_cursor.offset = psy_dsp_beatpos_make_real(
			step / (double)self->start_cursor.lpb,
			psy_dsp_DEFAULT_PPQ);
		return NULL;
	}
	end_cursor = song->sequence_.cursor;
	steprow = psy_audio_sequencecursor_line(&end_cursor) / NUMSTEPS;
	step += steprow * NUMSTEPS;
	end_cursor.channel_++;
	end_cursor.column = 0;
	end_cursor.offset = psy_dsp_beatpos_make_real(
		(step + 1) / (double)end_cursor.lpb,
		psy_dsp_DEFAULT_PPQ);
	psy_audio_blockselection_init_all(&selection,
		self->start_cursor, end_cursor);
	// patterncmds_init(&cmds, NULL, NULL, &self->workspace->undoredo,
	//	self->workspace);	
	// patterncmds_block_delete(&cmds, selection);
	return &self->states->note_state.state;
}

StepSequencerState* stepsequencerclearstate_shift_press(
	StepSequencerClearState* self)
{
	return &self->states->note_state.state;
}

void stepsequencerclearstate_stepgroup_tweak( StepSequencerClearState*
	self, psy_audio_ChoiceMachineParam* sender, double value)
{
	assert(self);
	
	if (self->workspace->song) {
		uintptr_t group;	
		psy_audio_SequenceCursor cursor;
	
		group = (uintptr_t)((value * (sender->maxval - sender->minval) + 0.5f) +
			sender->minval);
		cursor = self->workspace->song->sequence_.cursor;
		psy_audio_sequencecursor_set_offset(&cursor,
			psy_dsp_beatpos_make_real(
				(double)group * NUMSTEPS * 1.0/(double)cursor.lpb,
				psy_dsp_DEFAULT_PPQ));
		psy_audio_sequence_set_cursor(&self->workspace->song->sequence_,
			cursor);
	}
}

/* StepSequencerShiftState */

/* prototypes */
static void stepsequencershiftstate_dispose(StepSequencerShiftState*);
static void stepsequencershiftstate_enter(StepSequencerShiftState*);
static StepSequencerState* stepsequencershiftstate_tile_press(
	StepSequencerShiftState*, uintptr_t step);
static StepSequencerState* stepsequencershiftstate_shift_press(
	StepSequencerShiftState*);
static psy_audio_MachineParam* stepsequencershiftstate_param(
	StepSequencerShiftState*, uintptr_t index);

/* vtable */
static StepSequencerStateVtable stepsequencershiftstate_vtable;
static bool stepsequencershiftstate_vtable_initialized = FALSE;

static void stepsequencershiftstate_vtable_init(StepSequencerShiftState* self)
{
	if (!stepsequencershiftstate_vtable_initialized) {
		stepsequencershiftstate_vtable = *(self->state.vtable);
		stepsequencershiftstate_vtable.dispose =
			(fp_stepsequencerstate)
			stepsequencershiftstate_dispose;
		stepsequencershiftstate_vtable.enter =
			(fp_stepsequencerstate)
			stepsequencershiftstate_enter;
		stepsequencershiftstate_vtable.tile_press =
			(fp_stepsequencerstate_tile_press)
			stepsequencershiftstate_tile_press;
		stepsequencershiftstate_vtable.shift_press =
			(fp_stepsequencerstate_shift_press)	
			stepsequencershiftstate_shift_press;		
		stepsequencershiftstate_vtable.param =
			(fp_stepsequencerstate_param)
			stepsequencershiftstate_param;		
		stepsequencershiftstate_vtable_initialized = TRUE;
	}
	self->state.vtable = &stepsequencershiftstate_vtable;
}

/* implementation */
void stepsequencershiftstate_init(StepSequencerShiftState* self,
	StepsequencerStates* states, Workspace* workspace)
{
	assert(self);
	assert(workspace);

	stepsequencerstate_init(&self->state);
	stepsequencershiftstate_vtable_init(self);	
	self->workspace = workspace;	
	self->states = states;
	self->tiles = NULL;
	psy_audio_intmachineparam_init(&self->param_note,
		"Shift", "Shift", MPF_STATE | MPF_SMALL,
		NULL, 0, 16);	
}

void stepsequencershiftstate_dispose(StepSequencerShiftState* self)
{
	assert(self);
		
	psy_audio_intmachineparam_dispose(&self->param_note);	
	stepsequencerstate__dispose(&self->state);
}

void stepsequencershiftstate_enter(StepSequencerShiftState* self)
{
	assert(self);
	
	if (self->tiles) {
		stepsequencertiles_enable_shift(self->tiles);
	}
}

psy_audio_MachineParam* stepsequencershiftstate_param(
	StepSequencerShiftState* self, uintptr_t index)
{
	if (index == 0) {
		return &self->param_note.machineparam;
	}
	return NULL;
}

StepSequencerState* stepsequencershiftstate_tile_press(
	StepSequencerShiftState* self, uintptr_t step)
{		
	assert(self);
	
	switch (step) {
	case STEPSEQUENCERSHIFT_LAST_STEP:
		return &self->states->laststep_state.state;
	case STEPSEQUENCERSHIFT_CLEAR:
		return &self->states->clear_state.state;
	default:
		return NULL;
	}	
}

StepSequencerState* stepsequencershiftstate_shift_press(
	StepSequencerShiftState* self)
{
	return &self->states->note_state.state;
}


/* StepSequencerTile */

/* implementation */
void stepsequencertile_init(StepSequencerTile* self, psy_ui_Component* parent)
{
	const psy_ui_Font* font;
	
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_ui_label_init_text(&self->label, &self->component, "");
	psy_ui_label_prevent_translation(&self->label);
	psy_ui_component_prevent_app_focus_out(psy_ui_label_base(&self->label));
	font = psy_ui_component_font(&self->component);
	if (font) {
		psy_ui_FontInfo fontinfo;		

		fontinfo = psy_ui_font_font_info(font);
		fontinfo.lfHeight = floor(fontinfo.lfHeight * 0.6);
		psy_ui_component_set_font_info(&self->label.component, fontinfo);		
	}	
	psy_ui_component_set_align(&self->label.component, psy_ui_ALIGN_BOTTOM);	
	psy_ui_component_init(&self->client, &self->component, NULL);
	psy_ui_component_prevent_app_focus_out(&self->client);
	psy_ui_component_set_style_type(&self->client, STYLE_STEPSEQUENCER_STEP);
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_TOP);
	psy_ui_component_init(&self->button, &self->client, NULL);
	psy_ui_component_set_align(&self->button, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_style_type(&self->button, STYLE_STEPSEQUENCER_TILE);
	psy_ui_component_set_style_type_select(&self->button,
		STYLE_STEPSEQUENCER_TILE_SELECT);		
	psy_ui_component_set_style_type_active(&self->button,
		STYLE_STEPSEQUENCER_TILE_ACTIVE);
	psy_ui_component_set_preferred_size(&self->button,
		psy_ui_size_make_em(5.0, 2.0));	
	psy_ui_component_prevent_app_focus_out(&self->button);
}

StepSequencerTile* stepsequencertile_alloc(void)
{
	return (StepSequencerTile*)malloc(sizeof(StepSequencerTile));
}

StepSequencerTile* stepsequencertile_allocinit(psy_ui_Component* parent)
{
	StepSequencerTile* rv;

	rv = stepsequencertile_alloc();
	if (rv) {
		stepsequencertile_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(
			stepsequencertile_base(rv));		
	}
	return rv;
}

void stepsequencertile_set_text(StepSequencerTile* self, const char* text)
{
	assert(self);
	
	psy_ui_label_set_text(&self->label, text);
}

void stepsequencertile_turn_on(StepSequencerTile* self)
{
	assert(self);
	
	psy_ui_component_add_style_state(&self->button,
		psy_ui_STYLESTATE_SELECT);
}

void stepsequencertile_turn_off(StepSequencerTile* self)
{
	assert(self);

	psy_ui_component_remove_style_state(&self->button,
		psy_ui_STYLESTATE_SELECT);	
}

void stepsequencertile_play(StepSequencerTile* self)
{
	assert(self);

	psy_ui_component_add_style_state(&self->button,
		psy_ui_STYLESTATE_ACTIVE);
}

void stepsequencertile_reset_play(StepSequencerTile* self)
{
	assert(self);

	psy_ui_component_remove_style_state(&self->button,
		psy_ui_STYLESTATE_ACTIVE);
}


/* StepsequencerBar */

/* prototypes */
static void stepsequencertiles_on_destroyed(StepsequencerTiles*);
static void stepsequencertiles_build(StepsequencerTiles*);
static void stepsequencertiles_on_mouse_down(StepsequencerTiles*,
	psy_ui_MouseEvent*);
static void stepsequencertiles_set_tile_text(StepsequencerTiles*,
	uintptr_t tile, const char* text);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(StepsequencerTiles* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			stepsequencertiles_on_destroyed;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			stepsequencertiles_on_mouse_down;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(stepsequencertiles_base(self),
		&vtable);
}

/* implementation */
void stepsequencertiles_init(StepsequencerTiles* self, psy_ui_Component* parent,
	StepsequencerView* view, StepSequencerState* state)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->state = state;
	self->view = view;
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_table_init_keysize(&self->tiles, 37);
	stepsequencertiles_build(self);
}

void stepsequencertiles_on_destroyed(StepsequencerTiles* self)
{
	assert(self);

	psy_table_dispose(&self->tiles);	
}

void stepsequencertiles_build(StepsequencerTiles* self)
{	
	uintptr_t i;

	assert(self);

	psy_ui_component_clear(&self->component);
	psy_table_clear(&self->tiles);
	for (i = 0; i < 16; ++i) {
		StepSequencerTile* tile;
		
		tile = stepsequencertile_allocinit(&self->component);
		if ((i % 4) == 0) {
			psy_ui_component_set_style_type(&tile->client,
				STYLE_STEPSEQUENCER_BAR);
		}
		psy_table_insert(&self->tiles, i, (void*)tile);						
	}
	stepsequencertiles_set_tile_text(self, STEPSEQUENCERSHIFT_LAST_STEP,
		"LAST STEP");
	stepsequencertiles_set_tile_text(self, STEPSEQUENCERSHIFT_CLEAR,
		"CLEAR");
	psy_ui_component_align(&self->component);
}

void stepsequencertiles_set_tile_text(StepsequencerTiles* self, uintptr_t index,
	const char* text)
{
	StepSequencerTile* tile;

	tile = (StepSequencerTile*)psy_table_at(&self->tiles, index);
	if (tile) {
		stepsequencertile_set_text(tile, text);
	}
}

void stepsequencertiles_on_mouse_down(StepsequencerTiles* self,
	psy_ui_MouseEvent* ev)
{	
	uintptr_t step;	
	
	assert(self);
	
	psy_ui_component_intersect(&self->component, psy_ui_mouseevent_offset(ev), 
		&step);
	stepsequencerview_enter_state(self->view, stepsequencerstate_tile_press(
		self->state, step));	
}

void stepsequencertiles_enable_shift(StepsequencerTiles* self)
{
	uintptr_t i;
	
	assert(self);	
	
	for (i = 0; i < 16; ++i) {
		StepSequencerTile* tile;

		tile = (StepSequencerTile*)psy_table_at(&self->tiles, i);
		if (tile) {
			psy_ui_component_set_style_type(&tile->button,
				STYLE_STEPSEQUENCER_TILE_SHIFT);
			stepsequencertile_turn_off(tile);
		}
	}	
	psy_ui_component_invalidate(&self->component);	
}

void stepsequencertiles_disable_shift(StepsequencerTiles* self)
{
	uintptr_t i;
	
	assert(self);
		
	for (i = 0; i < 16; ++i) {
		StepSequencerTile* tile;

		tile = (StepSequencerTile*)psy_table_at(&self->tiles, i);
		if (tile) {
			psy_ui_component_set_style_type(&tile->button,
				STYLE_STEPSEQUENCER_TILE);
		}
	}
	psy_ui_component_invalidate(&self->component);	
}


/* StepSequencerOptions */

/* prototypes */
static void stepsequenceroptions_on_timer(StepSequencerOptions*,
	uintptr_t timer_id);
	
/* vtable */
static psy_ui_ComponentVtable stepsequenceroptions_vtable;
static bool stepsequenceroptions_vtable_initialized = FALSE;

static void stepsequenceroptions_vtable_init(StepSequencerOptions* self)
{
	assert(self);
	
	if (!stepsequenceroptions_vtable_initialized) {
		stepsequenceroptions_vtable = *(self->component.vtable);		
		stepsequenceroptions_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			stepsequenceroptions_on_timer;
		stepsequenceroptions_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(stepsequenceroptions_base(self),
		&stepsequenceroptions_vtable);
}

/* implementation */
void stepsequenceroptions_init(StepSequencerOptions* self,
	psy_ui_Component* parent, StepSequencerState* state)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	stepsequenceroptions_vtable_init(self);	
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_ui_component_init_align(&self->left, &self->component, NULL,
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_default_align(&self->left, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.5, 0.0, 0.0, 0.0));					
	psy_ui_component_prevent_app_focus_out(&self->left);
	psy_ui_button_init_text(&self->shift, &self->left, "Shift");
	psy_ui_component_init_align(&self->client, &self->component, NULL,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_default_align(&self->client, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 8.0));			
	knobui_init(&self->note, &self->client, NULL, psy_INDEX_INVALID,
		stepsequencerstate_param(state, 0));
	knobui_init(&self->stepgroup, &self->client, NULL, psy_INDEX_INVALID,
		stepsequencerstate_param(state, 1));	
	psy_ui_component_start_timer(&self->component, 0, 50);	
}

void stepsequenceroptions_set_state(StepSequencerOptions* self,
	StepSequencerState* state)
{
	assert(self);
		
	knobui_set_param(&self->note, NULL, psy_INDEX_INVALID,
		stepsequencerstate_param(state, 0));
	knobui_set_param(&self->stepgroup, NULL, psy_INDEX_INVALID,
		stepsequencerstate_param(state, 1));
}

void stepsequenceroptions_on_timer(StepSequencerOptions* self,
	uintptr_t timer_id)
{
	assert(self);
	
	psy_ui_component_invalidate(knobui_base(&self->note));
	psy_ui_component_invalidate(knobui_base(&self->stepgroup));
}


/* StepSequencerStates */

/* implementation */
void stepsequencerstates_init(StepsequencerStates* self, Workspace* workspace)
{
	assert(self);
	
	stepsequencernotestate_init(&self->note_state, self, workspace);
	stepsequencershiftstate_init(&self->shift_state, self, workspace);
	stepsequencerlaststepstate_init(&self->laststep_state, self, workspace);
	stepsequencerclearstate_init(&self->clear_state, self, workspace);
}

void stepsequencerstates_dispose(StepsequencerStates* self)
{
	assert(self);
	
	stepsequencerstate_dispose(&self->note_state.state);
	stepsequencerstate_dispose(&self->shift_state.state);
	stepsequencerstate_dispose(&self->laststep_state.state);
	stepsequencerstate_dispose(&self->clear_state.state);
}

/* StepSequencerView */

/* prototypes */
static void stepsequencerview_on_destroyed(StepsequencerView*);
static void stepsequencerview_connect_workspace(StepsequencerView*, Workspace*);
static void stepsequencerview_connect_song(StepsequencerView*, psy_audio_Song*);
static void stepsequencerview_on_playline_changed(StepsequencerView*,
	psy_audio_Player* sender);
static void stepsequencerview_on_playstatus_changed(StepsequencerView*,
	psy_audio_Player* sender);
static void stepsequencerview_on_song_changed(StepsequencerView*,
	psy_audio_Player* sender);
static void stepsequencerview_on_cursor_changed(StepsequencerView*,
	psy_audio_Sequence* sender);
static void stepgroup_tweak(StepsequencerView*, psy_audio_ChoiceMachineParam*
	sender, double value);
static void stepsequencerview_on_shift(StepsequencerView*,
	psy_ui_Button* sender);
static void stepsequencerview_on_lpb_changed(
	StepsequencerView*, psy_audio_Player* sender, uintptr_t lpb);
static void stepsequencerview_on_sequence_track_reposition(
	StepsequencerView*, psy_audio_Sequence* sender, uintptr_t track_idx);

/* vtable */
static psy_ui_ComponentVtable stepsequencerview_vtable;
static bool stepsequencerview_vtable_initialized = FALSE;

static void stepsequencerview_vtable_init(StepsequencerView* self)
{
	if (!stepsequencerview_vtable_initialized) {
		stepsequencerview_vtable = *(self->component.vtable);
		stepsequencerview_vtable.on_destroyed =
			(psy_ui_fp_component)
			stepsequencerview_on_destroyed;		
		stepsequencerview_vtable_initialized = TRUE;
	}
	self->component.vtable = &stepsequencerview_vtable;
}

/* implementation */
void stepsequencerview_init(StepsequencerView* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	stepsequencerview_vtable_init(self);	
	psy_ui_component_set_style_type(stepsequencerview_base(self), STYLE_SIDE_VIEW);
	psy_ui_component_prevent_app_focus_out(stepsequencerview_base(self));
	closebar_init(&self->close_bar_, stepsequencerview_base(self),
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(workspace)),
			"bench.showstepsequencer"));
	psy_ui_component_set_default_align(stepsequencerview_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_margin_make_em(0.5, 0.0, 0.5, 2.0));
	stepsequencerstates_init(&self->states, workspace);
	self->state_ = &self->states.note_state.state;
	stepsequenceroptions_init(&self->options_, stepsequencerview_base(self),
		self->state_);
	psy_ui_component_set_align(stepsequenceroptions_base(&self->options_),
		psy_ui_ALIGN_TOP);
	psy_ui_component_init(&self->tiles_row_, stepsequencerview_base(self),
		NULL);
	stepsequencertiles_init(&self->tiles_, &self->tiles_row_, self,
		self->state_);
	self->states.note_state.tiles = &self->tiles_;
	self->states.shift_state.tiles = &self->tiles_;
	self->states.laststep_state.tiles = &self->tiles_;
	self->states.clear_state.tiles = &self->tiles_;
	psy_ui_component_set_align(stepsequencertiles_base(&self->tiles_),
		psy_ui_ALIGN_TOP);
	stepsequencerview_connect_workspace(self, workspace);
	stepsequencerview_connect_song(self, workspace_song(workspace));
	psy_signal_connect(&self->options_.shift.signal_clicked, self,
		stepsequencerview_on_shift);	
}

void stepsequencerview_on_destroyed(StepsequencerView* self)
{
	assert(self);

	stepsequencerstates_dispose(&self->states);
}

void stepsequencerview_connect_workspace(StepsequencerView* self,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		stepsequencerview_on_song_changed);	
	psy_signal_connect(&workspace->player_.sequencer.signal_play_line_changed,
		self, stepsequencerview_on_playline_changed);
	psy_signal_connect(&workspace->player_.sequencer.signal_play_status_changed,
		self, stepsequencerview_on_playstatus_changed);
	psy_signal_connect(&workspace_player(workspace)->signal_lpbchanged,
		self, stepsequencerview_on_lpb_changed);
}

void stepsequencerview_on_playline_changed(StepsequencerView* self,
	psy_audio_Player* sender)
{
	assert(self);

	if (psy_ui_component_visible(stepsequencerview_base(self))) {
		stepsequencerstate_update_playline(self->state_);			
	}
}

void stepsequencerview_on_playstatus_changed(StepsequencerView* self,
	psy_audio_Player* sender)
{	
	assert(self);

	if (psy_ui_component_visible(stepsequencerview_base(self))) {
		stepsequencerstate_update_playline(self->state_);			
	}
}	

void stepsequencerview_on_cursor_changed(StepsequencerView* self,
	psy_audio_Sequence* sender)
{
	assert(self);
			
	stepsequencerstate_update(self->state_);
}

void stepsequencerview_on_song_changed(StepsequencerView* self,
	psy_audio_Player* sender)
{	
	assert(self);

	stepsequencerview_connect_song(self, psy_audio_player_song(sender));	
	stepsequencerstate_update(self->state_);
}

void stepsequencerview_connect_song(StepsequencerView* self,
	psy_audio_Song* song)
{
	assert(self);
	
	if (song) {
		psy_audio_Sequence* sequence;	
		
		sequence = psy_audio_song_sequence(song);
		psy_signal_connect(&sequence->signal_cursor_changed,
			self, stepsequencerview_on_cursor_changed);
		psy_signal_connect(&sequence->signal_track_reposition,
			self, stepsequencerview_on_sequence_track_reposition);
	}
}

void stepsequencerview_on_shift(StepsequencerView* self, psy_ui_Button* sender)
{	
	assert(self);
		
	stepsequencerview_enter_state(self, stepsequencerstate_shift_press(
		self->state_));
}

void stepsequencerview_on_lpb_changed(StepsequencerView* self,
	psy_audio_Player* sender, uintptr_t lpb)
{
	assert(self);
	
	stepsequencerstate_update(self->state_);
}

void stepsequencerview_enter_state(StepsequencerView* self,
	StepSequencerState* state)
{
	assert(self);
	
	if (state) {
		stepsequencertiles_set_state(&self->tiles_, state);
		stepsequenceroptions_set_state(&self->options_, state);
		self->state_ = state;
		stepsequencerstate_enter(self->state_);
	}
}

void stepsequencerview_on_sequence_track_reposition(StepsequencerView* self,
	psy_audio_Sequence* sender, uintptr_t track_idx)
{
	assert(self);		
			
	stepsequencerstate_update(self->state_);
}

#endif /* PSYCLE_USE_STEPSEQUENCER */
