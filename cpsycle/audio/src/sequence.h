/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SEQUENCE_H
#define psy_audio_SEQUENCE_H

/* local */
#include "patterns.h"
#include "sequenceentry.h"
#include "sequenceselection.h"
/* container */
#include <list.h>
#include <signal.h>
#include <command.h>

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef psy_List psy_audio_SequenceEntryNode;

#define psy_audio_GLOBALTRACK 65535

/* psy_audio_Order */
typedef struct psy_audio_Order {
	psy_audio_OrderIndex index;
	psy_audio_SequenceEntry* entry;
} psy_audio_Order;

void psy_audio_order_init(psy_audio_Order*);
void psy_audio_order_dispose(psy_audio_Order*);

void psy_audio_order_set_entry(psy_audio_Order*,
	const psy_audio_SequenceEntry*);

typedef struct psy_audio_SequencePaste {
	psy_List* nodes;
} psy_audio_SequencePaste;

void psy_audio_sequencepaste_init(psy_audio_SequencePaste*);
void psy_audio_sequencepaste_dispose(psy_audio_SequencePaste*);

void psy_audio_sequencepaste_clear(psy_audio_SequencePaste*);
void psy_audio_sequencepaste_copy(psy_audio_SequencePaste*,
	struct psy_audio_Sequence*, psy_audio_SequenceSelection*);

/*!
** @struct psy_audio_SequenceTrack
** @brief A list of SequenceEntries sorted according to the playorder
*/
typedef struct psy_audio_SequenceTrack {
	psy_audio_SequenceEntryNode* nodes;
	psy_Table entries;
	char* name_;
	double height_;
} psy_audio_SequenceTrack;

void psy_audio_sequencetrack_init(psy_audio_SequenceTrack*);
void psy_audio_sequencetrack_dispose(psy_audio_SequenceTrack*);

psy_audio_SequenceTrack* psy_audio_sequencetrack_alloc(void);
psy_audio_SequenceTrack* psy_audio_sequencetrack_alloc_init(void);
psy_audio_SequenceTrack* psy_audio_sequencetrack_clone(
	psy_audio_SequenceTrack* src);
void psy_audio_sequencetrack_deallocate(psy_audio_SequenceTrack*);

psy_dsp_beatpos_t psy_audio_sequencetrack_duration(
	const psy_audio_SequenceTrack*, const psy_audio_Patterns*);

void psy_audio_sequencetrack_set_name(psy_audio_SequenceTrack*,
	const char* name);

INLINE const char* psy_audio_sequencetrack_name(
	const psy_audio_SequenceTrack* self)
{
	assert(self);

	return self->name_;
}

INLINE void psy_audio_sequencetrack_set_height(psy_audio_SequenceTrack* self,
	double height)
{
	assert(self);

	self->height_ = height;
}

INLINE double psy_audio_sequencetrack_height(
	const psy_audio_SequenceTrack* self)
{
	assert(self);

	return self->height_;
}

void psy_audio_sequencetrack_insert(psy_audio_SequenceTrack*,
	uintptr_t order, psy_audio_Patterns*, uintptr_t patidx);
uintptr_t psy_audio_sequencetrack_remove(psy_audio_SequenceTrack*,
	uintptr_t order);
psy_audio_SequenceEntry* psy_audio_sequencetrack_entry(psy_audio_SequenceTrack*,
	uintptr_t row);
psy_audio_SequenceEntryNode* psy_audio_sequencetrack_node_at_offset(
	psy_audio_SequenceTrack*, psy_dsp_beatpos_t offset);
uintptr_t psy_audio_sequencetrack_size(const psy_audio_SequenceTrack*);

/*!
** @struct psy_audio_SequenceTrackIterator
** @brief Iterator the player uses to advance through the track and pattern.
*/
typedef struct psy_audio_SequenceTrackIterator {
	psy_audio_Patterns* patterns;
	psy_audio_SequenceTrack* track;	
	psy_audio_SequenceEntryNode* sequencentrynode;
	psy_audio_PatternNode* patternnode;
	psy_audio_PatternNode* prevpatternnode;
} psy_audio_SequenceTrackIterator;

