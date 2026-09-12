/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequence.h"
/* local */
#include "sequencer.h" /* calculate duration */
#include "sequencecmds.h"
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"

/* psy_audio_Order */
void psy_audio_order_init(psy_audio_Order* self)
{
	self->index = psy_audio_orderindex_make_invalid();
	self->entry = NULL;
}

void psy_audio_order_dispose(psy_audio_Order* self)
{
	free(self->entry);
	self->entry = NULL;
}

void psy_audio_order_set_entry(psy_audio_Order* self,
	const psy_audio_SequenceEntry* entry)
{
	free(self->entry);
	self->entry = NULL;
	self->entry = psy_audio_sequenceentry_clone(entry);
}

/* psy_audio_SequenceTrack */

static void psy_audio_sequencetrack_update_entries(
	psy_audio_SequenceTrack*);

void psy_audio_sequencetrack_init(psy_audio_SequenceTrack* self)
{
	assert(self);

	self->nodes = NULL;
	psy_table_init(&self->entries);
	self->name_ = psy_strdup("seq");
	self->height_ = 0.0;
}

void psy_audio_sequencetrack_dispose(psy_audio_SequenceTrack* self)
{
	assert(self);

	psy_table_dispose(&self->entries);
	psy_list_deallocate(&self->nodes,
		(psy_fp_disposefunc)psy_audio_sequenceentry_dispose);
	free(self->name_);
	self->name_ = NULL;
}

psy_audio_SequenceTrack* psy_audio_sequencetrack_alloc(void)
{
	return malloc(sizeof(psy_audio_SequenceTrack));
}

psy_audio_SequenceTrack* psy_audio_sequencetrack_alloc_init(void)
{
	psy_audio_SequenceTrack* rv;

	rv = psy_audio_sequencetrack_alloc();
	if (rv) {
		psy_audio_sequencetrack_init(rv);
	}
	return rv;
}

void psy_audio_sequencetrack_deallocate(psy_audio_SequenceTrack* self)
{
	psy_audio_sequencetrack_dispose(self);
	free(self);
	self = NULL;
}

psy_audio_SequenceTrack* psy_audio_sequencetrack_clone(
	psy_audio_SequenceTrack* src)
{
	psy_audio_SequenceTrack* rv;

	assert(src);

	rv = psy_audio_sequencetrack_alloc_init();
	if (rv) {
		psy_List* p;
		uintptr_t i;
				
		for (p = src->nodes, i = 0; p != NULL; psy_list_next(&p), ++i) {
			psy_audio_SequenceEntry* seqentry;
			psy_List* q;
			
			seqentry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (seqentry) {
				q = psy_list_append(&rv->nodes, psy_audio_sequenceentry_clone(
					seqentry));
			} else {
				q = psy_list_append(&rv->nodes, NULL);
			}
			psy_table_insert(&rv->entries, i, q);
		}
		psy_strreset(&rv->name_, src->name_);
		rv->height_ = src->height_;
	}
	return rv;
}

psy_dsp_beatpos_t psy_audio_sequencetrack_duration(
	const psy_audio_SequenceTrack* self, const psy_audio_Patterns* patterns)
{
	psy_dsp_beatpos_t rv;
	const psy_List* p;

	assert(self);

	rv = psy_dsp_beatpos_zero();
	p = self->nodes;
	if (p) {		
		const psy_audio_SequenceEntry* seqentry;

		p = p->tail;
		seqentry = (const psy_audio_SequenceEntry*)p->entry;
		if (seqentry) {
			psy_dsp_beatpos_t length;

			length = psy_audio_sequenceentry_length(seqentry);
			if (psy_dsp_beatpos_less(rv, psy_dsp_beatpos_add(
					psy_audio_sequenceentry_offset(seqentry), length))) {
				rv = psy_dsp_beatpos_add(psy_audio_sequenceentry_offset(
					seqentry), length);
			}
		}
	}
	return rv;
}

void psy_audio_sequencetrack_set_name(psy_audio_SequenceTrack* self,
	const char* name)
{
	assert(self);

	psy_strreset(&self->name_, name);	
}

void psy_audio_sequencetrack_insert(psy_audio_SequenceTrack* self,
	uintptr_t order, psy_audio_Patterns* patterns, uintptr_t pat_idx)
{	
	psy_List* p;
	psy_audio_SequencePatternEntry* entry;

	assert(self);

	p = psy_list_at(self->nodes, order);
	if (!p) {
		p = psy_list_last(self->nodes);
	}	
	entry = psy_audio_sequencepatternentry_alloc_init(pat_idx,
		psy_dsp_beatpos_zero());	
	entry->patterns = patterns;
	p = psy_list_insert(&self->nodes, p, entry);
	psy_table_insert(&self->entries, order, p);
}

uintptr_t psy_audio_sequencetrack_remove(psy_audio_SequenceTrack* self,
	uintptr_t order)
{	
	psy_List* p;

	assert(self);

	p = psy_list_at(self->nodes, order);
	if (p) {
		psy_audio_SequenceEntry* entry;
		uintptr_t patidx;

		entry = (psy_audio_SequenceEntry*)p->entry;
		if (entry) {
			patidx = psy_INDEX_INVALID;
			if (entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				patidx = ((psy_audio_SequencePatternEntry*)entry)->patternslot;
			}
			psy_list_remove(&self->nodes, p);
			psy_audio_sequenceentry_dispose(entry);
			free(entry);
			entry = NULL;
			psy_table_remove(&self->entries, order);
			return patidx;
		}		
	}
	return psy_INDEX_INVALID;
}

void psy_audio_sequencetrack_update_entries(psy_audio_SequenceTrack* self)
{
	psy_audio_SequenceEntryNode* t;
	uintptr_t i;

	assert(self);

	psy_table_clear(&self->entries);
	t = self->nodes;
	i = 0;
	while (t != NULL) {		
		psy_table_insert(&self->entries, i, t);
		++i;
		t = t->next;
	}
}

psy_audio_SequenceEntry* psy_audio_sequencetrack_entry(psy_audio_SequenceTrack*
	self, uintptr_t row)
{	
	psy_List* p;
	
	assert(self);
		
	p = psy_table_at(&self->entries, row);
	if (p) {
		return (psy_audio_SequenceEntry*)p->entry;
	}
	return NULL;
}

psy_audio_SequenceEntryNode* psy_audio_sequencetrack_node_at_offset(
	psy_audio_SequenceTrack* self, psy_dsp_beatpos_t offset)
{	
	uintptr_t mid;
	uintptr_t l;
	uintptr_t r;	

	if (psy_audio_sequencetrack_size(self) == 0) {
		return NULL;
	}
	l = 0;
	r = psy_audio_sequencetrack_size(self) - 1;	
	while (r >= l) {
		psy_audio_SequenceEntry* entry;
		psy_audio_SequenceEntry* next_entry;
		psy_dsp_beatpos_t reposition;

		mid = (l + r) / 2;
		entry = psy_audio_sequencetrack_entry(self, mid);
		if (!entry) {
			mid = psy_INDEX_INVALID;
			break;
		}
		next_entry = psy_audio_sequencetrack_entry(self, mid + 1);
		if (next_entry) {
			reposition = psy_audio_sequenceentry_reposition_offset(next_entry);
		} else {
			reposition = psy_dsp_beatpos_zero();
		}
		if (psy_dsp_beatpos_greater_equal(offset,
				psy_audio_sequenceentry_offset(entry)) &&
					psy_dsp_beatpos_less(offset, psy_dsp_beatpos_add(
					psy_audio_sequenceentry_right_offset(entry), reposition))) {
			break;
		} else {
			if (psy_dsp_beatpos_greater(offset,
					psy_audio_sequenceentry_offset(entry))) {
				l = mid + 1;
			} else {
				r = mid - 1;
			}
		}
	}
	if (mid > 0) {
		psy_audio_SequenceEntry* entry;

		entry = psy_audio_sequencetrack_entry(self, mid);
		if (entry) {
			psy_dsp_beatpos_t offset;

			offset = psy_audio_sequenceentry_offset(entry);
			l = 0;			
			r = mid - 1;
			do {
				psy_audio_SequenceEntry* entry;

				entry = psy_audio_sequencetrack_entry(self, r);
				if (entry && psy_dsp_beatpos_less(
						psy_audio_sequenceentry_offset(entry), offset)) {
					break;
				}
				mid = r;
				if (r > 0) {
					--r;
				}
			} while (r >= 0);
		}
	}
	return psy_table_at(&self->entries, mid);	
}

