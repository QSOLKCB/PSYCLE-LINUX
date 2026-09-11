/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNNAVIGATOR_H)
#define PATTERNNAVIGATOR_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* host */

#include "patternviewstate.h"
#ifdef PSYCLE_USE_TRACKERVIEW
#include "trackergridstate.h"
#endif
#ifdef PSYCLE_USE_PIANOROLL
#include "pianokeyboardstate.h"
#endif


/* PatternLineNavigator */

typedef struct PatternLineNavigator {
	/*! @internal */
	PatternViewState* state;
	bool wrap;
	bool wrap_around;
} PatternLineNavigator;

void patternlinenavigator_init(PatternLineNavigator*, PatternViewState*,
	bool wraparound);

psy_audio_SequenceCursor patternlinenavigator_up(PatternLineNavigator*,
	uintptr_t lines, const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patternlinenavigator_down(PatternLineNavigator*,
	uintptr_t lines, const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patternlinenavigator_home(PatternLineNavigator*,
	const psy_audio_SequenceCursor*); /* ft2 home */
psy_audio_SequenceCursor patternlinenavigator_end(PatternLineNavigator*,
	const psy_audio_SequenceCursor*); /* ft2 end */

INLINE bool patternlinennavigator_wrap(const PatternLineNavigator* self)
{
	return self->wrap;
}

/* PatternColNavigator */

#ifdef PSYCLE_USE_TRACKERVIEW

typedef struct PatternColNavigator {
	/*! @internal */
	TrackerState* state;
	bool wrap;
	bool wrap_around;
} PatternColNavigator;

void patterncolnavigator_init(PatternColNavigator*, TrackerState*,
	bool wraparound);

psy_audio_SequenceCursor patterncolnavigator_prev_track(PatternColNavigator*,
	const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patterncolnavigator_next_track(PatternColNavigator*,
	const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patterncolnavigator_prev_col(PatternColNavigator*,
	const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patterncolnavigator_next_col(PatternColNavigator*,
	const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patterncolnavigator_home(PatternColNavigator*,
	const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patterncolnavigator_end(PatternColNavigator*,
	const psy_audio_SequenceCursor*);


INLINE bool patterncolnavigator_wrap(const PatternColNavigator* self)
{
	return self->wrap;
}

#endif

#ifdef PSYCLE_USE_PIANOROLL

/* PatternKeyNavigator */

typedef struct PatternKeyNavigator {
	/*! @internal */
	KeyboardState* state;
	bool wrap;
	bool wrap_around;
} PatternKeyNavigator;

void patternkeynavigator_init(PatternKeyNavigator*, KeyboardState*,
	bool wraparound);

psy_audio_SequenceCursor patternkeynavigator_up(PatternKeyNavigator*,
	uint8_t keys, const psy_audio_SequenceCursor*);
psy_audio_SequenceCursor patternkeynavigator_down(PatternKeyNavigator*,
	uint8_t keys, const psy_audio_SequenceCursor*);

INLINE bool patternkeynavigator_wrap(const PatternKeyNavigator* self)
{
	return self->wrap;
}

#endif

#endif /* PSYCLE_USE_PATTERN_VIEW */

#endif /* PATTERNNAVIGATOR_H */