void psy_audio_sequencetrackiterator_init(psy_audio_SequenceTrackIterator*);
void psy_audio_sequencetrackiterator_dispose(psy_audio_SequenceTrackIterator*);

psy_audio_SequenceTrackIterator* psy_audio_sequencetrackiterator_alloc(void);
psy_audio_SequenceTrackIterator* psy_audio_sequencetrackiterator_alloc_init(void);

bool psy_audio_sequencetrackiterator_end(const psy_audio_SequenceTrackIterator*);
void psy_audio_sequencetrackiterator_inc(psy_audio_SequenceTrackIterator*);
void psy_audio_sequencetrackiterator_inc_entry(psy_audio_SequenceTrackIterator*);
void psy_audio_sequencetrackiterator_dec_entry(psy_audio_SequenceTrackIterator*);
bool psy_audio_sequencetrackiterator_has_next_entry(
	const psy_audio_SequenceTrackIterator*);

INLINE psy_audio_PatternNode* psy_audio_sequencetrackiterator_pattern_node(
	psy_audio_SequenceTrackIterator* self)
{
	return self->patternnode;
}

INLINE psy_audio_SequenceEntry* psy_audio_sequencetrackiterator_entry(
	psy_audio_SequenceTrackIterator* self)
{
	return (self->sequencentrynode)
		? (psy_audio_SequenceEntry*)psy_list_entry(self->sequencentrynode)
		: NULL;
}

INLINE psy_audio_PatternEntry* psy_audio_sequencetrackiterator_pattern_entry(
	psy_audio_SequenceTrackIterator* self)
{
	return (self->patternnode)
		? (psy_audio_PatternEntry*)(self->patternnode)->entry
		: NULL;
}

INLINE uintptr_t psy_audio_sequencetrackiterator_pattern_index(
	psy_audio_SequenceTrackIterator* self)
{		
	psy_audio_SequenceEntry* entry;

	entry = psy_audio_sequencetrackiterator_entry(self);
	if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
		psy_audio_SequencePatternEntry* seqpatternentry;

		seqpatternentry = (psy_audio_SequencePatternEntry*)
			self->sequencentrynode->entry;
		return seqpatternentry->patternslot;
	}	
	return psy_INDEX_INVALID;
}

INLINE psy_audio_Pattern* psy_audio_sequencetrackiterator_pattern(
	psy_audio_SequenceTrackIterator* self)
{
	if (self->patterns) {
		uintptr_t pat_idx;

		pat_idx = psy_audio_sequencetrackiterator_pattern_index(self);
		if (pat_idx != psy_INDEX_INVALID) {
			return psy_audio_patterns_at(self->patterns, pat_idx);
		}
	}
	return NULL;
}

void psy_audio_sequencetrackiterator_reset_pattern_node(
	psy_audio_SequenceTrackIterator*);

INLINE psy_dsp_beatpos_t psy_audio_sequencetrackiterator_seq_offset(
	psy_audio_SequenceTrackIterator* self)
{	
	psy_audio_SequenceEntry* seqentry;

	seqentry = psy_audio_sequencetrackiterator_entry(self);
	if (seqentry) {
		return psy_audio_sequenceentry_offset(seqentry);
	}
	return psy_dsp_beatpos_zero();
}

INLINE psy_dsp_beatpos_t psy_audio_sequencetrackiterator_offset(
	psy_audio_SequenceTrackIterator* self)
{			
	psy_audio_PatternEntry* pattern_entry;

	if (self->patternnode) {
		pattern_entry = psy_audio_sequencetrackiterator_pattern_entry(self);
	} else {
		pattern_entry = NULL;
	}
	return psy_dsp_beatpos_add(	
		psy_audio_sequencetrackiterator_seq_offset(self),
			(self->patternnode)
			? psy_audio_patternentry_offset(pattern_entry)
			: psy_dsp_beatpos_zero());
}

