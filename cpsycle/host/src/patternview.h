/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEW_H)
#define PATTERNVIEW_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* host */
#include "interpolatecurveview.h"
#include "patternheader.h"
#include "patternproperties.h"
#include "patternviewstyles.h"
#include "patternviewbar.h"
#include "patternviewmenu.h"
#include "patternviewtabbar.h"
#include "pianoroll.h"
#include "patterndefaultline.h"
#include "trackerlinenumbers.h"
#include "tracknameedit.h"
#include "transformpatternview.h"
#include "stepbox.h"
#include "swingfillview.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uilabel.h>
#include <uinotebook.h>
#include <uisplitbar.h>
#include <uitabbar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct PatternView
** @brief Displays the tracker and/or pianoroll
**
** @details
** Editor/Viewer for a single pattern or the whole sequence.
** Composite of TrackerView and Pianoroll.
*/
typedef struct PatternView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	PatternViewTabBar tabbar;
	PatternViewBar patternviewbar;	
	psy_ui_Notebook notebook;
#ifdef PSYCLE_USE_TRACKERVIEW		
	PatternDefaultLine defaultline;
	TrackerHeaderView header;
	TrackerView trackerview;
	TrackConfig track_config;
	TrackerState state;
#endif	
#ifdef PSYCLE_USE_PIANOROLL
	Pianoroll pianoroll;
#endif	
	PatternViewStyles pattern_styles;
	PatternStyleConfigurator pattern_style_configurator;
	PatternProperties properties;
	PatternMenu blockmenu;
	TransformPatternView transformpattern;
	InterpolateCurveView interpolatecurveview;
	psy_ui_Splitter splitter;
	SwingFillView swing_fill_;
	TrackNameEdit tracknames;	
	PatternViewState pvstate;	
	PatternDisplayMode display;
	bool prevent_grid_scroll;
	/* references */
	psy_Configuration* pat_cfg;
	Workspace* workspace;
} PatternView;

void patternview_init(PatternView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, psy_Configuration* pat_config,
	Workspace*);

INLINE psy_ui_Component* patternview_base(PatternView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_PATTERN_VIEW */

#endif /* PATTERNVIEW_H */
