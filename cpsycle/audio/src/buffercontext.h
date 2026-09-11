/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_BUFFERCONTEXT_H
#define psy_audio_BUFFERCONTEXT_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Buffer;
struct psy_List;

/*! 
** @struct psy_audio_BufferContext
** @brief Parameter object to perform a machine work calls
*/
typedef struct psy_audio_BufferContext {
	struct psy_audio_Buffer* input_;
	struct psy_audio_Buffer* output_;
	struct psy_List* events_;
	struct psy_List* out_events_;
	uintptr_t num_samples_;
	uintptr_t num_song_tracks_;
} psy_audio_BufferContext;


void psy_audio_buffercontext_init(psy_audio_BufferContext*,
	struct psy_List* events,
	struct psy_audio_Buffer* input,
	struct psy_audio_Buffer* output,
	uintptr_t num_samples,
	uintptr_t num_song_tracks);
void psy_audio_buffercontext_dispose(psy_audio_BufferContext*);

void psy_audio_buffercontext_set_offset(psy_audio_BufferContext*,
	uintptr_t offset);

INLINE void psy_audio_buffercontext_set_num_samples(
	psy_audio_BufferContext* self, uintptr_t amount)
{
	self->num_samples_ = amount;
}

INLINE uintptr_t psy_audio_buffercontext_num_samples(
	const psy_audio_BufferContext* self)
{
	return self->num_samples_;
}

INLINE uintptr_t psy_audio_buffercontext_num_song_tracks(
	const psy_audio_BufferContext* self)
{
	return self->num_song_tracks_;
}

INLINE struct psy_List* psy_audio_buffercontext_events_begin(
	psy_audio_BufferContext* self)
{
	return self->events_;
}

INLINE struct psy_List* psy_audio_buffercontext_out_events_begin(
	psy_audio_BufferContext* self)
{
	return self->out_events_;
}

INLINE struct psy_audio_Buffer* psy_audio_buffercontext_input(
	psy_audio_BufferContext* self)
{
	return self->input_;
}

INLINE struct psy_audio_Buffer* psy_audio_buffercontext_output(
	psy_audio_BufferContext* self)
{
	return self->output_;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_BUFFERCONTEXT_H */
