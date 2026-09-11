/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "effectui.h"
/* host */
#include "styles.h"
/* ui*/
#include <uiapp.h>


/* prototypes */
static void effectui_on_destroyed(EffectUi*);
static void effectui_on_mouse_down(EffectUi*, psy_ui_MouseEvent*);
static void effectui_onmousedoubleclick(EffectUi*, psy_ui_MouseEvent*);
static void effectui_move(EffectUi*, psy_ui_Point topleft);
static void effectui_updatevolumedisplay(EffectUi*);
static void effectui_on_machine_renamed(EffectUi*, psy_audio_Machines* sender,
	uintptr_t mac_id);
static void effectui_on_timer(EffectUi*, uintptr_t timerid);
static void effectui_on_update_styles(EffectUi*);	
	
/* vtable */
static psy_ui_ComponentVtable effectui_vtable;
static psy_ui_ComponentVtable effectui_super_vtable;
static bool effectui_vtable_initialized = FALSE;

static void effectui_vtable_init(EffectUi* self)
{
	assert(self);

	if (!effectui_vtable_initialized) {
		effectui_vtable = *(self->component.vtable);
		effectui_super_vtable = effectui_vtable;
		effectui_vtable.on_destroyed =
			(psy_ui_fp_component)
			effectui_on_destroyed;
		effectui_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			effectui_on_mouse_down;
		effectui_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			effectui_onmousedoubleclick;
		effectui_vtable.move =
			(psy_ui_fp_component_move)
			effectui_move;		
		effectui_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			effectui_on_timer;
		effectui_vtable.onupdatestyles = 
			(psy_ui_fp_component)
			effectui_on_update_styles;
		effectui_vtable_initialized = TRUE;	
	}
	psy_ui_component_set_vtable(&self->component, &effectui_vtable);	
}