INLINE psy_dsp_beatpos_t psy_audio_sequencetrackiterator_entry_length(
	psy_audio_SequenceTrackIterator* self)
{	
	psy_audio_SequenceEntry* seqentry;

	seqentry = psy_audio_sequencetrackiterator_entry(self);
	if (seqentry) {
		return psy_audio_sequenceentry_length(seqentry);
	}
	return psy_dsp_beatpos_zero();
}

typedef psy_List psy_audio_SequenceTrackNode;


typedef struct SequenceTraverseParams {	
	psy_audio_SequenceEntryNode* seq_node;
	psy_audio_PatternNode* node;
	psy_audio_PatternNode* prev_node;
	psy_audio_Pattern* pattern;
	psy_audio_SequenceTrack* track;
	bool stop;
	psy_dsp_beatpos_t offset;
	psy_dsp_beatpos_t seqoffset;
	psy_dsp_beatpos_t bpl;
	uintptr_t lpb;
	uintptr_t channel;
	uintptr_t track_idx;
	bool has_event;
	bool prev_set;
} SequenceTraverseParams;

INLINE SequenceTraverseParams sequencetraverseparams_make_all(	
	psy_audio_SequenceEntryNode* seq_node,
	psy_audio_PatternNode* prev_node,
	psy_audio_PatternNode* node, psy_audio_Pattern* pattern,
	psy_audio_SequenceTrack* track,
	psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t seqoffset,
	psy_dsp_beatpos_t bpl,
	uintptr_t lpb,
	uintptr_t channel,
	uintptr_t track_idx,
	bool has_event)
{
	SequenceTraverseParams rv;
		
	rv.seq_node = seq_node;
	rv.prev_node = prev_node;
	rv.node = node;
	rv.pattern = pattern;
	rv.track = track;
	rv.offset = offset;
	rv.seqoffset = seqoffset;
	rv.bpl = bpl;
	rv.lpb = lpb;
	rv.channel = channel;
	rv.track_idx = track_idx;
	rv.stop = FALSE;
	rv.has_event = has_event;
	rv.prev_set = FALSE;
	return rv;
}


/*!
** \struct psy_audio_Sequence
** \brief contains the playorder list of a song (multisequence)
*/

typedef struct psy_audio_Sequence {
	/* signals */	
	psy_Signal signal_insert;
	psy_Signal signal_remove;
	psy_Signal signal_reorder;
	psy_Signal signal_track_insert;
	psy_Signal signal_track_remove;
	psy_Signal signal_track_swap;
	psy_Signal signal_track_reposition;
	psy_Signal signal_clear;
	psy_Signal signal_mute_changed;
	psy_Signal signal_solo_changed;
	psy_Signal signal_cursor_changed;
	psy_Signal signal_tweak;
	/*! @internal */	
	psy_Table tracks_;
	psy_audio_SequenceTrack globaltrack;
	psy_audio_Pattern globalpattern;
	psy_audio_TrackState trackstate;
	bool preventreposition;
	/* editposition */
	psy_audio_SequenceCursor cursor;
	psy_audio_SequenceCursor lastcursor;
	psy_audio_SequenceSelection selection;
	/* calcduration */
	struct psy_audio_Sequencer* sequencerduration;
	psy_dsp_seconds_t durationms;
	/* references */
	psy_audio_Patterns* patterns;
	psy_audio_Samples* samples;	
} psy_audio_Sequence;

void psy_audio_sequence_init(psy_audio_Sequence*, psy_audio_Patterns*,
	psy_audio_Samples*);
void psy_audio_sequence_dispose(psy_audio_Sequence*);
void psy_audio_sequence_copy(psy_audio_Sequence*, psy_audio_Sequence* other);

