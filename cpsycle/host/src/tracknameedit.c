/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "tracknameedit.h"
/* host */
#include "styles.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* platform */
#include "../../detail/portable.h"


static void headerstyleswitch_on_check(HeaderStyleSwitch*,
	psy_ui_Button* sender);
static void headerstyleswitch_on_header_classic(HeaderStyleSwitch*,
	psy_Property* sender);

/* implementation */
void headerstyleswitch_init(HeaderStyleSwitch* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_style_type(&self->component, STYLE_BOX_BRIGHT);
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 1.0));
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));
	psy_ui_label_init_text(&self->label, &self->component,
		"pv.headerlabel");
	psy_ui_label_set_char_number(&self->label, 30.0);
	psy_ui_label_enable_wrap(&self->label);
	psy_ui_button_init_check_connect(&self->classic, &self->component,
		"pv.headermodeclassic",
		self, headerstyleswitch_on_check);
	psy_ui_button_init_check_connect(&self->text, &self->component,
		"pv.headermodetext",
		self, headerstyleswitch_on_check);
	psy_ui_label_init_text(&self->hint, &self->component,
		"pv.headerhint");
	psy_ui_label_set_char_number(&self->hint, 30.0);
	psy_ui_label_enable_wrap(&self->hint);
	psy_configuration_connect(
		psycleconfig_patview(workspace_cfg(self->workspace)),
		"theme.pattern_header_classic",
		self, headerstyleswitch_on_header_classic);
	psy_ui_button_highlight(&self->classic);
}

void headerstyleswitch_on_check(HeaderStyleSwitch* self,
	psy_ui_Button* sender)
{
	psy_Property* p;

	assert(self);

	if (sender == &self->classic) {
		psy_ui_button_highlight(&self->classic);
		psy_ui_button_disable_highlight(&self->text);
	} else {
		psy_ui_button_highlight(&self->text);
		psy_ui_button_disable_highlight(&self->classic);
	}
	p = psy_configuration_at(
			psycleconfig_patview(workspace_cfg(self->workspace)),
			"theme.pattern_header_classic");
	if (p) {		
		psy_property_set_item_bool(p, (sender == &self->classic));
	}
}

void headerstyleswitch_on_header_classic(HeaderStyleSwitch* self,
	psy_Property* sender)
{
	assert(self);

	if (psy_property_item_bool(sender)) {
		psy_ui_button_highlight(&self->classic);
		psy_ui_button_disable_highlight(&self->text);		
	} else {
		psy_ui_button_highlight(&self->text);
		psy_ui_button_disable_highlight(&self->classic);		
	}	
}


/* TrackNaming */

/* prototypes */
void tracknaming_on_share(TrackNaming*, psy_ui_Button* sender);

/* implementation */
void tracknaming_init(TrackNaming* self, psy_ui_Component* parent,
	PatternViewState* state)
{
	assert(self);
	assert(state);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	self->state = state;
	psy_ui_component_set_style_type(&self->component, STYLE_BOX_BRIGHT);
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 1.0));
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));	
	psy_ui_button_init_check_connect(&self->names_share, &self->component,
		"pv.tracknames_use_same",
		self, tracknaming_on_share);
	psy_ui_button_init_check_connect(&self->names_individual, &self->component,
		"pv.tracknames_use_individual",
		self, tracknaming_on_share);
	psy_ui_button_highlight(&self->names_share);
}

void tracknaming_on_share(TrackNaming* self, psy_ui_Button* sender)
{
	psy_audio_Patterns* patterns;
	
	assert(self);
		
	patterns = patternviewstate_patterns(self->state);		
	if (sender == &self->names_share) {
		psy_ui_button_highlight(&self->names_share);
		psy_ui_button_disable_highlight(&self->names_individual);
		if (patterns) {
			psy_audio_patterns_share_names(patterns);
		}		
	} else {
		psy_ui_button_highlight(&self->names_individual);
		psy_ui_button_disable_highlight(&self->names_share);
		if (patterns) {
			psy_audio_patterns_use_individual_names(patterns);
		}
	}
}

/* TrackNameEdit */

/* prototypes */
static void tracknameedit_on_key_down(TrackNameEdit*, psy_ui_KeyboardEvent*);
static void tracknameedit_on_key_up(TrackNameEdit*, psy_ui_KeyboardEvent*);
static void tracknameedit_on_focus(TrackNameEdit*);
static void tracknameedit_on_edit_changed(TrackNameEdit*,
	psy_ui_Edit* sender);
static void tracknameedit_connect_song(TrackNameEdit*, psy_audio_Song*);
static void tracknameedit_on_cursor_changed(TrackNameEdit*,
	psy_audio_Sequence* sender);	
static void tracknameedit_on_song_changed(TrackNameEdit*,
	psy_audio_Player* sender);
static void tracknameedit_select(TrackNameEdit*, uintptr_t pattern_index);
static void tracknameedit_on_selected(TrackNameEdit*, psy_ui_ListBox* sender);
static void tracknameedit_fill(TrackNameEdit*);
static void tracknameedit_on_track_name_changed(TrackNameEdit*,
	psy_audio_Patterns* sender, uintptr_t track_index);