/* implementation */
void effectui_init(EffectUi* self, psy_ui_Component* parent,
	uintptr_t slot, ParamViews* paramviews, psy_audio_Machines* machines,
	psy_Configuration* config)
{
	assert(self);
	assert(machines);	

	psy_ui_component_init(&self->component, parent, NULL);	
	effectui_vtable_init(self);	
	psy_ui_component_set_style_type(&self->component, STYLE_MV_EFFECT);
	self->paramviews = paramviews;
	self->machines = machines;	
	self->machine = psy_audio_machines_at(self->machines, slot);
	self->config_ = config;
	assert(self->machine);
	self->prevent_machine_pos = FALSE;
	/* mute */
	psy_ui_component_init(&self->mute, &self->component, NULL);
	psy_ui_component_set_style_type(&self->mute, STYLE_MV_EFFECT_MUTE);
	psy_ui_component_set_style_type_select(&self->mute,
		STYLE_MV_EFFECT_MUTE_SELECT);
	psy_ui_label_init(&self->mute_label, &self->mute);	
	// psy_ui_component_set_style_type(psy_ui_label_base(&self->mute_label),
	//	STYLE_MV_EFFECT_MUTE_LABEL);
	psy_ui_component_hide(psy_ui_label_base(&self->mute_label));
	psy_ui_component_set_align(psy_ui_label_base(&self->mute_label),
		psy_ui_ALIGN_CLIENT);
	psy_ui_label_prevent_translation(&self->mute_label);
	psy_ui_label_set_text_alignment(&self->mute_label, psy_ui_ALIGNMENT_CENTER);
	psy_ui_label_set_text(&self->mute_label, "M");	
	/* bypass */
	psy_ui_component_init(&self->bypass, &self->component, NULL);
	psy_ui_component_set_style_type(&self->bypass, STYLE_MV_EFFECT_BYPASS);
	psy_ui_component_set_style_type_select(&self->bypass,
		STYLE_MV_EFFECT_BYPASS_SELECT);		
	psy_ui_label_init(&self->bypass_label, &self->bypass);
	// psy_ui_component_set_style_type(psy_ui_label_base(&self->bypass_label),
	//	STYLE_MV_EFFECT_BYPASS_LABEL);
	psy_ui_component_hide(psy_ui_label_base(&self->bypass_label));
	psy_ui_component_set_align(psy_ui_label_base(&self->bypass_label),
		psy_ui_ALIGN_CLIENT);
	psy_ui_label_prevent_translation(&self->bypass_label);	
	psy_ui_label_set_text(&self->bypass_label, "B");
	psy_ui_label_set_text_alignment(&self->bypass_label,
		psy_ui_ALIGNMENT_CENTER);
	editnameui_init(&self->editname, &self->component, self->machine,
		STYLE_MV_EFFECT_NAME);
	panui_init(&self->pan, &self->component, self->machine,
		STYLE_MV_EFFECT_PAN, STYLE_MV_EFFECT_PAN_SLIDER);
	vuui_init(&self->vu, &self->component, self->machine,
		STYLE_MV_EFFECT_VU, STYLE_MV_EFFECT_VU0, STYLE_MV_EFFECT_VUPEAK);
	self->counter = 0;
	psy_signal_connect(&machines->signal_renamed, self,
		effectui_on_machine_renamed);
	effectui_on_update_styles(self);
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void effectui_on_destroyed(EffectUi* self)
{
	assert(self);
	
	if (self->machines) {
		psy_signal_disconnect_context(&self->machines->signal_renamed, self);
	}
}

EffectUi* effectui_alloc(void)
{
	return (EffectUi*)malloc(sizeof(EffectUi));
}

EffectUi* effectui_alloc_init(psy_ui_Component* parent, uintptr_t mac_id,
	ParamViews* paramviews, psy_audio_Machines* machines,
	psy_Configuration* config)
{
	EffectUi* rv;
	
	rv = effectui_alloc();
	if (rv) {
		effectui_init(rv, parent, mac_id, paramviews, machines, config);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void effectui_move(EffectUi* self, psy_ui_Point topleft)
{
	assert(self);

	effectui_super_vtable.move(&self->component, topleft);	
	if (!self->prevent_machine_pos) {
		double zoom;
		
		zoom = psy_ui_app_zoom_rate(psy_ui_app());
		if (self->config_) {
			psy_Property* p;
				
			p = psy_configuration_at(self->config_, "zoom");
			if (p) {
				zoom *= psy_property_item_double(p);
			}			
		}
		psy_audio_machine_set_position(self->machine,
		psy_ui_value_px(&topleft.x, NULL, NULL) / zoom,
		psy_ui_value_px(&topleft.y, NULL, NULL) / zoom);
	}
}

void effectui_on_mouse_down(EffectUi* self, psy_ui_MouseEvent* ev)
{	
	if (psy_ui_mouseevent_button(ev) == 1) {
		if (psy_audio_machine_slot(self->machine) !=
				psy_audio_machines_selected(self->machines)) {
			if (psy_ui_mouseevent_ctrl_key(ev)) {
				psy_audio_machineselection_select(&self->machines->selection,
					psy_audio_machineindex_make(psy_audio_machine_slot(
						self->machine)));
			} else {
				psy_audio_machines_select(self->machines,
					psy_audio_machine_slot(self->machine));
			}
		}
		if (psy_ui_mouseevent_target(ev) == &self->bypass ||
				(psy_ui_mouseevent_target(ev) == psy_ui_label_base(
					&self->bypass_label))) {
			if (psy_audio_machine_bypassed(self->machine)) {
				psy_audio_machine_unbypass(self->machine);
			} else {
				psy_audio_machine_bypass(self->machine);
			}
			psy_ui_mouseevent_stop_propagation(ev);
		} else if (psy_ui_mouseevent_target(ev) == &self->mute ||
				(psy_ui_mouseevent_target(ev) == psy_ui_label_base(
					&self->mute_label))) {
			if (psy_audio_machine_muted(self->machine)) {
				psy_audio_machine_unmute(self->machine);
			} else {
				psy_audio_machine_mute(self->machine);
			}
			psy_ui_mouseevent_stop_propagation(ev);
		}		
	}
}

void effectui_onmousedoubleclick(EffectUi* self, psy_ui_MouseEvent* ev)
{
	if (self->paramviews && 
			psy_ui_mouseevent_button(ev) == 1 &&
			psy_ui_mouseevent_target(ev) != &self->bypass &&
			psy_ui_mouseevent_target(ev) != psy_ui_label_base(
				&self->bypass_label) &&
			psy_ui_mouseevent_target(ev) != &self->mute &&
			psy_ui_mouseevent_target(ev) != psy_ui_label_base(
				&self->mute_label)) {
		paramviews_show(self->paramviews, psy_audio_machine_slot(
			self->machine));
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void effectui_on_machine_renamed(EffectUi* self,
	psy_audio_Machines* sender, uintptr_t mac_id)
{
	assert(self);
	
	if (psy_audio_machine_slot(self->machine) == mac_id) {
		editnameui_update(&self->editname);
	}
}

void effectui_on_timer(EffectUi* self, uintptr_t timerid)
{
	if (!psy_ui_component_draw_visible(&self->component)) {
		return;
	}	
	if (self->counter > 0) {
		--self->counter;
	}
	if (self->counter == 0) {		
		if (psy_audio_machine_muted(self->machine)) {
			psy_ui_component_add_style_state(&self->mute,
				psy_ui_STYLESTATE_SELECT);
		} else {
			psy_ui_component_remove_style_state(&self->mute,
				psy_ui_STYLESTATE_SELECT);
		}
		if (psy_audio_machine_bypassed(self->machine)) {
			psy_ui_component_add_style_state(&self->bypass,
				psy_ui_STYLESTATE_SELECT);
		} else {
			psy_ui_component_remove_style_state(&self->bypass,
				psy_ui_STYLESTATE_SELECT);
		}
		self->counter = 4;
	}
	if (vuui_update(&self->vu)) {
		psy_ui_component_invalidate(vuui_base(&self->vu));	
	}
}

void effectui_on_update_styles(EffectUi* self)
{ 
	psy_ui_Style* style;
		
	style = psy_ui_style(STYLE_MV_EFFECT_MUTE_LABEL);
	if (style && style->display_set && style->display == psy_ui_DISPLAY_NONE) {
		if (psy_ui_component_visible(psy_ui_label_base(&self->mute_label))) {
			psy_ui_component_hide(psy_ui_label_base(&self->mute_label));
		}
	} else if (!psy_ui_component_visible(&self->mute_label.component)) {
		psy_ui_component_show(psy_ui_label_base(&self->mute_label));
	}
	style = psy_ui_style(STYLE_MV_EFFECT_BYPASS_LABEL);
	if (style && style->display_set && style->display == psy_ui_DISPLAY_NONE) {
		if (psy_ui_component_visible(psy_ui_label_base(&self->bypass_label))) {
			psy_ui_component_hide(psy_ui_label_base(&self->bypass_label));
		}
	} else if (!psy_ui_component_visible(psy_ui_label_base(
			&self->bypass_label))) {
		psy_ui_component_show(psy_ui_label_base(&self->bypass_label));
	}
}