uintptr_t psy_audio_sequencetrack_size(const psy_audio_SequenceTrack* self)
{
	return psy_table_size(&self->entries);
}


/* Sequence */

/* prototypes */
static void psy_audio_sequence_initsignals(psy_audio_Sequence*);
static void psy_audio_sequence_initglobaltrack(psy_audio_Sequence*);
static void psy_audio_sequence_dispose_signals(psy_audio_Sequence*);
static void sequence_onpatternlengthchanged(psy_audio_Sequence*,
	psy_audio_Pattern* sender);
static void sequence_makeiterator(psy_audio_Sequence*, psy_List* entries,
	psy_audio_SequenceTrackIterator* rv);	

/* implementation */
void psy_audio_sequence_init(psy_audio_Sequence* self,
	psy_audio_Patterns* patterns, psy_audio_Samples* samples)
{
	assert(self);

	psy_audio_sequence_initsignals(self);	
	psy_table_init(&self->tracks_);
	self->patterns = patterns;
	self->samples = samples;
	self->preventreposition = FALSE;
	self->sequencerduration = NULL;
	psy_audio_sequence_initsignals(self);	
	psy_audio_sequence_initglobaltrack(self);
	psy_audio_trackstate_init(&self->trackstate);
	/* init editposition */
	psy_audio_sequencecursor_init(&self->cursor);
	self->lastcursor = self->cursor;
	psy_audio_sequenceselection_init(&self->selection);
	psy_audio_sequenceselection_select(&self->selection,
		psy_audio_orderindex_make(0, 0));	
}

void psy_audio_sequence_initglobaltrack(psy_audio_Sequence* self)
{	
	psy_audio_SequencePatternEntry* entry;

	psy_audio_sequencetrack_init(&self->globaltrack);	
	entry = psy_audio_sequencepatternentry_alloc_init(psy_audio_GLOBALPATTERN,
		psy_dsp_beatpos_zero());
	entry->patterns = self->patterns;
	psy_list_append(&self->globaltrack.nodes, entry);
	psy_audio_sequence_reposition_track(self, &self->globaltrack);
}

void psy_audio_sequence_initsignals(psy_audio_Sequence* self)
{	
	psy_signal_init(&self->signal_clear);
	psy_signal_init(&self->signal_insert);
	psy_signal_init(&self->signal_remove);
	psy_signal_init(&self->signal_reorder);
	psy_signal_init(&self->signal_track_insert);
	psy_signal_init(&self->signal_track_remove);
	psy_signal_init(&self->signal_track_swap);
	psy_signal_init(&self->signal_track_reposition);
	psy_signal_init(&self->signal_solo_changed);
	psy_signal_init(&self->signal_mute_changed);
	psy_signal_init(&self->signal_cursor_changed);
	psy_signal_init(&self->signal_tweak);
}

void psy_audio_sequence_dispose(psy_audio_Sequence* self)
{
	psy_table_dispose_all(&self->tracks_, (psy_fp_disposefunc)
		psy_audio_sequencetrack_dispose);
	psy_audio_sequencetrack_dispose(&self->globaltrack);	
	psy_audio_trackstate_dispose(&self->trackstate);	
	psy_audio_sequence_dispose_signals(self);	
	if (self->sequencerduration) {
		psy_audio_sequencer_dispose(self->sequencerduration);
		free(self->sequencerduration);
		self->sequencerduration = NULL;
	}
	psy_audio_sequenceselection_dispose(&self->selection);	
}

void psy_audio_sequence_dispose_signals(psy_audio_Sequence* self)
{	
	psy_signal_dispose(&self->signal_clear);
	psy_signal_dispose(&self->signal_insert);
	psy_signal_dispose(&self->signal_remove);
	psy_signal_dispose(&self->signal_track_swap);
	psy_signal_dispose(&self->signal_reorder);
	psy_signal_dispose(&self->signal_track_insert);
	psy_signal_dispose(&self->signal_track_remove);
	psy_signal_dispose(&self->signal_track_reposition);
	psy_signal_dispose(&self->signal_solo_changed);
	psy_signal_dispose(&self->signal_mute_changed);
	psy_signal_dispose(&self->signal_cursor_changed);
	psy_signal_dispose(&self->signal_tweak);
}

void psy_audio_sequence_copy(psy_audio_Sequence* self,
	psy_audio_Sequence* other)
{	
	uintptr_t t;

	psy_audio_sequence_clear(self);
	self->patterns = other->patterns;
	self->samples = other->samples;
	for (t = 0; t < psy_audio_sequence_num_tracks(other); ++t) {
		psy_audio_SequenceTrack* src_track;

		src_track = psy_audio_sequence_track_at(other, t);
		if (src_track) {
			psy_audio_sequence_append_track(self,
				psy_audio_sequencetrack_clone(src_track));
		}
	}
	psy_audio_trackstate_copy(&self->trackstate, &other->trackstate);	
}

void psy_audio_sequence_clear(psy_audio_Sequence* self)
{
	psy_table_dispose_all(&self->tracks_, (psy_fp_disposefunc)
		psy_audio_sequencetrack_dispose);
	psy_table_init(&self->tracks_);	
	psy_audio_trackstate_dispose(&self->trackstate);
	psy_audio_trackstate_init(&self->trackstate);
	psy_signal_emit(&self->signal_clear, self, 0);
}

void psy_audio_sequence_reposition_track(psy_audio_Sequence* self,
	psy_audio_SequenceTrack* track)
{	
	if (!self->preventreposition) {
		psy_dsp_beatpos_t curroffset;	
		psy_List* p;
		uintptr_t row;
		uintptr_t t;
			
		curroffset = psy_dsp_beatpos_zero();
		for (p = track->nodes, row = 0; p != NULL;
				psy_list_next(&p), ++row) {			
			psy_audio_SequenceEntry* seqentry;
		
			seqentry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (seqentry) {
				seqentry->row = row;
				seqentry->offset = psy_dsp_beatpos_add(
					curroffset, seqentry->repositionoffset);
				curroffset = psy_dsp_beatpos_add(
					psy_audio_sequenceentry_offset(seqentry),
					psy_audio_sequenceentry_length(seqentry));
			}
		}
		psy_audio_sequencetrack_update_entries(track);
		for (t = 0; t < psy_audio_sequence_num_tracks(self); ++t) {
			if (psy_audio_sequence_track_at(self, t) == track) {
				break;
			}			
		}		
		psy_signal_emit(&self->signal_track_reposition, self, 1, t);
	}	
}

void psy_audio_reposition(psy_audio_Sequence* self)
{
	uintptr_t t;

	for (t = 0; t < psy_audio_sequence_num_tracks(self); ++t) {
		psy_audio_SequenceTrack* track;

		track = psy_audio_sequence_track_at(self, t);
		if (track) {
			psy_audio_sequence_reposition_track(self, track);
		}
	}
}


