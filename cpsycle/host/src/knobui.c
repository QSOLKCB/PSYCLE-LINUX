/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "knobui.h"
/* host */
#include "styles.h"
#include "machineparamconfig.h"
/* audio */
#include <machine.h>
#include <plugin_interface.h>
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void knobui_on_destroyed(KnobUi*);
static void knobui_on_draw(KnobUi*, psy_ui_Graphics*);
static void knobui_on_preferred_size(KnobUi*, const psy_ui_Size* limit,
	psy_ui_Size* rv);
static void knobui_update_param(KnobUi*);
static void knobui_on_mouse_down(KnobUi*, psy_ui_MouseEvent*);
static void knobui_on_mouse_up(KnobUi*, psy_ui_MouseEvent*);
static void knobui_on_mouse_move(KnobUi*, psy_ui_MouseEvent*);
static void knobui_on_mouse_wheel(KnobUi*, psy_ui_MouseEvent*);
static void knobui_automate(KnobUi*);

/* vtable */
static psy_ui_ComponentVtable knobui_vtable;
static bool knobui_vtable_initialized = FALSE;

static void knobui_vtable_init(KnobUi* self)
{
	assert(self);

	if (!knobui_vtable_initialized) {
		knobui_vtable = *(self->component.vtable);
		knobui_vtable.on_destroyed =
			(psy_ui_fp_component)
			knobui_on_destroyed;
		knobui_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			knobui_on_draw;
		knobui_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			knobui_on_preferred_size;
		knobui_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			knobui_on_mouse_down;
		knobui_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			knobui_on_mouse_up;
		knobui_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			knobui_on_mouse_move;
		knobui_vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			knobui_on_mouse_wheel;
		knobui_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &knobui_vtable);
}

/* implementation */
void knobui_init(KnobUi* self, psy_ui_Component* parent,	
	psy_audio_Machine* machine, uintptr_t paramidx,
	psy_audio_MachineParam* param)
{
	assert(self);	

	psy_ui_component_init(&self->component, parent, NULL);
	knobui_vtable_init(self);
	psy_signal_init(&self->signal_edit);
	self->param = param;
	self->paramidx = paramidx;
	self->machine = machine;	
	psy_ui_component_set_style_type(&self->component, STYLE_MV_CHECK);
	psy_ui_component_prevent_app_focus_out(&self->component);	
	paramtweak_init(&self->paramtweak);	
}

KnobUi* knobui_alloc(void)
{
	return (KnobUi*)malloc(sizeof(KnobUi));
}

KnobUi* knobui_alloc_init(psy_ui_Component* parent,
	psy_audio_Machine* machine, uintptr_t paramidx,
	psy_audio_MachineParam* param)
{
	KnobUi* rv;

	rv = knobui_alloc();
	if (rv) {
		knobui_init(rv, parent, machine, paramidx, param);
		rv->component.deallocate = TRUE;
	}
	return rv;
}

void knobui_set_param(KnobUi* self,
	struct psy_audio_Machine* machine, uintptr_t paramidx,
	struct psy_audio_MachineParam* param)
{
	self->param = param;
	self->paramidx = paramidx;
	self->machine = machine;
	paramtweak_init(&self->paramtweak);
}

void knobui_on_destroyed(KnobUi* self)
{
	psy_signal_dispose(&self->signal_edit);
}

