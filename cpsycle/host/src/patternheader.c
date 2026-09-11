/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternheader.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "resources/resource.h"
#include "skinio.h"
#include "styles.h"
/* portable */
#include "../../detail/portable.h"


/* PatternTrackBox */

/* prototypes */
static void patterntrackbox_on_draw(PatternTrackBox*, psy_ui_Graphics*);
static void patterntrackbox_draw_number(PatternTrackBox*, psy_ui_Graphics*);
static void patterntrackbox_draw_digit(PatternTrackBox*, psy_ui_Graphics*,
	uintptr_t digit, psy_ui_Style*);
static void patterntrackbox_on_mouse_down(PatternTrackBox*, psy_ui_MouseEvent*);
static void patterntrackbox_on_preferred_size(PatternTrackBox*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void patterndefaultline_update_preferred_size(TrackerHeaderView*);
static void patterntrackbox_on_update_styles(PatternTrackBox*);
static void check_display(psy_ui_Component*, psy_ui_Style*);

/* vtable */
static psy_ui_ComponentVtable patterntrackbox_vtable;
static bool patterntrackbox_vtable_initialized = FALSE;

static void patterntrackbox_vtable_init(PatternTrackBox* self)
{
	if (!patterntrackbox_vtable_initialized) {
		patterntrackbox_vtable = *(self->component.vtable);
		patterntrackbox_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			patterntrackbox_on_draw;
		patterntrackbox_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			patterntrackbox_on_mouse_down;		
		patterntrackbox_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			patterntrackbox_on_preferred_size;
		patterntrackbox_vtable.onupdatestyles = 
			(psy_ui_fp_component)
			patterntrackbox_on_update_styles;
		patterntrackbox_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &patterntrackbox_vtable);
}

/* implementation */
void patterntrackbox_init(PatternTrackBox* self, psy_ui_Component* parent,
	uintptr_t index, TrackerState* state)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	patterntrackbox_vtable_init(self);
	self->state = state;
	self->index = index;
	psy_ui_component_set_style_type(&self->component, STYLE_PV_TRACK_HEADER);
	psy_ui_component_set_style_type_select(&self->component,
		STYLE_PV_TRACK_HEADER_SELECT);
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_NONE,
		psy_ui_margin_zero());	
	/* solo */
	psy_ui_component_init(&self->solo, &self->component, NULL);
	psy_ui_component_set_style_type(&self->solo,
		STYLE_PV_TRACK_HEADER_SOLO);
	psy_ui_component_set_style_type_select(&self->solo,
		STYLE_PV_TRACK_HEADER_SOLO_SELECT);
	psy_ui_component_prevent_app_focus_out(&self->solo);
	/* solo label */
	psy_ui_label_init(&self->solo_label, &self->component);
	psy_ui_component_set_style_type(psy_ui_label_base(&self->solo_label),
		STYLE_PV_TRACK_HEADER_SOLO_LABEL);	
	psy_ui_component_set_style_type_select(psy_ui_label_base(&self->solo_label),
		STYLE_PV_TRACK_HEADER_SOLO_LABEL_SELECT);			
	psy_ui_label_prevent_translation(&self->solo_label);	
	psy_ui_label_set_text(&self->solo_label, "S");
	psy_ui_label_set_text_alignment(&self->solo_label, psy_ui_ALIGNMENT_CENTER);
	/* mute */
	psy_ui_component_init(&self->mute, &self->component, NULL);
	psy_ui_component_set_style_type(&self->mute,
		STYLE_PV_TRACK_HEADER_MUTE);
	psy_ui_component_set_style_type_select(&self->mute,
		STYLE_PV_TRACK_HEADER_MUTE_SELECT);		
	psy_ui_component_prevent_app_focus_out(&self->mute);
	/* mute label */
	psy_ui_label_init(&self->mute_label, &self->component);	
	psy_ui_component_set_style_type(psy_ui_label_base(&self->mute_label),
		STYLE_PV_TRACK_HEADER_MUTE_LABEL);
	psy_ui_component_set_style_type_select(
		psy_ui_label_base(&self->mute_label),
		STYLE_PV_TRACK_HEADER_MUTE_LABEL_SELECT);
	psy_ui_component_hide(psy_ui_label_base(&self->mute_label));	
	psy_ui_label_prevent_translation(&self->mute_label);	
	psy_ui_label_set_text(&self->mute_label, "M");
	psy_ui_label_set_text_alignment(&self->mute_label,
		psy_ui_ALIGNMENT_CENTER);
	/* record */
	psy_ui_component_init(&self->record, &self->component, NULL);
	psy_ui_component_set_style_type(&self->record,
		STYLE_PV_TRACK_HEADER_RECORD);
	psy_ui_component_set_style_type_select(&self->record,
		STYLE_PV_TRACK_HEADER_RECORD_SELECT);
	psy_ui_component_prevent_app_focus_out(&self->record);
	/* record label */
	psy_ui_label_init(&self->record_label, &self->component);
	psy_ui_component_set_style_type(
		psy_ui_label_base(&self->record_label),
		STYLE_PV_TRACK_HEADER_RECORD_LABEL);
	psy_ui_component_set_style_type_select(
		psy_ui_label_base(&self->record_label),
		STYLE_PV_TRACK_HEADER_RECORD_LABEL_SELECT);
	psy_ui_component_hide(psy_ui_label_base(&self->record_label));	
	psy_ui_label_prevent_translation(&self->record_label);	
	psy_ui_label_set_text(&self->record_label, "R");
	psy_ui_label_set_text_alignment(&self->record_label,
		psy_ui_ALIGNMENT_CENTER);
	/* play */
	psy_ui_component_init(&self->play, &self->component, NULL);
	psy_ui_component_set_style_type(&self->play,
		STYLE_PV_TRACK_HEADER_PLAY);
	psy_ui_component_set_style_type_select(&self->play,
		STYLE_PV_TRACK_HEADER_PLAY_SELECT);
	psy_ui_component_prevent_app_focus_out(&self->play);
	/* text */
	psy_ui_label_init(&self->text, &self->component);
	psy_ui_component_set_style_type(
		psy_ui_label_base(&self->text),
		STYLE_PV_TRACK_HEADER_TEXT);	
	patterntrackbox_update(self);
	patterntrackbox_update_text(self);
	patterntrackbox_on_update_styles(self);	
}

