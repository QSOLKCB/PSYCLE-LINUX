/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TRACKERCOLUMN_H)
#define TRACKERCOLUMN_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "trackergridstate.h"
/* ui */
#include <uicomponent.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct TrackerColumn
** @brief A pattern channel track of the trackergrid
*/
typedef struct TrackerColumn {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	uintptr_t track;
	psy_ui_RealSize digitsize;		
	psy_ui_RealSize size;	
	psy_ui_Colour draw_restore_fg_colour;
	psy_ui_Colour draw_restore_bg_colour;
	/* references */
	TrackerState* state;	
	psy_audio_Sequencer* sequencer;
} TrackerColumn;

void trackercolumn_init(TrackerColumn*, psy_ui_Component* parent,
	uintptr_t index, TrackerState*, psy_audio_Sequencer*);

TrackerColumn* trackercolumn_alloc(void);
TrackerColumn* trackercolumn_alloc_init(psy_ui_Component* parent,
	uintptr_t index, TrackerState*, psy_audio_Sequencer*);

INLINE psy_ui_Component* trackercolumn_base(TrackerColumn* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* TRACKERCOLUMN_H */
