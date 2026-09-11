/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "songproperties.h"
/* host */
#include "styles.h"
/* audio */
#include <songio.h>
/* std */
#include <ctype.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void songpropertiesview_init_component(SongPropertiesView*,
	psy_ui_Component* tabbarparent);
static void songpropertiesview_init_title(SongPropertiesView*);
static void songpropertiesview_init_credits(SongPropertiesView*);
static void songpropertiesview_init_settings(SongPropertiesView*);
static void songpropertiesview_init_comments(SongPropertiesView*);
static void songpropertiesview_init_prevent_edit(SongPropertiesView*);
static void songpropertiesview_read(SongPropertiesView*);
static void songpropertiesview_on_song_changed(SongPropertiesView*,
	psy_audio_Player* sender);
static void songpropertiesview_on_edit_accept(SongPropertiesView*,
	psy_ui_Edit* sender);
static void songpropertiesview_on_edit_reject(SongPropertiesView*,
	psy_ui_Edit* sender);
static void songpropertiesview_on_filter_keys(SongPropertiesView*,
	psy_ui_Component* sender, psy_ui_KeyboardEvent*);
static void songpropertiesview_on_tempo_edit_change(SongPropertiesView*,
	psy_ui_NumberEdit* sender);
static void songpropertiesview_on_lpb_edit_change(SongPropertiesView*,
	psy_ui_NumberEdit* sender);
static void songpropertiesview_on_tpb_edit_change(SongPropertiesView*,
	psy_ui_NumberEdit* sender);
static void songpropertiesview_on_etpb_edit_change(SongPropertiesView*,
	psy_ui_NumberEdit* sender);
static void songpropertiesview_on_sampler_index_change(SongPropertiesView*,
	psy_ui_NumberEdit* sender);
static void songpropertiesview_on_comments_changed(SongPropertiesView*,
	psy_ui_Component* sender);
static void songpropertiesview_on_key_down(SongPropertiesView*,
	psy_ui_Component* sender, psy_ui_KeyboardEvent*);
static void songpropertiesview_on_key_up(SongPropertiesView*,
	psy_ui_Component* sender, psy_ui_KeyboardEvent*);
static bool songpropertiesview_has_edit_focus(SongPropertiesView*);
static int songpropertiesview_real_bpm(SongPropertiesView*);
static void songpropertiesview_update_real_speed(SongPropertiesView*);
static void songpropertiesview_enable_edit(SongPropertiesView*);
static void songpropertiesview_prevent_edit(SongPropertiesView*);
static void songpropertiesview_select_section(SongPropertiesView*,
	psy_ui_Component* sender, uintptr_t section, uintptr_t options);
static void songpropertiesview_on_prevent_edit(SongPropertiesView*,
	psy_ui_Button* sender);
static void songpropertiesview_on_auto_note_off(SongPropertiesView*,
	psy_ui_Button* sender);

/* implementation */
void songpropertiesview_init(SongPropertiesView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace* workspace)
{			
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	self->song = workspace_song(workspace);
	self->workspace = workspace;
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		songpropertiesview_on_song_changed);
	songpropertiesview_init_component(self, tabbarparent);
	songpropertiesview_init_title(self);
	songpropertiesview_init_credits(self);
	songpropertiesview_init_settings(self);
	songpropertiesview_init_comments(self);
	songpropertiesview_init_prevent_edit(self);					
	songpropertiesview_read(self);
}

void songpropertiesview_init_component(SongPropertiesView* self,
	psy_ui_Component* tabbarparent)
{
	assert(self);
		
	psy_ui_component_set_id(songpropertiesview_base(self),
		VIEW_ID_SONGPROPERTIES);
	psy_ui_component_set_title(songpropertiesview_base(self),
		"main.properties");
	psy_ui_component_set_style_type(&self->component, STYLE_CLIENT_VIEW);
	psy_ui_component_init(&self->viewtabbar, tabbarparent, NULL);
	psy_ui_component_set_id(&self->viewtabbar, VIEW_ID_SONGPROPERTIES);
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_em(0.5, 1.0, 0.5, 1.0));
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_signal_connect(&self->component.signal_select_section, self,
		songpropertiesview_select_section);
	psy_signal_connect(&self->component.signal_key_down, self,
		songpropertiesview_on_key_down);
	psy_signal_connect(&self->component.signal_key_up, self,
		songpropertiesview_on_key_up);	
}