PatternTrackBox* patterntrackbox_alloc(void)
{
	return (PatternTrackBox*)malloc(sizeof(PatternTrackBox));
}

PatternTrackBox* patterntrackbox_allocinit(psy_ui_Component* parent,
	uintptr_t index, TrackerState* state)
{
	PatternTrackBox* rv;

	rv = patterntrackbox_alloc();
	if (rv) {
		patterntrackbox_init(rv, parent, index, state);
		psy_ui_component_deallocate_after_destroyed(patterntrackbox_base(rv));
	}
	return rv;
}

void patterntrackbox_play_on(PatternTrackBox* self)
{
	assert(self);
	
	psy_ui_component_add_style_state(&self->play, psy_ui_STYLESTATE_SELECT);
}

void patterntrackbox_play_off(PatternTrackBox* self)
{
	assert(self);
	
	psy_ui_component_remove_style_state(&self->play, psy_ui_STYLESTATE_SELECT);
}

void patterntrackbox_on_draw(PatternTrackBox* self, psy_ui_Graphics* g)
{
	assert(self);
	
	patterntrackbox_draw_number(self, g);	
}

void patterntrackbox_draw_number(PatternTrackBox* self, psy_ui_Graphics* g)
{	
	assert(self);
		
	patterntrackbox_draw_digit(self, g, self->index / 10, 
		psy_ui_style(STYLE_PV_TRACK_HEADER_DIGITX0));
	patterntrackbox_draw_digit(self, g, self->index % 10, 
		psy_ui_style(STYLE_PV_TRACK_HEADER_DIGIT0X));	
}

