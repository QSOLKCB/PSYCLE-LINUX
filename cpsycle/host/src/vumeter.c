/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "vumeter.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* dsp */
#include <operations.h>
#include <rms.h>
/* std */
#include <stdio.h>
#include <math.h>


/* prototypes */
static void vumeter_on_draw(Vumeter*, psy_ui_Graphics*);
static void vumeter_draw_rect(Vumeter*, psy_ui_Graphics*,
	double x, double y, double width, double height, psy_ui_Colour);
static double vumeter_scale(Vumeter*, double val, double old, double width);
static void vumeter_on_size(Vumeter*);

/* vtable */
static psy_ui_ComponentVtable vumeter_vtable;
static bool vumeter_vtable_initialized = FALSE;

static void vumeter_vtable_init(Vumeter* self)
{
	assert(self);
	
	if (!vumeter_vtable_initialized) {
		vumeter_vtable = *(self->component.vtable);
		vumeter_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			vumeter_on_draw;
		vumeter_vtable.onsize =
			(psy_ui_fp_component)
			vumeter_on_size;
		vumeter_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vumeter_vtable);
}

/* implementation */
void vumeter_init(Vumeter* self, psy_ui_Component* parent)
{					
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vumeter_vtable_init(self);
	self->l_max_ = self->r_max_ = 0.0;
	self->l_rms_ = self->r_rms_ = 0.0;
	self->l_log_ = self->r_log_ = 0.0;
	self->vu_prev_l_ = self->vu_prev_r_ = 0.0;	
	self->machine_ = NULL;	
	self->channel_size_ = psy_ui_realsize_zero();
	psy_ui_component_set_aligner(&self->component, NULL);
	psy_ui_component_set_style_type(&self->component, STYLE_MAIN_VU);
	vumeter_update_style(self);	
}

void vumeter_on_draw(Vumeter* self, psy_ui_Graphics* g)
{
	double half;	
		
	assert(self);
			
	if (!self->machine_) {
		return;
	}
	
	half = self->channel_size_.height;	
	if ((self->l_log_ > 0.0) || (self->vu_prev_l_ > 0.0)) {
		vumeter_draw_rect(self, g, 0.0, 0.0, self->l_log_, half, self->vu_);
		if (self->vu_prev_l_ > self->l_log_) {			
			vumeter_draw_rect(self, g, self->l_log_, 0.0, self->vu_prev_l_ -
				self->l_log_, half, self->peak_);
		}
	}
	if ((self->r_log_ > 0.0) || (self->vu_prev_r_ > 0.0)) {
		vumeter_draw_rect(self, g, 0.0, half + 1.0, self->r_log_, half,
			self->vu_);
		if (self->vu_prev_r_ > self->r_log_) {
			vumeter_draw_rect(self, g, self->r_log_, half + 1.0,
				self->vu_prev_r_ - self->r_log_, half, self->peak_);			
		}
	}
}

void vumeter_draw_rect(Vumeter* self, psy_ui_Graphics* g, double x, double y,
	double width, double height, psy_ui_Colour colour)
{
	psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(x, y), psy_ui_realsize_make(width, height)),
		colour);
}

void vumeter_idle(Vumeter* self)
{		
	psy_audio_Buffer* memory;
		
	assert(self);
	
	if (!self->machine_) {
		return;
	}			
	memory = psy_audio_machine_buffermemory(self->machine_);
	if (memory && memory->rms) {
		double range;
		double l_rms;
		double r_rms;		
		double l_max;
		double r_max;		
		
		range = psy_audio_buffer_range_factor(memory, PSY_DSP_AMP_RANGE_VST);
		l_rms = memory->rms->data.previousLeft * range;
		r_rms = memory->rms->data.previousRight * range;		
		l_max = dsp.maxvol(memory->samples[0], memory->numsamples) * range;
		r_max = dsp.maxvol(memory->samples[1], memory->numsamples) * range;
		/* 
		** Auto decrease effect for the Master peak vu-meters (rms one is
		** always showing the current value)
		*/
		self->l_max_ = psy_max(self->l_max_ * 0.707, l_max);
		self->r_max_ = psy_max(self->r_max_ * 0.707, r_max);
		// if ((l_rms != self->l_rms_) || (r_rms != self->r_rms_) || 
		//	 (self->l_log_ != 0.0) || (self->r_log_ != 0.0)) {
			/* rms */
			self->l_rms_ = l_rms;
			self->r_rms_ = r_rms;			
			self->l_log_ = vumeter_scale(self, 5.0 * log10(1.0 + l_rms),
				self->l_log_, self->channel_size_.width);
			self->r_log_ = vumeter_scale(self, 5.0 * log10(1.0 + r_rms),
				self->r_log_, self->channel_size_.width);
			/* peak */			
			self->vu_prev_l_ = 5.0 * log10(1.0 + self->l_max_)
				* self->channel_size_.width;
			self->vu_prev_r_ = 5.0 * log10(1.0 + self->r_max_)
				* self->channel_size_.width;
#ifndef PSYCLE_DEBUG_PREVENT_TIMER_DRAW
			psy_ui_component_invalidate(&self->component);
#endif
		//}
	}
}

double vumeter_scale(Vumeter* self, double val, double old, double width)
{			
	/* clip values */
	val *= width;
	if (val > width) {
		return width;
	}
	if (val > 0.0) {
		return val;
	} else if (old > 2.0) {
		/* decrease values */
		return old - 2.0;
	}
	return 0.0;	
}

void vumeter_set_machine(Vumeter* self, psy_audio_Machine* machine)
{
	assert(self);
		
	self->machine_ = machine;
}

void vumeter_on_size(Vumeter* self)
{
	psy_ui_RealSize size;
	
	assert(self);
	
	size = psy_ui_component_scroll_size_px(&self->component);	
	self->channel_size_ = psy_ui_realsize_make(size.width,
		psy_max(1.0, size.height / 2.0 - 2.0));
}

void vumeter_update_style(Vumeter* self)
{
	assert(self);
			
	self->vu_ = psy_ui_style_colour(psy_ui_style(STYLE_MAIN_VU));
	self->peak_ = psy_ui_style_background_colour(psy_ui_style(
		STYLE_CLIPBOX_SELECT));
}
