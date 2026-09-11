/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESLOOPEDIT_H)
#define SAMPLESLOOPEDIT_H

/* audio */
#include <sample.h>
/* ui */
#include <uicombobox.h>
#include <uinumberedit.h>
#include <uitext.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SamplesLoopEdit */

struct SamplesView;

typedef struct SamplesLoopEdit {
	/*! @extends  */
	psy_ui_Component component;	
	/*! @internal */
	psy_ui_Component cont;
	psy_ui_Label loopheaderlabel;
	psy_ui_ComboBox loopdir;	
	psy_ui_NumberEdit loopstartedit;	
	psy_ui_NumberEdit loopendedit;
	psy_ui_Component sustain;
	psy_ui_Label sustainloopheaderlabel;
	psy_ui_ComboBox sustainloopdir;	
	psy_ui_NumberEdit sustainloopstartedit;	
	psy_ui_NumberEdit sustainloopendedit;	
	psy_audio_SampleLoop currloop;
	psy_audio_SampleLoop currsustainloop;
	bool prevent_update;
	/* references */	
	psy_audio_Sample* sample;
} SamplesLoopEdit;

void samplesloopedit_init(SamplesLoopEdit*, psy_ui_Component* parent);

void samplesloopedit_set_sample(SamplesLoopEdit*, psy_audio_Sample*);

INLINE psy_ui_Component* samplesloopedit_base(SamplesLoopEdit* self)
{
	return &self->component;
}


#ifdef __cplusplus
}
#endif

#endif /* SAMPLESLOOPEDIT_H */
