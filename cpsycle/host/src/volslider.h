/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(VOLSLIDER_H)
#define VOLSLIDER_H


/* ui */
#include <uislider.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Machine;

/*!
** @struct Volume slider
** @brief Controls the volume of the Master
*/
typedef struct VolSlider {
	/*! @extends */
	psy_ui_Slider slider;
	/*! @internal */	
	uintptr_t volume_param_idx_;
	/* references */
	struct psy_audio_Machine* machine_;
} VolSlider;

void volslider_init(VolSlider*, psy_ui_Component* parent);

INLINE void volslider_set_machine(VolSlider* self, struct psy_audio_Machine*
	machine)
{
	assert(self);
	
	self->machine_ = machine;
}

INLINE psy_ui_Component* volslider_base(VolSlider* self)
{
	assert(self);

	return psy_ui_slider_base(&self->slider);
}

#ifdef __cplusplus
}
#endif

#endif /* VOLSLIDER_H */
