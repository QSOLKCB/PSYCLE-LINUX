/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


/* host */
#include "paramtweak.h"
/* audio */
#include <machine.h>
#include <plugin_interface.h>
/* ui */
#include <uievents.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static double paramtweak_norm_value(const ParamTweak*);
static bool paramtweak_is_switch_or_check(const ParamTweak*);
static intptr_t paramtweak_type(const ParamTweak*);
static bool paramtweak_check_fine_tweak(const ParamTweak*, psy_ui_MouseEvent*);
static double paramtweak_scale(const ParamTweak*, bool fine_tweak);
static double paramtweak_range(const ParamTweak*);
static void paramtweak_tweak(ParamTweak*, double val);
static bool paramtweak_update_param(ParamTweak*);

/* implementation */
void paramtweak_init(ParamTweak* self)
{
	assert(self);

	self->machine_ = NULL;
	self->param_idx_ = psy_INDEX_INVALID;
	self->param_ = NULL;	
	self->active_ = FALSE;
	self->tweak_scale_ = 200.0;
}

void paramtweak_begin(ParamTweak* self, psy_audio_Machine* machine_,
	uintptr_t paramindex, psy_audio_MachineParam* param_)
{
	assert(self);

	self->machine_ = machine_;
	self->param_idx_ = paramindex;
	self->param_ = param_;
	self->active_ = TRUE;
	paramtweak_update_param(self);
}

void paramtweak_end(ParamTweak* self)
{
	assert(self);

	self->machine_ = NULL;	
	self->param_idx_ = psy_INDEX_INVALID;
	self->param_ = NULL;
	self->active_ = FALSE;
}

void paramtweak_on_mouse_down(ParamTweak* self, psy_ui_MouseEvent* ev)
{		
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 1 && paramtweak_update_param(self)) {			
		self->tweak_base_ = (double)psy_ui_mouseevent_pt(ev).y;
		self->tweak_val_ = paramtweak_norm_value(self);
		if (paramtweak_is_switch_or_check(self)) {
			if (self->tweak_val_ == 0.0) {
				paramtweak_tweak(self, 1.0);					
			} else {
				paramtweak_tweak(self, 0.0);					
			}
		}
	}	
}

double paramtweak_norm_value(const ParamTweak* self)
{
	assert(self);

	if (self->machine_) {
		return psy_audio_machine_parameter_norm_value(
			self->machine_, self->param_);
	}
	if (self->param_) {
		return psy_audio_machineparam_norm_value(self->param_);
	}
	return 0.0;
}


void paramtweak_on_mouse_move(ParamTweak* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (self->active_ && paramtweak_update_param(self) &&
			!paramtweak_is_switch_or_check(self)) {
		paramtweak_tweak(self, self->tweak_val_ +
			(self->tweak_base_ - psy_ui_mouseevent_pt(ev).y) /
			paramtweak_scale(self, paramtweak_check_fine_tweak(self, ev)));
		psy_ui_mouseevent_stop_propagation(ev);
	}	
}

double paramtweak_scale(const ParamTweak* self, bool fine_tweak)
{
	assert(self);

	if (fine_tweak) {
		return self->tweak_scale_ * 1000.0 * (paramtweak_range(self) / 65536.0);
	}
	return self->tweak_scale_;
}


bool paramtweak_is_switch_or_check(const ParamTweak* self)
{
	assert(self);

	return (paramtweak_type(self) == MPF_CHECK || paramtweak_type(self) ==
		MPF_SWITCH);
}

intptr_t paramtweak_type(const ParamTweak* self)
{
	assert(self);

	if (self->machine_) {	
		return (psy_audio_machine_parameter_type(self->machine_,
			self->param_) & 0xFF);
	}
	if (self->param_) {		
		return (psy_audio_machineparam_type(
			self->param_) & 0xFF);
	}
	return MPF_STATE;
}

double paramtweak_range(const ParamTweak* self)
{
	assert(self);

	if (self->machine_) {
		intptr_t minval, maxval;

		psy_audio_machine_parameter_range(self->machine_,
			self->param_, &minval, &maxval);
		return (double)(maxval - minval + 1);
	}
	if (self->param_) {
		intptr_t minval, maxval;

		psy_audio_machineparam_range(self->param_, &minval, &maxval);
		return (double)(maxval - minval + 1);
	}
	return (double)0x10000; /* best guess */
}

bool paramtweak_check_fine_tweak(const ParamTweak* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	/*
	** shift and ctrl are both fine tweak qualifiers in the VST
	** world (depending on manufacturer), so it's most intuitive
	** when both are supported
	*/
	return (psy_ui_mouseevent_shift_key(ev) ||
		psy_ui_mouseevent_ctrl_key(ev));
}

void paramtweak_tweak(ParamTweak* self, double val)
{
	assert(self);

	if (val > 1.0) {
		val = 1.0;
	} else if (val < 0.0) {
		val = 0.0;
	}
	if (self->machine_) {
		psy_audio_machine_parameter_tweak(self->machine_, self->param_, val);
	} else if (self->param_) {
		psy_audio_machineparam_tweak(self->param_, val);
	}
}

bool paramtweak_update_param(ParamTweak* self)
{
	assert(self);

	if (self->machine_ && self->param_idx_ != psy_INDEX_INVALID) {
		self->param_ = psy_audio_machine_parameter(self->machine_,
			self->param_idx_);
	}
	return (self->param_ != NULL);
}
