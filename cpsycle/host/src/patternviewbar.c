/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewbar.h"
/* local */
#include "patternview.h"
/* platform */
#include "../../detail/portable.h"


/* PatternViewStatus */

/* prototypes */
static void patternviewstatus_on_octave_changed(PatternViewStatus*,
	psy_audio_Player* sender);
static void patternviewstatus_on_edit_mode(PatternViewStatus*,
	psy_Property* sender);
static void patternviewstatus_on_follow_song(PatternViewStatus*,
	psy_Property* sender);

/* implementation */
void patternviewstatus_init(PatternViewStatus* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 0.5, 0.0, 0.0));
	/* pattern index */
	psy_ui_label_init(&self->pat_desc, &self->component);
	psy_ui_label_prevent_translation(&self->pat_desc);
	psy_ui_label_set_text(&self->pat_desc, "Pat");
	psy_ui_label_init(&self->pat, &self->component);
	psy_ui_label_prevent_translation(&self->pat);
	psy_ui_label_set_char_number(&self->pat, 3.0);
	/* line */
	psy_ui_label_init(&self->ln_desc, &self->component);
	psy_ui_label_prevent_translation(&self->ln_desc);
	psy_ui_label_set_text(&self->ln_desc, "Ln");
	psy_ui_label_init(&self->ln, &self->component);
	psy_ui_label_prevent_translation(&self->ln);
	psy_ui_label_set_char_number(&self->ln, 4.0);
	/* track */
	psy_ui_label_init(&self->trk_desc, &self->component);
	psy_ui_label_prevent_translation(&self->trk_desc);
	psy_ui_label_set_text(&self->trk_desc, "Trk");
	psy_ui_label_init(&self->trk, &self->component);
	psy_ui_label_prevent_translation(&self->trk);
	psy_ui_label_set_char_number(&self->trk, 2.0);
	/* col */
	psy_ui_label_init(&self->col_desc, &self->component);
	psy_ui_label_prevent_translation(&self->col_desc);
	psy_ui_label_set_text(&self->col_desc, "Col");
	psy_ui_label_init(&self->col, &self->component);
	psy_ui_label_prevent_translation(&self->col);
	psy_ui_label_set_char_number(&self->col, 4.0);
	/* oct */
	psy_ui_label_init(&self->oct_desc, &self->component);
	psy_ui_label_prevent_translation(&self->oct_desc);
	psy_ui_label_set_text(&self->oct_desc, "Oct");
	psy_ui_label_init(&self->oct, &self->component);
	psy_ui_label_prevent_translation(&self->oct);
	psy_ui_label_set_char_number(&self->oct, 2.0);
	psy_signal_connect(&workspace_player(self->workspace)->signal_octavechanged,
		self, patternviewstatus_on_octave_changed);
	/* edit mode */	
	psy_ui_label_init(&self->mode, &self->component);
	psy_ui_label_prevent_translation(&self->mode);
	psy_ui_label_set_char_number(&self->mode, 4.0);
	psy_property_connect(&workspace->edit_mode, self,
		patternviewstatus_on_edit_mode);
	/* follow song */
	psy_ui_label_init(&self->follow, &self->component);
	psy_ui_label_prevent_translation(&self->follow);
	psy_ui_label_set_char_number(&self->follow, 6.0);	
	psy_configuration_connect(psycleconfig_misc(workspace_cfg(self->workspace)),
		"followsong", self, patternviewstatus_on_follow_song);	
}

void patternviewstatus_update(PatternViewStatus* self)
{
	psy_audio_SequenceCursor cursor;
	uintptr_t patternid;
	char text[256];

	assert(self);
	assert(self->workspace);

	patternid = psy_INDEX_INVALID;
	if (workspace_song(self->workspace)) {
		cursor = self->workspace->song->sequence_.cursor;
		patternid = psy_audio_sequencecursor_pattern_id(&cursor,
			psy_audio_song_sequence(workspace_song(self->workspace)));
	}
	if (patternid == psy_INDEX_INVALID || !workspace_song(self->workspace)) {
		psy_ui_label_set_text(&self->pat, "--");
		psy_ui_label_set_text(&self->ln, "--");
		psy_ui_label_set_text(&self->trk, "--");
		psy_ui_label_set_text(&self->col, "--");		
	} else {		
		uintptr_t line;

		if (psy_audio_player_playing(&self->workspace->player_)) {
			patternid = psy_audio_sequencecursor_pattern_id(
				&self->workspace->player_.sequencer.hostseqtime.currplaycursor,
				psy_audio_song_sequence(workspace_song(self->workspace)));
			line = psy_audio_sequencecursor_line(&self->workspace->player_.sequencer.hostseqtime.currplaycursor);
			// line -= psy_audio_sequencecursor_seqline(&self->workspace->player.sequencer.hostseqtime.currplaycursor);
		} else {
			line = psy_audio_sequencecursor_line(&cursor);
			// line -= psy_audio_sequencecursor_seqline(&cursor);
		}
		psy_snprintf(text, 256, "%d", (int)patternid);
		psy_ui_label_set_text(&self->pat, text);
		psy_snprintf(text, 256, "%d", (int)line);		
		psy_ui_label_set_text(&self->ln, text);
		psy_snprintf(text, 256, "%d",
			(int)psy_audio_sequencecursor_channel(&cursor));
		psy_ui_label_set_text(&self->trk, text);
		psy_snprintf(text, 256, "%d: %d",
			(int)psy_audio_sequencecursor_column(&cursor),
			(int)psy_audio_sequencecursor_digit(&cursor));
		psy_ui_label_set_text(&self->col, text);		
	}
	psy_snprintf(text, 256, "%d", (int)psy_audio_player_octave(workspace_player(
		self->workspace)));		
	psy_ui_label_set_text(&self->oct, text);
	if (psy_audio_player_edit_mode(workspace_player(self->workspace))) {
		psy_ui_label_set_text(&self->mode, "Edt");
	} else {
		psy_ui_label_set_text(&self->mode, "");
	}	
	if (self->workspace->player_.follow_song) {
		psy_ui_label_set_text(&self->follow, "Follow");
	} else {
		psy_ui_label_set_text(&self->follow, "");
	}
}

