/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PATTERN_H
#define psy_audio_PATTERN_H

/* local */
#include "blockselection.h"
#include "patternentry.h"
#include "sequencecursor.h"
/* container */
#include <hashtbl.h>
#include <signal.h>


#ifdef __cplusplus
extern "C" {
#endif


#define psy_audio_GLOBALPATTERN INT32_MAX - 1
#define psy_audio_GLOBALPATTERN_TIMESIGTRACK 0
#define psy_audio_GLOBALPATTERN_LOOPTRACK 1

typedef psy_List psy_audio_PatternNode;

INLINE psy_audio_PatternEntry* psy_audio_patternnode_entry(
	psy_audio_PatternNode* self)
{
	assert(self);

	return (psy_audio_PatternEntry*)self->entry;
}

INLINE const psy_audio_PatternEntry* psy_audio_patternnode_entry_const(
	const psy_audio_PatternNode* self)
{
	assert(self);

	return (const psy_audio_PatternEntry*)self->entry;
}

INLINE void psy_audio_patternnode_next(psy_audio_PatternNode** self)
{
	assert(self);

	psy_list_next(self);
}

struct psy_audio_SequenceTrackIterator;

/*! 
** @struct psy_audio_Pattern
** @brief Small section of your song placed in a sequence with other patterns
*/

typedef struct psy_audio_Pattern {
	/* public */
	psy_Signal signal_name_changed;
	psy_Signal signal_length_changed;
	/*
	** incremented by each operation, the ui is using
	** this flag to synchronize its views
	*/
	uintptr_t opcount_;
	/*! @internal */
	psy_audio_PatternNode* events_;
	psy_dsp_beatpos_t length_;
	/* used by the paste pattern, player uses songtracks of patterns */
	uintptr_t max_song_tracks_;
	char* name_;
	psy_audio_PatternEvent time_sig_;	
	psy_Table track_names_;	
} psy_audio_Pattern;

/* initializes a pattern */
void psy_audio_pattern_init(psy_audio_Pattern*);
/* frees all memory used */
void psy_audio_pattern_dispose(psy_audio_Pattern*);
/* copies a pattern */
void psy_audio_pattern_copy(psy_audio_Pattern* dst, const psy_audio_Pattern* src);
/*
** allocates a pattern
** \return allocates a pattern
*/
psy_audio_Pattern* psy_audio_pattern_alloc(void);
/*
** allocates and initializes a pattern
** \return allocates and initializes a pattern
*/
psy_audio_Pattern* psy_audio_pattern_alloc_init(void);
/*
** allocates a new pattern with a copy of all events
** \return allocates a new pattern with a copy of all events
*/
psy_audio_Pattern* psy_audio_pattern_clone(const psy_audio_Pattern*);

void psy_audio_pattern_clear(psy_audio_Pattern*);
/*
** inserts an pattern entry by copy
** \return the pattern node pointing to the inserted entry
*/
psy_audio_PatternNode* psy_audio_pattern_insert(psy_audio_Pattern*,
	psy_audio_PatternNode* prev, uintptr_t track, psy_dsp_beatpos_t offset,
	const psy_audio_PatternEntry*);
/* removes a pattern node */
void psy_audio_pattern_remove(psy_audio_Pattern*, psy_audio_PatternNode*);
/* finds the pattern node greater or equal than the offset */
/* \return the pattern node greater or equal than the offset */
psy_audio_PatternNode* psy_audio_pattern_greater_equal(psy_audio_Pattern*,
	psy_dsp_beatpos_t offset, psy_audio_PatternNode** prev);
psy_audio_PatternNode* psy_audio_pattern_greater_equal_track(psy_audio_Pattern*,
	uintptr_t track, psy_dsp_beatpos_t offset);
/* finds a pattern node */
/* \return the pattern node */
psy_audio_PatternNode* psy_audio_pattern_find_node(psy_audio_Pattern* pattern,
	uintptr_t track, psy_dsp_beatpos_t offset, psy_dsp_beatpos_t bpl,
	psy_audio_PatternNode** prev);
psy_audio_PatternNode* psy_audio_pattern_findnode_cursor(psy_audio_Pattern*,
	psy_audio_SequenceCursor, psy_audio_PatternNode** prev);

INLINE const psy_audio_PatternNode* psy_audio_pattern_findnode_cursor_const(
	const psy_audio_Pattern* self, psy_audio_SequenceCursor cursor,
	psy_audio_PatternNode** prev)
{
	return psy_audio_pattern_findnode_cursor((psy_audio_Pattern*)self, cursor,
		prev);
}
/* gets the first pattern */
/* \return the first pattern node */
INLINE psy_audio_PatternNode* psy_audio_pattern_begin(
	const psy_audio_Pattern* self)
{
	assert(self);
	
	return self->events_;
}
/* finds the next pattern node on a track */
/* \return next pattern node on a track or NULL */
psy_audio_PatternNode* psy_audio_patternnode_next_track(
	psy_audio_PatternNode* node, uintptr_t track);
/* finds the prev pattern node on a track */
/* \return prev pattern node on a track or NULL */
psy_audio_PatternNode* psy_audio_patternnode_prev_track(
	psy_audio_PatternNode* node, uintptr_t track);
/* test if track is used */
/* \return TRUE if track used else FALSE */
bool psy_audio_pattern_track_used(const psy_audio_Pattern*, uintptr_t track);
/* sets the pattern description */
void psy_audio_pattern_set_name(psy_audio_Pattern*, const char*);
/* \return pattern description */
INLINE const char* psy_audio_pattern_name(const psy_audio_Pattern* self)
{
	assert(self);
	
	return self->name_;
}

void psy_audio_pattern_set_track_name(psy_audio_Pattern*, uintptr_t track,
	const char* name);
const char* psy_audio_pattern_track_name(const psy_audio_Pattern*,
	uintptr_t track);

/* sets the pattern length */
void psy_audio_pattern_set_length(psy_audio_Pattern*, psy_dsp_beatpos_t length);
/* \return length of the pattern */
INLINE psy_dsp_beatpos_t psy_audio_pattern_length(const psy_audio_Pattern* self)
{	
	assert(self);

	return self->length_;
}
/* tells if the pattern contains events */
/* \return tells if the pattern contains events */
INLINE int psy_audio_pattern_empty(const psy_audio_Pattern* self)
{
	assert(self);

	return (self->events_ == NULL);
}
/* \set the event at the cursor position */
psy_audio_PatternNode* psy_audio_pattern_set_event_at_cursor(
	psy_audio_Pattern*, psy_audio_SequenceCursor,
	psy_audio_PatternEvent);
/* \return gets the event or an empty event at the cursor position */
psy_audio_PatternEvent psy_audio_pattern_event_at_cursor(
	const psy_audio_Pattern*, psy_audio_SequenceCursor);

/* gets the op count to determine changes */
INLINE uintptr_t psy_audio_pattern_opcount(const psy_audio_Pattern* self)
{
	assert(self);
	
	return self->opcount_;
}

INLINE void psy_audio_pattern_inc_opcount(psy_audio_Pattern* self)
{
	assert(self);

	++self->opcount_;
}

/* sets the maximum number of songtracks */
/* used by the paste pattern, player uses songtracks of patterns */
void psy_audio_pattern_set_max_song_tracks(psy_audio_Pattern*, uintptr_t num);
/* returns the maximum number of songtracks */
/* used by the paste pattern, player uses songtracks of patterns */
uintptr_t psy_audio_pattern_max_song_tracks(const psy_audio_Pattern*);
/* number of lines a pattern will be created */
void psy_audio_pattern_set_num_default_lines(uintptr_t numlines);
/* return number of lines a pattern will be created */
uintptr_t psy_audio_pattern_num_default_lines(void);

void psy_audio_add_seqiterator(struct psy_audio_SequenceTrackIterator*);
void psy_audio_remove_seqiterator(struct psy_audio_SequenceTrackIterator*);
void psy_audio_dispose_seqiterator(void);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PATTERN_H */
