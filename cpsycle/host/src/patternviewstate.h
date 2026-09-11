/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEWSTATE_H)
#define PATTERNVIEWSTATE_H

/* host */
#include "beatconvert.h"
#include "keyboardmiscconfig.h"
#include "patternviewconfig.h"
#include "patternhostcmds.h"
#include "styles.h"
#include "patternselection.h"
/* ui */
#include <uiapp.h>
#include <trackercmds.h>
/* audio */
#include <player.h>
#include <pattern.h>
#include <sequence.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	PATTERNCURSOR_STEP_BEAT,
	PATTERNCURSOR_STEP_4BEAT,
	PATTERNCURSOR_STEP_LINES
} PatternCursorStepMode;


/* PatternViewState */

typedef struct PatternViewState {
	BeatLine beat_line;
	/*! @internal */	
	psy_audio_SequenceCursor cursor;
	PatternSelection selection;	
	psy_audio_Pattern patternpaste;
	bool chord;
	uintptr_t chord_begin;
	psy_dsp_beatpos_t insert_duration; /* 0 : endless */
	bool singlemode;	
	/* references */	
	struct psy_audio_Player* player;
	psy_audio_Sequence* sequence;	
	psy_Configuration* patconfig;
	psy_Configuration* misc_cfg_;
	psy_UndoRedo* undo_redo;
} PatternViewState;

void patternviewstate_init(PatternViewState*, psy_Configuration* pat_cfg,
	psy_Configuration* misc_cfg, psy_audio_Player* player, psy_audio_Sequence*,
	psy_UndoRedo*);
void patternviewstate_dispose(PatternViewState*);

INLINE void patternviewstate_set_cursor(PatternViewState* self,
	psy_audio_SequenceCursor cursor)
{
	self->cursor = cursor;
	beatline_set_lpb(&self->beat_line, cursor.lpb);
}

INLINE const psy_audio_SequenceCursor* patternviewstate_cursor(const
	PatternViewState* self)
{
	return &self->cursor;
}

void patternviewstate_sync_cursor_to_sequence(PatternViewState* self);

INLINE void patternviewstate_set_sequence(PatternViewState* self,
	psy_audio_Sequence* sequence)
{
	assert(self);
	
	self->sequence = sequence;	
}

INLINE psy_audio_Sequence* patternviewstate_sequence(PatternViewState* self)
{
	assert(self);

	return self->sequence;
}

INLINE psy_audio_Sequence* patternviewstate_sequence_const(
	const PatternViewState* self)
{
	assert(self);

	return self->sequence;
}

INLINE psy_audio_Pattern* patternviewstate_pattern(PatternViewState* self)
{
	assert(self);	

	if (!self->sequence) {
		return NULL;
	}
	return psy_audio_sequence_pattern(self->sequence,
		psy_audio_sequencecursor_order_index(&self->cursor));
}

INLINE const psy_audio_Pattern* patternviewstate_pattern_const(
	const PatternViewState* self)
{
	assert(self);

	return patternviewstate_pattern((PatternViewState*)self);
}

INLINE psy_audio_Patterns* patternviewstate_patterns(PatternViewState* self)
{
	assert(self);

	if (self->sequence) {
		return psy_audio_sequence_patterns(self->sequence);
	}
	return NULL;
}

INLINE const psy_audio_Patterns* patternviewstate_patterns_const(
	const PatternViewState* self)
{
	assert(self);

	if (self->sequence) {
		return psy_audio_sequence_patterns_const(self->sequence);
	}
	return NULL;
}

INLINE bool patternviewstate_single_mode(const PatternViewState* self)
{	
	assert(self);

	return self->singlemode;	
}

