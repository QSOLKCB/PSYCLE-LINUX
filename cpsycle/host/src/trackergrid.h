/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TRACKERGRID)
#define TRACKERGRID

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "trackercolumn.h"
#include "trackergridstate.h"
#include "patternhostcmds.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct TrackerGrid
** @brief A spreadsheet to view and edit the patterns
**
** @details
** The TrackerGrid is where you enter notes. It displays a Pattern selected by
** the SeqView as a tracker grid.
*/
typedef struct TrackerGrid {
	/*! @extends */
	psy_ui_Component component;
	/* signals */	
	psy_Signal signal_col_resize;
	/*! @internal */
	psy_audio_SequenceCursor old_cursor;
	psy_audio_SequenceCursor last_drag_cursor;
	bool effcursor_always_down;	
	bool prevent_cursor;
	psy_Table columns;
	bool prevent_event_driver;	
	bool down;
	PatternCmds cmds;
	/* references */
	TrackerState* state;	
	psy_audio_Player* player;	
} TrackerGrid;

void trackergrid_init(TrackerGrid*, psy_ui_Component* parent,
	TrackerState*, psy_DiskOp*);

void trackergrid_build(TrackerGrid*);
void trackergrid_show_empty_data(TrackerGrid*, int showstate);
void trackergrid_invalidate_playbar(TrackerGrid*);
void trackergrid_invalidate_cursor(TrackerGrid*);
void trackergrid_invalidate_internal_cursor(TrackerGrid*,
	psy_audio_SequenceCursor);
void trackergrid_center_on_cursor(TrackerGrid*);
void trackergrid_set_center_mode(TrackerGrid*, intptr_t mode);
void trackergrid_scroll_down(TrackerGrid*, psy_audio_SequenceCursor,
	bool set);

INLINE bool trackergrid_midline(TrackerGrid* self)
{
	return self->state->midline;
}

INLINE psy_ui_Component* trackergrid_base(TrackerGrid* self)
{
	assert(self);

	return &self->component;
}

INLINE bool trackergrid_check_update(const TrackerGrid* self)
{
	const psy_audio_Pattern* pattern;

	pattern = patternviewstate_pattern_const(self->state->pv);
	if (pattern) {
		bool rv;
		uintptr_t opcount;

		opcount = psy_audio_pattern_opcount(pattern);
		rv = (opcount != self->component.opcount);
		((TrackerGrid*)self)->component.opcount = opcount;
		return rv;
	}
	return FALSE;
}


#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* TRACKERGRID */
