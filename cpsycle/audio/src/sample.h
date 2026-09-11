/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SAMPLE_H
#define psy_audio_SAMPLE_H

#include "buffer.h"
#include "sampleiterator.h"
/* container */
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_Sample
** @brief WaveData with some extra data
**
** @detail
** (psycle-mfc: XMInstrument::WaveData)
** Psycle is made around the concept of audio generators (machines),
** which usually generate audio by themselves depending on the settings.
** But sometimes, recorded or sampled audio is desired, and Psycle offers
** two distinct machines that are able to play samples, the sampler machine,
** and Sampulse. They use instruments (psy_audio_Instrument) that map notes
** to certain samples. Samples are stored in a sample pool (psy_audio_Samples).
*/

/* Wave Form Types */
typedef enum {
	psy_audio_WAVEFORMS_SINUS   = 0x0,
	psy_audio_WAVEFORMS_SQUARE  = 0x1,
	psy_audio_WAVEFORMS_SAWUP   = 0x2,
	psy_audio_WAVEFORMS_SAWDOWN = 0x3,
	psy_audio_WAVEFORMS_RANDOM  = 0x4
} psy_audio_WaveForms;

typedef struct {
	/*
	** 0..255   0 means autovibrato is disabled.
	**			1 means shortest attack.
	**			255 means longest attack.
	*/
	uint8_t attack;
	/* 0..64	0 no vibrato. 64 fastest vibrato */
	uint8_t speed;
	/* 0..32	0 no pitch change. 32 highest pitch change. */
	uint8_t depth;
	psy_audio_WaveForms type;
} psy_audio_Vibrato;

INLINE void psy_audio_vibrato_init(psy_audio_Vibrato* self)
{
	assert(self);

	self->attack = 0;
	self->depth = 0;
	self->speed = 0;
	self->type = psy_audio_WAVEFORMS_SINUS;
}

typedef struct psy_audio_SampleLoop {
	uintptr_t start;
	uintptr_t end;
	psy_audio_SampleLoopType type;	
} psy_audio_SampleLoop;

INLINE void psy_audio_sampleloop_init(psy_audio_SampleLoop* self)
{
	self->start = 0;
	self->end = 0;
	self->type = psy_audio_SAMPLE_LOOP_DO_NOT;	
}

INLINE void psy_audio_sampleloop_init_all(psy_audio_SampleLoop* self,
	psy_audio_SampleLoopType type, uintptr_t loopstart, uintptr_t loopend)
{
	self->start = loopstart;
	self->end = loopend;
	self->type = type;
}

INLINE psy_audio_SampleLoop psy_audio_sampleloop_make(
	psy_audio_SampleLoopType type, uintptr_t loopstart, uintptr_t loopend)
{
	psy_audio_SampleLoop rv;

	rv.start = loopstart;
	rv.end = loopend;
	rv.type = type;
	return rv;
}

INLINE bool psy_audio_sampleloop_equal(psy_audio_SampleLoop* self,
	psy_audio_SampleLoop* other)
{
	return (self->type == other->type &&
		self->start == other->start &&
		self->end == other->end);
}

#define psy_audio_PAN_CENTER 0.5f;
#define psy_audio_PAN_LEFT 0.f;
#define psy_audio_PAN_RIGHT 1.f;

typedef enum {
	psy_audio_ZONESET_NONE = 0,	
	psy_audio_ZONESET_TUNE = 1,
	psy_audio_ZONESET_FINETUNE = 2
} psy_audio_ZoneSet;

typedef struct psy_audio_Zone {
	/*
	** Tuning for the center note(value that is added to the note received).
	** values from -60 to 59. 0 = C-5 (middle C, i.e. play at original speed
	** with note C-5);
	*/
	int16_t tune;	
	/* [-100 .. 100] full range = -/ +1 seminote */
	int16_t finetune;
	int zoneset;
} psy_audio_Zone;

typedef struct psy_audio_Sample {
	psy_Signal signal_loop_changed;
	char* name;
	/* Sample(wave) length in frames(samples). */
	uintptr_t numframes;	
	/*
	** Difference between Glob volumeand defVolume is that defVolume
	** determines the volume if no volume is specified in the pattern, while
	** globVolume is an attenuator for all notes of this sample.
	*/
	double globalvolume; /* range(0..4) (-inf to + 12dB) */
	uint16_t defaultvolume; /* range(0..80 hex) */
	psy_audio_SampleLoop loop;
	psy_audio_SampleLoop sustainloop;
	double samplerate;
	psy_audio_Zone zone;
	bool stereo;
	/*
	** Wave data, (use psy_audio_sample_alloc_wave_data to create the memory,
	**             else only the channel structure is created without
	**             a buffer memory)
	*/
	psy_audio_Buffer channels;
	bool panenabled;
	/* Default position for panning(0..1) 0 = left 1 = right. */
	double panfactor;
	bool surround;
	psy_audio_Vibrato vibrato;
} psy_audio_Sample;

void psy_audio_sample_init(psy_audio_Sample*, uintptr_t numchannels);
void psy_audio_sample_dispose(psy_audio_Sample*);
void psy_audio_sample_copy(psy_audio_Sample*, const psy_audio_Sample*);