void psy_audio_sequence_begin(psy_audio_Sequence* self,
	uintptr_t track_idx, psy_dsp_beatpos_t position,
	psy_audio_SequenceTrackIterator* rv)
{			
	psy_audio_SequenceEntry* entry;	
	psy_audio_SequenceTrack* track;
	
	rv->patterns = self->patterns;
	if (track_idx == psy_audio_GLOBALTRACK) {		
		track = &self->globaltrack;
	} else {
		track = psy_audio_sequence_track_at(self, track_idx);
	}
	rv->track = track;
	if (!track) {
		rv->sequencentrynode = NULL;		
		rv->patternnode = rv->prevpatternnode = NULL;
	} else {
		rv->sequencentrynode = psy_audio_sequencetrack_node_at_offset(
			track, position);
		if (rv->sequencentrynode) {
			entry = (psy_audio_SequenceEntry*)rv->sequencentrynode->entry;
			if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				psy_audio_Pattern* pattern;

				psy_audio_sequencetrackiterator_reset_pattern_node(rv);	
				pattern = psy_audio_sequencetrackiterator_pattern(rv);
				if (pattern) {
					rv->patternnode = psy_audio_pattern_greater_equal(pattern,
						psy_dsp_beatpos_sub(position, entry->offset),
						&rv->prevpatternnode);					
				} else {
					rv->patternnode = rv->prevpatternnode = NULL;
				}
			} else {			
				rv->patternnode = rv->prevpatternnode = NULL;
			}
		} else {			
			rv->patternnode = rv->prevpatternnode = NULL;
		}
	}
}

void psy_audio_sequencetrackiterator_init(psy_audio_SequenceTrackIterator* self)
{	
	self->track = NULL;
	self->patterns = NULL;
	self->sequencentrynode = NULL;
	self->patternnode = self->prevpatternnode = NULL;
	psy_audio_add_seqiterator(self);
}

void psy_audio_sequencetrackiterator_dispose(psy_audio_SequenceTrackIterator* self)
{
	psy_audio_remove_seqiterator(self);	
}

psy_audio_SequenceTrackIterator* psy_audio_sequencetrackiterator_alloc(void)
{
	return (psy_audio_SequenceTrackIterator*)malloc(sizeof(
		psy_audio_SequenceTrackIterator));
}

psy_audio_SequenceTrackIterator* psy_audio_sequencetrackiterator_alloc_init(void)
{
	psy_audio_SequenceTrackIterator* rv;

	rv = psy_audio_sequencetrackiterator_alloc();
	if (rv) {
		psy_audio_sequencetrackiterator_init(rv);
	}
	return rv;
}

void psy_audio_sequencetrackiterator_reset_pattern_node(
	psy_audio_SequenceTrackIterator* self)
{
	psy_audio_Pattern* pattern;

	pattern = psy_audio_sequencetrackiterator_pattern(self);
	if (pattern) {
		self->patternnode = psy_audio_pattern_begin(pattern);
	} else {
		self->patternnode = NULL;
	}	
}

bool psy_audio_sequencetrackiterator_end(const psy_audio_SequenceTrackIterator* self)
{
	return (self->sequencentrynode == NULL);			
}

void psy_audio_sequencetrackiterator_inc(psy_audio_SequenceTrackIterator* self)
{	
	if (self->patternnode) {		
		self->patternnode = self->patternnode->next;		
		if (self->patternnode == NULL && self->sequencentrynode) {
			if (self->sequencentrynode->next) {
				psy_audio_SequenceEntry* entry;				

				do {
					self->sequencentrynode = self->sequencentrynode->next;
					entry = (psy_audio_SequenceEntry*)self->sequencentrynode->entry;
				} while (entry->type != psy_audio_SEQUENCEENTRY_PATTERN);
				psy_audio_sequencetrackiterator_reset_pattern_node(self);				
			}
		}
	}
}

void psy_audio_sequencetrackiterator_inc_entry(psy_audio_SequenceTrackIterator* self)
{	
	if (self->sequencentrynode) {
		self->sequencentrynode = self->sequencentrynode->next;
		psy_audio_sequencetrackiterator_reset_pattern_node(self);		
	}
}

void psy_audio_sequencetrackiterator_dec_entry(psy_audio_SequenceTrackIterator* self)
{	
	if (self->sequencentrynode) {
		self->sequencentrynode = self->sequencentrynode->prev;
		psy_audio_sequencetrackiterator_reset_pattern_node(self);
	}
}

bool psy_audio_sequencetrackiterator_has_next_entry(
	const psy_audio_SequenceTrackIterator* self)
{
	return ((self->sequencentrynode) && (self->sequencentrynode->next));
}

void sequence_makeiterator(psy_audio_Sequence* self, psy_List* entries,
	psy_audio_SequenceTrackIterator* rv)
{	
	rv->patterns = self->patterns;
	rv->sequencentrynode = entries;
	psy_audio_sequencetrackiterator_reset_pattern_node(rv);	
}

void psy_audio_sequence_append_track(psy_audio_Sequence* self,
	psy_audio_SequenceTrack* track)
{	
	uintptr_t newindex;
	
	assert(self);

	newindex = psy_table_size(&self->tracks_);
	psy_table_insert(&self->tracks_, newindex, track);
	psy_signal_emit(&self->signal_track_insert, self, 1, newindex);	
}

uintptr_t psy_audio_sequence_set_track(psy_audio_Sequence* self,
	psy_audio_SequenceTrack* track, uintptr_t index)
{
	assert(self);
		
	psy_table_insert(&self->tracks_, index, track);
	psy_signal_emit(&self->signal_track_insert, self, 1, index);	
	return (index + 1);
}

void psy_audio_sequence_remove_track(psy_audio_Sequence* self,
	uintptr_t track_idx)
{
	psy_audio_SequenceTrack* track;
	uintptr_t size;
	uintptr_t t;

	assert(self);	
	
	size = psy_table_size(&self->tracks_);
	if (size == 0) {
		return;
	}
	track = psy_audio_sequence_track_at(self, track_idx);
	for (t = track_idx; t < size; ++t) {
		psy_audio_SequenceTrack* curr;

		curr = psy_audio_sequence_track_at(self, t + 1);
		psy_table_insert(&self->tracks_, t, curr);
	}	
	psy_table_remove(&self->tracks_, size - 1);	
	if (track) {
		psy_audio_sequencetrack_dispose(track);
	}
	psy_signal_emit(&self->signal_track_remove, self, 1, track_idx);	
}

psy_audio_SequenceTrack* psy_audio_sequence_track_at(psy_audio_Sequence* self,
	uintptr_t index)
{
	assert(self);
	
	return (psy_audio_SequenceTrack*)psy_table_at(&self->tracks_, index);
}

const psy_audio_SequenceTrack* psy_audio_sequence_track_at_const(const
	psy_audio_Sequence* self, uintptr_t index)
{
	assert(self);

	return psy_audio_sequence_track_at((psy_audio_Sequence*)self, index);
}

uintptr_t psy_audio_sequence_num_tracks(const psy_audio_Sequence* self)
{
	return psy_table_size(&self->tracks_);
}

uintptr_t psy_audio_sequence_width(const psy_audio_Sequence* self)
{
	return psy_table_size(&self->tracks_);	
}

bool psy_audio_sequence_patternused(psy_audio_Sequence* self, uintptr_t patternslot)
{
	int rv = FALSE;
	uintptr_t t;

	assert(self);
	
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_audio_SequenceEntryNode* p;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		p = track->nodes;
		while (p) {
			psy_audio_SequencePatternEntry* sequenceentry;

			sequenceentry = (psy_audio_SequencePatternEntry*)psy_list_entry(p);
			if (sequenceentry) {
				if (psy_audio_sequencepatternentry_patternslot(sequenceentry) ==
					patternslot) {
					rv = TRUE;
					break;
				}
			}
			psy_list_next(&p);
		}		
	}
	return rv;
}

