/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PIANORULER_H)
#define PIANORULER_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PIANOROLL

/* host */
#include "pianogridstate.h"
/* ui */
#include <uicomponent.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct PianoRuler
** @brief Beat marker bar for the pianogrid
*/
typedef struct PianoRuler {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Colour overlaycolour;
	psy_ui_Colour linecolour;
	psy_ui_Colour linebeatcolour;
	psy_ui_Colour linebeat4colour;
	/* references */
	PianoGridState* gridstate;	
} PianoRuler;

void pianoruler_init(PianoRuler*, psy_ui_Component* parent, PianoGridState*);

INLINE psy_ui_Component* pianoruler_base(PianoRuler* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_PIANOROLL */

#endif /* PIANORULER_H */