void knobui_on_draw(KnobUi* self, psy_ui_Graphics* g)
{	
	psy_ui_RealRectangle r_top;
	psy_ui_RealRectangle r_bottom;
	psy_ui_RealSize size;
	psy_ui_Style* top_style;
	psy_ui_Style* bottom_style;
	psy_ui_Style* htop_style;
	psy_ui_Style* hbottom_style;
	psy_ui_Style* knob_style;

	top_style = psy_ui_style(STYLE_MACPARAM_TOP);
	bottom_style = psy_ui_style(STYLE_MACPARAM_BOTTOM);
	htop_style = psy_ui_style(STYLE_MACPARAM_TOP_ACTIVE);
	hbottom_style = psy_ui_style(STYLE_MACPARAM_BOTTOM_ACTIVE);
	knob_style = psy_ui_style(STYLE_MACPARAM_KNOB);
	if (!top_style || !bottom_style) {
		return;
	}
	size = psy_ui_component_size_px(&self->component);
	knobui_update_param(self);	
	r_top = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0.0, 0.0),
		psy_ui_realsize_make(knob_style->background.animation.framesize.width,
			size.height));
	psy_ui_graphics_draw_solid_rectangle(g, r_top, psy_ui_style_background_colour(bottom_style));
	r_top = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(knob_style->background.animation.framesize.width, 0.0),
		psy_ui_realsize_make(size.width -
			knob_style->background.animation.framesize.width, size.height / 2));
	psy_ui_graphics_draw_solid_rectangle(g, r_top, psy_ui_style_background_colour(top_style));
	r_bottom = r_top;
	psy_ui_realrectangle_set_topleft(&r_bottom,
		psy_ui_realpoint_make(
			r_top.left,
			r_top.top + psy_ui_realrectangle_height(&r_top)));
	psy_ui_graphics_draw_solid_rectangle(g, r_bottom, psy_ui_style_background_colour(bottom_style));
	if (self->param) {
		char label[128];
		char str[128];
		
		if (self->machine) {
			if (!psy_audio_machine_parameter_name(self->machine, self->param, label)) {
				if (!psy_audio_machine_parameter_label(self->machine, self->param, label)) {
					psy_snprintf(label, 128, "%s", "");
				}
			}
			if (!psy_audio_machine_parameter_describe(self->machine, self->param, str)) {
				psy_snprintf(str, 128, "%d",
					(int)psy_audio_machineparam_scaled_value(self->param));
			}
		} else {
			if (!psy_audio_machineparam_name(self->param, label)) {
				if (!psy_audio_machineparam_label(self->param, label)) {
					psy_snprintf(label, 128, "%s", "");
				}
			}
			if (!psy_audio_machineparam_describe(self->param, str)) {
				psy_snprintf(str, 128, "%d",
					(int)psy_audio_machineparam_scaled_value(self->param));
			}
		}
		if ((paramtweak_active(&self->paramtweak))) {
			psy_ui_set_background_colour(g, psy_ui_style_background_colour(htop_style));
			psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(htop_style));
		} else {
			psy_ui_set_background_colour(g, psy_ui_style_background_colour(top_style));
			psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(top_style));
		}
		psy_ui_graphics_textout_rectangle(g, psy_ui_realrectangle_topleft(&r_top),
			psy_ui_ETO_OPAQUE, r_top, label, psy_strlen(label));
		psy_ui_set_background_colour(g,
			(paramtweak_active(&self->paramtweak))
			? psy_ui_style_background_colour(hbottom_style)
			: psy_ui_style_background_colour(bottom_style));
		psy_ui_graphics_set_text_colour(g,
			(paramtweak_active(&self->paramtweak))
			? psy_ui_style_colour(hbottom_style)
			: psy_ui_style_colour(bottom_style));
		psy_ui_graphics_textout_rectangle(g, psy_ui_realrectangle_topleft(&r_bottom),
			psy_ui_ETO_OPAQUE, r_bottom, str, psy_strlen(str));
		if (!psy_ui_bitmap_empty(&knob_style->background.bitmap)) {
			intptr_t knob_frame;

			if (self->param) {
				if (self->machine) {
					knob_frame = (intptr_t)(
						(psy_audio_machine_parameter_norm_value(self->machine,
							self->param) * 63.f));
				} else {
					knob_frame = (intptr_t)(
						(psy_audio_machineparam_norm_value(self->param) *
						63.f));
				}
			} else {
				knob_frame = 0;
			}
			if (size.height < knob_style->background.animation.framesize.height) {
				double ratio;
				double w;

				ratio = size.height / (double)knob_style->background.animation.framesize.height;
				w = ratio * knob_style->background.animation.framesize.width;
				psy_ui_graphics_draw_stretched_bitmap(g, &knob_style->background.bitmap,
					psy_ui_realrectangle_make(psy_ui_realpoint_zero(),
						psy_ui_realsize_make(w, size.height)),
					psy_ui_realpoint_make(knob_frame *
						knob_style->background.animation.framesize.height, 0.0),
					knob_style->background.animation.framesize);
			} else {
				psy_ui_graphics_draw_bitmap(g, &knob_style->background.bitmap,
					psy_ui_realrectangle_make(
						psy_ui_realpoint_zero(),
						knob_style->background.animation.framesize),
						psy_ui_realpoint_make(knob_frame *
						knob_style->background.animation.framesize.width, 0));
			}
		}
	}
	psy_ui_graphics_set_colour(g, psy_ui_colour_make(0x00232323));
	psy_ui_drawline(g,
		psy_ui_realpoint_make(0, size.height - 1),
		psy_ui_realpoint_make(size.width, size.height - 1));
	psy_ui_drawline(g,
		psy_ui_realpoint_make(size.width - 1, 0),
		psy_ui_realpoint_make(size.width - 1, size.height - 1));
}