psy_audio_OrderIndex psy_audio_sequence_pattern_first_used(psy_audio_Sequence* self,
	uintptr_t patternslot)
{
	psy_audio_OrderIndex rv;
	uintptr_t t;	

	assert(self);
	rv.track = psy_INDEX_INVALID;
	rv.order = psy_INDEX_INVALID;
			
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_audio_SequenceEntryNode* p;
		uintptr_t currorderidx;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		currorderidx = 0;
		p = track->nodes;
		while (p) {
			psy_audio_SequenceEntry* entry;

			entry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				psy_audio_SequencePatternEntry* seqpatternentry;

				seqpatternentry = (psy_audio_SequencePatternEntry*)entry;
				if (psy_audio_sequencepatternentry_patternslot(seqpatternentry)
						== patternslot) {
					psy_audio_Pattern* pattern;

					pattern = psy_audio_patterns_at(self->patterns, patternslot);
					rv.track = t;
					rv.order = currorderidx;
					break;
				}
			}
			psy_list_next(&p);
			++currorderidx;
		}				
	}
	return rv;
}

psy_dsp_beatpos_t psy_audio_sequence_duration(const psy_audio_Sequence* self)
{	
	psy_dsp_beatpos_t rv;
	uintptr_t t;

	assert(self);

	rv = psy_dsp_beatpos_zero();
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		const psy_audio_SequenceTrack* track;          
		psy_dsp_beatpos_t trackduration;

		track = psy_audio_sequence_track_at_const(self, t);
		if (!track) {
			continue;
		}
		trackduration = psy_audio_sequencetrack_duration(track,
			self->patterns);		
		if (psy_dsp_beatpos_less(rv, trackduration)) {
			rv = trackduration;
		}
	}
	return rv;
}

uintptr_t psy_audio_sequence_maxtracksize(psy_audio_Sequence* self)
{
	uintptr_t rv = 0;
	uintptr_t t;
	
	assert(self);

	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		uintptr_t tracksize;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		tracksize = psy_list_size(track->nodes);
		if (tracksize > rv) {
			rv = tracksize;
		}
	}
	return rv;
}

void psy_audio_sequence_set_play_selection(psy_audio_Sequence* self,
	psy_audio_SequenceSelection* selection)
{
	psy_List* p;

	psy_audio_sequence_clear_play_selection(self);
	for (p = selection->entries; p != NULL; psy_list_next(&p)) {
		psy_audio_OrderIndex* index;
		psy_audio_SequenceEntry* entry;

		index = (psy_audio_OrderIndex*)psy_list_entry(p);		
		entry = psy_audio_sequence_entry(self, *index);
		if (entry) {
			entry->selplay = TRUE;
		}
	}
}

void psy_audio_sequence_clear_play_selection(psy_audio_Sequence* self)
{
	uintptr_t t;
	
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_List* p;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		for (p = track->nodes; p != NULL; psy_list_next(&p)) {
			psy_audio_SequencePatternEntry* entry;

			entry = (psy_audio_SequencePatternEntry*)p->entry;
			if (entry) {
				entry->entry.selplay = FALSE;
			}
		}
	}
}

void psy_audio_sequence_start_calc_duration_in_ms(psy_audio_Sequence* self)
{
	self->durationms = 0.0;
	if (!self->sequencerduration) {
		self->sequencerduration =
			(psy_audio_Sequencer*)malloc(sizeof(psy_audio_Sequencer));
	}
	if (!self->sequencerduration) {
		return;
	}
	psy_audio_sequencer_init(self->sequencerduration, self, NULL);
	psy_audio_sequencer_stop_loop(self->sequencerduration);
	psy_audio_sequencer_start(self->sequencerduration);
	self->sequencerduration->calcduration = TRUE;
}

psy_dsp_seconds_t psy_audio_sequence_endcalcdurationinmsresult(psy_audio_Sequence* self)
{	
	if (self->sequencerduration) {
		psy_dsp_seconds_t rv;

		rv = psy_audio_sequencer_curr_play_time(self->sequencerduration);
		psy_audio_sequencer_dispose(self->sequencerduration);
		free(self->sequencerduration);
		self->sequencerduration = NULL;
		return rv;
	}
	return 0.0;
}

bool psy_audio_sequence_calc_duration_in_ms(psy_audio_Sequence* self)
{		
	uintptr_t maxamount;
	uintptr_t amount;
	uintptr_t numsamplex;

	if (!self->sequencerduration) {
		return FALSE;
	}		
	numsamplex = 8192; /* psy_audio_MAX_STREAM_SIZE; */
	maxamount = numsamplex;		
	do {
		amount = maxamount;
		if (amount > numsamplex) {
			amount = numsamplex;
		}
		if (self->sequencerduration->linetickcount <= 
				psy_audio_sequencer_frame_to_offset(self->sequencerduration, amount)) {
			if (self->sequencerduration->linetickcount > 0.0) {					
				uintptr_t pre;

				pre = psy_audio_sequencer_frames(self->sequencerduration,
					self->sequencerduration->linetickcount);
				if (pre) {
					pre--;
					if (pre) {
						psy_audio_sequencer_frame_tick(self->sequencerduration, pre);
						numsamplex -= pre;
						amount -= pre;
						self->sequencerduration->linetickcount -=							
							psy_audio_sequencer_frame_to_offset(
								self->sequencerduration, pre);
					}
				}
			}					
			psy_audio_sequencer_on_newline(self->sequencerduration);
		}			
		if (amount > 0) {
			psy_audio_sequencer_frame_tick(self->sequencerduration, amount);
			numsamplex -= amount;
			self->sequencerduration->linetickcount -= 				
				psy_audio_sequencer_frame_to_offset(self->sequencerduration,
					amount);
		}
	} while (numsamplex > 0);
	return (psy_audio_sequencer_playing(self->sequencerduration));	
}

void sequence_onpatternlengthchanged(psy_audio_Sequence* self,
	psy_audio_Pattern* sender)
{
	psy_audio_reposition(self);	
}

/* TrackState */
void psy_audio_sequence_activate_solo_track(psy_audio_Sequence* self,
	uintptr_t track_index)
{
	assert(self);

	psy_audio_trackstate_activatesolotrack(&self->trackstate, track_index);
	psy_signal_emit(&self->signal_solo_changed, self, 1, track_index);
}

void psy_audio_sequence_deactivate_solo_track(psy_audio_Sequence* self)
{
	uintptr_t soloedtrack;

	assert(self);	
	
	soloedtrack = self->trackstate.soloedtrack;	
	psy_audio_trackstate_deactivatesolotrack(&self->trackstate);
	psy_signal_emit(&self->signal_solo_changed, self, 1, soloedtrack);
}

void psy_audio_sequence_mute_track(psy_audio_Sequence* self, uintptr_t track_index)
{
	assert(self);

	psy_audio_trackstate_mutetrack(&self->trackstate, track_index);
	psy_signal_emit(&self->signal_mute_changed, self, 1, track_index);
}

void psy_audio_sequence_unmute_track(psy_audio_Sequence* self, uintptr_t track_index)
{
	assert(self);

	psy_audio_trackstate_unmutetrack(&self->trackstate, track_index);
	psy_signal_emit(&self->signal_mute_changed, self, 1, track_index);
}

int psy_audio_sequence_is_track_muted(const psy_audio_Sequence* self,
	uintptr_t track_index)
{
	assert(self);

	return psy_audio_trackstate_istrackmuted(&self->trackstate, track_index);
}

