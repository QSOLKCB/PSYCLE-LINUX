/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNSELECTION_H)
#define PATTERNSELECTION_H

/* container */
#include <signal.h>
/* audio */
#include <blockselection.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Sequence;

/*!
** @struct PatternSelection
*/
typedef struct PatternSelection {
	/* signals */
	psy_Signal signal_changed;
	psy_audio_BlockSelection block;
	bool selecting;
} PatternSelection;

void patternselection_init(PatternSelection*);
void patternselection_dispose(PatternSelection*);

void patternselection_notify(PatternSelection*);
void patternselection_set(PatternSelection*, psy_audio_SequenceCursor topleft,
	psy_audio_SequenceCursor bottomright);
void patternselection_enable(PatternSelection*);
void patternselection_disable(PatternSelection*);
void patternselection_start_drag(PatternSelection*, psy_audio_SequenceCursor);
void patternselection_drag(PatternSelection*, psy_audio_SequenceCursor,
	struct psy_audio_Sequence*);
void patternselection_stop_drag(PatternSelection*);
void patternselection_select_all(PatternSelection*,
	struct psy_audio_Sequence*, bool single);
void patternselection_select_col(PatternSelection*,
	struct psy_audio_Sequence*, bool single);
void patternselection_select_bar(PatternSelection*,
	struct psy_audio_Sequence*, bool single);


#ifdef __cplusplus
}
#endif

#endif /* PATTERNSELECTION_H */
