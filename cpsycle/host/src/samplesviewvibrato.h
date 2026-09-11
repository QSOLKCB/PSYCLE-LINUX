/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESVIEWVIBRATO_H)
#define SAMPLESVIEWVIBRATO_H

/* host */
#include "workspace.h"
/* ui */
#include <uilabel.h>
#include <uislider.h>

#ifdef __cplusplus
extern "C" {
#endif


/*!
** @struct SamplesViewVibrato
*/
typedef struct SamplesViewVibrato {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component header;
	psy_ui_Label waveformheaderlabel;
	psy_ui_ComboBox waveformbox;
	psy_ui_Slider attack;
	psy_ui_Slider speed;
	psy_ui_Slider depth;
	/* references */
	psy_audio_Player* player;
	psy_audio_Sample* sample;
} SamplesViewVibrato;

void samplesviewvibrato_init(SamplesViewVibrato*, psy_ui_Component* parent,
	psy_audio_Player* player);

void samplesviewvibrato_set_sample(SamplesViewVibrato*, psy_audio_Sample*);

INLINE psy_ui_Component* samplesviewvibrato_base(SamplesViewVibrato* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLESVIEWVIBRATO_H */