void patterntrackbox_draw_digit(PatternTrackBox* self, psy_ui_Graphics* g,
	uintptr_t digit, psy_ui_Style* style)
{
	const psy_ui_TextMetric* tm;
	
	tm = psy_ui_component_textmetric(&self->component);	
	if (psy_ui_bitmap_empty(&style->background.bitmap)) {
		char text[8];
		psy_ui_Rectangle rc;		
				
		psy_snprintf(text, 8, "%d", (int)digit);
		rc = psy_ui_position_rectangle(&style->position);
		psy_ui_graphics_textout(g,
			psy_ui_realpoint_make(
				psy_ui_value_px(&rc.topleft.x, tm, 0),
				psy_ui_value_px(&rc.topleft.y, tm, 0)),
			text, strlen(text));
	} else {
		psy_ui_RealRectangle r;
		psy_ui_RealSize bg_size;
		double src;		
		
		bg_size = psy_ui_size_px(&style->background.size, tm, NULL);
		src = digit * bg_size.width;		
		r = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(
			psy_ui_value_px(&style->padding.left, tm, 0),
			psy_ui_value_px(&style->padding.top, tm, 0)),
			bg_size);		
		psy_ui_graphics_draw_bitmap(g, &style->background.bitmap, r,
			psy_ui_realpoint_make(
				-style->background.position.x + src,
				-style->background.position.y));
	}
}

void patterntrackbox_on_mouse_down(PatternTrackBox* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (patternviewstate_patterns(self->state->pv)) {
		psy_audio_Patterns* patterns;		
				
		patterns = patternviewstate_patterns(self->state->pv);		
		if (psy_ui_mouseevent_target(ev) == &self->solo) {
			if (psy_audio_patterns_is_track_soloed(patterns, self->index)) {
				psy_audio_patterns_deactivate_solo_track(patterns);
			} else {
				psy_audio_patterns_activate_solo_track(patterns, self->index);
			}			
		} else if (psy_ui_mouseevent_target(ev) == &self->mute) {
			if (psy_audio_patterns_is_track_muted(patterns, self->index)) {
				psy_audio_patterns_unmute_track(patterns, self->index);
			} else {
				psy_audio_patterns_mute_track(patterns, self->index);
			}			
		} else if (psy_ui_mouseevent_target(ev) == &self->record) {
			if (psy_audio_patterns_is_track_armed(patterns, self->index)) {
				psy_audio_patterns_unarm_track(patterns, self->index);
			} else {
				psy_audio_patterns_arm_track(patterns, self->index);
			}			
		} else if (self->index != psy_audio_sequencecursor_channel(
				patternviewstate_cursor(self->state->pv))) {
			psy_audio_SequenceCursor cursor;
			
			cursor = *patternviewstate_cursor(self->state->pv);			
			psy_audio_sequencecursor_set_channel(&cursor, self->index);
			psy_audio_sequence_set_cursor(self->state->pv->sequence, cursor);
		}
	}
}

