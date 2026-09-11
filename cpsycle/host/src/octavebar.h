/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(OCTAVEBAR_H)
#define OCTAVEBAR_H

/* ui */
#include "uicombobox.h"
#include "uilabel.h"
#include "workspace.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
** Sets the note commands octave, ranging from 0 to 8
*/

typedef struct OctaveBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label desc;
	psy_ui_ComboBox octavebox;
	/* references */
	psy_audio_Player* player;
} OctaveBar;

void octavebar_init(OctaveBar*, psy_ui_Component* parent, psy_audio_Player*);

INLINE psy_ui_Component* octavebar_base(OctaveBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* OCTAVEBAR_H */
