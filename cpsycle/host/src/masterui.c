/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "masterui.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void masterui_move(MasterUi*, psy_ui_Point top_left);
static void masterui_on_mouse_double_click(MasterUi*, psy_ui_MouseEvent*);
static void masterui_on_update_styles(MasterUi*);

/* vtable */
static psy_ui_ComponentVtable masterui_vtable;
static psy_ui_ComponentVtable masterui_super_vtable;
static bool masterui_vtable_initialized = FALSE;

static void masterui_vtable_init(MasterUi* self)
{
	assert(self);

	if (!masterui_vtable_initialized) {
		masterui_vtable = *(self->component.vtable);
		masterui_super_vtable = masterui_vtable;		
		masterui_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			masterui_on_mouse_double_click;
		masterui_vtable.move =
			(psy_ui_fp_component_move)
			masterui_move;
		masterui_vtable.onupdatestyles = 
			(psy_ui_fp_component)
			masterui_on_update_styles;		
		masterui_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(masterui_base(self), &masterui_vtable);	
}

/* implementation */
void masterui_init(MasterUi* self, psy_ui_Component* parent,
	ParamViews* paramviews, psy_audio_Machines* machines,
	psy_Configuration* config)
{
	assert(self);
	assert(machines);	

	psy_ui_component_init(&self->component, parent, NULL);	
	masterui_vtable_init(self);
	self->config_ = config;
	psy_ui_component_set_style_type(masterui_base(self), STYLE_MV_MASTER);
	self->param_views_ = paramviews;
	self->machine_ = psy_audio_machines_master(machines);
	assert(self->machine_);
	self->prevent_machine_pos_ = FALSE;
	psy_ui_label_init_text(&self->name_, masterui_base(self),
		"mv.master");	
	psy_ui_component_set_style_type(psy_ui_label_base(&self->name_),
		STYLE_MV_MASTER_NAME);
	psy_ui_component_hide(psy_ui_label_base(&self->name_));
	psy_ui_component_set_align(psy_ui_label_base(&self->name_),
		psy_ui_ALIGN_CLIENT);	
	psy_ui_label_set_text_alignment(&self->name_, psy_ui_ALIGNMENT_CENTER);
	masterui_on_update_styles(self);
}

MasterUi* masterui_alloc(void)
{
	return (MasterUi*)malloc(sizeof(MasterUi));
}

MasterUi* masterui_alloc_init(psy_ui_Component* parent, ParamViews* param_views,
	psy_audio_Machines* machines, psy_Configuration* config)
{
	MasterUi* rv;
	
	rv = masterui_alloc();
	if (rv) {
		masterui_init(rv, parent, param_views, machines, config);
		psy_ui_component_deallocate_after_destroyed(masterui_base(rv));
	}
	return rv;
}

void masterui_move(MasterUi* self, psy_ui_Point top_left)
{
	assert(self);

	masterui_super_vtable.move(masterui_base(self), top_left);
	if (!self->prevent_machine_pos_) {
		double zoom;
		
		zoom = psy_ui_app_zoom_rate(psy_ui_app());
		if (self->config_) {
			psy_Property* p;
				
			p = psy_configuration_at(self->config_, "zoom");
			if (p) {
				zoom *= psy_property_item_double(p);
			}			
		}
		psy_audio_machine_set_position(self->machine_,
			psy_ui_value_px(&top_left.x, NULL, NULL) / zoom,
			psy_ui_value_px(&top_left.y, NULL, NULL) / zoom);
	}
}

void masterui_on_mouse_double_click(MasterUi* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (psy_ui_mouseevent_button(ev) == 1 && self->param_views_) {
		paramviews_show(self->param_views_, psy_audio_machine_slot(
			self->machine_));		
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void masterui_on_update_styles(MasterUi* self)
{ 
	psy_ui_Style* style;
	
	assert(self);
		
	style = psy_ui_style(STYLE_MV_MASTER_NAME);	
	if (style && style->display_set && style->display == psy_ui_DISPLAY_NONE) {		
		if (psy_ui_component_visible(psy_ui_label_base(&self->name_))) {
			psy_ui_component_hide(psy_ui_label_base(&self->name_));
		}
	} else if (!psy_ui_component_visible(&self->name_.component)) {
		psy_ui_component_show(psy_ui_label_base(&self->name_));
	}	
}
