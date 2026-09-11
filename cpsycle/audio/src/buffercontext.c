/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "buffercontext.h"

/* audio */
#include "buffer.h"
/* container */
#include <list.h>


/* implementation */
void psy_audio_buffercontext_init(psy_audio_BufferContext* self,
	psy_List* events,
	psy_audio_Buffer* input,
	psy_audio_Buffer* output,
	uintptr_t num_samples,
	uintptr_t num_song_tracks)
{	
	self->events_ = events;
	self->out_events_ = NULL;
	self->input_ = input;
	self->output_ = output;
	self->num_samples_ = num_samples;
	self->num_song_tracks_ = num_song_tracks;
}

void psy_audio_buffercontext_dispose(psy_audio_BufferContext* self)
{
}

void psy_audio_buffercontext_set_offset(psy_audio_BufferContext* self,
	uintptr_t offset)
{
	if (self->input_) {
		psy_audio_buffer_set_offset(self->input_, offset);
	}
	if (self->output_) {
		psy_audio_buffer_set_offset(self->output_, offset);
	}
}
