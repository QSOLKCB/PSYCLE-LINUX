/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "timebar.h"
/* host */
#include "styles.h"
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/portable.h"


#define CTRL_UNSET = FALSE;
#define CTRL_SET = TRUE;


/* prototypes */
static void timebar_on_less_clicked(TimeBar*, psy_ui_Button* sender);
static void timebar_on_more_clicked(TimeBar*, psy_ui_Button* sender);
#ifdef PSYCLE_TIMEBAR_OLD
static void timebar_on_lessless_clicked(TimeBar*, psy_ui_Button* sender);
static void timebar_on_moremore_clicked(TimeBar*, psy_ui_Button* sender);
#endif
static void timebar_change_bpm(TimeBar*, double bpm_offset);
static double timebar_offset(bool is_ctrl_set);

/* implementation */
void timebar_init(TimeBar* self, psy_ui_Component* parent,
	psy_audio_Player* player)
{
	assert(self);
	assert(player);

	psy_ui_component_init(timebar_base(self), parent, NULL);
	self->player_ = player;
	psy_ui_component_set_align_expand(timebar_base(self), psy_ui_HEXPAND);
	psy_ui_component_set_default_align(timebar_base(self), psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	/* bpm description label */
	psy_ui_label_init_text(&self->desc_, timebar_base(self), "timebar.tempo");
#ifdef PSYCLE_TIMEBAR_OLD
	/* bpm - 10 */
	psy_ui_button_init_icon_connect(&self->lessless_, timebar_base(self),
		&self->lessless, self, timebar_on_lessless_clicked);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->lessless_));
#endif
	/* bpm -1 */
	psy_ui_button_init_resource_connect(&self->less_, timebar_base(self),
		"img.icon-less", self, timebar_on_less_clicked);
	psy_ui_button_set_repeat(&self->less_, 100, 500);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->less_));
	/* bpm (realbpm) number label */
	valueui_init(&self->bpm_, timebar_base(self), NULL, 0,
		&player->tempo_param_.machineparam);
	self->bpm_.paramtweak.tweak_scale_ = 999;
	psy_ui_component_prevent_app_focus_out(valueui_base(&self->bpm_));
	/* bpm +1 */
	psy_ui_button_init_resource_connect(&self->more_, timebar_base(self),
		"img.icon-more", self, timebar_on_more_clicked);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->more_));
	psy_ui_button_set_repeat(&self->more_, 100, 500);	
#ifdef PSYCLE_TIMEBAR_OLD
	/* bpm + 10 */
	psy_ui_button_init_icon_connect(&self->moremore_, timebar_base(self),
		&self->moremore, self, timebar_on_moremore_clicked);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->moremore_));
#endif	
}

void timebar_on_less_clicked(TimeBar* self, psy_ui_Button* sender)
{
	assert(self);
		
	timebar_change_bpm(self, -timebar_offset(psy_ui_button_hasctrl(sender)));
}

void timebar_on_more_clicked(TimeBar* self, psy_ui_Button* sender)
{
	assert(self);

	timebar_change_bpm(self, timebar_offset(psy_ui_button_hasctrl(sender)));
}

#ifdef PSYCLE_TIMEBAR_OLD
void timebar_on_lessless_clicked(TimeBar* self, psy_ui_Button* sender)
{
	assert(self);

	timebar_change_bpm(self, -timebar_offset(CTRL_SET));
}

void timebar_on_moremore_clicked(TimeBar* self, psy_ui_Button* sender)
{
	assert(self);

	timebar_change_bpm(self, timebar_offset(CTRL_SET));
}
#endif

double timebar_offset(bool is_ctrl_set)
{
	return ((is_ctrl_set)
		? 10.0
		: 1.0);
}

void timebar_change_bpm(TimeBar* self, double bpm_offset)
{
	assert(self);

	psy_audio_player_set_bpm(self->player_, psy_audio_player_bpm(self->player_)
		+ bpm_offset);
}
