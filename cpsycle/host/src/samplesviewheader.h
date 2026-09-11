/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESVIEWHEADER_H)
#define SAMPLESVIEWHEADER_H

/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uitext.h>
/* audio */
#include <instruments.h>
#include <samples.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SamplesView;

/*! 
** @struct SamplesViewHeader
*/
typedef struct SamplesViewHeader {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label name_desc_;
	psy_ui_Edit name_;
	psy_ui_Button prev_;
	psy_ui_Button next_;
	psy_ui_Label sample_rate_desc_;
	psy_ui_Edit sample_rate_edit_;
	psy_ui_Label num_frames_desc_;
	psy_ui_Label num_frames_;
	psy_ui_Label channels_;
	/* references */
	struct SamplesView* view_;
	psy_audio_Sample* sample_;
	psy_audio_Instruments* instruments_;
} SamplesViewHeader;

void samplesviewheader_init(SamplesViewHeader*, psy_ui_Component* parent,
	psy_audio_Instruments*, struct SamplesView*);

void samplesviewheader_set_sample(SamplesViewHeader*, psy_audio_Sample*);

INLINE psy_ui_Component* samplesviewheader_base(SamplesViewHeader* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLESVIEWHEADER_H */
