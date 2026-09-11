/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "playbar.h"
/* host */
#include "cmdsgeneral.h"
#include "resources/resource.h"
#include "styles.h"
/* audio */
#include <exclusivelock.h>
/* platform */
#include "../../detail/portable.h"

#define PLAYBAR_TIMERINTERVAL 100

/* combobox index */
enum {
	PLAY_SONG  = 0,
	PLAY_SEL   = 1,
	PLAY_BEATS = 2,
	PLAY_FILES = 3
};

/* prototypes */
static void playbar_on_destroyed(PlayBar*);
static void playbar_update_text(PlayBar*);
static void playbar_on_loop(PlayBar*, psy_ui_Component* sender);
static void playbar_on_play_mode(PlayBar*, psy_ui_ComboBox* sender);
static void playbar_on_num_play_beats_less(PlayBar*, psy_ui_Button* sender);
static void playbar_on_num_play_beats_more(PlayBar*, psy_ui_Button* sender);
static void playbar_on_play(PlayBar*, psy_ui_Component* sender);
static void playbar_play(PlayBar*);
static void playbar_on_stop(PlayBar*, psy_ui_Component* sender);
static void playbar_on_timer(PlayBar*, uintptr_t timerid);
static void playbar_on_language_changed(PlayBar*);
static psy_audio_SequencerPlayMode playbar_combobox_to_play_mode(
	const PlayBar*);
static void playbar_on_play_status_changed(PlayBar*,
	psy_audio_Sequencer* sequencer);
static void playbar_update_play_button(PlayBar*);
static void playbar_on_play_mode(PlayBar*, psy_ui_ComboBox* sender);
static void playbar_on_bar_rec(PlayBar*, psy_ui_Button* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PlayBar* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			playbar_on_destroyed;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			playbar_on_timer;
		vtable.on_language_changed =
			(psy_ui_fp_component)
			playbar_on_language_changed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(playbar_base(self), &vtable);	
}

/* implementation */
void playbar_init(PlayBar* self, psy_ui_Component* parent, Workspace* workspace)
{					
	psy_ui_component_init(playbar_base(self), parent, NULL);
	vtable_init(self);
	psy_ui_component_set_style_type(playbar_base(self), STYLE_PLAYBAR);	
	psy_ui_component_init(&self->buttons, playbar_base(self), NULL);	
	psy_ui_component_set_align(&self->buttons, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->buttons, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_minimum_size(playbar_base(self), 
		psy_ui_size_make_em(30.0, 0.0));
	self->workspace = workspace;
	self->player = workspace_player(workspace);
	/* loop */
	psy_ui_button_init_text_connect(&self->loop, &self->buttons,
		"play.loop", self, playbar_on_loop);	
	psy_ui_button_set_svg(&self->loop, psy_ui_app_svg(psy_ui_app(), "img.repeat"));
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->loop));
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->loop), 0);
	/* record/edit mode */
	psy_ui_button_init_text_connect(&self->recordnotes, &self->buttons,
		"play.record-notes", self, playbar_on_bar_rec);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->recordnotes));
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->recordnotes), 0);
	/* play */
	psy_ui_button_init_text_connect(&self->play, &self->buttons,
		"play.play", self, playbar_on_play);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->play), 0);
	psy_ui_component_set_id(psy_ui_button_base(&self->play), CMD_IMM_PLAYSONG);
	psy_ui_button_set_svg(&self->play, psy_ui_app_svg(psy_ui_app(), "img.play"));
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->play));
	psy_signal_connect(&self->player->sequencer.signal_play_status_changed,
		self, playbar_on_play_status_changed);
	playbar_update_play_button(self);
	/* play mode */
	psy_ui_combobox_init(&self->playmode, &self->buttons);
	psy_ui_combobox_set_char_number(&self->playmode, 8.0);	
	/* play beat num */
	psy_ui_edit_init(&self->loopbeatsedit, &self->buttons);	
	psy_ui_component_set_tab_index(psy_ui_edit_base(&self->loopbeatsedit),
		0);
	psy_ui_edit_set_text(&self->loopbeatsedit, "4.00");
	psy_ui_edit_set_char_number(&self->loopbeatsedit, 6);
	psy_ui_button_init_resource_connect(&self->loopbeatsless, &self->buttons,
		"img.icon-less",
		self, playbar_on_num_play_beats_less);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->loopbeatsless));
	psy_ui_button_init_resource_connect(&self->loopbeatsmore, &self->buttons,
		"img.icon-more",
		self, playbar_on_num_play_beats_more);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->loopbeatsmore));
	/* stop */
	psy_ui_button_init_text_connect(&self->stop, &self->buttons,
		"play.stop", self, playbar_on_stop);	
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->stop), 0);
	psy_ui_component_set_id(psy_ui_button_base(&self->stop), CMD_IMM_PLAYSTOP);
	psy_ui_button_set_svg(&self->stop, psy_ui_app_svg(psy_ui_app(), "img.stop"));
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->stop));
	psy_ui_button_init_resource(&self->metronome, &self->buttons, "img.metronome");
	psy_ui_button_prevent_text(&self->metronome);
	psy_ui_button_exchange(&self->metronome, psy_configuration_at(
		psycleconfig_metronome(workspace_cfg(workspace)), "show"));		
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->metronome), 0);
	playbar_update_text(self);
	psy_ui_combobox_select(&self->playmode, 0);
	psy_signal_connect(&self->playmode.signal_selchanged, self,
		playbar_on_play_mode);	
	psy_ui_component_start_timer(playbar_base(self), 0, PLAYBAR_TIMERINTERVAL);	
}