void songpropertiesview_init_title(SongPropertiesView* self)
{
	assert(self);
	
	psy_ui_label_init_text(&self->label_title, &self->component,
		"songprop.title");	
	psy_ui_component_set_margin(&self->label_title.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 1.0));
	psy_ui_edit_init(&self->edit_title, &self->component);
	psy_ui_component_set_style_type(&self->edit_title.component,
		STYLE_BOX_BRIGHT);		
	psy_ui_component_set_padding(&self->edit_title.component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 1.0));
	psy_ui_edit_enable_input_field(&self->edit_title);	
	psy_signal_connect(&self->edit_title.signal_accept, self,
		songpropertiesview_on_edit_accept);
	psy_signal_connect(&self->edit_title.signal_reject, self,
		songpropertiesview_on_edit_reject);
	psy_signal_connect(&self->edit_title.component.signal_key_down, self,
		songpropertiesview_on_filter_keys);
}

void songpropertiesview_init_credits(SongPropertiesView* self)
{
	assert(self);

	psy_ui_label_init_text(&self->label_credits, &self->component,
		"songprop.credits");
	psy_ui_component_set_margin(&self->label_credits.component,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 1.0));
	psy_ui_label_set_char_number(&self->label_credits, 12.0);	
	psy_ui_edit_init(&self->edit_credits, &self->component);
	psy_ui_component_set_style_type(&self->edit_credits.component,
		STYLE_BOX_BRIGHT);		
	psy_ui_component_set_padding(&self->edit_credits.component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 1.0));
	psy_ui_edit_enable_input_field(&self->edit_credits);
	psy_ui_component_set_align(&self->edit_credits.component, psy_ui_ALIGN_TOP);	
	psy_signal_connect(&self->edit_credits.signal_accept, self,
		songpropertiesview_on_edit_accept);
	psy_signal_connect(&self->edit_credits.signal_reject, self,
		songpropertiesview_on_edit_reject);
	psy_signal_connect(&self->edit_credits.component.signal_key_down, self,
		songpropertiesview_on_filter_keys);
}

void songpropertiesview_init_settings(SongPropertiesView* self)
{
	assert(self);

	psy_ui_component_init(&self->speedbar, &self->component, NULL);
	psy_ui_component_set_margin(&self->speedbar,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 1.0));
	psy_ui_component_set_default_align(&self->speedbar, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	/* SamplerIndex */
	psy_ui_numberedit_init_connect(&self->samplerindex, &self->speedbar,
		"Sampler Index", 0x3E, 0, 0x3F,
		self, songpropertiesview_on_sampler_index_change);	
	/* auto note off */
	psy_ui_button_init_check_connect(&self->auto_note_off, &self->speedbar,
		"Generate Auto Note Offs",
		self, songpropertiesview_on_auto_note_off);
	/* Tempo */
	psy_ui_numberedit_init_connect(&self->tempo, &self->speedbar,
		"songprop.tempo",
		32, 32, 999, self, songpropertiesview_on_tempo_edit_change);
	psy_ui_numberedit_init_connect(&self->lpb, &self->speedbar,
		"songprop.lpb",
		1, 1, 32, self, songpropertiesview_on_lpb_edit_change);
	psy_ui_numberedit_init_connect(&self->tpb, &self->speedbar,
		"songprop.tpb",
		1, 1, 99, self, songpropertiesview_on_tpb_edit_change);
	psy_ui_numberedit_init_connect(&self->etpb, &self->speedbar,
		"songprop.etpb",
		0, 0, 99, self, songpropertiesview_on_etpb_edit_change);	
	/* Real Speed */
	psy_ui_component_init(&self->realtempo, &self->speedbar, NULL);
	psy_ui_component_set_default_align(&self->realtempo, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_component_set_align_expand(&self->realtempo, psy_ui_HEXPAND);
	psy_ui_label_init_text(&self->realtempo_desc, &self->realtempo,
		"songprop.realtempo");
	psy_ui_label_init(&self->realtempo_value, &self->realtempo);
	psy_ui_label_set_char_number(&self->realtempo_value, 8);
	psy_ui_label_init_text(&self->realticksperbeat_desc, &self->speedbar,
		"songprop.realtpb");	
	psy_ui_label_init(&self->realticksperbeat, &self->speedbar);
	psy_ui_label_set_char_number(&self->realticksperbeat, 8);
}

void songpropertiesview_init_comments(SongPropertiesView* self)
{
	assert(self);
	
	psy_ui_label_init_text(&self->label_comments, &self->component,
		"songprop.extcomments");	
	psy_ui_component_set_margin(&self->label_comments.component,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 1.0));
	psy_ui_textarea_init(&self->edit_comments, &self->component);
	psy_ui_component_set_margin(&self->edit_comments.component,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_style_type(&self->edit_comments.component,
		STYLE_BOX_BRIGHT);		
	psy_ui_component_set_padding(&self->edit_comments.component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 1.0));
	psy_ui_component_set_align(&self->edit_comments.component,
		psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->edit_comments.pane.signal_change, self,
		songpropertiesview_on_comments_changed);
	psy_signal_connect(&self->edit_comments.component.signal_key_down, self,
		songpropertiesview_on_filter_keys);
}

