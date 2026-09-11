/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNHEADER_H)
#define PATTERNHEADER_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "trackergridstate.h"
#include "trackerheaderstyles.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>


/*!
** @struct PatternTrackBox
** @brief Track header of a pattern track
*/
typedef struct PatternTrackBox {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component play;
	psy_ui_Component solo;
	psy_ui_Label solo_label;
	psy_ui_Component mute;
	psy_ui_Label mute_label;
	psy_ui_Component record;
	psy_ui_Label record_label;
	psy_ui_Label text;
	uintptr_t index;
	/* references */
	TrackerState* state;
} PatternTrackBox;

void patterntrackbox_init(PatternTrackBox*, psy_ui_Component* parent,
	uintptr_t index, TrackerState*);

PatternTrackBox* patterntrackbox_alloc(void);
PatternTrackBox* patterntrackbox_allocinit(psy_ui_Component* parent,
	uintptr_t index, TrackerState*);

void patterntrackbox_playon(PatternTrackBox*);
void patterntrackbox_playoff(PatternTrackBox*);
void patterntrackbox_update(PatternTrackBox*);
void patterntrackbox_update_text(PatternTrackBox*);

INLINE psy_ui_Component* patterntrackbox_base(PatternTrackBox* self)
{
	return &self->component;
}


/*!
** @struct PatternTrack
** @brief Centers a PatternTrackBox
*/
typedef struct PatternTrack {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	PatternTrackBox trackbox;
} PatternTrack;

void patterntrack_init(PatternTrack*, psy_ui_Component* parent,
	uintptr_t index, TrackerState*);

PatternTrack* patterntrack_alloc(void);
PatternTrack* patterntrack_allocinit(psy_ui_Component* parent,
	uintptr_t index, TrackerState*);

INLINE psy_ui_Component* patterntrack_base(PatternTrack* self)
{
	return &self->component;
}


/*!
** @struct TrackerHeader
** @brief Track headers for each pattern track
*/
typedef struct TrackerHeader {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_Table boxes;
	/* references */
	TrackerState* state;
	Workspace* workspace;
} TrackerHeader;

void trackerheader_init(TrackerHeader*, psy_ui_Component* parent, TrackConfig*,
	TrackerState*, Workspace*);
void trackerheader_build(TrackerHeader*);
void trackerheader_update_styles(TrackerHeader*);
void trackerheader_update_states(TrackerHeader*);

INLINE psy_ui_Component* trackerheader_base(TrackerHeader* self)
{
	return &self->component;
}


/*!
** @struct TrackerHeaderView
** @brief Scrollable track headers
*/
typedef struct TrackerHeaderView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component pane;
	psy_ui_Button desc;
	TrackerHeader header;
	TrackerHeaderStyles header_styles;
	/* references */
	Workspace* workspace;
	psy_Configuration* pat_cfg;	
} TrackerHeaderView;

void trackerheaderview_init(TrackerHeaderView*, psy_ui_Component* parent,
	TrackConfig*, TrackerState*, Workspace*);

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* PATTERNHEADER_H */
