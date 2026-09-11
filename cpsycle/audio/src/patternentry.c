/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternentry.h"
/* container */
#include <list.h>
/* platform */
#include "../../detail/trace.h"
#include "../../detail/portable.h"


/* implementation */
void psy_audio_patternentry_init(psy_audio_PatternEntry* self)
{
	assert(self);

	self->events_ = NULL;	
	self->offset_ = self->delta = psy_dsp_beatpos_zero();	
	self->bpm_ = psy_dsp_BPM_UNSET;
	self->track_ = 0;
	self->interpolate_mode_ = psy_audio_INTERPOLATE_MODE_LINEAR;
	self->priority_ = 0;
	psy_audio_patternentry_add_event(self, psy_audio_patternevent_zero());	
}

void psy_audio_patternentry_init_all(psy_audio_PatternEntry* self,
	psy_audio_PatternEvent event,
	psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t delta,
	double bpm,
	uintptr_t track)	
{
	assert(self);

	self->events_ = NULL;
	self->offset_ = offset;
	self->delta = delta;
	self->bpm_ = bpm;
	self->track_ = track;
	self->interpolate_mode_ = psy_audio_INTERPOLATE_MODE_LINEAR;
	self->priority_ = 0;
	psy_audio_patternentry_add_event(self, event);	
}

void psy_audio_patternentry_dispose(psy_audio_PatternEntry* self)
{
	psy_List* p;

	assert(self);

	for (p = self->events_; p != NULL; psy_list_next(&p)) {
		free(psy_list_entry(p));
	}
	psy_list_free(self->events_);
	self->events_ = NULL;
}

psy_audio_PatternEntry* psy_audio_patternentry_alloc(void)
{
	return (psy_audio_PatternEntry*)malloc(sizeof(psy_audio_PatternEntry));
}

psy_audio_PatternEntry* psy_audio_patternentry_alloc_init(void)
{
	psy_audio_PatternEntry* rv;
	
	rv = psy_audio_patternentry_alloc();
	if (rv) {
		psy_audio_patternentry_init(rv);
	}
	return rv;
}

psy_audio_PatternEntry* psy_audio_patternentry_alloc_init_all(
	psy_audio_PatternEvent ev,
	psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t delta,
	double bpm,
	uintptr_t track)
{
	psy_audio_PatternEntry* rv;

	rv = psy_audio_patternentry_alloc();
	if (rv) {
		psy_audio_patternentry_init_all(rv, ev, offset, delta, bpm, track);
	}
	return rv;
}

psy_audio_PatternEntry* psy_audio_patternentry_clone(const psy_audio_PatternEntry* src)
{
	psy_audio_PatternEntry* rv;
	
	if (src) {
		psy_List* p;
		rv = psy_audio_patternentry_alloc();
		rv->bpm_ = src->bpm_;
		rv->delta = src->delta;
		rv->offset_ = src->offset_;
		rv->track_ = src->track_;
		rv->interpolate_mode_ = src->interpolate_mode_;
		rv->events_ = NULL;
		rv->priority_ = src->priority_;
		for (p = src->events_; p != NULL; psy_list_next(&p)) {
			psy_audio_PatternEvent* copy;

			copy = (psy_audio_PatternEvent*)
				malloc(sizeof(psy_audio_PatternEvent));
			if (copy) {
				*copy = *((psy_audio_PatternEvent*)p->entry);
				psy_list_append(&rv->events_, copy);
			}
		}		
	} else {
		rv = 0;
	}
	return rv;
}

void psy_audio_patternentry_copy(psy_audio_PatternEntry* self,
	const psy_audio_PatternEntry* src)
{
	psy_List* p;
	
	assert(src);
	
	for (p = self->events_; p != NULL; psy_list_next(&p)) {
		free(psy_list_entry(p));
	}
	psy_list_free(self->events_);
	self->events_ = 0;
	self->bpm_ = src->bpm_;
	self->delta = src->delta;
	self->offset_ = src->offset_;
	self->track_ = src->track_;
	self->priority_ = src->priority_;
	self->interpolate_mode_ =src->interpolate_mode_;
	for (p = src->events_; p != NULL; psy_list_next(&p)) {
		psy_audio_PatternEvent* copy;

		copy = (psy_audio_PatternEvent*)
			malloc(sizeof(psy_audio_PatternEvent));
		if (copy) {
			*copy = *((psy_audio_PatternEvent*)p->entry);
			psy_list_append(&self->events_, copy);
		}
	}	
}

void psy_audio_patternentry_add_event(psy_audio_PatternEntry* self,
	psy_audio_PatternEvent event)
{
	psy_audio_PatternEvent* copy;

	assert(self);

	copy = (psy_audio_PatternEvent*)malloc(sizeof(psy_audio_PatternEvent));
	if (copy) {
		*copy = event;
		psy_list_append(&self->events_, copy);
	}
}

void psy_audio_patternentry_set_event(psy_audio_PatternEntry* self,
	psy_audio_PatternEvent ev, uintptr_t noteindex)
{
	uintptr_t i;
	psy_List* p;
	
	assert(self);
	
	for (i = 0, p = self->events_; i <= noteindex; ++i) {
		if (!p) {			
			psy_audio_patternentry_add_event(self,
				psy_audio_patternevent_zero());
			p = psy_list_last(self->events_);
		}
		if (p && (i == noteindex)) {
			psy_audio_PatternEvent* curr;
			
			curr = (psy_audio_PatternEvent*)p->entry;
			*curr = ev;			
		}
		if (p) {
			p = p->next;
		}
	}
}

bool psy_audio_patternentry_empty(const psy_audio_PatternEntry* self)
{
	bool rv;
	psy_List* p;
		
	assert(self);
	
	rv = TRUE;
	p = self->events_;
	while (p != NULL) {
		psy_audio_PatternEvent* curr;
			
		curr = (psy_audio_PatternEvent*)p->entry;
		if (!psy_audio_patternevent_empty(curr)) {
			rv = FALSE;
			break;
		}
		p = p->next;
	}
	return rv;
}

void psy_audio_patternentry_set_interpolate_mode(psy_audio_PatternEntry* self,
	psy_audio_InterpolateMode mode)
{
	assert(self);
	
	self->interpolate_mode_ = mode;
}

psy_audio_InterpolateMode psy_audio_patternentry_interpolate_mode(
	const psy_audio_PatternEntry* self)
{
	assert(self);
	
	return self->interpolate_mode_;
}

void psy_audio_patternentry_trace(const psy_audio_PatternEntry* self)
{
	char text[256];

	assert(self);
	
	psy_snprintf(text, 256, "pe: %d %f \n", (int)self->track_, (float)
		psy_dsp_beatpos_real(self->offset_));
	TRACE(text);
}
