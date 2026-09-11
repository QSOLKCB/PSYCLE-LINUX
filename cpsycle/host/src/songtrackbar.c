/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "songtrackbar.h"
/* platform */
#include "../../detail/portable.h"


#define MIN_TRACKS 4


/* prototypes */
static void songtrackbar_build(SongTrackBar*);
static void songtrackbar_on_sel_change(SongTrackBar*, psy_ui_ComboBox* sender);
static void songtrackbar_on_num_song_tracks_changed(SongTrackBar*,
	psy_audio_Patterns* sender);
static void songtrackbar_on_song_changed(SongTrackBar*, psy_audio_Player* sender);
static void songtrackbar_connect_song(SongTrackBar*, psy_audio_Song*);

/* implementation */
void songtrackbar_init(SongTrackBar* self, psy_ui_Component* parent,
	Workspace* workspace)
{		
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace_ = workspace;
	psy_ui_component_set_align_expand(songtrackbar_base(self), psy_ui_HEXPAND);
	psy_ui_component_set_default_align(songtrackbar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_label_init(&self->desc_, songtrackbar_base(self));
	psy_ui_label_set_text(&self->desc_, "trackbar.tracks");
	psy_ui_combobox_init(&self->track_numbers_, songtrackbar_base(self));
	psy_ui_combobox_set_char_number(&self->track_numbers_, 3.0);
	psy_ui_component_set_tab_index(psy_ui_combobox_base(&self->track_numbers_), 0);
	songtrackbar_build(self);	
	psy_signal_connect(&self->track_numbers_.signal_selchanged, self,
		songtrackbar_on_sel_change);
	songtrackbar_connect_song(self, workspace_song(workspace));	
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		songtrackbar_on_song_changed);	
}

void songtrackbar_build(SongTrackBar* self)
{
	uintptr_t track;
	char text[64];

	assert(self);

	for (track = MIN_TRACKS; track < 65; ++track) {
		psy_snprintf(text, 64, "%d", (int)track);
		psy_ui_combobox_add_text(&self->track_numbers_, text);
	}	
	if (workspace_song(self->workspace_)) {
		psy_ui_combobox_select(&self->track_numbers_,
			psy_audio_song_num_song_tracks(
					workspace_song(self->workspace_)) -
				MIN_TRACKS);
	}
}

void songtrackbar_on_sel_change(SongTrackBar* self, psy_ui_ComboBox* sender)	
{		
	assert(self);

	if (workspace_song(self->workspace_)) {
		psy_audio_song_set_num_song_tracks(workspace_song(self->workspace_),
			psy_ui_combobox_cur_sel(sender) + MIN_TRACKS);
	}
}

void songtrackbar_on_num_song_tracks_changed(SongTrackBar* self,
	psy_audio_Patterns* sender)
{	
	assert(self);	
	
	psy_ui_combobox_select(&self->track_numbers_,
		psy_audio_patterns_num_tracks(sender) - MIN_TRACKS);
}

void songtrackbar_on_song_changed(SongTrackBar* self, psy_audio_Player* sender)
{	
	assert(self);

	if (psy_audio_player_song(sender)) {
		psy_ui_combobox_select(&self->track_numbers_,
			psy_audio_song_num_song_tracks(psy_audio_player_song(sender)) - MIN_TRACKS);
		songtrackbar_connect_song(self, psy_audio_player_song(sender));		
		songtrackbar_on_num_song_tracks_changed(self, psy_audio_song_patterns(
			psy_audio_player_song(sender)));
	}
}

void songtrackbar_connect_song(SongTrackBar* self, psy_audio_Song* song)
{
	assert(self);

	if (!song) {
		return;
	}
	psy_signal_connect(
		&psy_audio_song_patterns(song)->signal_numsongtrackschanged,
		self, songtrackbar_on_num_song_tracks_changed);
}
