/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "linesperbeatbar.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void linesperbeatbar_init_layout(LinesPerBeatBar*);
static void linesperbeatbar_init_lpb(LinesPerBeatBar*);
static void linesperbeatbar_init_less(LinesPerBeatBar*);
static void linesperbeatbar_init_more(LinesPerBeatBar*);
static void linesperbeatbar_on_less(LinesPerBeatBar*,
	psy_ui_Component* sender);
static void linesperbeatbar_on_more(LinesPerBeatBar*,
	psy_ui_Component* sender);
static void linesperbeatbar_update(LinesPerBeatBar*);

/* implementation */
void linesperbeatbar_init(LinesPerBeatBar* self, psy_ui_Component* parent,
	psy_audio_Player* player)
{		
	assert(self);	
	assert(player);

	psy_ui_component_init(linesperbeatbar_base(self), parent, NULL);	
	self->player_ = player;
	linesperbeatbar_init_layout(self);	
	psy_ui_label_init_text(&self->desc_, linesperbeatbar_base(self),
		"lpb.lines-per-beat");	
	linesperbeatbar_init_less(self);
	linesperbeatbar_init_lpb(self);
	linesperbeatbar_init_more(self);
}

void linesperbeatbar_init_layout(LinesPerBeatBar* self)
{
	assert(self);

	psy_ui_component_set_align_expand(linesperbeatbar_base(self),
		psy_ui_HEXPAND);
	psy_ui_component_set_default_align(linesperbeatbar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_prevent_app_focus_out(linesperbeatbar_base(self));
}

void linesperbeatbar_init_lpb(LinesPerBeatBar* self)
{
	assert(self);

	valueui_init(&self->number_, linesperbeatbar_base(self), NULL, 0,
		&self->player_->lpb_param_.machineparam);
	psy_ui_component_set_preferred_width(valueui_base(&self->number_),
		psy_ui_value_make_ew(4.0));
	psy_ui_component_prevent_app_focus_out(valueui_base(&self->number_));
}

void linesperbeatbar_init_less(LinesPerBeatBar* self)
{
	assert(self);	

	psy_ui_button_init_resource_connect(&self->less_, linesperbeatbar_base(self),
		"img.icon-less", self, linesperbeatbar_on_less);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->less_));			
}

void linesperbeatbar_init_more(LinesPerBeatBar* self)
{
	assert(self);

	psy_ui_button_init_resource_connect(&self->more_, linesperbeatbar_base(self),
		"img.icon-more", self, linesperbeatbar_on_more);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->more_));
}

void linesperbeatbar_on_less(LinesPerBeatBar* self, psy_ui_Component* sender)
{
	assert(self);

	if (psy_audio_player_lpb(self->player_) > 1) {
		psy_audio_player_set_lpb(self->player_,
			psy_audio_player_lpb(self->player_) - 1);
	}
}

void linesperbeatbar_on_more(LinesPerBeatBar* self, psy_ui_Component* sender)
{
	assert(self);

	psy_audio_player_set_lpb(self->player_,
		psy_audio_player_lpb(self->player_) + 1);
}
