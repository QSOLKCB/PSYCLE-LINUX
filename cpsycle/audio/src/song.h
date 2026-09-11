/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_SONG_H
#define psy_audio_SONG_H

/* local */
#include "instruments.h"
#include "machines.h"
#include "patterns.h"
#include "samples.h"
#include "sequence.h"
/* container */
#include <logger.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_audio_SongProperties {
	/*! @internal */
	uint8_t octave_;
	uintptr_t lpb_;
	uintptr_t tpb_;
	uintptr_t extra_ticks_per_beat_;
	double bpm_;
	uintptr_t sampler_index_;
	bool auto_note_off_;
	char* title_;
	char* credits_;
	char* comments_;	
} psy_audio_SongProperties;

void psy_audio_songproperties_init(psy_audio_SongProperties*, const char* title,
	const char* credits, const char* comments);
void psy_audio_songproperties_init_all(psy_audio_SongProperties*, const char* title,
	const char* credits, const char* comments,	
	int octave,
	uintptr_t lpb,
	int tpb,
	int extra_ticks_per_beat,
	double bpm,
	uintptr_t sampler_index,
	bool auto_note_off);
void psy_audio_songproperties_copy(psy_audio_SongProperties*, const psy_audio_SongProperties* other);
void psy_audio_songproperties_dispose(psy_audio_SongProperties*);

/* Properties */

INLINE void psy_audio_songproperties_set_bpm(psy_audio_SongProperties* self,
	double bpm)
{
	assert(self);

	if (bpm < 32) {
		self->bpm_ = 32;
	} else if (bpm > 999) {
		self->bpm_ = 999;
	} else {
		self->bpm_ = bpm;
	}
}

INLINE double psy_audio_songproperties_bpm(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->bpm_;
}

INLINE void psy_audio_songproperties_set_lpb(psy_audio_SongProperties* self,
	uintptr_t lpb)
{
	assert(self);

	self->lpb_ = lpb;
}

INLINE uintptr_t psy_audio_songproperties_lpb(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->lpb_;
}

INLINE void psy_audio_songproperties_set_octave(psy_audio_SongProperties* self,
	uint8_t octave)
{
	assert(self);

	self->octave_ = octave;
}

INLINE uint8_t psy_audio_songproperties_octave(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->octave_;
}

INLINE void psy_audio_songproperties_set_tpb(psy_audio_SongProperties* self,
	uintptr_t tpb)
{
	assert(self);

	self->tpb_ = tpb;
}

INLINE uintptr_t psy_audio_songproperties_tpb(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->tpb_;
}

INLINE void psy_audio_songproperties_set_extra_ticks_per_beat(
	psy_audio_SongProperties* self, uintptr_t extra_ticks_per_beat)
{
	assert(self);

	self->extra_ticks_per_beat_ = extra_ticks_per_beat;
}

INLINE uintptr_t psy_audio_songproperties_extra_ticks_per_beat(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->extra_ticks_per_beat_;
}

INLINE const char* psy_audio_songproperties_title(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->title_;
}

void psy_audio_songproperties_set_title(psy_audio_SongProperties* self,
	const char* title);

void psy_audio_songproperties_set_credits(psy_audio_SongProperties* self,
	const char* credits);

INLINE const char* psy_audio_songproperties_credits(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->credits_;
}

void psy_audio_songproperties_set_comments(psy_audio_SongProperties* self,
	const char* comments);

INLINE const char* psy_audio_songproperties_comments(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->comments_;
}

INLINE uintptr_t psy_audio_songproperties_sampler_index(const
	psy_audio_SongProperties* self)
{
	assert(self);

	return self->sampler_index_;
}

INLINE void psy_audio_songproperties_set_sampler_index(
	psy_audio_SongProperties* self, uintptr_t sampler_index)
{
	assert(self);

	self->sampler_index_ = sampler_index;
}

INLINE void psy_audio_songproperties_enable_auto_note_off(
	psy_audio_SongProperties* self)
{
	assert(self);

	self->auto_note_off_ = TRUE;
}