void patterntrackbox_on_preferred_size(PatternTrackBox* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{	
	psy_ui_Style* style;	
	
	assert(self);
	
	style = psy_ui_style(STYLE_PV_TRACK_HEADER);	
	*rv = psy_ui_style_background_size(style);
}

void patterntrackbox_update(PatternTrackBox* self)
{
	psy_audio_Patterns* patterns;
	
	assert(self);
		
	patterns = patternviewstate_patterns(self->state->pv);
	if (psy_audio_patterns_is_track_soloed(patterns, self->index)) {
		psy_ui_component_add_style_state(&self->solo,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_add_style_state(&self->solo_label.component,
			psy_ui_STYLESTATE_SELECT);
	} else {
		psy_ui_component_remove_style_state(&self->solo,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_remove_style_state(&self->solo_label.component,
			psy_ui_STYLESTATE_SELECT);
	}
	if (psy_audio_patterns_is_track_muted(patterns, self->index)) {
		psy_ui_component_add_style_state(&self->mute,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_add_style_state(&self->mute_label.component,
			psy_ui_STYLESTATE_SELECT);
	} else {
		psy_ui_component_remove_style_state(&self->mute,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_remove_style_state(&self->mute_label.component,
			psy_ui_STYLESTATE_SELECT);
	}
	if (psy_audio_patterns_is_track_armed(patterns, self->index)) {
		psy_ui_component_add_style_state(&self->record,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_add_style_state(&self->record_label.component,
			psy_ui_STYLESTATE_SELECT);
	} else {
		psy_ui_component_remove_style_state(&self->record,
			psy_ui_STYLESTATE_SELECT);
		psy_ui_component_remove_style_state(&self->record_label.component,
			psy_ui_STYLESTATE_SELECT);
	}
	if (self->index == psy_audio_sequencecursor_channel(
			&self->state->pv->cursor)) {
		psy_ui_component_add_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	} else {
		psy_ui_component_remove_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	}
}

void patterntrackbox_update_text(PatternTrackBox* self)
{
	const char* track_name;
	uintptr_t curr_pattern_index;
	psy_audio_Sequence* sequence;
	
	assert(self);
	
	sequence = patternviewstate_sequence(self->state->pv);
	if (!sequence) {
		return;
	}
	if (!sequence->patterns) {
		return;
	}
	curr_pattern_index = psy_audio_sequence_patternindex(sequence,
		sequence->cursor.order_index);
	track_name = psy_audio_patterns_track_name(sequence->patterns,
		curr_pattern_index, self->index);
	psy_ui_label_set_text(&self->text, track_name);
}

void patterntrackbox_on_update_styles(PatternTrackBox* self)
{
	assert(self);
				
	check_display(psy_ui_label_base(&self->mute_label),
		psy_ui_style(STYLE_PV_TRACK_HEADER_MUTE_LABEL));
	check_display(psy_ui_label_base(&self->solo_label),
		psy_ui_style(STYLE_PV_TRACK_HEADER_SOLO_LABEL));
	check_display(psy_ui_label_base(&self->record_label),
		psy_ui_style(STYLE_PV_TRACK_HEADER_RECORD_LABEL));	
	check_display(psy_ui_label_base(&self->text),
		psy_ui_style(STYLE_PV_TRACK_HEADER_TEXT));
}

void check_display(psy_ui_Component* component, psy_ui_Style* style)
{
	assert(component);
	assert(style);
	
	if (psy_ui_style_hidden(style)) {		
		psy_ui_component_hide(component);		
	} else {
		psy_ui_component_show(component);
	}
}

/* PatternTrack */

/* prototypes */
static void patterntrack_onpreferredsize(PatternTrack*,
	const psy_ui_Size* limit, psy_ui_Size* rv);

/* vtable */
static psy_ui_ComponentVtable patterntrack_vtable;
static bool patterntrack_vtable_initialized = FALSE;

static void patterntrack_vtable_init(PatternTrack* self)
{
	if (!patterntrack_vtable_initialized) {
		patterntrack_vtable = *(self->component.vtable);		
		patterntrack_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			patterntrack_onpreferredsize;
		patterntrack_vtable_initialized = TRUE;
	}
	self->component.vtable = &patterntrack_vtable;
}

/* implementation */
void patterntrack_init(PatternTrack* self, psy_ui_Component* parent,
	uintptr_t index, TrackerState* state)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	patterntrack_vtable_init(self);
	psy_ui_component_prevent_app_focus_out(&self->component);
	patterntrackbox_init(&self->trackbox, &self->component, index, state);
	psy_ui_component_set_align(&self->trackbox.component, psy_ui_ALIGN_CENTER);
}

PatternTrack* patterntrack_alloc(void)
{
	return (PatternTrack*)malloc(sizeof(PatternTrack));
}

PatternTrack* patterntrack_allocinit(psy_ui_Component* parent,
	uintptr_t index, TrackerState* state)
{
	PatternTrack* rv;

	rv = patterntrack_alloc();
	if (rv) {
		patterntrack_init(rv, parent, index, state);
		psy_ui_component_deallocate_after_destroyed(patterntrack_base(rv));
	}
	return rv;
}

void patterntrack_onpreferredsize(PatternTrack* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{
	psy_ui_Style* style;
	
	assert(self);

	style = psy_ui_style(STYLE_PV_TRACK_HEADER);
	psy_ui_size_init_all(rv,
		psy_ui_value_make_px(trackerstate_trackwidth(self->trackbox.state,
			self->trackbox.index,
			psy_ui_component_textmetric(&self->component))),
		style->background.size.height);
}


/* TrackerHeader */

/* prototypes */
static void trackerheader_on_destroyed(TrackerHeader*);
static void trackerheader_on_song_changed(TrackerHeader*,
	psy_audio_Player* sender);
static void trackerheader_connect_song(TrackerHeader*);
static void trackerheader_on_cursor_changed(TrackerHeader*,
	psy_audio_Sequence*);
static void trackerheader_on_timer(TrackerHeader*, uintptr_t timer_id);
static void trackerheader_update_play_status(TrackerHeader*);
static void trackerheader_on_mouse_wheel(TrackerHeader*, psy_ui_MouseEvent*);
static void trackerheader_on_track_state_changed(TrackerHeader*,
	psy_audio_TrackState* sender);
static void trackerheader_on_track_name_changed(TrackerHeader*,
	psy_audio_Patterns* sender, uintptr_t track_index);

/* vtable */
static psy_ui_ComponentVtable trackerheader_vtable;
static bool trackerheader_vtable_initialized = FALSE;

static void trackerheader_vtable_init(TrackerHeader* self)
{
	if (!trackerheader_vtable_initialized) {
		trackerheader_vtable = *(self->component.vtable);
		trackerheader_vtable.on_destroyed =
			(psy_ui_fp_component)
			trackerheader_on_destroyed;
		trackerheader_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			trackerheader_on_timer;
		trackerheader_vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			trackerheader_on_mouse_wheel;
		trackerheader_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &trackerheader_vtable);	
}

/* implementation */
void trackerheader_init(TrackerHeader* self, psy_ui_Component* parent,
	TrackConfig* track_config, TrackerState* state, Workspace* workspace)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	trackerheader_vtable_init(self);
	self->state = state;	
	self->workspace = workspace;
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);	
	psy_table_init_keysize(&self->boxes, 67);	
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		trackerheader_on_song_changed);
	trackerheader_connect_song(self);	
	trackerheader_build(self);	
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void trackerheader_on_destroyed(TrackerHeader* self)
{
	assert(self);
	
	psy_table_dispose(&self->boxes);
}

void trackerheader_build(TrackerHeader* self)
{
	uintptr_t track;
	uintptr_t numtracks;
	
	assert(self);

	psy_ui_component_clear(&self->component);
	psy_table_clear(&self->boxes);
	numtracks = patternviewstate_num_song_tracks(self->state->pv);
	for (track = 0; track < numtracks; ++track) {		
		psy_table_insert(&self->boxes, track,
			psy_ui_component_set_align(
				patterntrack_base(patterntrack_allocinit(
					trackerheader_base(self), track, self->state)),
				psy_ui_ALIGN_LEFT));
	}	
	psy_ui_component_align(trackerheader_base(self));
}

void trackerheader_on_song_changed(TrackerHeader* self,
	psy_audio_Player* sender)
{
	assert(self);
	
	trackerheader_connect_song(self);
}

void trackerheader_connect_song(TrackerHeader* self)
{
	assert(self);
	
	if (workspace_song(self->workspace)) {
		psy_signal_connect(&psy_audio_song_sequence(workspace_song(
			self->workspace))->signal_cursor_changed,
			self, trackerheader_on_cursor_changed);
		psy_signal_connect(&psy_audio_song_patterns(workspace_song(
			self->workspace))->trackstate.signal_changed,
			self, trackerheader_on_track_state_changed);
		psy_signal_connect(&psy_audio_song_patterns(workspace_song(
			self->workspace))->signal_tracknamechanged,
			self, trackerheader_on_track_name_changed);
	}
}

void trackerheader_on_cursor_changed(TrackerHeader* self,
	psy_audio_Sequence* sender)
{		
	assert(self);
	
	if (psy_audio_sequencecursor_channel(&sender->lastcursor) !=
			psy_audio_sequencecursor_channel(&sender->cursor)) {		
		psy_ui_component_invalidate(&self->component);	
	}	
	if (sender->patterns && !sender->patterns->sharetracknames) {
		if (!psy_audio_orderindex_equal(
				&sender->cursor.order_index,
				sender->lastcursor.order_index)) {
			psy_TableIterator it;

			for (it = psy_table_begin(&self->boxes);
					!psy_tableiterator_equal(&it, psy_table_end());
					psy_tableiterator_inc(&it)) {
				PatternTrack* track;

				track = (PatternTrack*)psy_tableiterator_value(&it);		
				patterntrackbox_update_text(&track->trackbox);
			}		
		}
	}	
}

void trackerheader_on_timer(TrackerHeader* self, uintptr_t timer_id)
{	
	assert(self);
	
	trackerheader_update_play_status(self);
}

void trackerheader_update_play_status(TrackerHeader* self)
{
	psy_TableIterator it;

	assert(self);

	for (it = psy_table_begin(&self->boxes);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		PatternTrack* track;

		track = (PatternTrack*)psy_tableiterator_value(&it);
		if ((psy_audio_activechannels_play_on(&self->workspace->player_.playon,
				psy_tableiterator_key(&it)))) {
			patterntrackbox_play_on(&track->trackbox);
		} else {
			patterntrackbox_play_off(&track->trackbox);
		}
	}
}

void trackerheader_on_mouse_wheel(TrackerHeader* self, psy_ui_MouseEvent* ev)
{	
	assert(self);
		
	inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), "tracker",
		psy_eventdrivercmd_make_cmd(
			(psy_ui_mouseevent_delta(ev) > 0)
			? CMD_COLUMNNEXT
			: CMD_COLUMNPREV));	
}

void trackerheader_on_track_state_changed(TrackerHeader* self,
	psy_audio_TrackState* sender)
{
	trackerheader_update_states(self);
}

void trackerheader_update_states(TrackerHeader* self)
{
	psy_TableIterator it;

	assert(self);

	for (it = psy_table_begin(&self->boxes);
		!psy_tableiterator_equal(&it, psy_table_end());
		psy_tableiterator_inc(&it)) {
		PatternTrack* track;

		track = (PatternTrack*)psy_tableiterator_value(&it);		
		patterntrackbox_update(&track->trackbox);
	}
	psy_ui_component_invalidate(&self->component);
}

void trackerheader_on_track_name_changed(TrackerHeader* self,
	psy_audio_Patterns* sender, uintptr_t track_index)
{	
	assert(self);
	
	if (track_index == psy_INDEX_INVALID) {
		psy_TableIterator it;

		for (it = psy_table_begin(&self->boxes);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
			PatternTrack* track;

			track = (PatternTrack*)psy_tableiterator_value(&it);		
			patterntrackbox_update_text(&track->trackbox);
		}
	} else {
		PatternTrack* track;
		
		track = (PatternTrack*)psy_table_at(&self->boxes, track_index);
		if (track) {
			patterntrackbox_update_text(&track->trackbox);		
		}
	}
}

void trackerheader_update_styles(TrackerHeader* self)
{		
	psy_TableIterator it;
	
	assert(self);
				
	for (it = psy_table_begin(&self->boxes);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		PatternTrack* track;
		psy_ui_Component* component;

		track = (PatternTrack*)psy_tableiterator_value(&it);
		component = &track->trackbox.component;
		component->vtable->onupdatestyles(component);		
	}			
}


/* TrackerHeaderView */

/* prototypes*/
static void trackerheaderview_connect_config(TrackerHeaderView*);
static void trackerheaderview_configure(TrackerHeaderView*);
static void trackerheaderview_on_switch_header_line(TrackerHeaderView*,
	psy_ui_Component* sender);
static void trackerheaderview_on_update_size(TrackerHeaderView*,
	psy_Property* sender);
static void trackerheaderview_update_preferred_size(TrackerHeaderView*);
static void trackerheaderview_on_header_classic_or_text(TrackerHeaderView*,
	psy_Property* sender);
static void trackerheaderview_make_default_header_styles(TrackerHeaderView*);
static void trackerheaderview_make_default_em_header_styles(TrackerHeaderView*);
static void trackerheaderview_on_header_skin(TrackerHeaderView*,
	psy_Property* sender);

/* implementation */
void trackerheaderview_init(TrackerHeaderView* self, psy_ui_Component* parent,
	TrackConfig* config, TrackerState* state, Workspace* workspace)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_style_type(&self->component,
		STYLE_PATTERNVIEW);
	self->workspace = workspace;
	self->pat_cfg = psycleconfig_patview(workspace_cfg(self->workspace));
	/* line button */
	psy_ui_button_init_text_connect(&self->desc, &self->component,
		"pv.line", self, trackerheaderview_on_switch_header_line);
	psy_ui_component_set_style_types(psy_ui_button_base(&self->desc),
		psy_ui_STYLE_LABEL, psy_INDEX_INVALID, psy_INDEX_INVALID,
		psy_INDEX_INVALID);	
	psy_ui_component_set_align(psy_ui_button_base(&self->desc),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_preferred_size(psy_ui_button_base(&self->desc),
		psy_ui_size_make_em(8.0, 1.0));
	trackerheaderstyles_init(&self->header_styles,
		psycleconfig_directories(workspace_cfg(self->workspace)));
	/* tracks */
	psy_ui_component_init(&self->pane, &self->component, NULL);
	psy_ui_component_set_align(&self->pane, psy_ui_ALIGN_CLIENT);
	trackerheader_init(&self->header, &self->pane, config, state, workspace);
	psy_ui_component_set_align(trackerheader_base(&self->header),
		psy_ui_ALIGN_FIXED);
	psy_ui_component_set_overflow(trackerheader_base(&self->header),
		psy_ui_OVERFLOW_HSCROLL);	
	/* configuration */
	trackerheaderview_connect_config(self);
	trackerheaderview_configure(self);
	trackerheaderview_update_preferred_size(self);
}

void trackerheaderview_connect_config(TrackerHeaderView* self)
{
	assert(self);
	
	psy_configuration_connect(self->pat_cfg, "linenumbers",
		self, trackerheaderview_on_update_size);
	psy_configuration_connect(self->pat_cfg, "beatoffset",
		self, trackerheaderview_on_update_size);
	psy_configuration_connect(self->pat_cfg, "displaysinglepattern",
		self, trackerheaderview_on_update_size);		
	psy_configuration_connect(self->pat_cfg, "theme.pattern_header_classic",
		self, trackerheaderview_on_header_classic_or_text);
	psy_configuration_connect(self->pat_cfg, "theme.pattern_header_skin",
		self, trackerheaderview_on_header_skin);
}

void trackerheaderview_configure(TrackerHeaderView* self)
{
	assert(self);
		
	psy_configuration_configure(self->pat_cfg, "linenumbers");		
	psy_configuration_configure(self->pat_cfg, "beatoffset");
	psy_configuration_configure(self->pat_cfg, "displaysinglepattern");
	psy_configuration_configure(self->pat_cfg, "theme.pattern_header_skin");
	psy_configuration_configure(self->pat_cfg, "theme.pattern_header_classic");	
}

void trackerheaderview_on_update_size(TrackerHeaderView* self,
	psy_Property* sender)
{
	assert(self);
	
	trackerheaderview_update_preferred_size(self);
}

void trackerheaderview_update_preferred_size(TrackerHeaderView* self)
{	
	assert(self);
	
	if (patternviewstate_linenumber_num_digits(self->header.state->pv) == 0.0) {
		psy_ui_component_hide(&self->desc.component);
	} else {
		psy_ui_component_set_preferred_size(&self->desc.component,
			psy_ui_size_make(
				psy_ui_mul_values(
					psy_ui_value_make_ew(
						patternviewstate_linenumber_num_digits(
							self->header.state->pv)),
					self->header.state->track_config->flatsize,
					psy_ui_component_textmetric(&self->component),
					NULL),
				psy_ui_value_make_eh(1.0)));
		psy_ui_component_show(&self->desc.component);
	}
	psy_ui_component_align_invalidate(&self->component);	
}

void trackerheaderview_on_switch_header_line(TrackerHeaderView* self,
	psy_ui_Component* sender)
{
	psy_Property* p;
	
	assert(self);
	
	p = psy_configuration_at(
			psycleconfig_patview(workspace_cfg(self->workspace)),
			"theme.pattern_header_classic");
	if (p) {
		psy_property_set_item_bool(p, !psy_property_item_bool(p));
	}	
}

void trackerheaderview_on_header_classic_or_text(TrackerHeaderView* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		trackerheaderstyles_set_classic(&self->header_styles);
	} else {		
		trackerheaderstyles_set_text(&self->header_styles);
	}
	trackerheader_update_styles(&self->header);
	trackerheaderview_update_preferred_size(self);
}

void trackerheaderview_on_header_skin(TrackerHeaderView* self,
	psy_Property* sender)
{	
	assert(self);
	
	trackerheaderstyles_load(&self->header_styles, psy_property_item_str(
		sender));	
	psy_configuration_configure(self->pat_cfg, "theme.pattern_header_classic");
	trackerheader_update_styles(&self->header);	
}

#endif /* PSYCLE_USE_TRACKERVIEW */
