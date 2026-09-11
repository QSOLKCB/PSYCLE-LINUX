/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SAMPLESVIEW_H)
#define SAMPLESVIEW_H

/* host */
#include <uitabbar.h>
#include "samplesbox.h"
#include "sampleeditor.h"
#include "samplesimportview.h"
#include "samplesloopedit.h"
#include "samplesviewgeneral.h"
#include "samplesviewheader.h"
#include "samplesviewvibrato.h"
#include "wavebox.h"
#include "workspace.h"
/* dsp */
#include <notestab.h>
/* ui */
#include <uibutton.h>
#include <uicombobox.h>
#include <uitext.h>
#include <uilabel.h>
#include <uinotebook.h>
#include <uislider.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SamplesViewButtons */
typedef struct SamplesViewButtons {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button load;
	psy_ui_Button save;
	psy_ui_Button duplicate;
	psy_ui_Button del;
} SamplesViewButtons;

void samplesviewbuttons_init(SamplesViewButtons*, psy_ui_Component* parent);

/* SamplesView */
typedef struct SamplesView {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_TabBar clienttabbar;
	psy_ui_Notebook clientnotebook;
	psy_ui_Component mainview;
	psy_ui_Component importview;
	psy_ui_Notebook notebook;
	SamplesBox samplesbox;
	psy_ui_Component left;
	SamplesViewButtons buttons;	
	psy_ui_Component right;
	psy_ui_Component client;
	SamplesImportView songimport;
	SampleEditor sampleeditor;
	SamplesViewHeader header;
	psy_ui_TabBar tabbar;
	SamplesViewGeneral general;
	SamplesViewVibrato vibrato;
	psy_ui_Component loop;
	SamplesLoopEdit waveloop;	
	WaveBox wavebox;	
	/* references */
	Workspace* workspace;	
} SamplesView;

void samplesview_init(SamplesView*, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace*);

void samplesview_set_sample(SamplesView*, psy_audio_SampleIndex);
void samplesview_connect_status_bar(SamplesView*);

INLINE psy_ui_Component* samplesview_base(SamplesView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SAMPLESVIEW_H */
