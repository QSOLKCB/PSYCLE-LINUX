/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(BEATCONVERT_H)
#define BEATCONVERT_H

/* dsp */
#include <dsptypes.h>
/* std */
#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct BeatLine {	
	uintptr_t lpb;
	double bpl;
} BeatLine;

void beatline_init(BeatLine*, uintptr_t lpb);

INLINE void beatline_set_lpb(BeatLine* self, uintptr_t lpb)
{
	assert(self);
	
	if (self->lpb != lpb) {
		self->lpb = lpb;
		self->bpl = 1.0 / (double)lpb;
	}
}

INLINE intptr_t beatline_beat_to_line(const BeatLine* self,
	psy_dsp_beatpos_t offset)
{
	assert(self);

	return (uintptr_t)(psy_dsp_beatpos_real(offset) * self->lpb);
}

INLINE psy_dsp_beatpos_t beatline_quantize(const BeatLine* self,
	psy_dsp_beatpos_t position)
{		
	assert(self);
		
	return psy_dsp_beatpos_make_real(
		beatline_beat_to_line(self, position) *
		((double)1.0 / self->lpb),
		psy_dsp_DEFAULT_PPQ);
}

typedef struct BeatConvert {
	BeatLine* beat_line;	
	double line_px;	
} BeatConvert;

void beatconvert_init(BeatConvert*, BeatLine* beat_line,  double line_px);

INLINE void beatconvert_set_line_px(BeatConvert* self, double line_px)
{
	assert(self);
	
	self->line_px = line_px;	
}

/* quantized */
INLINE double beatconvert_beat_to_px(const BeatConvert* self,
	psy_dsp_beatpos_t position)
{	
	assert(self);
	
	return self->line_px * beatline_beat_to_line(self->beat_line, position);
}

/* not quantized */
INLINE double beatconvert_raw_beat_to_px(const BeatConvert* self,
	psy_dsp_beatpos_t position)
{	
	assert(self);
	
	return self->line_px * (double)self->beat_line->lpb * 
		psy_dsp_beatpos_real(position);
}

/* quantized */
INLINE psy_dsp_beatpos_t beatconvert_px_to_beat(const BeatConvert* self,
	double px)
{	
	uintptr_t line;
		
	assert(self);
	
	line = (uintptr_t)(px / self->line_px);
	return psy_dsp_beatpos_make_real(
		line / (double)self->beat_line->lpb,
		psy_dsp_DEFAULT_PPQ);
}

/* BeatClip */

typedef struct BeatClip {
	psy_dsp_beatpos_t begin;
	psy_dsp_beatpos_t end;	
} BeatClip;

void beatclip_init(BeatClip*, BeatConvert*, double begin_px, double end_px);


#ifdef __cplusplus
}
#endif

#endif /* BEATCONVERT_H */
