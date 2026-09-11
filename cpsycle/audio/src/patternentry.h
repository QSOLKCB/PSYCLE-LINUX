/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PATTERNENTRY_H
#define psy_audio_PATTERNENTRY_H

#include "patternevent.h"

/* dsp */
#include <dsptypes.h>
/* std */
#include <assert.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef psy_List psy_audio_PatternEventNode;

typedef enum {
	psy_audio_INTERPOLATE_MODE_LINEAR,
	psy_audio_INTERPOLATE_MODE_HERMITE
} psy_audio_InterpolateMode;

/*!
** @struct psy_audio_PatternEntry
** @brief Multievents (psy_audio_PatternEvent) with time and channel position
*/
typedef struct psy_audio_PatternEntry {
	/* list of psy_audio_PatternEvent */
	psy_audio_PatternEventNode* events_;
	/* position */
	psy_dsp_beatpos_t offset_;
	/*	
	** buffer frame position of current soundcard fill request
	** or
	** event offset of sequencer generated events (retrigger, tws, ..)
	*/
	psy_dsp_beatpos_t delta;
	/* current sequencer bpm */
	double bpm_;
	/* the tracker channel */
	uintptr_t track_; 
	/*
	** not used right now, aim is to sort events at the same
	** beatpos
	*/
	uintptr_t priority_;
	psy_audio_InterpolateMode interpolate_mode_;	
} psy_audio_PatternEntry;

void psy_audio_patternentry_init(psy_audio_PatternEntry*);
void psy_audio_patternentry_init_all(psy_audio_PatternEntry*,
	psy_audio_PatternEvent event,
	psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t delta,
	double bpm,
	uintptr_t track);
void psy_audio_patternentry_dispose(psy_audio_PatternEntry*);

psy_audio_PatternEntry* psy_audio_patternentry_alloc(void);
psy_audio_PatternEntry* psy_audio_patternentry_alloc_init(void);
psy_audio_PatternEntry* psy_audio_patternentry_alloc_init_all(
	psy_audio_PatternEvent event,
	psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t delta,
	double bpm,
	uintptr_t track);
psy_audio_PatternEntry* psy_audio_patternentry_clone(
	const psy_audio_PatternEntry*);
void psy_audio_patternentry_copy(psy_audio_PatternEntry*,
	const psy_audio_PatternEntry* src);

INLINE psy_audio_PatternEventNode* psy_audio_patternentry_begin(
	psy_audio_PatternEntry* self)
{
	assert(self);

	return self->events_;
}

INLINE const psy_audio_PatternEventNode* psy_audio_patternentry_begin_const(
	const psy_audio_PatternEntry* self)
{
	assert(self);

	return self->events_;
}

INLINE psy_audio_PatternEvent* psy_audio_patternentry_front(
	psy_audio_PatternEntry* self)
{
	assert(self);

	return (psy_audio_PatternEvent*)(self->events_->entry);
}

INLINE const psy_audio_PatternEvent* psy_audio_patternentry_front_const(
	const psy_audio_PatternEntry* self)
{
	assert(self);

	return (const psy_audio_PatternEvent*)(self->events_->entry);
}

INLINE psy_audio_PatternEvent* psy_audio_patternentry_at(
	psy_audio_PatternEntry* self, uintptr_t index)
{
	psy_List* p;

	p = psy_list_at(self->events_, index);
	if (p) {
		return (psy_audio_PatternEvent*)p->entry;
	}
	return NULL;
}

INLINE const psy_audio_PatternEvent* psy_audio_patternentry_at_const(
	const psy_audio_PatternEntry* self, uintptr_t index)
{
	const psy_List* p;

	p = psy_list_at_const(self->events_, index);
	if (p) {
		return (const psy_audio_PatternEvent*)p->entry;
	}
	return NULL;
}

INLINE void psy_audio_patternentry_set_bpm(psy_audio_PatternEntry* self,
	double bpm)
{
	assert(self);

	self->bpm_ = bpm;
}

INLINE double psy_audio_patternentry_bpm(const psy_audio_PatternEntry* self)
{
	assert(self);

	return self->bpm_;
}

INLINE psy_dsp_beatpos_t psy_audio_patternentry_offset(
	const psy_audio_PatternEntry* self)
{
	return self->offset_;
}

INLINE void psy_audio_patternentry_set_track(
	psy_audio_PatternEntry* self, uintptr_t track)
{
	self->track_ = track;
}

INLINE uintptr_t psy_audio_patternentry_track(
	const psy_audio_PatternEntry* self)
{
	return self->track_;
}

void psy_audio_patternentry_add_event(psy_audio_PatternEntry*,
	psy_audio_PatternEvent);
void psy_audio_patternentry_set_event(psy_audio_PatternEntry*,
	psy_audio_PatternEvent, uintptr_t noteindex);
bool psy_audio_patternentry_empty(const psy_audio_PatternEntry*);

void psy_audio_patternentry_set_interpolate_mode(psy_audio_PatternEntry*,
	psy_audio_InterpolateMode);
psy_audio_InterpolateMode psy_audio_patternentry_interpolate_mode(
	const psy_audio_PatternEntry*);

void psy_audio_patternentry_trace(const psy_audio_PatternEntry*);


#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PATTERNENTRY_H */
