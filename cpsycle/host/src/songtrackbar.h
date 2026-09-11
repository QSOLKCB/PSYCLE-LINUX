/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SONGTRACKBAR_H)
#define SONGTRACKBAR_H

/* host */
#include "workspace.h"
/* ui */
#include "uicombobox.h"
#include "uilabel.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct SongTrackBar
** @brief Sets the number of tracks for your patterns.
**
** @detail
** This is a global setting affecting all patterns and sequence tracks. The
** player will only play the tracks up to this number.
*/
typedef struct SongTrackBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label desc_;
	psy_ui_ComboBox track_numbers_;
	/* references */
	Workspace* workspace_;
} SongTrackBar;

void songtrackbar_init(SongTrackBar*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* songtrackbar_base(SongTrackBar* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SONGTRACKBAR_H */