void songpropertiesview_init_prevent_edit(SongPropertiesView* self)
{
	assert(self);

	psy_ui_component_init(&self->bottom, &self->component, NULL);
	psy_ui_component_set_align(&self->bottom, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_default_align(&self->bottom,
		psy_ui_ALIGN_LEFT, psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_button_init_check_connect(&self->prevent_edit, &self->bottom,
		"songprop.preventedit", self, songpropertiesview_on_prevent_edit);	
	psy_ui_sizer_init(&self->sizer, &self->bottom);	
	psy_ui_component_hide(psy_ui_sizer_base(&self->sizer));
	self->component.sizer_ = psy_ui_sizer_base(&self->sizer);
}

void songpropertiesview_read(SongPropertiesView* self)
{	
	assert(self);
	
	if (!self->song) {
		return;
	}
	psy_ui_edit_set_text(&self->edit_title,			
		psy_audio_song_title(self->song));
	psy_ui_edit_set_text(&self->edit_credits,			
		psy_audio_song_credits(self->song));
	psy_ui_textarea_set_text(&self->edit_comments,
		psy_audio_song_comments(self->song));		
	psy_ui_numberedit_set_value(&self->tempo,
		(int)psy_audio_song_bpm(self->song));
	psy_ui_numberedit_set_value(&self->lpb,
		(int)psy_audio_song_lpb(self->song));
	psy_ui_numberedit_set_value(&self->tpb,
		(int)psy_audio_song_tpb(self->song));	
	psy_ui_numberedit_set_value(&self->etpb,		
		(int)psy_audio_song_extra_ticks_per_beat(self->song));			
	psy_ui_numberedit_set_value(&self->samplerindex,
		(int)psy_audio_song_sampler_index(self->song));			
	songpropertiesview_update_real_speed(self);
	if (psy_audio_song_auto_note_off(self->song)) {
		psy_ui_button_highlight(&self->auto_note_off);
	} else {
		psy_ui_button_disable_highlight(&self->auto_note_off);
	}	
}

void songpropertiesview_on_song_changed(SongPropertiesView* self,
	psy_audio_Player* sender)
{
	assert(self);
		
	self->song = psy_audio_player_song(sender);
	songpropertiesview_read(self);
}

void songpropertiesview_enable_edit(SongPropertiesView* self)
{
	assert(self);
	
	psy_ui_edit_enable(&self->edit_title);
	psy_ui_edit_enable(&self->edit_credits);
	psy_ui_textarea_enable(&self->edit_comments);
	psy_ui_numberedit_enable(&self->tpb);
	psy_ui_numberedit_enable(&self->etpb);	
	psy_ui_button_disable_highlight(&self->prevent_edit);
}

void songpropertiesview_prevent_edit(SongPropertiesView* self)
{
	assert(self);
	
	psy_ui_edit_prevent(&self->edit_title);
	psy_ui_edit_prevent(&self->edit_credits);
	psy_ui_textarea_prevent(&self->edit_comments);
	psy_ui_numberedit_prevent(&self->tpb);
	psy_ui_numberedit_prevent(&self->etpb);	
	psy_ui_button_highlight(&self->prevent_edit);
}

void songpropertiesview_on_edit_accept(SongPropertiesView* self,
	psy_ui_Edit* sender)
{
	assert(self);

	if (self->song) {
		if (psy_strlen(psy_ui_edit_text(sender)) == 0) {
			if (sender == &self->edit_title) {
				psy_ui_edit_set_text(sender, "Untitled");
			}
		}
		if (sender == &self->edit_title) {
			psy_audio_song_set_title(self->song, psy_ui_edit_text(sender));
		}
		else if (sender == &self->edit_credits) {
			psy_audio_song_set_credits(self->song,
				psy_ui_edit_text(sender));
		}
	}
	psy_ui_component_set_focus(psy_ui_component_parent(&self->component));
}

void songpropertiesview_on_edit_reject(SongPropertiesView* self,
	psy_ui_Edit* sender)
{
	assert(self);

	if (self->song) {
		if (sender == &self->edit_title) {
			psy_ui_edit_set_text(&self->edit_title,
				psy_audio_song_title(self->song));
		}
		else if (sender == &self->edit_credits) {
			psy_ui_edit_set_text(&self->edit_credits,
				psy_audio_song_credits(self->song));
		}
	}	
	psy_ui_component_set_focus(psy_ui_component_parent(&self->component));
}

void songpropertiesview_on_filter_keys(SongPropertiesView* self,
	psy_ui_Component* sender, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN &&
			sender != &self->edit_comments.component) {
		psy_ui_component_set_focus(&self->component);		
		psy_ui_keyboardevent_prevent_default(ev);
	} else
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		psy_ui_component_set_focus(&self->component);
		psy_ui_keyboardevent_prevent_default(ev);
	}	
}

