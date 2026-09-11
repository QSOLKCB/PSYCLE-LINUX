/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "instprev.h"
/* local */
#include "constants.h"
#include "exclusivelock.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

/* implementation */
void psy_audio_instprev_init(psy_audio_InstPrev* self)
{
	assert(self);

	self->playing = FALSE;
	psy_audio_song_init(&self->song, NULL);
	psy_audio_machinecallback_init(&self->machinecallback);
	psy_audio_machinecallback_set_song(&self->machinecallback, &self->song);
	psy_audio_sampler_init(&self->sampler, &self->machinecallback);			
}

void psy_audio_instprev_dispose(psy_audio_InstPrev* self)
{
	assert(self);
	
	psy_audio_machine_dispose(psy_audio_sampler_base(&self->sampler));	
	psy_audio_song_dispose(&self->song);	
}

void psy_audio_instprev_work(psy_audio_InstPrev* self, psy_audio_Buffer* output,
	uintptr_t amount)
{
	assert(self);

	if (self->playing) {		
		psy_audio_BufferContext bc;
		psy_List* events;

		events = NULL;		
		psy_audio_buffercontext_init(&bc, events, output, output, amount,
			MAX_TRACKS);
		psy_audio_buffer_scale(output, psy_audio_machine_amp_range(
			psy_audio_sampler_base(&self->sampler)),
			amount);
		psy_audio_machine_work(psy_audio_sampler_base(&self->sampler), &bc);
	}
}

void psy_audio_instprev_play(psy_audio_InstPrev* self, psy_audio_Buffer* output,
	const char* path)
{
	psy_audio_Sample* sample;
	psy_audio_SampleIndex sample_index;
	psy_audio_Instrument* instrument;
	psy_audio_InstrumentEntry entry;
	psy_audio_InstrumentIndex inst;

	assert(self);

	inst = psy_audio_instrumentindex_make(0, 0);
	sample_index = psy_audio_sampleindex_make(0, 0);	
	sample = psy_audio_sample_alloc_init(0);
	if (psy_audio_sample_load(sample, path) == PSY_OK) {		
		psy_audio_BufferContext bc;
		psy_List* events;
		uintptr_t i;

		events = NULL;		
		psy_audio_buffercontext_init(&bc, events, output, output, 256,
			MAX_TRACKS);
		psy_audio_exclusivelock_enter();
		psy_audio_machine_stop(psy_audio_sampler_base(&self->sampler));		
		for (i = 0; i < 10; ++i) {
			/* work fastrelease in sampler */
			psy_audio_machine_work(psy_audio_sampler_base(&self->sampler), &bc);
		}
		psy_audio_samples_insert(psy_audio_song_samples(&self->song),
			sample, sample_index);
		instrument = psy_audio_instrument_allocinit();
		psy_audio_instrumententry_init(&entry);
		entry.sampleindex = sample_index;
		psy_audio_instrument_add_entry(instrument, &entry);
		psy_audio_instrument_set_name(instrument, psy_audio_sample_name(sample));
		psy_audio_instruments_insert(psy_audio_song_instruments(&self->song),
			instrument, inst);
		psy_audio_instruments_select(psy_audio_song_instruments(&self->song),
			inst);
		{
			psy_audio_PatternEvent ev;

			self->playing = TRUE;
			psy_audio_patternevent_init_all(&ev, 60, 0, 0, 0, 0, 0);
			psy_audio_machine_seq_tick(psy_audio_sampler_base(&self->sampler),
				0, &ev);
		}
		psy_audio_exclusivelock_leave();
	} else {
		psy_audio_sample_deallocate(sample);
	}
}

void psy_audio_instprev_play_sample(psy_audio_InstPrev* self,
	psy_audio_Buffer* output, const psy_audio_Sample* source)
{
	psy_audio_Sample* sample;
	psy_audio_SampleIndex sample_index;
	psy_audio_Instrument* instrument;
	psy_audio_InstrumentEntry entry;
	psy_audio_InstrumentIndex inst;

	assert(self);

	if (!source) {
		return;
	}
	inst = psy_audio_instrumentindex_make(0, 0);
	sample_index = psy_audio_sampleindex_make(0, 0);	
	sample = psy_audio_sample_clone(source);
	if (sample) {
		psy_audio_BufferContext bc;
		psy_List* events;
		uintptr_t i;

		events = NULL;		
		psy_audio_buffercontext_init(&bc, events, output, output, 256,
			MAX_TRACKS);
		psy_audio_exclusivelock_enter();
		psy_audio_machine_stop(psy_audio_sampler_base(&self->sampler));		
		for (i = 0; i < 10; ++i) {
			/* work fastrelease in sampler */
			psy_audio_machine_work(psy_audio_sampler_base(&self->sampler), &bc);
		}
		psy_audio_samples_insert(psy_audio_song_samples(&self->song),
			sample, sample_index);
		instrument = psy_audio_instrument_allocinit();
		psy_audio_instrumententry_init(&entry);
		entry.sampleindex = sample_index;
		psy_audio_instrument_add_entry(instrument, &entry);
		psy_audio_instrument_set_name(instrument, psy_audio_sample_name(sample));
		psy_audio_instruments_insert(psy_audio_song_instruments(&self->song),
			instrument, inst);
		psy_audio_instruments_select(psy_audio_song_instruments(&self->song),
			inst);
		{
			psy_audio_PatternEvent ev;

			self->playing = TRUE;
			psy_audio_patternevent_init_all(&ev, 60, 0, 0, 0, 0, 0);
			psy_audio_machine_seq_tick(psy_audio_sampler_base(&self->sampler),
				0, &ev);
		}
		psy_audio_exclusivelock_leave();
	} else {
		psy_audio_sample_deallocate(sample);
	}
}

void psy_audio_instprev_stop(psy_audio_InstPrev* self)
{		
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_machine_stop(psy_audio_sampler_base(&self->sampler));
	self->playing = FALSE;	
	psy_audio_exclusivelock_leave();
}
