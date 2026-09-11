/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TIMEBAR_H)
#define TIMEBAR_H

/* host */
#include "valueui.h"
/* audio */
#include <player.h>
/* ui */
#include <uibutton.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct TimeBar
** @brief Sets the speed of the player.
** 
** @detail
** Sets the speed of the player in beats per minute, ranging from 33 to 999 BPM
** If you want to increase or decrease the BPM by 10, you can press the CTRL
** key while you click the button
** The older two button version can be set with #define PSYCLE_TIMEBAR_OLD
** see details/psyconf.h
*/
typedef struct TimeBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label desc_;
	ValueUi bpm_;
	psy_ui_Button lessless_;
	psy_ui_Button less_;
	psy_ui_Button more_;
	psy_ui_Button moremore_;	
	/* references */
	psy_audio_Player* player_;
} TimeBar;

void timebar_init(TimeBar*, psy_ui_Component* parent, psy_audio_Player*);

INLINE psy_ui_Component* timebar_base(TimeBar* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* TIMEBAR_H */