INLINE void psy_audio_songproperties_disable_auto_note_off(
	psy_audio_SongProperties* self)
{
	assert(self);

	self->auto_note_off_ = FALSE;
}

INLINE bool psy_audio_songproperties_auto_note_off(
	const psy_audio_SongProperties* self)
{
	assert(self);

	return self->auto_note_off_;
}

/*!
** @struct psy_audio_Song
** @brief holds everything comprising a "tracker module".
**
** @details
** A song hold everything comprising a "tracker module",
** this includes patterns, pattern sequence, machines and their initial
** parameters and coordinates, wavetables, ...
*/

typedef struct psy_audio_Song {	
	/*! @internal */	
	psy_audio_SongProperties properties_;
	psy_audio_Machines machines_;
	psy_audio_Patterns patterns_;
	psy_audio_Sequence sequence_;
	psy_audio_Samples samples_;
	psy_audio_Instruments instruments_;	
	/* references */
	struct psy_audio_MachineFactory* machine_factory;
} psy_audio_Song;

/* initializes a song with a master and one sequence track/entry/pattern */
void psy_audio_song_init(psy_audio_Song*, struct psy_audio_MachineFactory*);
/* frees all internal memory used by the songstruct */
void psy_audio_song_dispose(psy_audio_Song*);
/*
** allocates a song
** \return allocates a song
*/
psy_audio_Song* psy_audio_song_alloc(void);
/*
** allocates and initializes a song
** \return allocates and initializes a song
*/
psy_audio_Song* psy_audio_song_alloc_init(struct psy_audio_MachineFactory*);
/* calls dispose and deallocates memory allocated by song_alloc */
void psy_audio_song_deallocate(psy_audio_Song*);
/* clears the song completly (no master, no pattern, no sequence track/entry) */
void psy_audio_song_clear(psy_audio_Song*);
/* adds a virtual generator */
void psy_audio_song_insert_virtual_generator(psy_audio_Song*,
	uintptr_t virtual_inst, uintptr_t mac_idx, uintptr_t inst_idx);
/* getter of the song parts */
/* return: Machines of song */
INLINE psy_audio_Machines* psy_audio_song_machines(psy_audio_Song* self)
{
	assert(self);

	return &self->machines_;
}
/* return: Patterns of song */
INLINE psy_audio_Patterns* psy_audio_song_patterns(psy_audio_Song* self)
{
	assert(self);

	return &self->patterns_;
}
/* return: Sequence of song */
INLINE psy_audio_Sequence* psy_audio_song_sequence(psy_audio_Song* self)
{
	assert(self);

	return &self->sequence_;
}
/* return: Samples of song */
INLINE psy_audio_Samples* psy_audio_song_samples(psy_audio_Song* self)
{
	assert(self);

	return &self->samples_;
}
/* return: Instruments of song */
INLINE psy_audio_Instruments* psy_audio_song_instruments(psy_audio_Song* self)
{
	assert(self);

	return &self->instruments_;
}

/* Properties */

/* set SongProperties */
void psy_audio_song_set_properties(psy_audio_Song*, const psy_audio_SongProperties*);

/* set song title */
INLINE void psy_audio_song_set_title(psy_audio_Song* self,
	const char* title)
{
	assert(self);

	psy_audio_songproperties_set_title(&self->properties_, title);
}

/* return song title */
INLINE const char* psy_audio_song_title(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_title(&self->properties_);
}

/* return song title */
INLINE const char* psy_audio_song_credits(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_credits(&self->properties_);
}

/* set song comments */
INLINE void psy_audio_song_set_comments(psy_audio_Song* self,
	const char* comments)
{
	assert(self);

	psy_audio_songproperties_set_comments(&self->properties_, comments);
}

/* return song comments */
INLINE const char* psy_audio_song_comments(const psy_audio_Song* self)
{
	return psy_audio_songproperties_comments(&self->properties_);
}