void songpropertiesview_on_tempo_edit_change(SongPropertiesView* self,
	psy_ui_NumberEdit* sender)
{
	assert(self);
	
	psy_audio_player_set_bpm(workspace_player(self->workspace),
		psy_ui_numberedit_value(sender));
	songpropertiesview_update_real_speed(self);
}

void songpropertiesview_on_lpb_edit_change(SongPropertiesView* self,
	psy_ui_NumberEdit* sender)
{
	assert(self);
	
	psy_audio_player_set_lpb(workspace_player(self->workspace), (uintptr_t)
		psy_ui_numberedit_value(sender));
	songpropertiesview_update_real_speed(self);
}

void songpropertiesview_on_tpb_edit_change(SongPropertiesView* self,
	psy_ui_NumberEdit* sender)
{
	assert(self);
	
	psy_audio_player_setticksperbeat(workspace_player(self->workspace),
		(uintptr_t)psy_ui_numberedit_value(sender));
	songpropertiesview_update_real_speed(self);
}

void songpropertiesview_on_etpb_edit_change(SongPropertiesView* self,
	psy_ui_NumberEdit* sender)
{
	assert(self);
	
	psy_audio_player_set_extra_ticks_per_beat(workspace_player(self->workspace),
		(uintptr_t)psy_ui_numberedit_value(sender));
	songpropertiesview_update_real_speed(self);
}

void songpropertiesview_on_sampler_index_change(SongPropertiesView* self,
	psy_ui_NumberEdit* sender)
{
	assert(self);
	
	psy_audio_player_set_sampler_index(workspace_player(self->workspace),
		(uintptr_t)psy_ui_numberedit_value(sender));	
}

void songpropertiesview_on_comments_changed(SongPropertiesView* self,
	psy_ui_Component* sender)
{
	assert(self);
	
	psy_audio_song_set_comments(self->song,	
		psy_ui_textarea_text(&self->edit_comments));
}

void songpropertiesview_on_key_down(SongPropertiesView* self,
	psy_ui_Component* sender, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	if (songpropertiesview_has_edit_focus(self)) {		
		psy_ui_keyboardevent_stop_propagation(ev);
	}
}

bool songpropertiesview_has_edit_focus(SongPropertiesView* self)
{
	assert(self);
	
	return (psy_ui_component_has_focus(&self->edit_comments.component) ||
		psy_ui_component_has_focus(&self->edit_credits.component) ||
		psy_ui_component_has_focus(&self->edit_title.component));
		/* psy_ui_component_has_focus(&self->edit_tpb.component)); */		
}

void songpropertiesview_on_key_up(SongPropertiesView* self,
	psy_ui_Component* sender, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	psy_ui_keyboardevent_stop_propagation(ev);
}

int songpropertiesview_real_bpm(SongPropertiesView* self)
{
	assert(self);
	
	return (int)((psy_audio_song_bpm(self->song) *
		psy_audio_song_tpb(self->song)) /
		(double)(psy_audio_song_extra_ticks_per_beat(self->song) *
			psy_audio_song_lpb(self->song) +
			psy_audio_song_tpb(self->song)));
}

void songpropertiesview_update_real_speed(SongPropertiesView* self)
{
	char text[128];
	
	assert(self);

	psy_snprintf(text, 128, "%d", (int)
		songpropertiesview_real_bpm(self));
	psy_ui_label_set_text(&self->realtempo_value, text);
	psy_snprintf(text, 128, "%d", (int)
		psy_audio_song_real_tpb(self->song));
	psy_ui_label_set_text(&self->realticksperbeat, text);
}

void songpropertiesview_select_section(SongPropertiesView* self,
	psy_ui_Component* sender, uintptr_t section, uintptr_t options)
{
	assert(self);
	
	if (section == SECTION_ID_SONGPROPERTIES_VIEW) {
		psy_ui_button_highlight(&self->prevent_edit);
		songpropertiesview_prevent_edit(self);
	}
}

void songpropertiesview_on_prevent_edit(SongPropertiesView* self,
	psy_ui_Button* sender)
{
	assert(self);
			
	if (psy_ui_button_highlighted(sender)) {
		songpropertiesview_enable_edit(self);		
	} else {
		songpropertiesview_prevent_edit(self);
	}
}

void songpropertiesview_on_auto_note_off(SongPropertiesView* self,
	psy_ui_Button* sender)
{
	assert(self);

	if (psy_ui_button_highlighted(&self->auto_note_off)) {
		psy_ui_button_disable_highlight(&self->auto_note_off);
		psy_audio_song_disable_auto_note_off(self->song);
	} else {
		psy_audio_song_enable_auto_note_off(self->song);
		psy_ui_button_highlight(&self->auto_note_off);
	}	
	self->workspace->player_.sequencer.auto_note_off =
		psy_audio_song_auto_note_off(self->song);		
}
