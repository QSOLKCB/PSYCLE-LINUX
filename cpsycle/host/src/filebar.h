/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FILEBAR_H)
#define FILEBAR_H

/* host */
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct FileBar
** @brief Tool Buttons for song create, disk ops and render
*/

typedef struct FileBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component client;
	psy_ui_Label desc_;
	psy_ui_Button song_new_;
	psy_ui_Button song_disk_op_;
	psy_ui_Button song_load_;
	psy_ui_Button song_save_;
	psy_ui_Button song_render_;	
	/* references */
	Workspace* workspace_;	
} FileBar;

void filebar_init(FileBar*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* filebar_base(FileBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* FILEBAR_H */