bool psy_audio_sequence_is_track_soloed(const psy_audio_Sequence* self,
	uintptr_t track_index)
{
	assert(self);

	return psy_audio_trackstate_istracksoloed(&self->trackstate, track_index);
}

double psy_audio_sequence_track_height(const psy_audio_Sequence* self,
	uintptr_t track_index)
{
	const psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at_const(self, track_index);
	if (track) {
		return psy_audio_sequencetrack_height(track);
	}
	return 0.0;
}

void psy_audio_sequence_set_track_height(psy_audio_Sequence* self,
	uintptr_t track_index, double height)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at(self, track_index);
	if (track) {
		psy_audio_sequencetrack_set_height(track, height);
	}	
}

/* EditPosition */

void psy_audio_sequence_set_cursor(psy_audio_Sequence* self,
	psy_audio_SequenceCursor cursor)
{
	// if (!psy_audio_sequencecursor_equal(&cursor, &self->cursor)) {
		self->lastcursor = self->cursor;
		self->cursor = cursor;
		if (!psy_audio_orderindex_equal(&self->lastcursor.order_index,
				self->cursor.order_index)) {					
			psy_audio_sequenceselection_deselect_all(&self->selection);			
			psy_audio_sequenceselection_select_first(&self->selection,
				self->cursor.order_index);
		}		
		psy_signal_emit(&self->signal_cursor_changed, self, 0);		
	// }
}

psy_dsp_beatpos_t psy_audio_sequence_seq_offset(const psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{
	const psy_audio_SequenceEntry* entry;
	
	entry = psy_audio_sequence_entry_const(self, index);
	if (entry) {
		return psy_audio_sequenceentry_offset(entry);
	}
	return psy_dsp_beatpos_zero();
}

void psy_audio_sequence_dec_seqpos(psy_audio_Sequence* self)
{
	if (psy_audio_sequenceselection_first(&self->selection).order
			> 0) {
		psy_audio_SequenceCursor cursor;

		cursor = self->cursor;
		psy_audio_sequencecursor_set_order_index(&cursor,
			psy_audio_orderindex_make(
				psy_audio_sequenceselection_first(&self->selection).track,
				psy_audio_sequenceselection_first(&self->selection).order - 1));
		psy_audio_sequence_set_cursor(self, cursor);
	}
}

void psy_audio_sequence_inc_seqpos(psy_audio_Sequence* self)
{
	if (psy_audio_sequenceselection_first(&self->selection).order + 1 <
			psy_audio_sequence_track_size(self,
				psy_audio_sequenceselection_first(
					&self->selection).track)) {
		psy_audio_SequenceCursor cursor;

		cursor = self->cursor;
		psy_audio_sequencecursor_set_order_index(&cursor,
			psy_audio_orderindex_make(
				psy_audio_sequenceselection_first(&self->selection).track,
				psy_audio_sequenceselection_first(&self->selection).order + 1));
		psy_audio_sequence_set_cursor(self, cursor);
	}
}


/* SequencePaste */
void psy_audio_sequencepaste_init(psy_audio_SequencePaste* self)
{
	self->nodes = NULL;	
}

void psy_audio_sequencepaste_dispose(psy_audio_SequencePaste* self)
{
	psy_list_deallocate(&self->nodes, (psy_fp_disposefunc)
		psy_audio_order_dispose);
}

void psy_audio_sequencepaste_clear(psy_audio_SequencePaste* self)
{
	psy_list_deallocate(&self->nodes, (psy_fp_disposefunc)
		psy_audio_order_dispose);
}

void psy_audio_sequencepaste_copy(psy_audio_SequencePaste* self,
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection)
{
	psy_List* p;	
	psy_audio_OrderIndex minindex;

	psy_audio_sequencepaste_clear(self);
	minindex.order = psy_INDEX_INVALID;
	minindex.track = psy_INDEX_INVALID;
	for (p = selection->entries; p != NULL; psy_list_next(&p)) {
		psy_audio_OrderIndex* index;
		psy_audio_SequenceEntry* entry;
		psy_audio_Order* order;

		index = (psy_audio_OrderIndex*)psy_list_entry(p);
		assert(index);
		entry = psy_audio_sequence_entry(sequence, *index);
		if (entry) {
			order = (psy_audio_Order*)malloc(sizeof(psy_audio_Order));	
			psy_audio_order_init(order);
			if (order) {				
				order->index = *index;
				if (order->index.order < minindex.order) {
					minindex.order = order->index.order;
				}
				if (order->index.track < minindex.track) {
					minindex.track = order->index.track;
				}
				psy_audio_order_set_entry(order, entry);				
				psy_list_append(&self->nodes, order);
			}
		}
	}
	for (p = self->nodes; p != NULL; psy_list_next(&p)) {
		psy_audio_Order* order;

		order = (psy_audio_Order*)psy_list_entry(p);
		if (psy_audio_orderindex_valid(&order->index)) {
			order->index.order -= minindex.order;
			order->index.track -= minindex.track;
		}
	}
}

/* Sequence */
void psy_audio_sequence_insert(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, uintptr_t patidx)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at(self, index.track);
	if (track) {
		psy_audio_sequencetrack_insert(track, index.order, self->patterns, patidx);		
		psy_audio_sequence_reposition_track(self, track);
		if (self->patterns) {
			psy_audio_Pattern* pattern;

			pattern = psy_audio_patterns_at(self->patterns, patidx);
			if (pattern) {
				psy_signal_connect(&pattern->signal_length_changed,
					self, sequence_onpatternlengthchanged);
			}
		}
		psy_signal_emit(&self->signal_insert, self, 1, &index);		
	}
}

void psy_audio_sequence_insert_sample(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, psy_audio_SampleIndex sampleindex)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at(self, index.track);
	if (track) {
		psy_List* p;
		psy_audio_SequenceSampleEntry* entry;

		p = psy_list_at(track->nodes, index.order);
		if (!p) {
			p = psy_list_last(track->nodes);
		}
		entry = psy_audio_sequencesampleentry_alloc_init(
			psy_dsp_beatpos_zero(), sampleindex);
		entry->samples = self->samples;
		psy_list_insert(&track->nodes, p, entry);
		psy_audio_sequence_reposition_track(self, track);
		psy_signal_emit(&self->signal_insert, self, 1, &index);		
	}
}


void psy_audio_sequence_insert_marker(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, const char* text)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at(self, index.track);
	if (track) {
		psy_List* p;
		psy_audio_SequenceMarkerEntry* entry;

		p = psy_list_at(track->nodes, index.order);
		if (!p) {
			p = psy_list_last(track->nodes);
		}
		entry = psy_audio_sequencemarkerentry_alloc_init(
			psy_dsp_beatpos_zero(), text);		
		psy_list_insert(&track->nodes, p, entry);
		psy_audio_sequence_reposition_track(self, track);		
		psy_signal_emit(&self->signal_insert, self, 1, &index);		
	}
}

void psy_audio_sequence_remove(psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at(self, index.track);
	if (track) {
		uintptr_t patidx;

		patidx = psy_audio_sequencetrack_remove(track, index.order);		
		psy_audio_sequence_reposition_track(self, track);
		if (self->patterns && patidx != psy_INDEX_INVALID &&
				!psy_audio_sequence_patternused(self, patidx)) {
			psy_audio_Pattern* pattern;

			pattern = psy_audio_patterns_at(self->patterns, patidx);
			if (pattern) {
				psy_signal_disconnect_context(&pattern->signal_length_changed, self);
				psy_signal_disconnect_context(&pattern->signal_name_changed, self);				
			}
			psy_signal_emit(&self->signal_remove, self, 1, &index);			
		}
	}
}

