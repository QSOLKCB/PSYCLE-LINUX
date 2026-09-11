/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_HOSTSEQUENCERTIME_H
#define psy_audio_HOSTSEQUENCERTIME_H

/* audio */
#include "sequencecursor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_audio_HostSequencerTime {
	bool playing;
	bool playend;
	psy_audio_SequenceCursor lastplaycursor;
	psy_audio_SequenceCursor currplaycursor;	
} psy_audio_HostSequencerTime;

void psy_audio_hostsequencertime_init(psy_audio_HostSequencerTime*);

void psy_audio_hostsequencertime_set_play_cursor(psy_audio_HostSequencerTime*,
	psy_audio_SequenceCursor);
void psy_audio_hostsequencertime_update_last_play_cursor(
	psy_audio_HostSequencerTime*);

INLINE bool psy_audio_hostsequencertime_playing(const
	psy_audio_HostSequencerTime* self)
{
	return (self->playing);
}

INLINE bool psy_audio_hostsequencertime_play_line_changed(
	const psy_audio_HostSequencerTime* self)
{	
	return (!psy_audio_orderindex_equal(&self->currplaycursor.order_index,
			self->lastplaycursor.order_index) ||	 
		psy_dsp_beatpos_not_equal(self->currplaycursor.offset,
			self->lastplaycursor.offset));
}


#ifdef __cplusplus
}
#endif

#endif /* psy_audio_HOSTSEQUENCERTIME_H */
