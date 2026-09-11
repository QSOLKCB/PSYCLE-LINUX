/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "pattern.h"
/* local */
#include "exclusivelock.h"
#include "sequence.h"
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


static uintptr_t defaultlines = 64;
static psy_List* seqiterators = NULL;

/* Pattern */

/* prototypes */
static void psy_audio_pattern_init_signals(psy_audio_Pattern*);
static void psy_audio_pattern_dispose_signals(psy_audio_Pattern*);
static void psy_audio_pattern_checkiterators(psy_audio_Pattern*,
	const psy_audio_PatternNode*);

/* implementation */
void psy_audio_pattern_init(psy_audio_Pattern* self)
{
	assert(self);

	self->events_ = NULL;
	/* todo needs player lpb to be correct */
	self->length_ = psy_dsp_beatpos_make_real(defaultlines / 4.0,
		psy_dsp_DEFAULT_PPQ);
	self->name_ = strdup("Untitled");
	self->opcount_ = 0;
	self->max_song_tracks_ = 0;			
	psy_table_init(&self->track_names_);
	psy_audio_patternevent_init_timesig(&self->time_sig_, 0, 0);
	psy_audio_pattern_init_signals(self);
}

void psy_audio_pattern_init_signals(psy_audio_Pattern* self)
{
	assert(self);

	psy_signal_init(&self->signal_name_changed);
	psy_signal_init(&self->signal_length_changed);
}

void psy_audio_pattern_dispose(psy_audio_Pattern* self)
{		
	assert(self);

	psy_list_deallocate(&self->events_, (psy_fp_disposefunc)
		psy_audio_patternentry_dispose);
	free(self->name_);
	self->name_ = NULL;		
	psy_table_dispose_all(&self->track_names_, (psy_fp_disposefunc)NULL);
	psy_audio_pattern_dispose_signals(self);
}

void psy_audio_pattern_dispose_signals(psy_audio_Pattern* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_name_changed);
	psy_signal_dispose(&self->signal_length_changed);
}

void psy_audio_pattern_copy(psy_audio_Pattern* self, const psy_audio_Pattern* src)
{	
	const psy_audio_PatternNode* p;
	uintptr_t opcount_;

	assert(self);

	opcount_ = self->opcount_;
	psy_audio_pattern_dispose(self);
	psy_audio_pattern_init(self);
	for (p = src->events_; p != NULL; p = p->next) {
		const psy_audio_PatternEntry* source_entry;
		psy_audio_PatternEntry* entry;

		source_entry = psy_audio_patternnode_entry_const(p);
		entry = psy_audio_patternentry_clone(source_entry);		
		if (entry) {
			psy_list_append(&self->events_, entry);
		}
	}
	self->length_ = src->length_;
	psy_strreset(&self->name_, src->name_);	
	++(self->opcount_);
}

psy_audio_Pattern* psy_audio_pattern_alloc(void)
{
	return (psy_audio_Pattern*)malloc(sizeof(psy_audio_Pattern));
}

psy_audio_Pattern* psy_audio_pattern_alloc_init(void)
{
	psy_audio_Pattern* rv;

	rv = psy_audio_pattern_alloc();
	if (rv) {
		psy_audio_pattern_init(rv);
	}
	return rv;
}

psy_audio_Pattern* psy_audio_pattern_clone(const psy_audio_Pattern* self)
{	
	psy_audio_Pattern* rv;	

	assert(self);
	rv = psy_audio_pattern_alloc_init();
	if (rv) {
		psy_audio_pattern_copy(rv, self);
	}
	return rv;
}

void psy_audio_pattern_clear(psy_audio_Pattern* self)
{
	assert(self);

	psy_audio_pattern_dispose(self);
	psy_audio_pattern_init(self);
}

void psy_audio_pattern_remove(psy_audio_Pattern* self, psy_audio_PatternNode*
	node)
{
	assert(self);

	if (node) {
		psy_audio_PatternEntry* entry;

		psy_audio_pattern_checkiterators(self, node);
		entry = psy_audio_patternnode_entry(node);
		psy_list_remove(&self->events_, node);
		psy_audio_patternentry_dispose(entry);
		free(entry);
		++self->opcount_;		
	}
}

void psy_audio_pattern_erase(psy_audio_Pattern* self, psy_audio_PatternNode*
	node)
{
	assert(self);

	if (node) {
		psy_audio_PatternEntry* entry;
				
		psy_audio_pattern_checkiterators(self, node);
		entry = psy_audio_patternnode_entry(node);
		psy_list_remove(&self->events_, node);		
	}
}

