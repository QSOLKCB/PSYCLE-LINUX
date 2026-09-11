/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MAINVIEWS_H)
#define MAINVIEWS_H

/* host */
#include "helpview.h"
#include "instrumentview.h"
#include "machineview.h"
#include "mainviewbar.h"
#ifdef PSYCLE_USE_PATTERN_VIEW
#include "patternview.h"
#endif
#include "renderview.h"
#include "samplesview.h"
#include "songproperties.h"
#include "startscript.h"
#ifdef PSYCLE_USE_STYLE_VIEW
#include "styleview.h"
#endif
#include "uiviewframe.h"
#include "workspace.h"
/* ui */
#include <uiterminal.h>
#include <uinotebook.h>
#include <uisplitbar.h>
/* container */
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif


/* MainViews */

typedef struct MainViews {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	MainViewBar mainviewbar;
	psy_ui_Notebook notebook;
	psy_ui_EmptyViewPage empty_page;
	MachineView machineview;
#ifdef PSYCLE_USE_PATTERN_VIEW	
	PatternView patternview;
#endif
	SamplesView samplesview;
	InstrumentView instrumentsview;
	SongPropertiesView songpropertiesview;	
	RenderView renderview;	
	PropertiesView settingsview;
#ifdef PSYCLE_USE_STYLE_VIEW	
	StyleView styleview;
#endif	
	HelpView helpview;
	ConfirmBox confirm;
	FileView fileview;
	Links links;	
	/* references */
	Workspace* workspace;
	PsycleConfig* cfg_;
} MainViews;

void mainviews_init(MainViews*, psy_ui_Component* parent,
	psy_ui_Component* pane, Workspace*);

void mainviews_add_link(MainViews*, Link*);

INLINE psy_ui_Component* mainviews_base(MainViews* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MAINVIEWS_H */
