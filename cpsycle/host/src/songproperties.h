/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SONGPROPERTIES_H)
#define SONGPROPERTIES_H

/* host */
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uinumberedit.h>
#include <uitext.h>
#include <uitextarea.h>
#include <uisizer.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct SongPropertiesView
** @brief Allows you to enter a name for the song, the author and commments.
**
** @details
** Since version 1.11, it is also possible to change from here the tempo and
** lines per beat, as well as setting ticks per beat and Extra ticks per line.
** These two are used for Sampulse commands, to emulate properly the ticks that
** commands such as volume slide use.
** The ticks per beat is the amount of ticks for each beat. The Extra tick is
** meant to alter this so that speeds like "5" and "7" can be properly emulated.
** Since extra ticks alter the line duration, "Real tempo" and "Real ticks per
** beat" is also shown.
*/
typedef struct SongPropertiesView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Component viewtabbar;	
	psy_ui_Label label_title;
	psy_ui_Edit edit_title;	
	psy_ui_Label label_credits;
	psy_ui_Edit edit_credits;
	/* settings */	
	psy_ui_Component speedbar;
	psy_ui_NumberEdit tempo;
	psy_ui_NumberEdit lpb;
	psy_ui_NumberEdit tpb;
	psy_ui_NumberEdit etpb;	
	psy_ui_NumberEdit samplerindex;	
	psy_ui_Component realtempo;
	psy_ui_Label realtempo_desc;
	psy_ui_Label realtempo_value;
	psy_ui_Label realticksperbeat_desc;
	psy_ui_Label realticksperbeat;
	psy_ui_Button auto_note_off;
	/* text */	
	psy_ui_Label label_comments;
	psy_ui_TextArea edit_comments;
	psy_ui_Component bottom;
	psy_ui_Button prevent_edit;
	/* sizer */
	psy_ui_Sizer sizer;
	/* references */
	psy_audio_Song* song;
	Workspace* workspace;
} SongPropertiesView;

void songpropertiesview_init(SongPropertiesView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace*);

INLINE psy_ui_Component* songpropertiesview_base(SongPropertiesView* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SONGPROPERTIES_H */