psy_audio_PatternNode* psy_audio_pattern_insert(psy_audio_Pattern* self,
	psy_audio_PatternNode* prev, uintptr_t track, psy_dsp_beatpos_t offset,
	const psy_audio_PatternEntry* src)
{
	psy_audio_PatternNode* rv;	

	assert(self);
	
	if (src) {
		psy_audio_PatternEntry* entry;
		
		entry = psy_audio_patternentry_clone(src);
		entry->offset_ = offset;
		psy_audio_patternentry_set_track(entry, track);
		if (!self->events_) {
			rv = self->events_ = psy_list_create(entry);
		} else {	
			rv = psy_list_insert(&self->events_, prev, entry);		
		}	
		++self->opcount_;
		return rv;
	}
	return NULL;
}

psy_audio_PatternEvent psy_audio_pattern_event_at_cursor(
	const psy_audio_Pattern* self, psy_audio_SequenceCursor cursor)
{
	psy_audio_PatternNode* prev;	
	const psy_audio_PatternNode* node;

	assert(self);
	
	node = psy_audio_pattern_findnode_cursor_const(self, cursor, &prev);
	if (node) {
		const psy_audio_PatternEvent* ev;
		
		ev = psy_audio_patternentry_at_const(psy_audio_patternnode_entry_const(
			node), cursor.noteindex);
		if (ev) {
			return *ev;
		}
	}
	return psy_audio_patternevent_zero();
}

psy_audio_PatternNode* psy_audio_pattern_set_event_at_cursor(
	psy_audio_Pattern* self, psy_audio_SequenceCursor cursor,
	psy_audio_PatternEvent ev)
{			
	psy_audio_PatternNode* node;
	psy_audio_PatternNode* prev;

	assert(self);		
	
	node = psy_audio_pattern_findnode_cursor(self, cursor, &prev);
	if (node) {
		psy_audio_PatternEntry* entry;
		
		entry = psy_audio_patternnode_entry(node);
		psy_audio_patternentry_set_event(entry, ev, 
			psy_audio_sequencecursor_note_index(&cursor));
		if (psy_audio_patternentry_empty(entry)) {
			psy_audio_pattern_remove(self, node);
			node = NULL;
		}
		++self->opcount_;		
	} else {
		psy_audio_PatternEntry entry;
		
		psy_audio_patternentry_init(&entry);
		psy_audio_patternentry_set_event(&entry, ev,
			psy_audio_sequencecursor_note_index(&cursor));
		if (!psy_audio_patternentry_empty(&entry)) {
			node = psy_audio_pattern_insert(self, prev,
				psy_audio_sequencecursor_channel(&cursor),
				psy_audio_sequencecursor_offset(&cursor),
				&entry);
		}
		psy_audio_patternentry_dispose(&entry);
		++self->opcount_;
	}
	return node;
}

psy_audio_PatternNode* psy_audio_pattern_greater_equal(psy_audio_Pattern* self,
	psy_dsp_beatpos_t offset, psy_audio_PatternNode** prev)
{
	psy_audio_PatternNode* p;

	assert(self);

	p = self->events_;
	*prev = NULL;
	while (p != NULL) {				
		if (psy_dsp_beatpos_greater_equal(psy_audio_patternentry_offset(
				psy_audio_patternnode_entry(p)), offset)) {
			break;
		}
		*prev = p;
		psy_audio_patternnode_next(&p);
	}	
	return p;
}

psy_audio_PatternNode* psy_audio_pattern_greater_equal_track(
	psy_audio_Pattern* self, uintptr_t track, psy_dsp_beatpos_t offset)
{
	psy_audio_PatternNode* p;

	assert(self);

	p = self->events_;
	while (p != NULL) {
		psy_audio_PatternEntry* entry;
		
		entry = psy_audio_patternnode_entry(p);
		if ((psy_audio_patternentry_track(entry) == track) &&
				psy_dsp_beatpos_greater_equal(psy_audio_patternentry_offset(
					entry), offset)) {
			break;
		}
		psy_audio_patternnode_next(&p);
	}
	return p;
}

psy_audio_PatternNode* psy_audio_pattern_findnode_cursor(psy_audio_Pattern* self,
	psy_audio_SequenceCursor cursor, psy_audio_PatternNode** prev)
{
	return psy_audio_pattern_find_node(self,
		psy_audio_sequencecursor_channel(&cursor),
		psy_audio_sequencecursor_offset(&cursor),
		psy_audio_sequencecursor_bpl(&cursor), prev);
}

psy_audio_PatternNode* psy_audio_pattern_find_node(psy_audio_Pattern* self,
	uintptr_t track, psy_dsp_beatpos_t offset, psy_dsp_beatpos_t bpl,
	psy_audio_PatternNode** prev)
{
	psy_audio_PatternNode* rv;	
	
	assert(self);

	rv = psy_audio_pattern_greater_equal(self, offset, prev);	
	while (rv) {
		psy_audio_PatternEntry* entry;
		
		entry = psy_audio_patternnode_entry(rv);
		if ((psy_audio_patternentry_track(entry) > track) ||
			psy_dsp_beatpos_greater_equal(
				psy_audio_patternentry_offset(entry),
				psy_dsp_beatpos_add(offset, bpl))) {		
			rv = NULL;
			break;
		}		
		if (psy_audio_patternentry_track(entry) == track) {
			break;
		}				
		*prev = rv;		
		psy_audio_patternnode_next(&rv);
	}
	return rv;
}

