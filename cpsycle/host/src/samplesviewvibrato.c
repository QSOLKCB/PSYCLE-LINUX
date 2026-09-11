/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesviewvibrato.h"
/* host */
#include "styles.h"
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"

/* prototypes */
static void vibratoview_ondescribe(SamplesViewVibrato*, psy_ui_Slider*, char* txt);
static void vibratoview_ontweak(SamplesViewVibrato*, psy_ui_Slider*, double value);
static void vibratoview_onvalue(SamplesViewVibrato*, psy_ui_Slider*, double* value);
static void vibratoview_onwaveformchange(SamplesViewVibrato*, psy_ui_ComboBox* sender);
static psy_audio_WaveForms vibratoview_comboboxtowaveform(int combobox_index);
static int vibratoview_waveformtocombobox(psy_audio_WaveForms waveform);

/* implementation */
void samplesviewvibrato_init(SamplesViewVibrato* self, psy_ui_Component* parent, psy_audio_Player* player)
{	
	uintptr_t i;
	psy_ui_Slider* sliders[] = {
		&self->attack,
		&self->speed,
		&self->depth,		
	};

	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));
	self->sample = 0;
	self->player = player;
	/* header */
	{
		psy_ui_Margin header_margin;

		psy_ui_margin_init_em(&header_margin, 0.0, 2.0, 0.0, 0.0);
		psy_ui_component_init(&self->header, &self->component, NULL);		
		psy_ui_component_set_align(&self->header, psy_ui_ALIGN_TOP);		
		psy_ui_label_init(&self->waveformheaderlabel, &self->header);
		psy_ui_label_set_text(&self->waveformheaderlabel, "Waveform");
		psy_ui_component_set_align(&self->waveformheaderlabel.component,
			psy_ui_ALIGN_LEFT);
		psy_ui_component_set_margin(&self->waveformheaderlabel.component,
			header_margin);
		psy_ui_combobox_init(&self->waveformbox, &self->header);
		psy_ui_combobox_set_char_number(&self->waveformbox, 15);
		psy_ui_combobox_add_text(&self->waveformbox, "Sinus");
		psy_ui_combobox_add_text(&self->waveformbox, "Square");
		psy_ui_combobox_add_text(&self->waveformbox, "RampUp");
		psy_ui_combobox_add_text(&self->waveformbox, "RampDown");
		psy_ui_combobox_add_text(&self->waveformbox, "Random");
		psy_ui_component_set_align(&self->waveformbox.component,
			psy_ui_ALIGN_LEFT);
		psy_ui_combobox_select(&self->waveformbox, 0);
		psy_signal_connect(&self->waveformbox.signal_selchanged, self,
			vibratoview_onwaveformchange);
	}
	psy_ui_slider_init(&self->attack, &self->component);
	psy_ui_slider_set_text(&self->attack, "Attack");	
	psy_ui_slider_init(&self->speed, &self->component);
	psy_ui_slider_set_text(&self->speed,"Speed");
	psy_ui_slider_init(&self->depth, &self->component);
	psy_ui_slider_set_text(&self->depth, "Depth");	
	for (i = 0; i < 3; ++i) {		
		psy_ui_slider_set_char_number(sliders[i], 16);
		psy_ui_slider_set_default_value(sliders[i], 0.0);
		psy_ui_slider_connect(sliders[i], self,
			(ui_slider_fpdescribe)vibratoview_ondescribe,
			(ui_slider_fptweak)vibratoview_ontweak,
			(ui_slider_fpvalue)vibratoview_onvalue);
	}	
}

void samplesviewvibrato_set_sample(SamplesViewVibrato* self, psy_audio_Sample* sample)
{
	assert(self);

	self->sample = sample;
	if (self->sample) {
		psy_ui_component_enable_input(&self->component, 1);
		psy_ui_combobox_select(&self->waveformbox,
			vibratoview_waveformtocombobox(self->sample->vibrato.type));
	} else {
		psy_ui_component_prevent_input(&self->component, 1);
		psy_ui_combobox_select(&self->waveformbox,
			vibratoview_waveformtocombobox(psy_audio_WAVEFORMS_SINUS));
	}
}