INLINE void psy_audio_song_set_credits(psy_audio_Song* self,
	const char* credits)
{
	assert(self);

	psy_audio_songproperties_set_credits(&self->properties_, credits);
}

/* set song properties bpm */
void psy_audio_song_set_bpm(psy_audio_Song*, double bpm);
/* return song properties bpm */
INLINE double psy_audio_song_bpm(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_bpm(&self->properties_);
}
/* set song properties lpb */
INLINE void psy_audio_song_set_lpb(psy_audio_Song* self, uintptr_t lpb)
{
	psy_audio_SequenceCursor cursor;

	assert(self);

	psy_audio_songproperties_set_lpb(&self->properties_, lpb);
	cursor = psy_audio_sequence_cursor(&self->sequence_);
	psy_audio_sequencecursor_set_lpb(&cursor, lpb);
	psy_audio_sequence_set_cursor(&self->sequence_, cursor);
}
/* return song properties lpb */
INLINE uintptr_t psy_audio_song_lpb(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_lpb(&self->properties_);
}
/* set song properties octave */
INLINE void psy_audio_song_set_octave(psy_audio_Song* self, uint8_t octave)
{
	assert(self);

	psy_audio_songproperties_set_octave(&self->properties_, octave);
}
/* return song properties lpb */
INLINE uint8_t psy_audio_song_octave(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_octave(&self->properties_);
}
/* set song properties songtrack (pattern channels) number */
INLINE void psy_audio_song_set_num_song_tracks(psy_audio_Song* self,
	uintptr_t numtracks)
{
	assert(self);

	psy_audio_patterns_set_num_tracks(&self->patterns_, numtracks);
}
/* return song numtracks (pattern channels) */
INLINE uintptr_t psy_audio_song_num_song_tracks(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_patterns_num_tracks(&self->patterns_);
}
/* return song properties tpb */
INLINE uintptr_t psy_audio_song_tpb(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_tpb(&self->properties_);
}
/* set song properties ticks per beat */
INLINE void psy_audio_song_set_tpb(psy_audio_Song* self, uintptr_t tpb)
{
	assert(self);

	psy_audio_songproperties_set_tpb(&self->properties_, tpb);
}
/* return song properties extraticksperbeat */
INLINE uintptr_t psy_audio_song_extra_ticks_per_beat(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_extra_ticks_per_beat(&self->properties_);
}
/* set song properties ticks per beat */
INLINE void psy_audio_song_set_extra_ticks_per_beat(psy_audio_Song* self,
	uintptr_t extra_ticks_per_beat)
{
	assert(self);

	psy_audio_songproperties_set_extra_ticks_per_beat(&self->properties_,
		extra_ticks_per_beat);
}
/* calculates real tpb */
INLINE uintptr_t psy_audio_song_real_tpb(psy_audio_Song* self)
{	
	assert(self);
	
	return (psy_audio_song_tpb(self) +
		(psy_audio_song_extra_ticks_per_beat(self) *
			psy_audio_song_lpb(self)));
}
/* return song sampler machine index */
INLINE uintptr_t psy_audio_song_sampler_index(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_sampler_index(&self->properties_);
}

/* set song sampler machine index */
INLINE void psy_audio_song_set_sampler_index(psy_audio_Song* self,
	uintptr_t sampler_index)
{
	assert(self);

	psy_audio_songproperties_set_sampler_index(&self->properties_,
		sampler_index);
}

INLINE void psy_audio_song_enable_auto_note_off(psy_audio_Song* self)
{
	assert(self);

	psy_audio_songproperties_enable_auto_note_off(&self->properties_);
}

INLINE void psy_audio_song_disable_auto_note_off(psy_audio_Song* self)
{
	assert(self);

	psy_audio_songproperties_disable_auto_note_off(&self->properties_);
}

INLINE bool psy_audio_song_auto_note_off(const psy_audio_Song* self)
{
	assert(self);

	return psy_audio_songproperties_auto_note_off(&self->properties_);
}
	
#ifdef __cplusplus
}
#endif

#endif /* psy_audio_SONG_H */