void psy_audio_sequence_clear(psy_audio_Sequence*);
/* sequenceentry methods */
void psy_audio_sequence_insert(psy_audio_Sequence*, psy_audio_OrderIndex,
	uintptr_t patidx);
void psy_audio_sequence_insert_sample(psy_audio_Sequence*,
	psy_audio_OrderIndex, psy_audio_SampleIndex);
void psy_audio_sequence_insert_marker(psy_audio_Sequence*,
	psy_audio_OrderIndex, const char* text);
void psy_audio_sequence_remove(psy_audio_Sequence*, psy_audio_OrderIndex);
void psy_audio_sequence_remove_selection(psy_audio_Sequence*,
	psy_audio_SequenceSelection*);
psy_audio_SequenceEntry* psy_audio_sequence_entry(psy_audio_Sequence*,
	psy_audio_OrderIndex);
const psy_audio_SequenceEntry* psy_audio_sequence_entry_const(const
	psy_audio_Sequence*, psy_audio_OrderIndex);
psy_audio_OrderIndex psy_audio_sequence_reorder(psy_audio_Sequence*,
	psy_audio_OrderIndex, psy_dsp_beatpos_t new_position);
void psy_audio_sequence_resetpatterns(psy_audio_Sequence*);
void psy_audio_sequence_fillpatterns(psy_audio_Sequence*);
void psy_audio_reposition(psy_audio_Sequence*);

void psy_audio_sequence_tweak(psy_audio_Sequence*);

uintptr_t psy_audio_sequence_order(const psy_audio_Sequence*,
	uintptr_t trackidx, psy_dsp_beatpos_t position);
psy_audio_Pattern* psy_audio_sequence_pattern(psy_audio_Sequence*,
	psy_audio_OrderIndex);	
void psy_audio_sequence_setpatternindex(psy_audio_Sequence*,
	psy_audio_OrderIndex, uintptr_t patidx);
uintptr_t psy_audio_sequence_patternindex(const psy_audio_Sequence*,
	psy_audio_OrderIndex);
bool psy_audio_sequence_patternused(psy_audio_Sequence*, uintptr_t patidx);
psy_audio_OrderIndex psy_audio_sequence_pattern_first_used(psy_audio_Sequence*,
	uintptr_t patidx);
/* track methods */
void psy_audio_sequence_append_track(psy_audio_Sequence*,
	psy_audio_SequenceTrack*);
uintptr_t psy_audio_sequence_set_track(psy_audio_Sequence*,
	psy_audio_SequenceTrack*, uintptr_t index);
void psy_audio_sequence_remove_track(
	psy_audio_Sequence*, uintptr_t track_idx);
uintptr_t psy_audio_sequence_width(const psy_audio_Sequence*);
psy_audio_SequenceTrack* psy_audio_sequence_track_at(psy_audio_Sequence*,
	uintptr_t trackidx);
const psy_audio_SequenceTrack* psy_audio_sequence_track_at_const(const
	psy_audio_Sequence*, uintptr_t trackidx);
uintptr_t psy_audio_sequence_num_tracks(const psy_audio_Sequence*);
uintptr_t psy_audio_sequence_track_size(const psy_audio_Sequence*,
	uintptr_t trackindex);
uintptr_t psy_audio_sequence_maxtracksize(psy_audio_Sequence*);
void psy_audio_sequence_reposition_track(psy_audio_Sequence*,
	psy_audio_SequenceTrack*);
psy_dsp_beatpos_t psy_audio_sequence_offset(const psy_audio_Sequence*,
	psy_audio_OrderIndex);
void psy_audio_sequence_swaptracks(psy_audio_Sequence*,
	uintptr_t src, uintptr_t dst);
/* play selection */
void psy_audio_sequence_set_play_selection(psy_audio_Sequence*,
	struct psy_audio_SequenceSelection*);