void vibratoview_ontweak(SamplesViewVibrato* self, psy_ui_Slider* slidergroup, double value)
{
	if (!self->sample) {
		return;
	}
	if (slidergroup == &self->attack) {
		self->sample->vibrato.attack = (unsigned char)(value * 255.f);
	} else
	if (slidergroup == &self->speed) {
		self->sample->vibrato.speed = (unsigned char)(value * 64.f);
	} else
	if (slidergroup == &self->depth) {
		self->sample->vibrato.depth = (unsigned char)(value * 32.f);
	}
}

void vibratoview_onvalue(SamplesViewVibrato* self, psy_ui_Slider* slidergroup,
	double* value)
{	
	if (slidergroup == &self->attack) {
		*value = self->sample ? self->sample->vibrato.attack / 255.f : 0.f;
	} else 
	if (slidergroup == &self->speed) {
		*value = self->sample ? self->sample->vibrato.speed / 64.f : 0.f;
	} else
	if (slidergroup == &self->depth) {
		*value = self->sample ? self->sample->vibrato.depth / 32.f : 0.f;
	}
}

void vibratoview_ondescribe(SamplesViewVibrato* self, psy_ui_Slider* slidergroup, char* txt)
{		
	if (slidergroup == &self->attack) {		
		if (!self->sample) {
			psy_snprintf(txt, 10, "No Delay");
		} else 
		if (self->sample->vibrato.attack == 0) {
			psy_snprintf(txt, 10, "No Delay");
		} else {
			psy_snprintf(txt, 10, "%.0fms", (4096000.0f*256)
				/(self->sample->vibrato.attack*44100.f));
		}		
	} else
	if (slidergroup == &self->speed) {		
		if (!self->sample) {
			psy_snprintf(txt, 10, "off");
		} else
		if (self->sample->vibrato.speed == 0) {
			psy_snprintf(txt, 10, "off");			
		} else {		
			psy_snprintf(txt, 10, "%.0fms", (256000.0f*256) 
				/ (self->sample->vibrato.speed*44100.f));
		}
	} else
	if (slidergroup == &self->depth) {		
		if (!self->sample) {
			psy_snprintf(txt, 10, "off");
		} else
		if (self->sample->vibrato.depth == 0) {
			psy_snprintf(txt, 10, "off");			
		} else {
			psy_snprintf(txt, 10, "%d", self->sample->vibrato.depth);
		}
	}
}

void vibratoview_onwaveformchange(SamplesViewVibrato* self,
	psy_ui_ComboBox* sender)
{
	if (self->sample) {
		self->sample->vibrato.type = vibratoview_comboboxtowaveform(
			(int)psy_ui_combobox_cur_sel(sender));
	}
}

int vibratoview_waveformtocombobox(psy_audio_WaveForms waveform)
{
	int rv = 0;

	switch (waveform) {
		case psy_audio_WAVEFORMS_SINUS: rv = 0; break;		
		case psy_audio_WAVEFORMS_SQUARE: rv = 1; break;					
		case psy_audio_WAVEFORMS_SAWUP: rv = 2; break;					
		case psy_audio_WAVEFORMS_SAWDOWN: rv = 3; break;					
		case psy_audio_WAVEFORMS_RANDOM: rv = 4; break;					
		default:
		break;		
	}
	return rv;
}

psy_audio_WaveForms vibratoview_comboboxtowaveform(int combobox_index)
{
	psy_audio_WaveForms rv = psy_audio_WAVEFORMS_SINUS;			
	
	switch (combobox_index) {					
		case 0: rv = psy_audio_WAVEFORMS_SINUS; break;
		case 1: rv = psy_audio_WAVEFORMS_SQUARE; break;
		case 2: rv = psy_audio_WAVEFORMS_SAWUP; break;
		case 3: rv = psy_audio_WAVEFORMS_SAWDOWN; break;
		case 4: rv = psy_audio_WAVEFORMS_RANDOM; break;
		default:
		break;				
	}
	return rv;
}
