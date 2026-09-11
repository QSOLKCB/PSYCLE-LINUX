/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "generatorui.h"
/* host */
#include "paramview.h"
#include "styles.h"
/* ui */
#include <uiapp.h>


/* GeneratorUi */

/* prototypes */
static void generatorui_on_destroyed(GeneratorUi*);
static void generatorui_on_mouse_down(GeneratorUi*, psy_ui_MouseEvent*);
static void generatorui_on_mouse_double_click(GeneratorUi*, psy_ui_MouseEvent*);
static void generatorui_move(GeneratorUi*, psy_ui_Point topleft);
static void generatorui_on_timer(GeneratorUi*, uintptr_t timerid);
static void generatorui_on_machine_renamed(GeneratorUi*,
	psy_audio_Machines* sender, uintptr_t mac_id);
static void generatorui_on_update_styles(GeneratorUi*);

/* vtable */
static psy_ui_ComponentVtable generatorui_vtable;
static psy_ui_ComponentVtable generatorui_super_vtable;
static bool generatorui_vtable_initialized = FALSE;

static void generatorui_vtable_init(GeneratorUi* self)
{
	assert(self);

	if (!generatorui_vtable_initialized) {
		generatorui_vtable = *(self->component.vtable);
		generatorui_super_vtable = generatorui_vtable;
		generatorui_vtable.on_destroyed =
			(psy_ui_fp_component)
			generatorui_on_destroyed;
		generatorui_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			generatorui_on_mouse_down;
		generatorui_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			generatorui_on_mouse_double_click;
		generatorui_vtable.move =
			(psy_ui_fp_component_move)
			generatorui_move;		
		generatorui_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			generatorui_on_timer;
		generatorui_vtable.onupdatestyles = 
			(psy_ui_fp_component)
			generatorui_on_update_styles;
		generatorui_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &generatorui_vtable);
}

