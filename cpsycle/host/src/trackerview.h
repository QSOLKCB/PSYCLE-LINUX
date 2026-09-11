/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TRACKERVIEW)
#define TRACKERVIEW

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "trackergrid.h"
#include "trackerlinenumbers.h"
#include "trackergrid.h"
/* ui */
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct TrackerView
** @brief The TrackerView is where you enter notes
**
** @details
** It displays a Pattern selected by the SeqView as a tracker grid and handles
** the scroll.
*/
typedef struct TrackerView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Scroller scroller;	
	TrackerGrid grid;
	TrackerLineNumberView lines;	
	psy_ui_ScrollBar hscroll;
	psy_ui_ScrollBar vscroll;	
	/* references */
	Workspace* workspace;	
} TrackerView;

void trackerview_init(TrackerView*, psy_ui_Component* parent, TrackerState*,
	psy_Configuration* pat_config, Workspace*);

void trackerview_align(TrackerView*);

void trackerview_make_cmds(psy_Property* parent);

INLINE psy_ui_Component* trackerview_base(TrackerView* self)
{
	assert(self);

	return &self->scroller.component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* TRACKERVIEW */
