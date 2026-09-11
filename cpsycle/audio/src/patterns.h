/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PATTERNS_H
#define psy_audio_PATTERNS_H

#include "pattern.h"
#include "trackstate.h"

#include <hashtbl.h>
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_Patterns
** @brief Container of patterns
*/

typedef struct psy_audio_Patterns {
	/* signals */
	psy_Signal signal_numsongtrackschanged;
	psy_Signal signal_name_changed;
	psy_Signal signal_tracknamechanged;
	psy_Signal signal_length_changed;
	/*! @internal */
	psy_Table slots;
	uintptr_t songtracks;
	bool sharetracknames;
	psy_audio_TrackState trackstate;
	psy_Table shared_track_names;	
} psy_audio_Patterns;

void psy_audio_patterns_init(psy_audio_Patterns*);
void psy_audio_patterns_dispose(psy_audio_Patterns*);

psy_audio_Pattern* psy_audio_patterns_insert(psy_audio_Patterns*,
	uintptr_t slot, psy_audio_Pattern*);
uintptr_t psy_audio_patterns_append(psy_audio_Patterns*, psy_audio_Pattern*);
psy_audio_Pattern* psy_audio_patterns_at(psy_audio_Patterns*, uintptr_t slot);
const psy_audio_Pattern* psy_audio_patterns_at_const(const psy_audio_Patterns*,
	uintptr_t slot);

INLINE psy_TableIterator psy_audio_patterns_begin(psy_audio_Patterns* self)
{
	assert(self);

	return psy_table_begin(&self->slots);
}

void psy_audio_patterns_clear(psy_audio_Patterns*);
void psy_audio_patterns_erase(psy_audio_Patterns*, uintptr_t slot);
void psy_audio_patterns_remove(psy_audio_Patterns*, uintptr_t slot);
uintptr_t psy_audio_patterns_size(psy_audio_Patterns*);
void psy_audio_patterns_activate_solo_track(psy_audio_Patterns*,
	uintptr_t track);
void psy_audio_patterns_set_solo_track(psy_audio_Patterns*, uintptr_t track);
void psy_audio_patterns_deactivate_solo_track(psy_audio_Patterns*);
void psy_audio_patterns_mute_track(psy_audio_Patterns*, uintptr_t track);
void psy_audio_patterns_unmute_track(psy_audio_Patterns*, uintptr_t track);
bool psy_audio_patterns_is_track_muted(const psy_audio_Patterns*,
	uintptr_t track);
bool psy_audio_patterns_is_track_soloed(const psy_audio_Patterns*,
	uintptr_t track);
uintptr_t psy_audio_patterns_track_soloed(const psy_audio_Patterns*);
void psy_audio_patterns_arm_track(psy_audio_Patterns*, uintptr_t track);
void psy_audio_patterns_unarm_track(psy_audio_Patterns*, uintptr_t track);
bool psy_audio_patterns_is_track_armed(const psy_audio_Patterns*,
	uintptr_t track);
uintptr_t psy_audio_psy_audio_patterns_track_armed_count(
	const psy_audio_Patterns*);
void psy_audio_patterns_set_num_tracks(psy_audio_Patterns*,
	uintptr_t numtracks);
uintptr_t psy_audio_patterns_num_tracks(const psy_audio_Patterns*);
uintptr_t psy_audio_patterns_slot(psy_audio_Patterns*, psy_audio_Pattern*);
void psy_audio_patterns_set_shared_track_name(psy_audio_Patterns*,
	uintptr_t track_index, const char* name);
const char* psy_audio_pattern_shared_track_name(const psy_audio_Patterns*,
	uintptr_t track);
const char* psy_audio_patterns_track_name(const psy_audio_Patterns*,
	uintptr_t slot, uintptr_t track_index);
void psy_audio_patterns_share_names(psy_audio_Patterns*);
void psy_audio_patterns_use_individual_names(psy_audio_Patterns*);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PATTERNS_H */