void playbar_on_destroyed(PlayBar* self)
{
	assert(self);

	// psy_ui_navhandler_dispose(&self->nav_handler);	
}

void playbar_update_text(PlayBar* self)
{
	assert(self);

	psy_ui_combobox_clear(&self->playmode);
	psy_ui_combobox_add_text(&self->playmode, psy_ui_translate("play.song"));
	psy_ui_combobox_add_text(&self->playmode, psy_ui_translate("play.sel"));
	psy_ui_combobox_add_text(&self->playmode, psy_ui_translate("play.beats"));
	psy_ui_combobox_add_text(&self->playmode, psy_ui_translate("play.files"));
}


void playbar_on_play_mode(PlayBar* self, psy_ui_ComboBox* sender)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_player_stop(self->player);
	psy_audio_sequencer_set_play_mode(&self->player->sequencer,
		playbar_combobox_to_play_mode(self));
	playbar_play(self);	
	psy_audio_exclusivelock_leave();
}

void playbar_on_num_play_beats_less(PlayBar* self, psy_ui_Button* sender)
{
	double playbeats;
	char text[40];
	
	assert(self);

	playbeats = (double) atof(psy_ui_edit_text(&self->loopbeatsedit));
	if (playbeats > 1) {
		playbeats -= 1;
	}
	psy_audio_sequencer_set_num_play_beats(&self->player->sequencer,
		psy_dsp_beatpos_make_real(playbeats, psy_dsp_DEFAULT_PPQ));
	psy_snprintf(text, 40, "%f", (double) playbeats);
	psy_ui_edit_set_text(&self->loopbeatsedit, text);
}

void playbar_on_num_play_beats_more(PlayBar* self, psy_ui_Button* sender)
{
	double playbeats;
	char text[40];
	
	assert(self);

	playbeats = (double) atof(psy_ui_edit_text(&self->loopbeatsedit));
	playbeats += 1.0;
	psy_audio_sequencer_set_num_play_beats(&self->player->sequencer,
		psy_dsp_beatpos_make_real(playbeats, psy_dsp_DEFAULT_PPQ));
	psy_snprintf(text, 40, "%f", (double) playbeats);
	psy_ui_edit_set_text(&self->loopbeatsedit, text);
}

void playbar_on_play(PlayBar* self, psy_ui_Component* sender)
{
	assert(self);

	psy_audio_sequencer_set_play_mode(&self->player->sequencer,
		playbar_combobox_to_play_mode(self));	
	playbar_play(self);	
}