void patternviewstatus_on_octave_changed(PatternViewStatus* self,
	psy_audio_Player* sender)
{
	assert(self);

	patternviewstatus_update(self);
}

void patternviewstatus_on_edit_mode(PatternViewStatus* self, psy_Property* sender)
{
	assert(self);

	patternviewstatus_update(self);	
}

void patternviewstatus_on_follow_song(PatternViewStatus* self, psy_Property* sender)
{
	assert(self);

	patternviewstatus_update(self);
}


/* PatternViewBar */

/* prototypes */
static void patternviewbar_on_cursor_changed(PatternViewBar*,
	psy_audio_Sequence* sender);
static void patternviewbar_on_playline_changed(PatternViewBar*, Workspace*
	sender);
static void patternviewbar_on_playstatus_changed(PatternViewBar*, Workspace*
	sender);
static void patternviewbar_on_song_changed(PatternViewBar*,
	psy_audio_Player* sender);
static void patternviewbar_connect_song(PatternViewBar*);
static void patternviewbar_update_status(PatternViewBar*);

/* implementation */
void patternviewbar_init(PatternViewBar* self, psy_ui_Component* parent,
	psy_Configuration* patconfig, Workspace* workspace)
{		
	assert(self);
	assert(patconfig);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);	
	self->patconfig = patconfig;
	self->workspace = workspace;	
	psy_ui_component_set_default_align(patternviewbar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_defaults_hmargin(psy_ui_defaults()));	
	/* zoom */
	zoombox_init(&self->zoombox, patternviewbar_base(self));		
	zoombox_exchange(&self->zoombox, psy_configuration_at(self->patconfig,
		"zoom"));
	/* cursor step */
	patterncursorstepbox_init(&self->cursorstep, &self->component, workspace);	
	/* Move cursor when paste */
	psy_ui_checkbox_init(&self->movecursorwhenpaste, patternviewbar_base(self));
	psy_ui_checkbox_prevent_wrap(&self->movecursorwhenpaste);
	psy_ui_checkbox_set_text(&self->movecursorwhenpaste,
		"settings.pv.move-cursor-when-paste");	
	psy_ui_checkbox_exchange(&self->movecursorwhenpaste,
		psy_configuration_at(self->patconfig,
		"movecursorwhenpaste"));
	/* Default line */
	psy_ui_checkbox_init(&self->defaultentries, patternviewbar_base(self));
	psy_ui_checkbox_prevent_wrap(&self->defaultentries);
	psy_ui_checkbox_set_text(&self->defaultentries,
		"settings.visual.default-line");
	psy_ui_checkbox_exchange(&self->defaultentries,
		psy_configuration_at(self->patconfig,
		"griddefaults"));	
	/* Single pattern display mode */
	psy_ui_checkbox_init(&self->displaysinglepattern,
		patternviewbar_base(self));
	psy_ui_checkbox_prevent_wrap(&self->displaysinglepattern);
	psy_ui_checkbox_set_text(&self->displaysinglepattern,
		"settings.pv.displaysinglepattern");
	psy_ui_checkbox_exchange(&self->displaysinglepattern,
		psy_configuration_at(self->patconfig,
		"displaysinglepattern"));
	patternviewstatus_init(&self->status, patternviewbar_base(self), workspace);
	psy_ui_sizer_init(&self->sizer_, patternviewbar_base(self));
	psy_ui_component_hide(psy_ui_sizer_base(&self->sizer_));
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		patternviewbar_on_song_changed);	
	patternviewbar_update_status(self);
	patternviewbar_connect_song(self);
	psy_signal_connect(&self->workspace->player_.sequencer.signal_play_line_changed, self,
		patternviewbar_on_playline_changed);
	psy_signal_connect(&self->workspace->player_.sequencer.signal_play_status_changed, self,
		patternviewbar_on_playstatus_changed);
}

void patternviewbar_on_song_changed(PatternViewBar* self,
	psy_audio_Player* sender)
{
	assert(self);

	patternviewbar_connect_song(self);
	patternviewbar_update_status(self);
}

void patternviewbar_connect_song(PatternViewBar* self)
{
	assert(self);
	assert(self->workspace);

	if (workspace_song(self->workspace)) {
		psy_signal_connect(
			&workspace_song(self->workspace)->sequence_.signal_cursor_changed,
			self, patternviewbar_on_cursor_changed);
	}
}

void patternviewbar_on_cursor_changed(PatternViewBar* self,
	psy_audio_Sequence* sender)
{
	assert(self);

	patternviewbar_update_status(self);	
}

void patternviewbar_on_playline_changed(PatternViewBar* self,
	Workspace* sender)
{		
	assert(self);

	if (!self->workspace->player_.follow_song) {
		patternviewbar_update_status(self);
	} /* else follow song handled with patternviewbar_on_cursor_changed */
}

void patternviewbar_on_playstatus_changed(PatternViewBar* self,
	Workspace* sender)
{
	assert(self);

	patternviewbar_update_status(self);
}

void patternviewbar_update_status(PatternViewBar* self)
{
	assert(self);
	
	patternviewstatus_update(&self->status);
}