void psy_audio_sequence_remove_selection(psy_audio_Sequence* self,
	psy_audio_SequenceSelection* selection)
{
	psy_audio_SequenceSelectionIterator ite;
	uintptr_t t;
	bool removed;
	uintptr_t c;
	uintptr_t trackidx;

	assert(self);
	
	removed = FALSE;
	ite = psy_audio_sequenceselection_begin(selection);
	for (; ite != NULL; psy_list_next(&ite)) {
		psy_audio_OrderIndex* orderindex;
		psy_audio_SequenceEntryNode* currnode;
		psy_audio_SequenceTrack* track;

		orderindex = ite->entry;
		assert(orderindex);
		track = NULL;
		currnode = psy_audio_sequence_node(self, *orderindex, &track);
		if (currnode && track) {
			psy_audio_sequenceentry_dispose(currnode->entry);			
			free(currnode->entry);
			currnode->entry = NULL;
		}		
	}
	trackidx = 0;
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_List* p;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}	
		c = 0;		
		p = track->nodes;
		while (p != NULL) {
			psy_audio_SequenceEntry* entry;

			entry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (!entry) {
				psy_audio_OrderIndex index;

				p = psy_list_remove(&track->nodes, p);
				index = psy_audio_orderindex_make(t, c);				
				psy_signal_emit(&self->signal_remove, self, 1, &index);
				removed = TRUE;
			} else {
				psy_list_next(&p);
				++c;
			}			
		}
		if (removed) {
			psy_audio_sequence_reposition_track(self, track);
		}
	}	
}

uintptr_t psy_audio_sequence_order(const psy_audio_Sequence* self,
	uintptr_t trackidx, psy_dsp_beatpos_t position)
{
	const psy_audio_SequenceTrack* track;

	assert(self);

	if (psy_dsp_beatpos_less(position, psy_dsp_beatpos_zero())) {
		return psy_INDEX_INVALID;
	}
	track = psy_audio_sequence_track_at_const(self, trackidx);
	if (track) {
		psy_dsp_beatpos_t curroffset;
		const psy_List* p;
		uintptr_t row;
		bool found;
		
		found = FALSE;
		curroffset = psy_dsp_beatpos_zero();
		for (p = track->nodes, row = 0; p != NULL;
				p = p->next, ++row) {			
			const psy_audio_SequenceEntry* seqentry;

			seqentry = (const psy_audio_SequenceEntry*)psy_list_entry_const(p);
			if (seqentry) {				
				if (psy_dsp_beatpos_less(position,
						psy_audio_sequenceentry_right_offset(seqentry))) {
					found = TRUE;
					break;
				}
			}
		}
		if (found) {
			return row;
		}
	}
	return psy_INDEX_INVALID;
}

psy_audio_SequenceEntry* psy_audio_sequence_entry(psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	if (psy_audio_orderindex_valid(&index)) {
		track = psy_audio_sequence_track_at(self, index.track);
		if (track) {
			return psy_audio_sequencetrack_entry(track, index.order);			
		}
	}
	return NULL;
}

psy_audio_SequenceEntryNode* psy_audio_sequence_node(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, psy_audio_SequenceTrack** rv)
{
	psy_audio_SequenceTrack* track;

	assert(self);

	if (psy_audio_orderindex_valid(&index)) {
		track = psy_audio_sequence_track_at(self, index.track);
		if (track) {
			psy_List* p;

			p = psy_list_at(track->nodes, index.order);
			if (p) {	
				*rv = track;
				return p;
			}
		}
	}
	*rv = NULL;
	return NULL;
}

const psy_audio_SequenceEntry* psy_audio_sequence_entry_const(const
	psy_audio_Sequence* self, psy_audio_OrderIndex index)
{
	return psy_audio_sequence_entry((psy_audio_Sequence*)self, index);
}

psy_audio_Pattern* psy_audio_sequence_pattern(psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{	
	psy_audio_SequenceEntry* entry;

	assert(self);

	entry = psy_audio_sequence_entry(self, index);
	if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
		return psy_audio_patterns_at(self->patterns,
			psy_audio_sequencepatternentry_patternslot(
				(const psy_audio_SequencePatternEntry*)entry));
	}
	return NULL;
}

const psy_audio_Pattern* psy_audio_sequence_pattern_const(
	const psy_audio_Sequence* self, psy_audio_OrderIndex orderindex)
{
	return psy_audio_sequence_pattern((psy_audio_Sequence*)self, orderindex);
}

uintptr_t psy_audio_sequence_track_size(const psy_audio_Sequence* self,
	uintptr_t trackindex)
{
	const psy_audio_SequenceTrack* track;

	assert(self);

	track = psy_audio_sequence_track_at((psy_audio_Sequence*)self, trackindex);
	if (track) {		
		return psy_list_size(track->nodes);
	}
	return 0;
}

void psy_audio_sequence_setpatternindex(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, uintptr_t patidx)
{
	psy_audio_SequenceEntry* entry;

	assert(self);

	entry = psy_audio_sequence_entry(self, index);
	if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
		psy_audio_Pattern* pattern;
		psy_dsp_beatpos_t oldlength;
		psy_audio_SequenceTrack* track;
		
		oldlength = psy_audio_sequenceentry_length(entry);
		pattern = psy_audio_patterns_at(self->patterns, patidx);
		if (pattern == NULL) {						
			pattern = psy_audio_patterns_insert(self->patterns, patidx,
				psy_audio_pattern_alloc_init());
			/* todo set correct default lines */
		}						
		psy_audio_sequencepatternentry_setpatternslot(
			(psy_audio_SequencePatternEntry*)entry, patidx);		
		track = psy_audio_sequence_track_at(self, index.track);
		if (track) {
			psy_audio_sequence_reposition_track(self, track);		
		}
	}
}

uintptr_t psy_audio_sequence_patternindex(const psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{
	const psy_audio_SequenceEntry* entry;

	assert(self);

	entry = psy_audio_sequence_entry_const(self, index);
	if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
		return ((const psy_audio_SequencePatternEntry*)entry)->patternslot;
	}
	return psy_INDEX_INVALID;
}

psy_dsp_beatpos_t psy_audio_sequence_offset(const psy_audio_Sequence* self,
	psy_audio_OrderIndex index)
{
	const psy_audio_SequenceEntry* seqentry;

	assert(self);

	seqentry = psy_audio_sequence_entry_const(self, index);
	if (seqentry) {
		return psy_audio_sequenceentry_offset(seqentry);
	}
	return psy_dsp_beatpos_zero();
}

void psy_audio_sequence_swaptracks(psy_audio_Sequence* self,
	uintptr_t src_id, uintptr_t dst_id)
{		
	psy_audio_SequenceTrack* src;
	psy_audio_SequenceTrack* dst;

	assert(self);
	
	if (src_id == dst_id) {
		return;
	}
	src = psy_audio_sequence_track_at(self, src_id);
	dst = psy_audio_sequence_track_at(self, dst_id);
	if (src && dst) {
		psy_audio_sequence_set_track(self, src, dst_id);
		psy_audio_sequence_set_track(self, dst, src_id);	
		psy_signal_emit(&self->signal_track_swap, self, 2, src_id, dst_id);		
	}
}