/* vtable */
static psy_ui_ComponentVtable tracknameedit_vtable;
static bool tracknameedit_vtable_initialized = FALSE;

static void tracknameedit_vtable_init(TrackNameEdit* self)
{
	assert(self);
	
	if (!tracknameedit_vtable_initialized) {
		tracknameedit_vtable = *(self->component.vtable);
		tracknameedit_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			tracknameedit_on_key_down;
		tracknameedit_vtable.onkeyup =
			(psy_ui_fp_component_on_key_event)
			tracknameedit_on_key_up;
		tracknameedit_vtable.on_focus =
			(psy_ui_fp_component)
			tracknameedit_on_focus;
		tracknameedit_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &tracknameedit_vtable);		
}

/* implementation */
void tracknameedit_init(TrackNameEdit* self, psy_ui_Component* parent,
	PatternViewState* state, Workspace* workspace)
{	
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	tracknameedit_vtable_init(self);
	psy_ui_component_set_style_type(&self->component, STYLE_SIDE_VIEW);
	self->pattern_index = psy_INDEX_INVALID;
	self->workspace = workspace;
	self->state = state;
	psy_ui_component_set_align(&self->component, psy_ui_ALIGN_RIGHT);
	psy_ui_component_hide(&self->component);
	closebar_init(&self->close_bar, tracknameedit_base(self), NULL);
	psy_ui_component_set_minimum_size(&self->component,
		psy_ui_size_make_em(70.0, 0.0));
	/* left */
	psy_ui_component_init(&self->left, &self->component, NULL);
	psy_ui_component_set_align(&self->left, psy_ui_ALIGN_LEFT);
	psy_ui_component_set_margin(&self->left,
		psy_ui_margin_make_em(2.0, 2.0, 0.0, 0.0));	
	psy_ui_component_set_default_align(&self->left, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));		
	headerstyleswitch_init(&self->style_switch, &self->left, workspace);
	tracknaming_init(&self->naming, &self->left, state);	
	/* client */
	psy_ui_component_init(&self->client, &self->component, NULL);
	psy_ui_component_set_margin(&self->client,
		psy_ui_margin_make_em(0.5, 0.0, 0.0, 0.0));	
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_CLIENT);	
	psy_ui_component_set_default_align(&self->client, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));	
	/* name */
	/* label */
	psy_ui_label_init_text(&self->name_label, &self->client, "pv.tracknames");
	/* edit */	
	psy_ui_edit_init(&self->name_edit, &self->client);
	psy_ui_component_set_style_type(&self->name_edit.pane.component,
		psy_ui_STYLE_LISTBOX);		
	psy_ui_edit_set_char_number(&self->name_edit, 40);	
	psy_signal_connect(&self->name_edit.signal_change, self,
		tracknameedit_on_edit_changed);
	psy_ui_listbox_init(&self->names, &self->client);	
	psy_ui_component_set_align(&self->names.component, psy_ui_ALIGN_CLIENT);	
	/* connect */
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		tracknameedit_on_song_changed);
	tracknameedit_connect_song(self, workspace_song(workspace));
	psy_signal_connect(&self->names.signal_selchanged, self,
		tracknameedit_on_selected);
	if (workspace_song(workspace)) {
		tracknameedit_select(self, psy_audio_sequence_patternindex(
			psy_audio_song_sequence(workspace_song(self->workspace)),
			self->state->cursor.order_index));
	}
}

void tracknameedit_on_edit_changed(TrackNameEdit* self,
	psy_ui_Edit* sender)
{
	psy_audio_Patterns* patterns;
	
	assert(self);
		
	patterns = patternviewstate_patterns(self->state);
	if (patterns) {
		uintptr_t track_index;
		
		track_index = psy_ui_listbox_cur_sel(&self->names);
		if (track_index == -1) {
			track_index = 0;
		}
		if (patterns->sharetracknames) {
			char text[64];
			
			psy_audio_patterns_set_shared_track_name(patterns,
				track_index, psy_ui_edit_text(sender));						
			psy_snprintf(text, 64, "%.2d: %s", track_index,
				psy_ui_edit_text(sender));
			psy_ui_listbox_set_text(&self->names, text, track_index);
			psy_ui_component_invalidate(psy_ui_listbox_base(&self->names));
		} else {
			uintptr_t pattern_index;
			psy_audio_Pattern* pattern;
			
			pattern_index = psy_audio_sequence_patternindex(
				psy_audio_song_sequence(workspace_song(self->workspace)),
				psy_audio_song_sequence(workspace_song(self->workspace))->cursor.order_index);
			pattern = psy_audio_patterns_at(patterns, pattern_index);
			if (pattern) {
				char text[64];
			
				psy_audio_pattern_set_track_name(pattern,
					track_index, psy_ui_edit_text(sender));
				psy_snprintf(text, 64, "%.2d: %s", track_index,
					psy_ui_edit_text(sender));
				psy_ui_listbox_set_text(&self->names, text, track_index);
				psy_ui_component_invalidate(psy_ui_listbox_base(&self->names));
				psy_signal_emit(&patterns->signal_tracknamechanged, 
					patterns, 1, track_index);
			}
		}
	}
}