psy_audio_PatternNode* psy_audio_patternnode_next_track(
	psy_audio_PatternNode* node, uintptr_t track)
{	
	if (node) {
		psy_audio_PatternNode* rv;

		rv = node->next;
		while (rv) {
			psy_audio_PatternEntry* entry;

			entry = psy_audio_patternnode_entry(rv);
			if (psy_audio_patternentry_track(entry) == track) {
				break;
			}
			psy_audio_patternnode_next(&rv);
		}
		return rv;
	}
	return NULL;
}

psy_audio_PatternNode* psy_audio_patternnode_prev_track(
	psy_audio_PatternNode* node, uintptr_t track)
{
	if (node) {
		psy_audio_PatternNode* rv;

		rv = node->prev;
		while (rv) {
			psy_audio_PatternEntry* entry;

			entry = psy_audio_patternnode_entry(rv);
			if (psy_audio_patternentry_track(entry) == track) {
				break;
			}
			rv = rv->prev;
		}
		return rv;
	}
	return NULL;
}

bool psy_audio_pattern_track_used(const psy_audio_Pattern* self,
	uintptr_t track)
{
	psy_audio_PatternNode* p;

	assert(self);

	p = self->events_;
	while (p != NULL) {
		psy_audio_PatternEntry* entry = (psy_audio_PatternEntry*)
			psy_list_entry(p);
		if (psy_audio_patternentry_track(entry) == track) {
			break;
		}
		psy_list_next(&p);
	}
	return p != NULL;
}

void psy_audio_pattern_set_name(psy_audio_Pattern* self, const char* text)
{
	assert(self);

	psy_strreset(&self->name_, text);
	++self->opcount_;
	psy_signal_emit(&self->signal_name_changed, self, 0);
}

void psy_audio_pattern_set_track_name(psy_audio_Pattern* self, uintptr_t track,
	const char* name)
{
	char* curr;
	
	assert(self);
	
	curr = (char*)psy_table_at(&self->track_names_, track);
	free(curr);
	curr = NULL;
	if (name) {
		psy_table_insert(&self->track_names_, track, psy_strdup(name));
	}
}

const char* psy_audio_pattern_track_name(const psy_audio_Pattern* self,
	uintptr_t track)
{	
	assert(self);
	
	return (const char*)psy_table_at_const(&self->track_names_, track);	
}

void psy_audio_pattern_set_length(psy_audio_Pattern* self,
	psy_dsp_beatpos_t length_)
{
	assert(self);

	if (psy_dsp_beatpos_not_equal(self->length_, length_)) {
		self->length_ = length_;
		++self->opcount_;
		psy_signal_emit(&self->signal_length_changed, self, 0);
	}
}

void psy_audio_pattern_set_max_song_tracks(psy_audio_Pattern* self, uintptr_t num)
{
	assert(self);

	self->max_song_tracks_ = num;
}

uintptr_t psy_audio_pattern_max_song_tracks(const psy_audio_Pattern* self)
{
	assert(self);

	return self->max_song_tracks_;
}

void psy_audio_pattern_set_num_default_lines(uintptr_t numlines)
{
	defaultlines = numlines;
}

uintptr_t psy_audio_pattern_num_default_lines(void)
{
	return defaultlines;
}


void psy_audio_add_seqiterator(psy_audio_SequenceTrackIterator* iter)
{		
	psy_list_append(&seqiterators, iter);
}

void psy_audio_remove_seqiterator(psy_audio_SequenceTrackIterator* iter)
{
	psy_List* p;

	p = psy_list_find_entry(seqiterators, iter);
	if (p) {
		psy_list_remove(&seqiterators, p);
	}	
}

void psy_audio_dispose_seqiterator(void)
{
	psy_list_free(seqiterators);
	seqiterators = NULL;
}

void psy_audio_pattern_checkiterators(psy_audio_Pattern* self,
	const psy_audio_PatternNode* node)
{
	psy_List* p;
	psy_List* q;

	assert(self);

	psy_audio_exclusivelock_enter();
	for (q = p = seqiterators; q != NULL; p = q) {		
		psy_audio_SequenceTrackIterator* it;
		
		q = p->next;
		it = (psy_audio_SequenceTrackIterator*)p->entry;
		if (it->patternnode == node) {			
			psy_audio_sequencetrackiterator_inc(it);			
		}
	}
	psy_audio_exclusivelock_leave();
}