psy_audio_Sample* psy_audio_sample_alloc(void);
psy_audio_Sample* psy_audio_sample_alloc_init(uintptr_t numchannels);
psy_audio_Sample* psy_audio_sample_clone(const psy_audio_Sample*);
void psy_audio_sample_deallocate(psy_audio_Sample*);

/* psycle - mfc: WaveDataController */
psy_audio_WaveDataController* psy_audio_sample_alloc_iterator(psy_audio_Sample*,
	psy_dsp_ResamplerQuality);
/* 
** allocates ALIGNED sample memory for all channels
** (psycle-mfc: The memory is NOT aligned.)
*/
void psy_audio_sample_alloc_wave_data(psy_audio_Sample*);

int psy_audio_sample_load(psy_audio_Sample*, const char* path);
void psy_audio_sample_save(psy_audio_Sample*, const char* path);

/* Properties */
void psy_audio_sample_set_name(psy_audio_Sample*, const char* name);

INLINE const char* psy_audio_sample_name(const psy_audio_Sample* self)
{
	return self->name;
}

/* psycle-mfc: WaveLength() */
INLINE uintptr_t psy_audio_sample_num_frames(const psy_audio_Sample* self)
{
	return self->numframes;
}

INLINE double psy_audio_sample_global_volume(const psy_audio_Sample* self)
{
	return self->globalvolume;
}

INLINE void psy_audio_sample_set_global_volume(psy_audio_Sample* self,
	double value)
{
	self->globalvolume = value;
}

INLINE uint16_t psy_audio_sample_volume(const psy_audio_Sample* self)
{
	return self->defaultvolume;
}

INLINE void psy_audio_sample_set_volume(psy_audio_Sample* self,
	uint16_t value)
{
	self->defaultvolume = value;
}

/* Default position for panning (0..1) 0 = left 1 = right. */
INLINE double psy_audio_sample_panning(const psy_audio_Sample* self)
{
	return self->panfactor;
}

INLINE void psy_audio_sample_set_panning(psy_audio_Sample* self,
	double value)
{
	self->panfactor = value;
}

INLINE bool psy_audio_sample_pan_enabled(const psy_audio_Sample* self)
{
	return self->panenabled;
}

INLINE void psy_audio_sample_enable_panning(psy_audio_Sample* self)
{
	self->panenabled = TRUE;
}

INLINE void psy_audio_sample_disable_panning(psy_audio_Sample* self)
{
	self->panenabled = FALSE;
}

INLINE bool psy_audio_sample_surround(const psy_audio_Sample* self)
{
	return self->surround;
}

INLINE void psy_audio_sample_enable_surround(psy_audio_Sample* self)
{
	self->surround = TRUE;
}

INLINE void psy_audio_sample_disable_surround(psy_audio_Sample* self)
{
	self->surround = FALSE;
}

INLINE psy_audio_SampleLoop psy_audio_sample_contloop(
	const psy_audio_Sample* self)
{
	return self->loop;
}

INLINE void psy_audio_sample_set_cont_loop(psy_audio_Sample* self,
	psy_audio_SampleLoop loop)
{
	self->loop = loop;
	psy_signal_emit(&self->signal_loop_changed, self, 0);
}

INLINE psy_audio_SampleLoop psy_audio_sample_sustain_loop(
	const psy_audio_Sample* self)
{
	return self->sustainloop;
}

INLINE void psy_audio_sample_set_sustain_loop(psy_audio_Sample* self,
	psy_audio_SampleLoop loop)
{
	self->sustainloop = loop;
	psy_signal_emit(&self->signal_loop_changed, self, 0);
}

INLINE int16_t psy_audio_sample_tune(const psy_audio_Sample* self)
{
	return self->zone.tune;
}

INLINE void psy_audio_sample_set_tune(psy_audio_Sample* self, int16_t tune)
{
	self->zone.tune = tune;
}

INLINE int16_t psy_audio_sample_fine_tune(const psy_audio_Sample* self)
{
	return self->zone.finetune;
}

INLINE void psy_audio_sample_set_fine_tune(psy_audio_Sample* self, int16_t tune)
{
	self->zone.finetune = tune;
}

INLINE bool psy_audio_sample_stero(const psy_audio_Sample* self)
{
	return self->stereo;
}

INLINE psy_audio_Vibrato psy_audio_sample_vibrato(const psy_audio_Sample* self)
{
	return self->vibrato;
}

INLINE void psy_audio_sample_set_vibrato(psy_audio_Sample* self,
	psy_audio_Vibrato vibrato)
{
	self->vibrato = vibrato;
}

INLINE bool psy_audio_sample_auto_vibrato(const psy_audio_Sample* self)
{
	return self->vibrato.depth && self->vibrato.speed;
}

INLINE uintptr_t psy_audio_sample_num_channels(const psy_audio_Sample* self)
{
	return psy_audio_buffer_num_channels(&self->channels);
}

INLINE void psy_audio_sample_resize(psy_audio_Sample* self,
	uintptr_t numchannels)
{
	psy_audio_buffer_resize(&self->channels, numchannels);	
	self->stereo = (numchannels > 1);	
}

INLINE void psy_audio_sample_set_sample_rate(psy_audio_Sample* self,
	double samplerate)
{
	self->samplerate = samplerate;
}

INLINE double psy_audio_sample_sample_rate(const
	psy_audio_Sample* self)
{
	return self->samplerate;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SAMPLE_H */
