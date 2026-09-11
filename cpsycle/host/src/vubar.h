/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(VUBAR_H)
#define VUBAR_H

/* host */
#include "clipbox.h"
#include "volslider.h"
#include "vumeter.h"
/* ui */
#include <uilabel.h>


#ifdef __cplusplus
extern "C" {
#endif

struct psy_Configuration;

/*!
** @struct VuBar
** @brief Composite of Vumeter, VolumeSlider and ClipBox.
** 
** @detail
** Displays and controls the
** Master volume and displays a clip warning if the amp range overflows
**
** psy_ui_ComponentImp
**          ^
**          |
**        VuBar <>------ Vumeter
**               |------ VolSlider
**               |------ ClipBox
**               |-------psy_ui_Label
*/
typedef struct VuBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label vu_label_;
	Vumeter vu_meter_;
	VolSlider vol_slider_;
	ClipBox clip_box_;	
} VuBar;

void vubar_init(VuBar*, psy_ui_Component* parent, struct psy_Configuration*);

void vubar_reset(VuBar*);
void vubar_idle(VuBar*);
void vubar_set_machine(VuBar*, psy_audio_Machine*);

INLINE psy_ui_Component* vubar_base(VuBar* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* VUBAR_H */