void tracknameedit_on_key_down(TrackNameEdit* self,
	psy_ui_KeyboardEvent* ev)
{	
	psy_ui_keyboardevent_stop_propagation(ev);
}

void tracknameedit_on_key_up(TrackNameEdit* self,
	psy_ui_KeyboardEvent* ev)
{
	psy_ui_keyboardevent_stop_propagation(ev);
}

void tracknameedit_on_focus(TrackNameEdit* self)
{
	
}

void tracknameedit_select(TrackNameEdit* self, uintptr_t pattern_index)
{	
	assert(self);
		
	tracknameedit_fill(self);		
	psy_ui_edit_set_text(&self->name_edit, "");	
}


void tracknameedit_on_song_changed(TrackNameEdit* self,
	psy_audio_Player* sender)
{
	assert(self);
	
	self->pattern_index = psy_INDEX_INVALID;
	tracknameedit_connect_song(self, psy_audio_player_song(sender));
	if (workspace_song(self->workspace)) {
		tracknameedit_select(self, psy_audio_sequence_patternindex(
			psy_audio_song_sequence(workspace_song(self->workspace)),
			psy_audio_song_sequence(workspace_song(self->workspace))->cursor.order_index));
		psy_ui_edit_set_text(&self->name_edit, "");	
	}
}
	
void tracknameedit_on_cursor_changed(TrackNameEdit* self,
	psy_audio_Sequence* sender)
{
	assert(self);	
			
	if (!workspace_song(self->workspace)) {
		tracknameedit_select(self, psy_INDEX_INVALID);		
	} else if (!psy_audio_orderindex_equal(&sender->cursor.order_index,
			sender->lastcursor.order_index)) {		
		tracknameedit_select(self, psy_audio_sequence_patternindex(
			psy_audio_song_sequence(workspace_song(self->workspace)),
			sender->cursor.order_index));
	}
}

void tracknameedit_connect_song(TrackNameEdit* self,
	psy_audio_Song* song)
{
	assert(self);
	
	if (song) {
		psy_signal_connect(
			&psy_audio_song_patterns(song)->signal_tracknamechanged,
			self, tracknameedit_on_track_name_changed);
		psy_signal_connect(
			&psy_audio_song_sequence(song)->signal_cursor_changed,
			self, tracknameedit_on_cursor_changed);
	}
}

void tracknameedit_fill(TrackNameEdit* self)
{
	psy_audio_Patterns* patterns;
	uintptr_t t;
	uintptr_t pattern_index;
	
	assert(self);
		
	psy_ui_listbox_clear(&self->names);
	patterns = patternviewstate_patterns(self->state);
	if (!patterns) {
		return;
	}
	pattern_index = psy_audio_sequence_patternindex(
		psy_audio_song_sequence(workspace_song(self->workspace)),
		psy_audio_song_sequence(workspace_song(self->workspace))->cursor.order_index);
	for (t = 0; t < psy_audio_patterns_num_tracks(patterns); ++t) {
		char text[64];
		const char* track_name;
		
		track_name = psy_audio_patterns_track_name(patterns,
			pattern_index, t);			
		psy_snprintf(text, 64, "%.2d: %s", (int)t,
			(track_name) ? track_name : "");
		psy_ui_listbox_add_text(&self->names, text);
	}	
	psy_ui_edit_set_text(&self->name_edit, "");	
}

void tracknameedit_on_selected(TrackNameEdit* self, psy_ui_ListBox* sender)
{		
	assert(self);
	
	if (psy_ui_listbox_cur_sel(sender) != psy_INDEX_INVALID) {
		psy_audio_Patterns* patterns;	
			
		patterns = patternviewstate_patterns(self->state);
		if (patterns) {
			const char* track_name;
			uintptr_t pattern_index;
			
			pattern_index = psy_audio_sequence_patternindex(
				psy_audio_song_sequence(workspace_song(self->workspace)),
				psy_audio_song_sequence(workspace_song(self->workspace))->cursor.order_index);			
			track_name = psy_audio_patterns_track_name(patterns,
				pattern_index, psy_ui_listbox_cur_sel(sender));
			if (track_name) {
				psy_ui_edit_set_text(&self->name_edit, track_name);
				return;
			}			
		}
	}
	psy_ui_edit_set_text(&self->name_edit, "");	
}

void tracknameedit_on_track_name_changed(TrackNameEdit* self,
	psy_audio_Patterns* sender, uintptr_t track_index)
{
	assert(self);
	
	if (track_index == psy_INDEX_INVALID) {
		tracknameedit_fill(self);
	}
}


#endif /* PSYCLE_USE_PATTERN_VIEW */