psy_audio_OrderIndex psy_audio_sequence_reorder(psy_audio_Sequence* self,
	psy_audio_OrderIndex index, psy_dsp_beatpos_t new_position)
{	
	psy_audio_SequenceTrack* track;	

	assert(self);	

	track = psy_audio_sequence_track_at(self, index.track);
	if (track && track->nodes) {
		psy_List* selected;

		selected = psy_list_at(track->nodes, index.order);
		if (selected) {
			psy_List* p;
			psy_dsp_beatpos_t curroffset;
			uintptr_t row;

			curroffset = psy_dsp_beatpos_zero();
			for (p = track->nodes, row = 0; p != NULL;
					p = p->next) {				
				psy_audio_SequenceEntry* entry;

				entry = (psy_audio_SequenceEntry*)p->entry;
				curroffset = psy_dsp_beatpos_add(curroffset,
					psy_dsp_beatpos_add(
						entry->repositionoffset,
						psy_audio_sequenceentry_length(entry)));
				if (psy_dsp_beatpos_less(new_position, curroffset)) {
					break;
				}
				++row;				
			}
			if (p != selected) {
				psy_audio_OrderIndex insertindex;

				if (!p) {
					p = track->nodes->tail;
				}
				psy_list_swap_entries(selected, p);
				psy_audio_sequence_reposition_track(self, track);
				insertindex = psy_audio_orderindex_make(index.track, row);
				psy_signal_emit(&self->signal_insert, self, 1, &insertindex);				
				return insertindex;
			}
		}				
	}
	return psy_audio_orderindex_make_invalid();
}

void psy_audio_sequence_resetpatterns(psy_audio_Sequence* self)
{
	uintptr_t t;	

	assert(self);
	
	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_List* p;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		for (p = track->nodes; p != NULL; psy_list_next(&p)) {
			psy_audio_SequenceEntry* entry;			

			entry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				/* todo default lines */
				psy_audio_SequencePatternEntry* seqpatternentry;

				seqpatternentry = (psy_audio_SequencePatternEntry*)entry;
				psy_audio_patterns_insert(self->patterns,
					seqpatternentry->patternslot, psy_audio_pattern_alloc_init());
			}
		}
	}
}

void psy_audio_sequence_fillpatterns(psy_audio_Sequence* self)
{
	uintptr_t t;

	assert(self);

	for (t = 0; t < psy_audio_sequence_width(self); ++t) {
		psy_audio_SequenceTrack* track;
		psy_List* p;

		track = psy_audio_sequence_track_at(self, t);
		if (!track) {
			continue;
		}
		for (p = track->nodes; p != NULL; psy_list_next(&p)) {
			psy_audio_SequenceEntry* entry;

			entry = (psy_audio_SequenceEntry*)psy_list_entry(p);
			if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				/* todo default lines */
				psy_audio_SequencePatternEntry* seqpatternentry;
				seqpatternentry = (psy_audio_SequencePatternEntry*)entry;
				if (!psy_audio_patterns_at(self->patterns,
						seqpatternentry->patternslot)) {
					psy_audio_patterns_insert(self->patterns,
						seqpatternentry->patternslot, psy_audio_pattern_alloc_init());
				}
			}
		}
	}
}

void psy_audio_sequence_block_copy_pattern(psy_audio_Sequence* self,
	psy_audio_BlockSelection selection, psy_audio_Pattern* dest)
{
	psy_audio_PatternCopyCmd cmd;

	assert(dest);
		
	psy_audio_patterncopycmd_init(&cmd, dest,
		psy_audio_sequencecursor_offset_abs(&selection.topleft, self),
		psy_audio_sequencecursor_channel(&selection.topleft));		
	psy_audio_pattern_clear(dest);
	psy_audio_pattern_set_max_song_tracks(dest,
		psy_audio_sequencecursor_channel(&selection.bottomright) -
		psy_audio_sequencecursor_channel(&selection.topleft));		
	psy_audio_pattern_set_length(dest,
		psy_dsp_beatpos_sub(
			psy_audio_sequencecursor_offset_abs(&selection.bottomright,
				self),
			psy_audio_sequencecursor_offset_abs(&selection.topleft,
				self)));		
	psy_audio_sequence_block_traverse(self, selection, &cmd.command);
	psy_command_dispose(&cmd.command);	
}

void psy_audio_sequence_block_traverse(psy_audio_Sequence* self,
	psy_audio_BlockSelection block, psy_Command* cmd)
{	
	psy_dsp_beatpos_t bpl;
	uintptr_t lpb;
	uintptr_t t;
	uintptr_t num_tracks;	

	assert(self);
	assert(cmd);
	
	if (!psy_audio_blockselection_valid(&block)) {
		return;
	}
	bpl = psy_audio_sequencecursor_bpl(&self->cursor);
	lpb = psy_audio_sequencecursor_lpb(&self->cursor);
	t = block.topleft.order_index.track;
	num_tracks = psy_audio_sequence_num_tracks(self);	
	while (t < num_tracks && t <= block.bottomright.order_index.track) {
		psy_audio_SequenceTrackIterator it;
		psy_dsp_beatpos_t abs_end;		
		
		psy_audio_sequencetrackiterator_init(&it);
		psy_audio_sequence_begin(self, t,
			psy_audio_sequencecursor_offset_abs(&block.topleft, self),
			&it);		
		abs_end = psy_audio_sequencecursor_offset_abs(
			&block.bottomright, self);		
		while (!psy_audio_sequencetrackiterator_end(&it) &&
				psy_dsp_beatpos_less(
					psy_audio_sequencetrackiterator_offset(&it),
					abs_end)) {
			if (it.patternnode) {
				psy_audio_PatternEntry* entry;

				entry = psy_audio_patternnode_entry(it.patternnode);
				if (psy_audio_patternentry_track(entry) >= psy_audio_sequencecursor_channel(&block.topleft) &&
						psy_audio_patternentry_track(entry) < psy_audio_sequencecursor_channel(&block.bottomright)) {
					SequenceTraverseParams params;
					
					params = sequencetraverseparams_make_all(						
						it.sequencentrynode,
						it.prevpatternnode,
						it.patternnode,
						psy_audio_sequencetrackiterator_pattern(&it),
						it.track, psy_audio_sequencetrackiterator_offset(&it),
						psy_audio_sequencetrackiterator_seq_offset(&it),
						bpl, lpb,
						psy_audio_patternentry_track(entry),
						t,
						TRUE);
					it.prevpatternnode = it.patternnode;
					psy_audio_sequencetrackiterator_inc(&it);				
					psy_command_execute(cmd, (uintptr_t)&params);
					if (params.prev_set) {
						it.prevpatternnode = params.prev_node;
					}
					if (params.stop) {
						t = psy_INDEX_INVALID;
						break;
					}
				} else {
					psy_audio_sequencetrackiterator_inc(&it);
				}
			} else {				
				psy_audio_sequencetrackiterator_inc_entry(&it);
			}			
		}
		psy_audio_sequencetrackiterator_dispose(&it);
		if (t == psy_INDEX_INVALID) {
			break;
		}
		++t;		
	}	
}

