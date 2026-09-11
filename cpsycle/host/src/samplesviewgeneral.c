/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesviewgeneral.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* dsp */
#include <convert.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static intptr_t map_1_128(double value)
{
	return (intptr_t)(value * 128.0);
}

static void samplesviewgeneral_pan_description(SamplesViewGeneral*,
	char* rv);
static void samplesviewgeneral_on_describe(SamplesViewGeneral*, psy_ui_Slider*,
	char* txt);
static void samplesviewgeneral_on_tweak(SamplesViewGeneral*, psy_ui_Slider*,
	double value);
static void samplesviewgeneral_on_value(SamplesViewGeneral*, psy_ui_Slider*,
	double* value);
static void samplesviewgeneral_update_text(SamplesViewGeneral*);
static void samplesviewgeneral_update_sliders(SamplesViewGeneral*);
static void samplesviewgeneral_on_note_tab_mode(SamplesViewGeneral*, psy_Property* sender);

/* implementation */
void samplesviewgeneral_init(SamplesViewGeneral* self, psy_ui_Component* parent,
	psy_Configuration* patview_cfg)
{	
	psy_ui_Margin margin;
	psy_ui_Slider* sliders[] = {
		&self->defaultvolume,
		&self->globalvolume,
		&self->panposition,
		&self->samplednote,
		&self->pitchfinetune,
		0
	};	
	uintptr_t i;	

	assert(self);	

	self->sample = NULL;	
	if (patview_cfg) {	
		if (psy_configuration_value_int(patview_cfg, "notetab", 0) == 0) {
			self->notes_tab_mode = psy_dsp_NOTESTAB_A440;
		}
		else {
			self->notes_tab_mode = psy_dsp_NOTESTAB_A220;
		}
		psy_configuration_connect(patview_cfg, "notetab",
			self, samplesviewgeneral_on_note_tab_mode);
	} else {
		self->notes_tab_mode = psy_dsp_NOTESTAB_A440;
	}
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_slider_init(&self->defaultvolume, &self->component);
	psy_ui_slider_set_default_value(&self->defaultvolume, 1.0);
	margin = psy_ui_defaults_vmargin(psy_ui_defaults());	
	psy_ui_slider_init(&self->globalvolume, &self->component);
	psy_ui_slider_init(&self->panposition, &self->component);
	psy_ui_slider_init(&self->samplednote, &self->component);
	psy_ui_slider_set_default_value(&self->samplednote, 60.0 / 119.0);
	psy_ui_slider_init(&self->pitchfinetune, &self->component);
	for (i = 0; sliders[i] != 0; ++i) {		
		psy_ui_slider_set_char_number(sliders[i], 16);
		psy_ui_slider_set_value_char_number(sliders[i], 10);
		psy_ui_slider_connect(sliders[i], self,
			(ui_slider_fpdescribe)samplesviewgeneral_on_describe,
			(ui_slider_fptweak)samplesviewgeneral_on_tweak,
			(ui_slider_fpvalue)samplesviewgeneral_on_value);
	}
	samplesviewgeneral_update_text(self);	
}

void samplesviewgeneral_update_text(SamplesViewGeneral* self)
{
	assert(self);

	psy_ui_slider_set_text(&self->defaultvolume, 
		"samplesview.default-volume");
	psy_ui_slider_set_text(&self->globalvolume,
		"samplesview.global-volume");
	psy_ui_slider_set_text(&self->panposition,
		"samplesview.pan-position");
	psy_ui_slider_set_text(&self->samplednote,
		"samplesview.sampled-note");
	psy_ui_slider_set_text(&self->pitchfinetune,
		"samplesview.pitch-finetune");
}

void samplesviewgeneral_update_sliders(SamplesViewGeneral* self)
{
	assert(self);

	psy_ui_slider_update(&self->defaultvolume);
	psy_ui_slider_update(&self->globalvolume);
	psy_ui_slider_update(&self->panposition);
	psy_ui_slider_update(&self->samplednote);
	psy_ui_slider_update(&self->pitchfinetune);
}

void samplesviewgeneral_set_sample(SamplesViewGeneral* self,
	psy_audio_Sample* sample)
{
	assert(self);

	self->sample = sample;
	if (self->sample) {
		psy_ui_component_enable_input(&self->component, 1);
	} else {
		psy_ui_component_prevent_input(&self->component, 1);
	}
	samplesviewgeneral_update_sliders(self);
}