/* implementation */
void generatorui_init(GeneratorUi* self, psy_ui_Component* parent,
	uintptr_t slot, ParamViews* paramviews, psy_audio_Machines* machines,
	psy_Configuration* config)
{
	assert(self);
	assert(machines);	

	psy_ui_component_init(&self->component, parent, NULL);
	generatorui_vtable_init(self);	
	self->paramviews = paramviews;
	self->machines = machines;
	self->config_ = config;
	assert(self->machines);
	self->machine = psy_audio_machines_at(self->machines, slot);
	assert(self->machine);	
	self->prevent_machine_pos = FALSE;
	/* mute */
	psy_ui_component_set_style_type(&self->component, STYLE_MV_GENERATOR);
	psy_ui_component_init(&self->mute, &self->component, NULL);
	psy_ui_component_set_style_type(&self->mute,
		STYLE_MV_GENERATOR_MUTE);
	psy_ui_component_set_style_type_select(&self->mute,
		STYLE_MV_GENERATOR_MUTE_SELECT);
	psy_ui_label_init(&self->mute_label, &self->mute);	
	psy_ui_component_set_style_type(psy_ui_label_base(&self->mute_label),
		STYLE_MV_GENERATOR_MUTE_LABEL);
	psy_ui_component_hide(psy_ui_label_base(&self->mute_label));
	psy_ui_component_set_align(psy_ui_label_base(&self->mute_label),
		psy_ui_ALIGN_CLIENT);
	psy_ui_label_prevent_translation(&self->mute_label);
	psy_ui_label_set_text(&self->mute_label, "M");	
	psy_ui_label_set_text_alignment(&self->mute_label, psy_ui_ALIGNMENT_CENTER);
	/* solo */
	psy_ui_component_init(&self->solo, &self->component, NULL);	
	psy_ui_component_set_style_type(&self->solo, STYLE_MV_GENERATOR_SOLO);
	psy_ui_component_set_style_type_select(&self->solo,
		STYLE_MV_GENERATOR_SOLO_SELECT);
	psy_ui_label_init(&self->solo_label, &self->solo);
	psy_ui_component_set_style_type(psy_ui_label_base(&self->solo_label),
		STYLE_MV_GENERATOR_SOLO_LABEL);
	psy_ui_component_hide(psy_ui_label_base(&self->solo_label));
	psy_ui_component_set_align(psy_ui_label_base(&self->solo_label),
		psy_ui_ALIGN_CLIENT);
	psy_ui_label_prevent_translation(&self->solo_label);	
	psy_ui_label_set_text(&self->solo_label, "S");
	psy_ui_label_set_text_alignment(&self->solo_label, psy_ui_ALIGNMENT_CENTER);
	editnameui_init(&self->editname, &self->component, self->machine,
		STYLE_MV_GENERATOR_NAME);
	panui_init(&self->pan, &self->component, self->machine,
		STYLE_MV_GENERATOR_PAN, STYLE_MV_GENERATOR_PAN_SLIDER);
	vuui_init(&self->vu, &self->component, self->machine,
		STYLE_MV_GENERATOR_VU, STYLE_MV_GENERATOR_VU0,
		STYLE_MV_GENERATOR_VUPEAK);	
	generatorui_on_update_styles(self);
	self->counter = 0;
	psy_signal_connect(&machines->signal_renamed, self,
		generatorui_on_machine_renamed);
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void generatorui_on_destroyed(GeneratorUi* self)
{
	assert(self);
	
	if (self->machines) {
		psy_signal_disconnect_context(&self->machines->signal_renamed, self);
	}
}

GeneratorUi* generatorui_alloc(void)
{
	return (GeneratorUi*)malloc(sizeof(GeneratorUi));
}

GeneratorUi* generatorui_alloc_init(psy_ui_Component* parent, uintptr_t mac_id,
	ParamViews* paramviews, psy_audio_Machines* machines,
	psy_Configuration* config)
{
	GeneratorUi* rv;
	
	rv = generatorui_alloc();
	if (rv) {
		generatorui_init(rv, parent, mac_id, paramviews, machines, config);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void generatorui_move(GeneratorUi* self, psy_ui_Point topleft)
{
	assert(self);

	generatorui_super_vtable.move(&self->component, topleft);
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

void generatorui_on_mouse_down(GeneratorUi* self, psy_ui_MouseEvent* ev)
{	
	assert(self);

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
		if (psy_ui_mouseevent_target(ev) == &self->mute ||
				(psy_ui_mouseevent_target(ev) == psy_ui_label_base(
					&self->mute_label))) {
			if (psy_audio_machine_muted(self->machine)) {
				psy_audio_machine_unmute(self->machine);
			} else {
				psy_audio_machine_mute(self->machine);
			}
			psy_ui_mouseevent_stop_propagation(ev);
		} else if (psy_ui_mouseevent_target(ev) == &self->solo ||
				(psy_ui_mouseevent_target(ev) == psy_ui_label_base(
					&self->solo_label))) {
			psy_audio_machines_solo(self->machines,
				psy_audio_machine_slot(self->machine));
			psy_ui_mouseevent_stop_propagation(ev);
		}		
	}
}

void generatorui_on_mouse_double_click(GeneratorUi* self,
	psy_ui_MouseEvent* ev)
{	
	if (psy_ui_mouseevent_button(ev) == 1 &&
			psy_ui_mouseevent_target(ev) != &self->solo &&
			psy_ui_mouseevent_target(ev) != &self->mute &&
			psy_ui_mouseevent_target(ev) != psy_ui_label_base(
				&self->mute_label) &&
			psy_ui_mouseevent_target(ev) != psy_ui_label_base(
				&self->solo_label) &&
			self->paramviews) {
		paramviews_show(self->paramviews,
			psy_audio_machine_slot(self->machine));
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void generatorui_on_machine_renamed(GeneratorUi* self,
	psy_audio_Machines* sender, uintptr_t mac_id)
{
	assert(self);
	
	if (psy_audio_machine_slot(self->machine) == mac_id) {
		editnameui_update(&self->editname);
	}
}

void generatorui_on_timer(GeneratorUi* self, uintptr_t timerid)
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
		if (psy_audio_machines_soloed(self->machines) ==
				psy_audio_machine_slot(self->machine)) {
			psy_ui_component_add_style_state(&self->solo,
				psy_ui_STYLESTATE_SELECT);
		} else {
			psy_ui_component_remove_style_state(&self->solo,
				psy_ui_STYLESTATE_SELECT);
		}
		self->counter = 4;
	}
	if (vuui_update(&self->vu)) {
		psy_ui_component_invalidate(vuui_base(&self->vu));		
	}
}

void generatorui_on_update_styles(GeneratorUi* self)
{ 
	psy_ui_Style* style;
		
	style = psy_ui_style(STYLE_MV_GENERATOR_MUTE_LABEL);
	if (psy_ui_style_hidden(style)) {		
		psy_ui_component_hide(psy_ui_label_base(&self->mute_label));		
	} else {
		psy_ui_component_show(&self->mute_label.component);
	}
	style = psy_ui_style(STYLE_MV_GENERATOR_SOLO_LABEL);
	if (psy_ui_style_hidden(style)) {		
		psy_ui_component_hide(&self->solo_label.component);		
	} else {			
		psy_ui_component_show(&self->solo_label.component);
	}
}
