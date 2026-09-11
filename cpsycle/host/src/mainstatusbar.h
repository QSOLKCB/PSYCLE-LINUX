/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MAINSTATUSBAR_H)
#define MAINSTATUSBAR_H

/* platform */
#include "../../detail/os.h"
/* host */
#include "clockbar.h"
#include "interpreter.h"
#include "workspace.h"
#include "zoombox.h"
/* ui */
#include <uilabel.h>
#include <uibutton.h>
#include <uinotebook.h>
#include <uiprogressbar.h>
#include <uisizer.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct StatusLabel
*/
typedef struct StatusLabel {
	/*! @extends */
	psy_ui_Component component;
	/*! @implements */
	psy_Logger logger;
	/*! @internal */
	psy_ui_ProgressBar progress_bar_;
	psy_ui_Label label_;
} StatusLabel;

void statuslabel_init(StatusLabel*, psy_ui_Component* parent);

void statuslabel_set_default_text(StatusLabel*, const char* text);
void statuslabel_tick_progress(StatusLabel*);

INLINE psy_ui_Component* statuslabel_base(StatusLabel* self)
{
	return &self->component;
}


/*!
** @struct MainStatusBar
*/
typedef struct MainStatusBar {
	/*! @extends */
	psy_ui_Component component;	
	/*! @internal */	
	StatusLabel status_label_;	
	psy_ui_Component right;
	psy_ui_Button terminal_;	
	psy_ui_Button togglekbd;
	psy_ui_Button cpu;
	psy_ui_Button midi;
	ClockBar clock_;	
	ZoomBox zoom_;
	psy_ui_Sizer sizer_;	
	int terminal_status;
	/* references */
	Workspace* workspace;
} MainStatusBar;

void mainstatusbar_init(MainStatusBar*, psy_ui_Component* parent, Workspace*);

void mainstatusbar_set_default_status_text(MainStatusBar*, const char* text);
void mainstatusbar_idle(MainStatusBar*);
void mainstatusbar_update_song(MainStatusBar*);

INLINE psy_ui_Component* mainstatusbar_base(MainStatusBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MAINSTATUSBAR_H */
