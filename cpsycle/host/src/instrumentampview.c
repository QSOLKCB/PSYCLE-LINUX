/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "instrumentampview.h"
/* dsp */
#include <valuemapper.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void instrumentvolumeview_update_sliders(InstrumentVolumeView*);
static void instrumentvolumeview_on_describe(InstrumentVolumeView*,
	psy_ui_Slider* sender, char* text);
static void instrumentvolumeview_on_tweak(InstrumentVolumeView*,
	psy_ui_Slider* sender, double value);
static void instrumentvolumeview_on_value(InstrumentVolumeView*,
	psy_ui_Slider* sender, double* value);
static void instrumentvolumeview_on_tweaked(InstrumentVolumeView*,
	psy_ui_Component*, uintptr_t point_index);
static void instrumentvolumeview_on_envelope_view_tweaked(InstrumentVolumeView*,
	psy_ui_Component*, uintptr_t point_index);

/* implementation */
void instrumentvolumeview_init(InstrumentVolumeView* self, 
	psy_ui_Component* parent, psy_audio_Instruments* instruments)
{		
	int i;
	psy_ui_Slider* sliders[] = {		
		&self->randomvolume,
		&self->volume_fade_speed
	};	

	assert(self);

	self->instruments = instruments;	
	self->instrument = 0;
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));	
	psy_ui_slider_init_text(&self->randomvolume, &self->component,
		"instview.swing");
	psy_ui_slider_set_default_value(&self->randomvolume, 0.0);	
	psy_ui_slider_init_text(&self->volume_fade_speed, &self->component,
		"instview.fadeout");
	psy_ui_slider_set_default_value(&self->volume_fade_speed, 0.0);
	envelopeview_init(&self->envelopeview, &self->component);
	envelopeview_settext(&self->envelopeview,
		"instview.amplitude-envelope");	
	psy_ui_component_set_align(&self->envelopeview.component,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_margin(&self->envelopeview.component,
		psy_ui_margin_make_em(1.0, 0.0, 1.0, 0.0));
	adsrsliders_init(&self->adsrsliders, &self->component);		
	psy_ui_component_set_align(&self->adsrsliders.component,
		psy_ui_ALIGN_BOTTOM);
	psy_signal_connect(&self->adsrsliders.signal_tweaked, self,
		instrumentvolumeview_on_tweaked);
	psy_signal_connect(&self->envelopeview.signal_tweaked, self,
		instrumentvolumeview_on_envelope_view_tweaked);
	for (i = 0; i < 2; ++i) {				
		psy_ui_slider_set_char_number(sliders[i], 21);
		psy_ui_slider_set_value_char_number(sliders[i], 15);
		psy_ui_slider_connect(sliders[i], self,
			(ui_slider_fpdescribe)instrumentvolumeview_on_describe,
			(ui_slider_fptweak)instrumentvolumeview_on_tweak,
			(ui_slider_fpvalue)instrumentvolumeview_on_value);
	}
	instrumentvolumeview_update_sliders(self);
}

void instrumentvolumeview_set_instrument(InstrumentVolumeView* self,
	psy_audio_Instrument* instrument)
{	
	assert(self);

	self->instrument = instrument;
	if (self->instrument) {
		adsrsliders_set_envelope(&self->adsrsliders,			
			&self->instrument->volumeenvelope);
		envelopeview_setenvelope(&self->envelopeview,
			&instrument->volumeenvelope);
	} else {
		adsrsliders_set_envelope(&self->adsrsliders,	NULL);
		envelopeview_setenvelope(&self->envelopeview, NULL);
	}
	instrumentvolumeview_update_sliders(self);	
}

void instrumentvolumeview_update_sliders(InstrumentVolumeView* self)
{	
	assert(self);
	
	psy_ui_slider_update(&self->randomvolume);
	psy_ui_slider_update(&self->volume_fade_speed);
}

void instrumentvolumeview_on_describe(InstrumentVolumeView* self,
	psy_ui_Slider* sender, char* text)
{	
	assert(self);
	
	if (sender == &self->randomvolume) {
		if (self->instrument &&
				psy_audio_instrument_randomvolume_enabled(self->instrument)) {
			psy_snprintf(text, 20, "%d%%", (int)
				(psy_audio_instrument_random_volume(self->instrument) * 100));
		} else {
			psy_snprintf(text, 20, "off");
		}		
	} else if (sender == &self->volume_fade_speed) {
		if (self->instrument && self->instrument->volume_fade_speed != 0.f) {			
			psy_snprintf(text, 20, "%d%%", (int)(
				psy_audio_instrument_volume_fade_speed(self->instrument) *
					100));
		} else {			
			psy_snprintf(text, 10, "off");
		}
	}	
}

void instrumentvolumeview_on_tweak(InstrumentVolumeView* self,
	psy_ui_Slider* sender, double value)
{
	assert(self);

	if (!self->instrument) {
		return;
	}
	if (sender == &self->randomvolume) {
		psy_audio_instrument_set_random_volume(self->instrument, value);
	} else if (sender == &self->volume_fade_speed) {
		psy_audio_instrument_set_volume_fade_speed(self->instrument, value);
	}
	envelopeview_update(&self->envelopeview);
}

void instrumentvolumeview_on_value(InstrumentVolumeView* self,
	psy_ui_Slider* sender, double* value)
{
	assert(self);

	if (sender == &self->randomvolume) {
		*value = (self->instrument)
			? psy_audio_instrument_random_volume(self->instrument)
			: 0.0f;
	} else if (sender == &self->volume_fade_speed) {
		*value = (self->instrument)
			? psy_audio_instrument_volume_fade_speed(self->instrument)
			: 0.0f;
	}
}

void instrumentvolumeview_on_tweaked(InstrumentVolumeView* self,
	psy_ui_Component* sender, uintptr_t point_index)
{
	assert(self);
	
	if (self->instrument) {
		envelopeview_update(&self->envelopeview);		
	}
}

void instrumentvolumeview_on_envelope_view_tweaked(InstrumentVolumeView* self,
	psy_ui_Component* sender, uintptr_t point_index)
{
	assert(self);
	
	if (self->instrument) {
		adsrsliders_update(&self->adsrsliders);		
	}
}
