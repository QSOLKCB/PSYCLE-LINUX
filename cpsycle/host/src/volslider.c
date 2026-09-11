/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "volslider.h"
/* audio */
#include <convert.h>
#include <machine.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void volslider_on_describe(VolSlider*, psy_ui_Slider*, char* text);
static void volslider_on_tweak(VolSlider*, psy_ui_Slider*, double value);
static void volslider_on_value(VolSlider*, psy_ui_Slider*, double* value);
static psy_audio_MachineParam* volslider_param(VolSlider*);

/* implementation */
void volslider_init(VolSlider* self, psy_ui_Component* parent)
{	
	assert(self);	
				
	psy_ui_slider_init(&self->slider, parent);
	self->slider.component.dbg = 10;
	self->machine_ = NULL;
	self->volume_param_idx_ = 0;
	psy_ui_component_hide(&self->slider.desc.component);
	psy_ui_slider_set_value_char_number(&self->slider, 10.0);
	psy_ui_component_set_margin(&self->slider.pane.component,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));		
	psy_ui_slider_start_poll(&self->slider);
	psy_ui_slider_set_wheel_step(&self->slider, 0.02);
	psy_ui_slider_connect(&self->slider, self,
		(ui_slider_fpdescribe)volslider_on_describe,
		(ui_slider_fptweak)volslider_on_tweak,
		(ui_slider_fpvalue)volslider_on_value);
}

void volslider_on_describe(VolSlider* self, psy_ui_Slider* sender, char* text)
{
	assert(self);

	if (self->machine_) {
		psy_audio_MachineParam* param;

		param = volslider_param(self);
		if (param) {
			double normval;
			double vol;

			normval = psy_audio_machineparam_norm_value(param);
			vol = (normval * 2.0) * (normval * 2.0);
			psy_snprintf(text, 10, "%.2f dB", psy_dsp_convert_amp_to_db(vol));
		}
	}
}

void volslider_on_tweak(VolSlider* self, psy_ui_Slider* sender, double value)
{
	assert(self);
	
	if (self->machine_) {
		psy_audio_MachineParam* param;

		param = volslider_param(self);
		if (param) {
			psy_audio_machineparam_tweak(param, value);
		}
	}
}

void volslider_on_value(VolSlider* self, psy_ui_Slider* sender, double* rv)
{	
	assert(self);

	if (self->machine_) {
		psy_audio_MachineParam* param;

		param = volslider_param(self);
		if (param) {
			*rv = psy_audio_machineparam_norm_value(param);
			return;
		}
	}
	*rv = 0.0;
}

psy_audio_MachineParam* volslider_param(VolSlider* self)
{
	assert(self);

	return psy_audio_machine_tweak_parameter(self->machine_,
		self->volume_param_idx_);
}
