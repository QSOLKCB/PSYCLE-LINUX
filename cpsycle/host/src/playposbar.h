/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PLAYPOSBAR_H)
#define PLAYPOSBAR_H

/* ui */
#include "uilabel.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Player;

/*!
** @struct PlayPosBar
** @brief Displays the player position in beats and seconds
*/

typedef struct PlayPosBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label header;
	psy_ui_Label position;
	/* references */
	struct psy_audio_Player* player;
} PlayPosBar;

void playposbar_init(PlayPosBar*, psy_ui_Component* parent,
	struct psy_audio_Player*);

void playposbar_idle(PlayPosBar*);

INLINE psy_ui_Component* playposbar_base(PlayPosBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PLAYPOSBAR_H */
