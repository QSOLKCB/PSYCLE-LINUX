/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESVIEWGENERAL_H)
#define SAMPLESVIEWGENERAL_H

/* audio */
#include <sample.h>
/* dsp */
#include <notestab.h>
/* ui */
#include <uislider.h>
/* container */
#include <configuration.h>


#ifdef __cplusplus
extern "C" {
#endif

/*! 
** @struct SamplesViewGeneral
*/
typedef struct SamplesViewGeneral {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Slider defaultvolume;
	psy_ui_Slider globalvolume;
	psy_ui_Slider panposition;
	psy_ui_Slider samplednote; 
	psy_ui_Slider pitchfinetune;	
	psy_dsp_NotesTabMode notes_tab_mode;
	/* references */
	psy_audio_Sample* sample;
} SamplesViewGeneral;

void samplesviewgeneral_init(SamplesViewGeneral*, psy_ui_Component* parent,
	psy_Configuration* patview_cfg);

void samplesviewgeneral_set_sample(SamplesViewGeneral*, psy_audio_Sample*);

INLINE psy_ui_Component* samplesviewgeneral_base(SamplesViewGeneral* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLESVIEWGENERAL_H */