void knobui_on_preferred_size(KnobUi* self, const psy_ui_Size* limit,
	psy_ui_Size* rv)
{		
	knobui_update_param(self);
	if (self->param) {
		if (psy_audio_machineparam_type(self->param) & MPF_SMALL) {
			psy_ui_size_setem(rv, PARAMWIDTH_SMALL, 2.0);
			return;
		}
	}
	psy_ui_size_setem(rv, PARAMWIDTH, 2.0);
}

void knobui_on_mouse_down(KnobUi* self, psy_ui_MouseEvent* ev)
{
	if (psy_ui_mouseevent_button(ev) == 1) {
		paramtweak_begin(&self->paramtweak, self->machine, self->paramidx,
			self->param);		
		paramtweak_on_mouse_down(&self->paramtweak, ev);
		psy_ui_component_capture(&self->component);
	}
	else if (psy_ui_mouseevent_button(ev) == 2) {
		knobui_update_param(self);
		psy_signal_emit(&self->signal_edit, self, 0);
	}
}

void knobui_on_mouse_move(KnobUi* self, psy_ui_MouseEvent* ev)
{		
	if (paramtweak_active(&self->paramtweak)) {		
		paramtweak_on_mouse_move(&self->paramtweak, ev);		
		psy_ui_component_invalidate(&self->component);
		knobui_automate(self);
	}
}

void knobui_on_mouse_up(KnobUi* self, psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);
	if (paramtweak_active(&self->paramtweak)) {
		paramtweak_end(&self->paramtweak);		
		psy_ui_component_invalidate(&self->component);
	}
}

void knobui_on_mouse_wheel(KnobUi* self, psy_ui_MouseEvent* ev)
{
	intptr_t minval;
	intptr_t maxval;
	intptr_t scaled;
	bool set;
	
	if (!self->param && psy_ui_mouseevent_delta(ev) != 0) {
		return;
	}	
	if (self->machine) {
		scaled = psy_audio_machine_parameter_scaled_value(self->machine,
			self->param);
		psy_audio_machine_parameter_range(self->machine, self->param,
			&minval, &maxval);
	} else {
		scaled = psy_audio_machineparam_scaled_value(self->param);
		psy_audio_machineparam_range(self->param, &minval, &maxval);
	}	
	set = FALSE;		
	if (psy_ui_mouseevent_delta(ev) > 0) {
		if (scaled + 1 < maxval) {
			++scaled;
			set = TRUE;
		}
	} else if (scaled > minval) {
		--scaled;
		set = TRUE;
	}
	if (set) {
		if (self->machine) {	
			psy_audio_machine_parameter_tweak_scaled(self->machine, self->param,
				scaled);
		} else {
			psy_audio_machineparam_tweak_scaled(self->param, scaled);			
		}
		knobui_automate(self);
	}
}	

void knobui_automate(KnobUi* self)
{
	if (self->machine) {
		psy_EventDriverCmd cmd;
		uint16_t param16;

		cmd.type = psy_EVENTDRIVER_AUTOMATE;
		cmd.id = psy_audio_machine_slot(self->machine);
		param16 = (uint16_t)self->paramidx;
		cmd.midi.byte0 = (uint8_t)(param16 & 0xFF);
		cmd.midi.byte1 = (uint8_t)((param16 >> 8) & 0xFF);
		inputhandler_send(psy_ui_app_input_handler(psy_ui_app()),
			"automate", cmd);
	}
}

void knobui_update_param(KnobUi* self)
{
	if (self->machine && self->paramidx != psy_INDEX_INVALID) {
		self->param = psy_audio_machine_parameter(self->machine,
			self->paramidx);
	}
}