void samplesviewgeneral_on_tweak(SamplesViewGeneral* self, psy_ui_Slider* slider,
	double value)
{
	assert(self);

	if (!self->sample) {
		return;
	}
	if (slider == &self->defaultvolume) {
		self->sample->defaultvolume = (uint16_t)(value * 128.0);
	} else if (slider == &self->globalvolume) {		
		self->sample->globalvolume = (value * value) * 4.0;
	} else if (slider == &self->panposition) {
		self->sample->panfactor = value;
	} else if (slider == &self->samplednote) {
		self->sample->zone.tune = (int)(value * 119.0) - 60;
	} else if (slider == &self->pitchfinetune) {
		self->sample->zone.finetune = (int)(value * 200.0) - 100;
	}	
}

void samplesviewgeneral_on_value(SamplesViewGeneral* self, psy_ui_Slider* slider,
	double* value)
{	
	assert(self);

	if (slider == &self->defaultvolume) {		
		*value = (self->sample)
			? psy_audio_sample_volume(self->sample) / 128.0
			: 1.0;		
	} else if (slider == &self->globalvolume) {
		*value = (self->sample)
			? sqrt(psy_audio_sample_global_volume(self->sample)) * 0.5
			: 0.5;
	} else if (slider == &self->panposition) {
		*value = (self->sample)
			? psy_audio_sample_panning(self->sample)
			: 0.5;
	} else if (slider == &self->samplednote) {
		*value = (self->sample)
			? (self->sample->zone.tune + 60) / 119.0
			: 0.5;
	} else if (slider == &self->pitchfinetune) {
		*value = (self->sample)
			? self->sample->zone.finetune / 200.0 + 0.5f
			: 0.0;
	} else {
		*value = 0.0;
	}
}

void samplesviewgeneral_on_describe(SamplesViewGeneral* self, psy_ui_Slider* slider, char* txt)
{	
	assert(self);

	if (slider == &self->defaultvolume) {		
		psy_snprintf(txt, 10, "C%02X", self->sample 
			? psy_audio_sample_volume(self->sample)
			: 0x80);
	} else if (slider == &self->globalvolume) {		
		if (!self->sample) {
			psy_snprintf(txt, 10, "-inf. dB");
		} else
		if (psy_audio_sample_global_volume(self->sample) == 0.0) {
			psy_snprintf(txt, 10, "-inf. dB");
		} else {			
			psy_snprintf(txt, 10, "%.2f dB", psy_dsp_convert_amp_to_db(
				self->sample->globalvolume));
		}
	} else if (slider == &self->panposition) {		
		samplesviewgeneral_pan_description(self, txt);
	} else if (slider == &self->samplednote) {			
		psy_snprintf(txt, 10, "%s",
			(self->sample)
			? psy_dsp_notetostr((psy_dsp_note_t)(self->sample->zone.tune + 60),
				self->notes_tab_mode)
			: psy_dsp_notetostr(60, self->notes_tab_mode));
	} else if (slider == &self->pitchfinetune) {
		psy_snprintf(txt, 10, "%d ct.",
			(self->sample)
			? self->sample->zone.finetune
			: 0);
	}
}

void samplesviewgeneral_pan_description(SamplesViewGeneral* self, char* rv)
{	
	assert(self);

	if (!self->sample) {
		psy_snprintf(rv, 10, "|64|");		
	} else if (psy_audio_sample_surround(self->sample)) {
		psy_snprintf(rv, 10, "SurrounD");
	} else {		
		intptr_t pos;
		
		pos = map_1_128(psy_audio_sample_panning(self->sample));
		if (pos == 0) {
			psy_snprintf(rv, 128, "||%02d  ", pos);
		} else if (pos < 32) {
			psy_snprintf(rv, 128, "<<%02d  ", pos);
		} else if (pos < 64) {
			psy_snprintf(rv, 128, " <%02d< ", pos);
		} else  if (pos == 64) {
			psy_snprintf(rv, 128, " |%02d| ", pos);
		} else if (pos <= 96) {
			psy_snprintf(rv, 128, " >%02d> ", pos);
		} else if (pos < 128) {
			psy_snprintf(rv, 128, "  %02d>>", pos);
		} else {
			psy_snprintf(rv, 128, "  %02d||", pos);
		}
	}	
}

void samplesviewgeneral_on_note_tab_mode(SamplesViewGeneral* self, psy_Property* sender)
{
	if (psy_property_item_bool(sender)) { /* is A440? */
		self->notes_tab_mode = psy_dsp_NOTESTAB_A440;
	} else {
		self->notes_tab_mode = psy_dsp_NOTESTAB_A220;
	}
	psy_ui_slider_update(&self->samplednote);
}
