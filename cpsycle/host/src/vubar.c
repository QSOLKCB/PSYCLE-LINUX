/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "vubar.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* container */
#include <configuration.h>


/* prototypes */
static void vubar_connect_configuration(VuBar*, psy_Configuration*);
static void vubar_configure(VuBar*, psy_Configuration*);
static void vubar_on_vu_colour(VuBar*, psy_Property* sender);
static void vubar_on_vu_bg_colour(VuBar*, psy_Property* sender);
static void vubar_on_vu_clip_colour(VuBar*, psy_Property* sender);

/* implementation */
void vubar_init(VuBar* self, psy_ui_Component* parent,
	psy_Configuration* configuration)
{
	assert(self);	

	psy_ui_component_init(vubar_base(self), parent, NULL);
	psy_ui_component_set_preferred_width(vubar_base(self),
		psy_ui_value_make_ew(34.0));	
	psy_ui_label_init(&self->vu_label_, vubar_base(self));
	psy_ui_label_prevent_translation(&self->vu_label_);
	psy_ui_label_set_text(&self->vu_label_, "Vu");	
	psy_ui_component_set_margin(psy_ui_label_base(&self->vu_label_),
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_component_set_align(psy_ui_label_base(&self->vu_label_),
		psy_ui_ALIGN_LEFT);
	clipbox_init(&self->clip_box_, vubar_base(self));
	psy_ui_component_set_align(clipbox_base(&self->clip_box_),
		psy_ui_ALIGN_RIGHT);
	vumeter_init(&self->vu_meter_, vubar_base(self));	
	psy_ui_component_set_align(vumeter_base(&self->vu_meter_),
		psy_ui_ALIGN_TOP);
	volslider_init(&self->vol_slider_, vubar_base(self));
	psy_ui_component_set_align(volslider_base(&self->vol_slider_),
		psy_ui_ALIGN_TOP);
	vubar_connect_configuration(self, configuration);
	vubar_configure(self, configuration);
}

void vubar_connect_configuration(VuBar* self, psy_Configuration* configuration)
{
	assert(self);
	
	psy_configuration_connect(configuration, "vucolors.vu1", self,
		vubar_on_vu_colour);
	psy_configuration_connect(configuration, "vucolors.vu2", self,
		vubar_on_vu_bg_colour);
	psy_configuration_connect(configuration, "vucolors.vu3", self,
		vubar_on_vu_clip_colour);
}

void vubar_configure(VuBar* self, psy_Configuration* config)
{
	assert(self);
	
	psy_configuration_configure(config, "vucolors.vu1");
	psy_configuration_configure(config, "vucolors.vu2");
	psy_configuration_configure(config, "vucolors.vu3");
}

void vubar_reset(VuBar* self)
{
	assert(self);

	clipbox_deactivate(&self->clip_box_);
}

void vubar_idle(VuBar* self)
{
	vumeter_idle(&self->vu_meter_);
}

void vubar_set_machine(VuBar* self, psy_audio_Machine* machine)
{
	assert(self);
	
	volslider_set_machine(&self->vol_slider_, machine);
	vumeter_set_machine(&self->vu_meter_, machine);
	clipbox_set_machine(&self->clip_box_, machine);
}

/* configuration */
void vubar_on_vu_colour(VuBar* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
			
	style = psy_ui_style(STYLE_MAIN_VU);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	vumeter_update_style(&self->vu_meter_);
}

void vubar_on_vu_bg_colour(VuBar* self, psy_Property* sender)	
{
	psy_ui_Style* style;
	
	assert(self);
			
	style = psy_ui_style(STYLE_MAIN_VU);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	vumeter_update_style(&self->vu_meter_);
}

void vubar_on_vu_clip_colour(VuBar* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
			
	style = psy_ui_style(STYLE_CLIPBOX_SELECT);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	vumeter_update_style(&self->vu_meter_);
}