psy_audio_SequencerPlayMode playbar_combobox_to_play_mode(const PlayBar* self)
{
	assert(self);

	switch (psy_ui_combobox_cur_sel(&self->playmode)) {
	case PLAY_SONG:
		return psy_audio_SEQUENCERPLAYMODE_PLAYALL;		
	case PLAY_SEL:
		return psy_audio_SEQUENCERPLAYMODE_PLAYSEL;		
	case PLAY_BEATS:
		return psy_audio_SEQUENCERPLAYMODE_PLAYNUMBEATS;		
	case PLAY_FILES:		
		return psy_audio_SEQUENCERPLAYMODE_PLAYFILES;
	default:
		return psy_audio_SEQUENCERPLAYMODE_PLAYALL;
	}	
}

void playbar_play(PlayBar* self)
{
	psy_audio_Sequence* sequence;	
	psy_audio_SequenceEntry* entry;
	double startposition;
	double numplaybeats;
	
	assert(self);

	psy_ui_button_highlight(&self->play);
	sequence = self->player->sequencer.sequence;	
	entry = psy_audio_sequence_entry(sequence,
		psy_audio_sequenceselection_first(&self->workspace->song->sequence_.selection));
	if (entry) {
		psy_audio_exclusivelock_enter();		
		psy_audio_player_stop(self->player);
		psy_audio_sequence_set_play_selection(sequence,
			&self->workspace->song->sequence_.selection);
		startposition = psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
			entry));
		if (psy_audio_sequencer_play_mode(&self->player->sequencer)
				== psy_audio_SEQUENCERPLAYMODE_PLAYNUMBEATS) {
			psy_audio_SequenceCursor editposition;

			editposition = self->workspace->song->sequence_.cursor;
			startposition += psy_dsp_beatpos_real(editposition.offset);
			numplaybeats = (double)atof(psy_ui_edit_text(
				&self->loopbeatsedit));
			self->player->sequencer.numplaybeats = psy_dsp_beatpos_make_real(
				numplaybeats, psy_dsp_DEFAULT_PPQ);
		}
		psy_audio_player_set_position(self->player, startposition);
		psy_audio_player_start(self->player);
		psy_audio_exclusivelock_leave();
	}
}

void playbar_on_stop(PlayBar* self, psy_ui_Component* sender)
{
	assert(self);

	psy_audio_player_stop(self->player);
}

void playbar_on_loop(PlayBar* self, psy_ui_Component* sender)
{
	assert(self);

	if (psy_audio_sequencer_looping(&self->player->sequencer)) {
		psy_audio_sequencer_stop_loop(&self->player->sequencer);
		psy_ui_button_disable_highlight(&self->loop);
	} else {
		psy_audio_sequencer_loop(&self->player->sequencer);
		psy_ui_button_highlight(&self->loop);
	}
}

void playbar_on_play_status_changed(PlayBar* self,
	psy_audio_Sequencer* sequencer)
{
	assert(self);

	playbar_update_play_button(self);
}

void playbar_update_play_button(PlayBar* self)
{
	assert(self);

	if (psy_audio_player_playing(self->player)) {
		psy_ui_button_highlight(&self->play);
	} else {
		psy_ui_button_disable_highlight(&self->play);
	}
}

void playbar_on_timer(PlayBar* self, uintptr_t timerid)
{		
	assert(self);

	if (psy_audio_sequencer_looping(&self->player->sequencer)) {
		psy_ui_button_highlight(&self->loop);
	} else {
		psy_ui_button_disable_highlight(&self->loop);
	}
	if (self->workspace->player_.follow_song && psy_property_item_bool(
			&self->workspace->edit_mode)) {
		psy_ui_button_highlight(&self->recordnotes);		
	} else {
		psy_ui_button_disable_highlight(&self->recordnotes);
	}
}

void playbar_on_language_changed(PlayBar* self)
{
	assert(self);

	playbar_update_text(self);
}

void playbar_on_bar_rec(PlayBar* self, psy_ui_Button* sender)
{
	assert(self);

	if (self->workspace->player_.follow_song && psy_property_item_bool(
			&self->workspace->edit_mode)) {
		psy_property_set_item_bool(&self->workspace->edit_mode, FALSE);
	} else {
		psy_Property* p;

		p = psy_configuration_at(psycleconfig_misc(workspace_cfg(self->workspace)),
			"followsong");
		if (p) {
			psy_property_set_item_bool(p, TRUE);
		}
		psy_property_set_item_bool(&self->workspace->edit_mode, TRUE);		
	}	
	playbar_on_timer(self, 0);
}