void psy_audio_sequence_block_grid_traverse(psy_audio_Sequence* self,
	psy_audio_BlockSelection block, psy_Command* cmd)
{	
	uintptr_t t;
	uintptr_t num_tracks;
	psy_dsp_beatpos_t bpl;
	uintptr_t lpb;

	assert(self);
	assert(cmd);
	
	if (!psy_audio_blockselection_valid(&block)) {
		return;
	}	
	t = block.topleft.order_index.track;
	num_tracks = psy_audio_sequence_num_tracks(self);
	bpl = psy_audio_sequencecursor_bpl(&self->cursor);
	lpb = psy_audio_sequencecursor_lpb(&self->cursor);
	while (t < num_tracks && t <= block.bottomright.order_index.track) {		
		psy_audio_SequenceTrackIterator ite;
		psy_dsp_beatpos_t abs_end;
		uintptr_t channel;		
		psy_dsp_beatpos_t offset;
		psy_dsp_beatpos_t seqoffset;
		psy_dsp_beatpos_t length;
		psy_audio_PatternNode* prev;		
		
		assert(self);
		
		psy_audio_sequencetrackiterator_init(&ite);			
		offset = psy_audio_sequencecursor_offset_abs(&block.topleft, self);
		psy_audio_sequence_begin(self, t,
			psy_audio_sequencecursor_offset_abs(&block.topleft, self),
			&ite);
		prev = ite.prevpatternnode;
		length = psy_audio_sequencetrackiterator_entry_length(&ite);
		seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);		
		abs_end = psy_audio_sequencecursor_offset_abs(
			&block.bottomright, self);		
		while (psy_dsp_beatpos_less(offset, abs_end)) {
			bool fill;
			psy_dsp_beatpos_t line_end;
			psy_dsp_beatpos_t rel_offset;
			psy_dsp_beatpos_t seq_entry_end;			

			rel_offset = psy_dsp_beatpos_sub(offset, seqoffset);
			seq_entry_end = psy_dsp_beatpos_add(seqoffset, length);
			line_end = psy_dsp_beatpos_add(offset, bpl);
			fill = !(psy_dsp_beatpos_greater_equal(offset, seqoffset) &&
				psy_dsp_beatpos_less(offset, seq_entry_end)) ||
				!ite.patternnode;
			/* fill line */
			for (channel = psy_audio_sequencecursor_channel(&block.topleft);
					channel < psy_audio_sequencecursor_channel(&block.bottomright);
					++channel) {
				bool has_event;				

				has_event = FALSE;
				while (!fill && ite.patternnode &&
					psy_audio_patternentry_track(psy_audio_sequencetrackiterator_pattern_entry(&ite)) <= channel &&
						psy_dsp_beatpos_testrange(
						psy_audio_sequencetrackiterator_offset(&ite), offset, bpl))
				{
					psy_audio_PatternEntry* entry;

					entry = psy_audio_sequencetrackiterator_pattern_entry(&ite);
					if (psy_audio_patternentry_track(entry) == channel) {
						SequenceTraverseParams params;
						
						params = sequencetraverseparams_make_all(							
							ite.sequencentrynode, prev, ite.patternnode,
							psy_audio_sequencetrackiterator_pattern(&ite),
							ite.track, offset, seqoffset, bpl, lpb, channel, t,
							TRUE);						
						prev = ite.patternnode;
						psy_list_next(&ite.patternnode);						
						psy_command_execute(cmd, (uintptr_t)&params);
						if (params.prev_set) {
							prev = params.prev_node;
						}
						if (params.stop) {
							t = psy_INDEX_INVALID;
							break;
						}						
						has_event = TRUE;
						break;
					}
					prev = ite.patternnode;			
					psy_list_next(&ite.patternnode);
				}
				if (!has_event) {
					if (psy_dsp_beatpos_less(offset, seq_entry_end)) {
						SequenceTraverseParams params;
						
						params = sequencetraverseparams_make_all(							
							ite.sequencentrynode, prev, NULL,
							psy_audio_sequencetrackiterator_pattern(&ite),
							ite.track, offset, seqoffset, bpl, lpb, channel, t,
							FALSE);
						psy_command_execute(cmd, (uintptr_t)&params);
						if (params.prev_set) {
							prev = params.prev_node;
						}
						if (params.stop) {
							t = psy_INDEX_INVALID;
							break;
						}						
					}				
				} else if (ite.patternnode && ((psy_audio_PatternEntry*)(
					ite.patternnode->entry))->track_ <= channel) {
					fill = TRUE;
				}				
			}
			/* skip remaining events of the line */		
			while (ite.patternnode && (psy_dsp_beatpos_less(
					psy_audio_sequencetrackiterator_offset(&ite), line_end))) {
				prev = ite.patternnode;
				psy_list_next(&ite.patternnode);
			}
			/* advance offset */
			offset = psy_dsp_beatpos_add(offset, bpl);	
			if (psy_dsp_beatpos_greater_equal(offset, seq_entry_end)) {
				psy_audio_SequenceEntry* seq_entry;
						
				psy_audio_sequencetrackiterator_inc_entry(&ite);
				seq_entry = psy_audio_sequencetrackiterator_entry(&ite);
				if (seq_entry) {				
					offset = seqoffset = psy_audio_sequenceentry_offset(seq_entry);
					length = psy_audio_sequenceentry_length(seq_entry);
					prev = NULL;
				} else {
					break;
				}
			}
		}	
		psy_audio_sequencetrackiterator_dispose(&ite);			
		if (t == psy_INDEX_INVALID) {
			break;
		}
		++t;
	}	
}

psy_audio_PatternEvent psy_audio_sequence_pattern_event_at_cursor(
	const psy_audio_Sequence* self, psy_audio_SequenceCursor cursor)
{
	const psy_audio_Pattern* pattern;	

	pattern = psy_audio_sequence_pattern_const(self, cursor.order_index);
	if (pattern) {
		return psy_audio_pattern_event_at_cursor(pattern, cursor);
	}
	return psy_audio_patternevent_zero();
}

void psy_audio_sequence_tweak(psy_audio_Sequence* self)
{
	psy_signal_emit(&self->signal_tweak, self, 0);	
}

psy_audio_BlockSelection psy_audio_sequence_block_selection_order(
	const psy_audio_Sequence* self, psy_audio_OrderIndex index)
{	
	const psy_audio_SequenceEntry* entry;
		
	assert(self);
	
	entry = psy_audio_sequence_entry_const(self, index);
	if (entry) {
		psy_audio_BlockSelection rv;
		
		psy_audio_blockselection_init_all(&rv,
			psy_audio_sequencecursor_make(index, 0, psy_dsp_beatpos_zero()),
			psy_audio_sequencecursor_make(index,
				(self->patterns)
				? psy_audio_patterns_num_tracks(self->patterns)
				: MAX_TRACKS,
				psy_audio_sequenceentry_length(entry)));
		rv.topleft.key = 0;
		rv.bottomright.key = psy_audio_NOTECOMMANDS_B9;
		return rv;
	}
	return psy_audio_blockselection_zero();
}

psy_audio_BlockSelection psy_audio_sequence_block_selection_track(
	const psy_audio_Sequence* self, uintptr_t seq_track_index)
{	
	const psy_audio_SequenceTrack* seqtrack;		
	const psy_audio_SequenceEntry* last_entry;
	uintptr_t num_orders;
	psy_audio_SequenceCursor top;
	psy_audio_SequenceCursor bottom;
	
	assert(self);
	
	seqtrack = psy_audio_sequence_track_at_const(self, seq_track_index);		
	if (!seqtrack) {
		return psy_audio_blockselection_zero();
	}
	num_orders = psy_audio_sequencetrack_size(seqtrack);		
	if (num_orders == 0) {
		return psy_audio_blockselection_zero();
	}
	last_entry = psy_audio_sequence_entry_const(self, psy_audio_orderindex_make(
		seq_track_index, num_orders - 1));
	if (!last_entry) {
		return psy_audio_blockselection_zero();
	}
	top = psy_audio_sequencecursor_make(
		psy_audio_orderindex_make(seq_track_index, 0),
		0, psy_dsp_beatpos_zero());
	top.key = 0;
	bottom = psy_audio_sequencecursor_make(
		psy_audio_orderindex_make(seq_track_index, num_orders - 1),			
		(self->patterns)
		? psy_audio_patterns_num_tracks(self->patterns)
		: MAX_TRACKS,
		psy_audio_sequenceentry_length(last_entry));
	bottom.key = psy_audio_NOTECOMMANDS_B9;
	return psy_audio_blockselection_make(top, bottom);
}
