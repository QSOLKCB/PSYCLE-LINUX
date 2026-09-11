/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(VUMETER_H)
#define VUMETER_H

/* ui */
#include "uicomponent.h"
/* audio */
#include <machine.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** Vumeter
**
** The VU Meter displays the left and right master volume during playback.
** vumeter_idle updates to the rms volume
**
** psy_ui_Component
**         ^
**         |     
**      Vumeter < >---- psy_audio_Machine
*/

typedef struct Vumeter {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	double l_rms_;
	double r_rms_;
	double l_max_;
	double r_max_;
	double l_log_;
	double r_log_;
	double vu_prev_l_;
	double vu_prev_r_;	
	psy_ui_RealSize channel_size_;
	psy_ui_Colour vu_;
	psy_ui_Colour peak_;
	/* references */
	psy_audio_Machine* machine_;	
} Vumeter;

void vumeter_init(Vumeter*, psy_ui_Component* parent);

void vumeter_set_machine(Vumeter*, psy_audio_Machine*);
void vumeter_idle(Vumeter*);
void vumeter_update_style(Vumeter*);

INLINE psy_ui_Component* vumeter_base(Vumeter* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* VUMETER_H */
