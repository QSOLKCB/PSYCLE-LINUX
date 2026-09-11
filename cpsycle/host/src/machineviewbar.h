/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEVIEWBAR_H)
#define MACHINEVIEWBAR_H

/* host */
#include "zoombox.h"
/* audio */
#include <player.h>
/* ui */
#include <uicheckbox.h>
#include <uisizer.h>
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MachineViewBar
** @brief Statusbar
**
** @details
** Displays machineview actions and options and adds a selector for the mixer,
** if new wires are added to the mixer inputs or sent/returns.
*/
typedef struct MachineViewBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	ZoomBox zoom_box_;
	psy_ui_CheckBox mixer_send_;
	psy_ui_Sizer sizer_;
	/* references */	
	psy_Configuration* cfg_;
	psy_audio_Player* player_;
} MachineViewBar;

void machineviewbar_init(MachineViewBar*, psy_ui_Component* parent,
	psy_Configuration*, psy_audio_Player*);

INLINE psy_ui_Component* machineviewbar_base(MachineViewBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEVIEWBAR_H */