INLINE psy_dsp_beatpos_t patternviewstate_length(const PatternViewState* self)
{
	assert(self);

	if (!self->sequence) {
		return psy_dsp_beatpos_zero();
	}
	if (patternviewstate_single_mode(self)) {
		psy_audio_SequenceEntry* seqentry;
		
		seqentry = psy_audio_sequence_entry(self->sequence,
			self->cursor.order_index);
		if (seqentry) {
			return psy_audio_sequenceentry_length(seqentry);			
		}		
	} else {
		psy_audio_SequenceTrack* track;

		track = psy_audio_sequence_track_at(self->sequence,
			self->cursor.order_index.track);
		if (track) {
			return psy_audio_sequencetrack_duration(track,
				patternviewstate_patterns_const(self));
		}		
	}
	return psy_dsp_beatpos_zero();
}

INLINE uintptr_t patternviewstate_num_song_tracks(const PatternViewState* self)
{
	if (patternviewstate_patterns_const(self)) {
		return psy_audio_patterns_num_tracks(
			patternviewstate_patterns_const(self));
	}
	return 0;
}

INLINE uintptr_t patternviewstate_numlines(const PatternViewState* self)
{		
	assert(self);
		
	return beatline_beat_to_line(&self->beat_line,
		patternviewstate_length(self));
}

INLINE psy_dsp_beatpos_t patternviewstate_draw_offset(const PatternViewState*
	self, psy_dsp_beatpos_t absoffset)
{
	return psy_dsp_beatpos_sub(absoffset,
		(patternviewstate_single_mode(self)
		? psy_audio_sequencecursor_seqoffset(&self->cursor, self->sequence)
		: psy_dsp_beatpos_zero()));
}

INLINE void patternviewstate_invalidate(PatternViewState* self)
{
	if (patternviewstate_pattern(self)) {
		psy_audio_pattern_inc_opcount(patternviewstate_pattern(self));
	}
}

INLINE bool patternviewstate_ft2home(const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return psy_configuration_value_bool(self->misc_cfg_, "ft2home", TRUE);
	}
	return TRUE;
}

INLINE bool patternviewstate_ft2delete(const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return psy_configuration_value_bool(self->misc_cfg_,
			"ft2delete", TRUE);		
	}
	return TRUE;
}

INLINE bool patternviewstate_move_cursor_one_step(const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return psy_configuration_value_bool(self->misc_cfg_,
			"movecursoronestep", TRUE);
	}
	return TRUE;
}

INLINE PatternCursorStepMode patternviewstate_pgupdowntype(
	const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return (PatternCursorStepMode)
			psy_configuration_value_int(self->misc_cfg_,
				"pgupdowntype", 4);
	}
	return PATTERNCURSOR_STEP_BEAT;
}

INLINE intptr_t patternviewstate_pgupdownstep(const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return psy_configuration_value_int(self->misc_cfg_,
			"pgupdownstep", 4);			
	}
	return 4;
}

INLINE intptr_t patternviewstate_curr_pgup_down_step(
	const PatternViewState* self)
{
	if (patternviewstate_pgupdowntype(self) == PATTERNCURSOR_STEP_BEAT) {
		return self->cursor.lpb;
	} else if (patternviewstate_pgupdowntype(self) ==
			PATTERNCURSOR_STEP_4BEAT) {
		return self->cursor.lpb * 4;
	}
	return patternviewstate_pgupdownstep(self);
}

INLINE intptr_t patternviewstate_cursor_step(
	const PatternViewState* self)
{
	if (self->misc_cfg_) {
		return psy_configuration_value_int(self->misc_cfg_, "cursorstep", 1);
	}
	return 1;
}

bool patternviewstate_hasmovecursorwhenpaste(const PatternViewState* self);


INLINE double patternviewstate_zoom(const PatternViewState* self)
{
	assert(self);
	
	return psy_ui_app_zoom_rate(psy_ui_app()) * 
		((self->patconfig)
		? psy_configuration_value_double(self->patconfig, "zoom", 1.0)
		: 1.0);
}

double patternviewstate_linenumber_num_digits(const PatternViewState*);

#ifdef __cplusplus
}
#endif

#endif /* PATTERNVIEWSTATE_H */
