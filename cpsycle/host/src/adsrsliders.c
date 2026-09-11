/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "adsrsliders.h"
/* ui */
#include <uiapp.h>
/* dsp */
#include <valuemapper.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void adsrsliders_on_destroyed(AdsrSliders*);
static void adsrsliders_on_volume_view_describe(AdsrSliders*,
	psy_ui_Slider*, char* txt);
static void adsrsliders_on_volume_view_tweak(AdsrSliders*,
	psy_ui_Slider*, double value);
static void adsrsliders_on_volume_view_value(AdsrSliders*,
	psy_ui_Slider*, double* value);
static void adsrsliders_output_tweak(AdsrSliders*, uintptr_t point_index);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(AdsrSliders* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			adsrsliders_on_destroyed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void adsrsliders_init(AdsrSliders* self, psy_ui_Component* parent)
{	
	uintptr_t i;
	
	psy_ui_Slider* sliders[] = {
		&self->attack,
		&self->decay,
		&self->sustain,
		&self->release,
		NULL		
	};
	
	assert(self);
	
	self->envelope = NULL;	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));	
	psy_ui_slider_init(&self->attack, &self->component);
	psy_ui_slider_set_text(&self->attack, "instview.attack");
	psy_ui_slider_init(&self->decay, &self->component);
	psy_ui_slider_set_text(&self->decay, "instview.decay");
	psy_ui_slider_init(&self->sustain, &self->component);
	psy_ui_slider_set_text(&self->sustain, "instview.sustain-level");
	psy_ui_slider_set_default_value(&self->sustain, 1.0);
	psy_ui_slider_init(&self->release, &self->component);
	psy_ui_slider_set_text(&self->release, "instview.release");
	psy_ui_slider_set_default_value(&self->attack, 0.005 /
		adsrsliders_max_time(self));
	psy_ui_slider_set_default_value(&self->decay, 0.005 /
		adsrsliders_max_time(self));
	psy_ui_slider_set_default_value(&self->sustain, 1.0);
	psy_ui_slider_set_default_value(&self->release, 0.005 /
		adsrsliders_max_time(self));
	for (i = 0; sliders[i] != NULL; ++i) {				
		psy_ui_slider_set_char_number(sliders[i], 21);
		psy_ui_slider_set_value_char_number(sliders[i], 15);
		psy_ui_slider_connect(sliders[i], self,
			(ui_slider_fpdescribe)adsrsliders_on_volume_view_describe,
			(ui_slider_fptweak)adsrsliders_on_volume_view_tweak,
			(ui_slider_fpvalue)adsrsliders_on_volume_view_value);
	}
	psy_signal_init(&self->signal_tweaked);
}

void adsrsliders_on_destroyed(AdsrSliders* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_tweaked);
}

void adsrsliders_set_envelope(AdsrSliders* self,
	psy_dsp_Envelope* envelope)
{	
	assert(self);
	
	self->envelope = envelope;
	adsrsliders_update(self);
}

psy_dsp_seconds_t adsrsliders_max_time(const AdsrSliders* self)
{
	assert(self);
	
	if (self->envelope && psy_dsp_envelope_in_ticks(self->envelope)) {
		return 300.0;	
	}
	return 5.0;
}

void adsrsliders_update(AdsrSliders* self)
{
	assert(self);
	
	psy_ui_slider_update(&self->attack);
	psy_ui_slider_update(&self->decay);
	psy_ui_slider_update(&self->sustain);
	psy_ui_slider_update(&self->release);
}

