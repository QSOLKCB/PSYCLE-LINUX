/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESIMPORTVIEW_H)
#define SAMPLESIMPORTVIEW_H

/* host */
#include "samplesbox.h"
#include "wavebox.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SamplesImportView */

struct SamplesView;

typedef struct SamplesImportView {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component header;
	psy_ui_Label label;
	psy_ui_Label songname;
	psy_ui_Button browse;
	psy_ui_CheckBox wave_prev_;
	SamplesBox samplesbox;
	psy_ui_Component bar;
	psy_ui_Button add;
	WaveBox samplebox;	
	/* references */
	psy_audio_Song* source;
	struct SamplesView* view;
	Workspace* workspace;	
} SamplesImportView;

void samplesimportview_init(SamplesImportView*, psy_ui_Component* parent,
	struct SamplesView* view, Workspace*);

INLINE psy_ui_Component* samplesimportview_base(SamplesImportView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLESVIEW_H */