void psy_audio_sequence_clear_play_selection(psy_audio_Sequence*);
/* methods for the sequencer */
void psy_audio_sequence_begin(psy_audio_Sequence*, uintptr_t track_idx,
	psy_dsp_beatpos_t position, psy_audio_SequenceTrackIterator* rv);
psy_audio_SequenceEntryNode* psy_audio_sequence_node(psy_audio_Sequence*,
	psy_audio_OrderIndex, psy_audio_SequenceTrack** rv);
/* calcduration */
psy_dsp_beatpos_t psy_audio_sequence_duration(const psy_audio_Sequence*);
void psy_audio_sequence_start_calc_duration_in_ms(psy_audio_Sequence*);
psy_dsp_seconds_t psy_audio_sequence_endcalcdurationinmsresult(
	psy_audio_Sequence*);
bool psy_audio_sequence_calc_duration_in_ms(psy_audio_Sequence*);
/* trackstate */
void psy_audio_sequence_activate_solo_track(psy_audio_Sequence*,
	uintptr_t track_index);
void psy_audio_sequence_deactivate_solo_track(psy_audio_Sequence*);
void psy_audio_sequence_mute_track(psy_audio_Sequence*,
	uintptr_t track_index);
void psy_audio_sequence_unmute_track(psy_audio_Sequence*,
	uintptr_t track_index);
int psy_audio_sequence_is_track_muted(const psy_audio_Sequence*,
	uintptr_t track_index);
bool psy_audio_sequence_is_track_soloed(const psy_audio_Sequence*,
	uintptr_t track_index);
/* gui stuff */
double psy_audio_sequence_track_height(const psy_audio_Sequence*,
	uintptr_t track_index);
void psy_audio_sequence_set_track_height(psy_audio_Sequence*,
	uintptr_t track_index, double height);
void psy_audio_sequence_set_cursor(psy_audio_Sequence*,
	psy_audio_SequenceCursor);
psy_dsp_beatpos_t psy_audio_sequence_seq_offset(const psy_audio_Sequence*,
	psy_audio_OrderIndex);
void psy_audio_sequence_dec_seqpos(psy_audio_Sequence*);
void psy_audio_sequence_inc_seqpos(psy_audio_Sequence*);

INLINE psy_audio_Patterns* psy_audio_sequence_patterns(
	psy_audio_Sequence* self)
{
	return self->patterns;
}

INLINE const psy_audio_Patterns* psy_audio_sequence_patterns_const(
	const psy_audio_Sequence* self)
{
	return self->patterns;
}

INLINE psy_audio_SequenceCursor psy_audio_sequence_cursor(
	const psy_audio_Sequence* self)
{
	return self->cursor;
}

INLINE bool psy_audio_sequence_lpb_changed(const psy_audio_Sequence* self)
{
	assert(self);

	return (psy_audio_sequencecursor_lpb(&self->cursor) !=
		psy_audio_sequencecursor_lpb(&self->lastcursor));
}

void psy_audio_sequence_block_copy_pattern(psy_audio_Sequence*,
	psy_audio_BlockSelection, psy_audio_Pattern* dest);

void psy_audio_sequence_block_traverse(psy_audio_Sequence*,
	psy_audio_BlockSelection, psy_Command*);
void psy_audio_sequence_block_grid_traverse(psy_audio_Sequence*,
	psy_audio_BlockSelection, psy_Command*);

psy_audio_PatternEvent psy_audio_sequence_pattern_event_at_cursor(
	const psy_audio_Sequence*, psy_audio_SequenceCursor);

const psy_audio_Pattern* psy_audio_sequence_pattern_const(
	const psy_audio_Sequence*, psy_audio_OrderIndex);

psy_audio_BlockSelection psy_audio_sequence_block_selection_order(
	const psy_audio_Sequence*, psy_audio_OrderIndex);
psy_audio_BlockSelection psy_audio_sequence_block_selection_track(
	const psy_audio_Sequence*, uintptr_t seq_track_index);	

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SEQUENCE_H */
