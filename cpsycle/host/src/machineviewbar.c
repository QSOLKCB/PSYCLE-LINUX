/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineviewbar.h"
/* host */
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void machineviewbar_on_song_changed(MachineViewBar*,
	psy_audio_Player* sender);
static void machineviewbar_connect_song(MachineViewBar*, psy_audio_Song*);
static void machineviewbar_on_mixer_connect_mode_click(MachineViewBar*,
	psy_ui_Component* sender);
static void machineviewbar_update_connect_as_mixer_send(MachineViewBar*);
static void machineviewbar_on_machine_insert(MachineViewBar*,
	psy_audio_Machines*, uintptr_t slot);
static void machineviewbar_on_machine_removed(MachineViewBar*,
	psy_audio_Machines*, uintptr_t slot);
static psy_audio_Machines* machineviewbar_machines(MachineViewBar*);

/* implementation */
void machineviewbar_init(MachineViewBar* self, psy_ui_Component* parent,
	psy_Configuration* config, psy_audio_Player* player)
{
	assert(self);
	assert(config);
	assert(player);
	
	psy_ui_component_init(machineviewbar_base(self), parent, NULL);
	self->player_ = player;
	self->cfg_ = config;
	psy_ui_component_set_style_type(&self->component, STYLE_MV_WIRES);
	psy_ui_component_set_default_align(machineviewbar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	zoombox_init(&self->zoom_box_, machineviewbar_base(self));		
	zoombox_exchange(&self->zoom_box_, psy_configuration_at(self->cfg_,
		"zoom"));
	psy_ui_checkbox_init_text(&self->mixer_send_, machineviewbar_base(self),
		"mv.connect-to-mixer-send-return-input");
	psy_ui_component_hide(psy_ui_checkbox_base(&self->mixer_send_));	
	psy_ui_sizer_init(&self->sizer_, machineviewbar_base(self));
	psy_ui_component_hide(psy_ui_sizer_base(&self->sizer_));
	psy_signal_connect(&self->mixer_send_.signal_clicked,
		self, machineviewbar_on_mixer_connect_mode_click);	
	psy_signal_connect(&player->signal_song_changed,
		self, machineviewbar_on_song_changed);
	machineviewbar_connect_song(self, psy_audio_player_song(player));
}

void machineviewbar_on_mixer_connect_mode_click(MachineViewBar* self,
	psy_ui_Component* sender)
{			
	assert(self);
	
	if (!machineviewbar_machines(self)) {
		return;
	}
	if (psy_ui_checkbox_checked(&self->mixer_send_)) {
		psy_audio_machines_connect_as_mixersend(machineviewbar_machines(self));		
	} else {
		psy_audio_machines_connect_as_mixerinput(machineviewbar_machines(self));		
	}    
}

void machineviewbar_update_connect_as_mixer_send(MachineViewBar* self)
{	
	assert(self);
	
	if (machineviewbar_machines(self) &&
			psy_audio_machines_is_connect_as_mixersend(
				machineviewbar_machines(self))) {
		psy_ui_checkbox_check(&self->mixer_send_);
	} else {
		psy_ui_checkbox_disable_check(&self->mixer_send_);
	}
}

void machineviewbar_on_song_changed(MachineViewBar* self,
	psy_audio_Player* sender)
{	
	assert(self);
		
	machineviewbar_connect_song(self, psy_audio_player_song(sender));			
}

void machineviewbar_connect_song(MachineViewBar* self, psy_audio_Song* song)
{	
	assert(self);
	
	if (song) {		
		psy_signal_connect(&song->machines_.signal_insert, self,
			machineviewbar_on_machine_insert);
		psy_signal_connect(&song->machines_.signal_removed, self,
			machineviewbar_on_machine_removed);
	}
	machineviewbar_update_connect_as_mixer_send(self);
}

void machineviewbar_on_machine_insert(MachineViewBar* self,
	psy_audio_Machines* sender, uintptr_t slot)
{
	assert(self);
	
	if (psy_audio_machines_hasmixer(sender)) {		
		psy_ui_component_show(psy_ui_checkbox_base(&self->mixer_send_));
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_component_parent(&self->component)));
	}
	if (psy_audio_machines_at(sender, slot)) {		
		psy_Logger* logger;					
			
		logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
		if (logger) {
			char text[128];
					
			psy_snprintf(text, 128, "%s inserted at bus %u",
				psy_audio_machine_edit_name(psy_audio_machines_at(sender,
					slot)), (unsigned int)slot);
			psy_logger_output(logger, text);
		}		
	}	
}

void machineviewbar_on_machine_removed(MachineViewBar* self,
	psy_audio_Machines* sender, uintptr_t slot)
{	
	char text[128];
	psy_Logger* logger;

	assert(self);
	
	if (!psy_audio_machines_hasmixer(sender)) {		
		psy_ui_component_hide(psy_ui_checkbox_base(&self->mixer_send_));
		psy_ui_component_align(psy_ui_component_parent(
			psy_ui_component_parent(&self->component)));
	}	
	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {		
		psy_snprintf(text, 128, "Machine removed from bus %u",
			(unsigned int)slot);
		psy_logger_output(logger, text);
	}	
}

psy_audio_Machines* machineviewbar_machines(MachineViewBar* self)
{
	assert(self);
	
	if (psy_audio_player_song(self->player_)) {
		return psy_audio_song_machines(psy_audio_player_song(self->player_));
	}
	return NULL;	
}