void adsrsliders_on_volume_view_describe(AdsrSliders* self,
	psy_ui_Slider* slider, char* txt)
{
	assert(self);
	
	if (slider == &self->attack) {
		if (self->envelope) {			
			if (psy_dsp_envelope_in_ticks(self->envelope)) {
				psy_snprintf(txt, 20, "%dt", (int)
					psy_dsp_envelope_attacktime(self->envelope));
			} else {
				psy_snprintf(txt, 20, "%.4fms",
					psy_dsp_envelope_attacktime(self->envelope) * 1000.0);
			}
		} else {			
			psy_snprintf(txt, 10, "0ms");
		}		
	} else if (slider == &self->decay) {
		if (self->envelope) {
			if (psy_dsp_envelope_in_ticks(self->envelope)) {
				psy_snprintf(txt, 20, "%dt", (int)
					psy_dsp_envelope_decay_time(self->envelope));
			} else {
				psy_snprintf(txt, 20, "%.4fms",
					psy_dsp_envelope_decay_time(self->envelope) * 1000.0);
			}
		} else {			
			psy_snprintf(txt, 10, "0ms");
		}		
	} else if (slider == &self->sustain) {
		if (self->envelope) {
			psy_snprintf(txt, 20, "%d%%", (int)(
				psy_dsp_envelope_sustain_value(self->envelope) * 100));	
		} else {			
			psy_snprintf(txt, 10, "0%%");
		}		
	} else
	if (slider == &self->release) {
		if (self->envelope) {
			if (psy_dsp_envelope_in_ticks(self->envelope)) {
				psy_snprintf(txt, 20, "%dt", (int)
					psy_dsp_envelope_release_time(self->envelope));
			} else {
				psy_snprintf(txt, 20, "%.4fms",
					psy_dsp_envelope_release_time(self->envelope) * 1000.0);
			}
		} else {			
			psy_snprintf(txt, 10, "0ms");
		}		
	}
}

void adsrsliders_on_volume_view_tweak(AdsrSliders* self,
	psy_ui_Slider* slider, double value)
{
	assert(self);
	
	if (!self->envelope) {
		return;
	}
	if (slider == &self->attack) {
		psy_dsp_envelope_set_attack_time(self->envelope,
			(psy_dsp_seconds_t)(value * adsrsliders_max_time(self)));
		adsrsliders_output_tweak(self, 1);
		psy_signal_emit(&self->signal_tweaked, self, 1, 1);
	} else if (slider == &self->decay) {
		psy_dsp_envelope_set_decay_time(self->envelope,
			((psy_dsp_seconds_t)(value * adsrsliders_max_time(self))));
		adsrsliders_output_tweak(self, 2);
		psy_signal_emit(&self->signal_tweaked, self, 1, 2);
	} else if (slider == &self->sustain) {
		psy_dsp_envelope_set_sustain_value(self->envelope, value);
		adsrsliders_output_tweak(self, 2);
		psy_signal_emit(&self->signal_tweaked, self, 1, 2);
	} else if (slider == &self->release) {
		psy_dsp_envelope_set_release_time(self->envelope,
			(psy_dsp_seconds_t)(value * adsrsliders_max_time(self)));
		adsrsliders_output_tweak(self, 3);
		psy_signal_emit(&self->signal_tweaked, self, 1, 3);
	}	
}

void adsrsliders_on_volume_view_value(AdsrSliders* self,
	psy_ui_Slider* slider, double* value)
{
	assert(self);
	
	if (slider == &self->attack) {
		if (self->envelope) {
			*value = (psy_dsp_seconds_t)
				(psy_dsp_envelope_attacktime(self->envelope) /
					adsrsliders_max_time(self));
		} else {
			*value = 0.f;
		}
	} else if (slider == &self->decay) {
		if (self->envelope) {
			*value = (psy_dsp_seconds_t)
				(psy_dsp_envelope_decay_time(self->envelope) /
					adsrsliders_max_time(self));
		} else {
			*value = 0.f;
		}	
	} else if (slider == &self->sustain) {
		if (self->envelope) {
			*value = psy_dsp_envelope_sustain_value(self->envelope);
		} else {
			*value = 0.5f;
		}	
	} else if (slider == &self->release) {
		if (self->envelope) {
			*value = (psy_dsp_seconds_t)
				(psy_dsp_envelope_release_time(self->envelope) /
					adsrsliders_max_time(self));
		} else {
			*value = 0.5f;
		}	
	}
}

void adsrsliders_output_tweak(AdsrSliders* self, uintptr_t point_index)
{
	char status[256];
	psy_dsp_EnvelopePoint pt;
	psy_Logger* logger;

	assert(self);
	
	if (!self->envelope) {
		return;
	}
	pt = psy_dsp_envelope_at(self->envelope, point_index);
	psy_snprintf(status, 256, "Point %d (%.4f, %.4f)", (int)point_index,
		(float)pt.time, (float)pt.value);
	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {
		psy_logger_output(logger, status);		
	}
}	
