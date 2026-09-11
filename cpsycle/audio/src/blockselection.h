/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_BLOCKSELECTION_H
#define psy_audio_BLOCKSELECTION_H

/* local */
#include "sequencecursor.h"

#ifdef __cplusplus
extern "C" {
#endif


#define psy_audio_GLOBALPATTERN INT32_MAX - 1
#define psy_audio_GLOBALPATTERN_TIMESIGTRACK 0
#define psy_audio_GLOBALPATTERN_LOOPTRACK 1

struct psy_audio_Sequence;

/*!
** @struct psy_audio_BlockSelection
** @brief Defines a block selection of a sequence
*/
typedef struct psy_audio_BlockSelection {
	psy_audio_SequenceCursor topleft;
	psy_audio_SequenceCursor bottomright;	
	psy_audio_SequenceCursor drag_base_;
	bool valid_;
} psy_audio_BlockSelection;

void psy_audio_blockselection_init(psy_audio_BlockSelection*);
void psy_audio_blockselection_init_all(psy_audio_BlockSelection*,
	psy_audio_SequenceCursor topleft, psy_audio_SequenceCursor bottomright);
psy_audio_BlockSelection psy_audio_blockselection_make(
	psy_audio_SequenceCursor topleft, psy_audio_SequenceCursor bottomright);
	
INLINE psy_audio_BlockSelection psy_audio_blockselection_zero(void)
{
	psy_audio_BlockSelection rv;
	
	psy_audio_blockselection_init(&rv);
	return rv;
}	

INLINE bool psy_audio_blockselection_valid(
	const psy_audio_BlockSelection* self)
{
	return self->valid_;
}

INLINE void psy_audio_blockselection_enable(psy_audio_BlockSelection* self)
{
	self->valid_ = TRUE;
}

INLINE void psy_audio_blockselection_disable(psy_audio_BlockSelection* self)
{
	self->valid_ = FALSE;
}

INLINE bool psy_audio_blockselection_test_track(
	const psy_audio_BlockSelection* self,
	uintptr_t track)
{
	return (track >= psy_audio_sequencecursor_channel(&self->topleft) &&
		track < psy_audio_sequencecursor_channel(&self->bottomright));
}

void psy_audio_blockselection_start_drag(psy_audio_BlockSelection*,	
	psy_audio_SequenceCursor);
void psy_audio_blockselection_drag(psy_audio_BlockSelection*,
	psy_audio_SequenceCursor, struct psy_audio_Sequence*);


#ifdef __cplusplus
}
#endif

#endif /* psy_audio_BLOCKSELECTION_H */
