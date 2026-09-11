/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_INSTPREV_H
#define psy_audio_INSTPREV_H

/* local */
#include "machinefactory.h"
#include "song.h"
#include "sampler.h"
/* container */
#include "../../thread/src/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

/* InstPrev */

typedef struct psy_audio_InstPrev {
	psy_audio_MachineCallback machinecallback;
	psy_audio_Sampler sampler;
	psy_audio_Song song;
	bool playing;
} psy_audio_InstPrev;
	
void psy_audio_instprev_init(psy_audio_InstPrev*);
void psy_audio_instprev_dispose(psy_audio_InstPrev*);

void psy_audio_instprev_play(psy_audio_InstPrev*, psy_audio_Buffer* output,
	const char* path);
void psy_audio_instprev_play_sample(psy_audio_InstPrev*,
	psy_audio_Buffer* output, const psy_audio_Sample* source);

INLINE bool psy_audio_instprev_playing(psy_audio_InstPrev* self)
{
	assert(self);

	return self->playing;
}

void psy_audio_instprev_stop(psy_audio_InstPrev*);

void psy_audio_instprev_work(psy_audio_InstPrev*, psy_audio_Buffer* output,
	uintptr_t amount);
	
#ifdef __cplusplus
}
#endif

#endif /* psy_audio_INSTPREV_H */
