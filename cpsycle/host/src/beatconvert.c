/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "beatconvert.h"
/* std */
#include <math.h>


/* BeatLine */

/* implementation */
void beatline_init(BeatLine* self, uintptr_t lpb)
{	
	assert(self);
	
	self->lpb = 0;
	beatline_set_lpb(self, lpb);	
}

/* BeatConvert */

/* implementation */
void beatconvert_init(BeatConvert* self, BeatLine* beat_line, double line_px)
{
	assert(self);
	
	self->beat_line = beat_line;	
	beatconvert_set_line_px(self, line_px);
}

/* BeatClip */

/* prototypes */
static void beatclip_clip(BeatClip*, BeatConvert*, double begin_px,
	double end_px);

/* implementation */
void beatclip_init(BeatClip* self, BeatConvert* convert, double begin_px,
	double end_px)
{	
	assert(self);	
		
	beatclip_clip(self, convert, begin_px, end_px);
}

void beatclip_clip(BeatClip* self, BeatConvert* convert,
	double begin_px, double end_px)
{
	assert(self);
	
	self->begin = beatconvert_px_to_beat(convert, psy_max(0.0, begin_px));
	self->end = beatconvert_px_to_beat(convert, psy_max(0.0, end_px));
}
