/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SEQUENCERTIME_H
#define psy_audio_SEQUENCERTIME_H

#include "../../detail/psydef.h"
/* audio */
#include "sequencecursor.h"
/* dsp */
#include <dsptypes.h>
/* std */
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_SequencerTime
** @brief Current sequencer time information
**
** @details
** The current Time information stores everthing needed to convert frames
** to pulses per quarter and is updated at ever batch work (mostly 256 frames)
** requested by the player work callback
**
** The host maintains a subset of the play positions with
** psy_audio_HostSequencerTime that is synced to the repaint of the
** different views 
*/
typedef struct psy_audio_SequencerTime {
	/* current play position in frames */
	psy_dsp_frame_t playcounter;	
	/* current samplerate in cycle/second (often 44100.0 Hz) */
	double samplerate;
	/* current play position in beats */
	double position;
	/* tempo in beats per minute */
	double bpm;
	/* last bar position in beats */
	double lastbarposition;
	/* samples to next midi clock */
	psy_dsp_frame_t samplestonextclock;
	/* current playtime in seconds */
	psy_dsp_seconds_t currplaytime;
	/* playing */
	bool playing;
	/* starting */
	bool playstarting;
	/* stopping */
	bool playstopping;
	bool playend;
	/* Time Signature Numerator */
	uintptr_t timesig_numerator;
	/* Time Signature Denominator */
	uintptr_t timesig_denominator;
} psy_audio_SequencerTime;

INLINE void psy_audio_sequencertime_init(psy_audio_SequencerTime* self)
{
	assert(self);
	
	self->playcounter = (psy_dsp_frame_t)0;
	self->samplerate = (double)44100.0;
	self->position = self->lastbarposition = (double)0.0;
	self->bpm = (double)125.0;
	self->samplestonextclock = 0;
	self->currplaytime = 0.0;
	self->timesig_numerator = 4;
	self->timesig_denominator = 4;
	self->playing = FALSE;
	self->playstarting = FALSE;
	self->playstopping = FALSE;
	self->playend = FALSE;
}

INLINE bool psy_audio_sequencertime_playing(const psy_audio_SequencerTime*
	self)
{
	assert(self);

	return self->playing;
}

INLINE psy_dsp_seconds_t psy_audio_sequencertime_play_time(
	const psy_audio_SequencerTime* self)
{
	return self->playcounter / (psy_dsp_seconds_t)self->samplerate;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SEQUENCERTIME_H */
