/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SONGBAR_H)
#define SONGBAR_H

#include "linesperbeatbar.h"
/* host */
#include "octavebar.h"
#include "songtrackbar.h"
#include "timebar.h"
#include "workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct SongBar
** @brief Song settings for track, bpm, lpb and octave.
*/
typedef struct SongBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	SongTrackBar song_track_bar_;
	TimeBar time_bar_;
	LinesPerBeatBar lines_per_beat_bar_;
	OctaveBar octave_bar_;
} SongBar;

void songbar_init(SongBar*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* songbar_base(SongBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SONGBAR_H */
