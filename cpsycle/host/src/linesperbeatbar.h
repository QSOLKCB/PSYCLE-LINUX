/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(LINESPERBEATBAR_H)
#define LINESPERBEATBAR_H

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

/*
** @struct LinesPerBeatBar
** @brief Sets the Lines per beat ranging from 1 to 31.
*/
typedef struct LinesPerBeatBar {
	/*! extends */
	psy_ui_Component component;
	/*! internal */
	/* description label */
	psy_ui_Label desc_;
	/* displays the player lpb */
	ValueUi number_;
	/* decrement lpb button by -1 */
	psy_ui_Button less_;
	/* increment lpb button by +1 */
	psy_ui_Button more_;	
	/* references */
	psy_audio_Player* player_;
} LinesPerBeatBar;

void linesperbeatbar_init(LinesPerBeatBar*, psy_ui_Component* parent,
	psy_audio_Player*);

INLINE psy_ui_Component* linesperbeatbar_base(LinesPerBeatBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* LINESPERBEATBAR_H */
