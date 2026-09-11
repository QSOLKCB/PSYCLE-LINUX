/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqlistview.h"

#ifdef PSYCLE_USE_SEQVIEW

/* host */
#include "trackercmds.h"
#include "styles.h"
/* audio */
#include <pattern.h>
#include <patterns.h>
/* platform */
#include "../../detail/portable.h"


/* SeqViewItem */

/* prototypes */
static void seqviewitem_on_draw(SeqViewItem*, psy_ui_Graphics*);
static void seqviewitem_draw_entry(SeqViewItem* self, psy_ui_Graphics*,
	bool rowplaying);
static void seqviewitem_draw_progress_bar(SeqViewItem*, psy_ui_Graphics*);
static void seqviewitem_textout_digit(SeqViewItem*, psy_ui_Graphics*,
	const char* str, uintptr_t num, psy_ui_RealPoint, double colwidth);
static void seqviewitem_text(SeqViewItem*, char* text);
static void seqviewitem_on_mouse_double_click(SeqViewItem*, psy_ui_MouseEvent*);
static void seqviewitem_on_mouse_down(SeqViewItem*, psy_ui_MouseEvent*);
static bool seqviewitem_playing(const SeqViewItem*);

/* vtable */
static psy_ui_ComponentVtable seqviewitem_vtable;
static bool seqviewitem_vtable_initialized = FALSE;

static void seqviewitem_vtable_init(SeqViewItem* self)
{
	if (!seqviewitem_vtable_initialized) {
		seqviewitem_vtable = *(self->component.vtable);		
		seqviewitem_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			seqviewitem_on_draw;
		seqviewitem_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			seqviewitem_on_mouse_down;
		seqviewitem_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			seqviewitem_on_mouse_double_click;
		seqviewitem_vtable_initialized = TRUE;
	}
	self->component.vtable = &seqviewitem_vtable;
}

/* implementation */
void seqviewitem_init(SeqViewItem* self, psy_ui_Component* parent,
	psy_audio_SequenceEntry* seq_entry, psy_audio_OrderIndex order_index,
	SeqViewState* state)
{
	assert(self);
	assert(seq_entry);
	assert(state->cmds->sequence);	
	
	psy_ui_component_init(&self->component, parent, NULL);
	seqviewitem_vtable_init(self);	
	self->seqentry = seq_entry;
	self->order_index = order_index;	
	self->state = state;
	psy_ui_component_set_style_type(&self->component,
		psy_ui_STYLE_LIST_ITEM);
	psy_ui_component_set_style_type_select(&self->component,
		psy_ui_STYLE_LIST_ITEM_SELECT);
	psy_ui_component_set_align(&self->component, psy_ui_ALIGN_TOP);	
	psy_ui_component_set_preferred_size(&self->component,
		self->state->item_size);
	if (psy_audio_sequenceselection_is_selected(
			&self->state->cmds->sequence->selection,
			self->order_index)) {
		psy_ui_component_add_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	}
	psy_ui_component_prevent_app_focus_out(&self->component);
}

SeqViewItem* seqviewitem_alloc(void)
{
	return (SeqViewItem*)malloc(sizeof(SeqViewItem));
}

SeqViewItem* seqviewitem_allocinit(psy_ui_Component* parent,
	psy_audio_SequenceEntry* entry, psy_audio_OrderIndex order_index,
	SeqViewState* state)
{
	SeqViewItem* rv;

	rv = seqviewitem_alloc();
	if (rv) {
		seqviewitem_init(rv, parent, entry, order_index, state);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void seqviewitem_on_draw(SeqViewItem* self, psy_ui_Graphics* g)
{	
	bool playing;
	
	if (!self->state->showpatternnames) {
		psy_ui_graphics_set_encoding(g, PSY_ENCODING_NONE);
	}
	playing = seqviewitem_playing(self);	
	if (playing) {
		seqviewitem_draw_progress_bar(self, g);
	}				
	seqviewitem_draw_entry(self, g, playing);
	psy_ui_graphics_set_encoding(g, PSY_ENCODING_UTF8);
}

bool seqviewitem_playing(const SeqViewItem* self)
{
	const psy_audio_HostSequencerTime* seqtime;
	
	seqtime = &self->state->cmds->player->sequencer.hostseqtime;
	return (psy_audio_hostsequencertime_playing(seqtime) && 
		psy_audio_orderindex_equal(&seqtime->currplaycursor.order_index,
			self->order_index));
}

void seqviewitem_draw_entry(SeqViewItem* self, psy_ui_Graphics* g,
	bool rowplaying)
{
	psy_ui_Colour bg;
	char text[256];	
	psy_ui_RealPoint cp;
	const psy_ui_TextMetric* tm;
	uintptr_t len;
	double digit_width_px;

	tm = psy_ui_component_textmetric(&self->component);	
	digit_width_px = tm->tmAveCharWidth * self->state->digit_width_em;
	psy_ui_colour_init(&bg);
	cp = psy_ui_realpoint_make(0.0, floor((psy_ui_value_px(
		&self->state->item_size.height, tm, NULL) - tm->tmHeight) /
		2.0 + 0.5));
	seqviewitem_text(self, text);
	len = psy_strlen(text);
	if (len == 0) {
		return;
	}
	if (rowplaying) {
		double pos;
		uintptr_t col;
		
		pos = psy_audio_player_row_progress(self->state->cmds->player,
			self->order_index.track) * 
			psy_ui_value_px(&self->state->item_size.width,
				psy_ui_component_textmetric(&self->component), NULL);		
		col = (uintptr_t)(pos / digit_width_px + 0.5);
		col = psy_min(col, strlen(text));
		psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(psy_ui_style_const(STYLE_SEQ_PROGRESS)));
		seqviewitem_textout_digit(self, g, text, col, cp, digit_width_px);
		psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(psy_ui_style_const(STYLE_BOX)));
		seqviewitem_textout_digit(self, g, text + col, 
			len - col, psy_ui_realpoint_make(col * digit_width_px, cp.y),
			digit_width_px);
	} else {		
		seqviewitem_textout_digit(self, g, text, len, cp, digit_width_px);
	}
}

void seqviewitem_textout_digit(SeqViewItem* self, psy_ui_Graphics* g,
	const char* str, uintptr_t num, psy_ui_RealPoint pt, double digit_width_px)
{
	uintptr_t count;
	uintptr_t digit;	
	psy_ui_RealPoint cp;
	char digitstr[2];
	
	count = psy_strlen(str);
	cp = pt;
	digitstr[1] = '\n';
	for (digit = 0; digit < count; ++digit) {
		digitstr[0] = str[digit];
		psy_ui_graphics_textout(g, cp, digitstr, 1);		
		cp.x += digit_width_px;
	}
}

void seqviewitem_draw_progress_bar(SeqViewItem* self, psy_ui_Graphics* g)
{	
	psy_ui_graphics_draw_solid_rectangle(g,
		psy_ui_realrectangle_make(psy_ui_realpoint_zero(),
			psy_ui_realsize_make(
				psy_audio_player_row_progress(self->state->cmds->player,
					self->order_index.track) *
				psy_ui_value_px(&self->state->item_size.width,
				psy_ui_component_textmetric(&self->component), NULL),
			psy_ui_value_px(&self->state->item_size.height,
				psy_ui_component_textmetric(&self->component), NULL))),
		psy_ui_style_background_colour(psy_ui_style_const(STYLE_SEQ_PROGRESS)));
}

void seqviewitem_text(SeqViewItem* self, char* rv)
{
	rv[0] = '\0';
	if (!self->seqentry) {
		return;
	}	
	switch (self->seqentry->type) {
	case psy_audio_SEQUENCEENTRY_PATTERN: {				
		if (self->state->showpatternnames) {			
			psy_snprintf(rv, 20, "%02X: %s %06.2f",
				self->order_index.order,
				psy_audio_sequenceentry_label(self->seqentry),
				psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
					self->seqentry)));			
		} else {			
			psy_audio_SequencePatternEntry* seqpatternentry;

			seqpatternentry = (psy_audio_SequencePatternEntry*)self->seqentry;
			psy_snprintf(rv, 20, "%02X:%02X  %06.2f",
				self->order_index.order,
				(int)psy_audio_sequencepatternentry_patternslot(
					seqpatternentry),
				psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
					self->seqentry)));			
		}
		break; }
	case psy_audio_SEQUENCEENTRY_SAMPLE: {
		psy_audio_SequenceSampleEntry* seqsampleentry;

		seqsampleentry = (psy_audio_SequenceSampleEntry*)self->seqentry;
		psy_snprintf(rv, 64, "%02X:%02X:%02X", self->order_index.order,
			(int)psy_audio_sequencesampleentry_samplesindex(seqsampleentry).slot,
			(int)psy_audio_sequencesampleentry_samplesindex(seqsampleentry).subslot);
		break; }
	case psy_audio_SEQUENCEENTRY_MARKER:
		psy_snprintf(rv, 20, "%02X:%s", self->order_index.order,
			psy_audio_sequenceentry_label(self->seqentry),
			psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
				self->seqentry)));		
		break;
	default:		
		break;
	}
}

void seqviewitem_on_mouse_down(SeqViewItem* self, psy_ui_MouseEvent* ev)
{
	psy_audio_SequenceCursor cursor;
	psy_audio_Sequence* sequence;
	
	if (psy_ui_mouseevent_button(ev) != 1 ||
			!self->state->cmds->workspace->song) {
		psy_ui_mouseevent_stop_propagation(ev);
		return;
	}
	sequence = &self->state->cmds->workspace->song->sequence_;
	cursor = psy_audio_sequence_cursor(sequence);
	psy_audio_sequencecursor_set_order_index(&cursor,
		self->order_index);
	if (psy_ui_mouseevent_ctrl_key(ev)) {		
		if (!psy_audio_sequenceselection_is_selected(
				&sequence->selection,
				self->order_index)) {
			psy_audio_sequenceselection_select_first(
				&sequence->selection,
				self->order_index);
		} else {
			psy_audio_sequenceselection_deselect(
				&sequence->selection,
				self->order_index);
		}
	} else {
		psy_audio_Player* player;
		
		psy_audio_sequence_set_cursor(sequence, cursor);
		player = &self->state->cmds->workspace->player_;
		if (psy_audio_player_following_song(player) &&
				psy_audio_player_playing(player)) {
			sequencecmds_change_play_position(self->state->cmds);
		}
	}
	psy_ui_mouseevent_stop_propagation(ev);	
}

void seqviewitem_on_mouse_double_click(SeqViewItem* self, psy_ui_MouseEvent* ev)
{		
	psy_audio_SequenceCursor cursor;
	psy_audio_Sequence* sequence;

	if (psy_ui_mouseevent_button(ev) != 1 ||
			!self->state->cmds->workspace->song) {
		psy_ui_mouseevent_stop_propagation(ev);
		return;
	}
	sequence = &self->state->cmds->workspace->song->sequence_;
	cursor = psy_audio_sequence_cursor(sequence);
	psy_audio_sequencecursor_set_order_index(&cursor, self->order_index);
	psy_audio_sequence_set_cursor(sequence, cursor);		
	sequencecmds_change_play_position(self->state->cmds);
	psy_ui_mouseevent_stop_propagation(ev);
}

/* SeqViewTrack */

/* prototypes */
static void seqviewtrack_on_destroyed(SeqViewTrack*);
static void seqviewtrack_on_mouse_down(SeqViewTrack*, psy_ui_MouseEvent*);
static void seqviewtrack_build(SeqViewTrack*);
static void seqviewtrack_on_sequence_insert(SeqViewTrack*,
	psy_audio_Sequence* sender, psy_audio_OrderIndex*);
static void seqviewtrack_on_sequence_remove(SeqViewTrack*,
	psy_audio_Sequence* sender, psy_audio_OrderIndex*);	

/* vtable */
static psy_ui_ComponentVtable seqviewtrack_vtable;
static bool seqviewtrack_vtable_initialized = FALSE;

static void seqviewtrack_vtable_init(SeqViewTrack* self)
{
	if (!seqviewtrack_vtable_initialized) {
		seqviewtrack_vtable = *(self->component.vtable);
		seqviewtrack_vtable.on_destroyed =
			(psy_ui_fp_component)
			seqviewtrack_on_destroyed;
		seqviewtrack_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			seqviewtrack_on_mouse_down;		
		seqviewtrack_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &seqviewtrack_vtable);
}

/* implementation */
void seqviewtrack_init(SeqViewTrack* self, psy_ui_Component* parent,
	uintptr_t track_index, SeqViewState* state)
{
	assert(self);
	assert(state->cmds->sequence);	
	
	psy_ui_component_init(&self->component, parent, NULL);
	seqviewtrack_vtable_init(self);	
	self->state = state;
	self->trackindex = track_index;	
	self->track = psy_audio_sequence_track_at(self->state->cmds->sequence,
		track_index);
	psy_ui_component_set_preferred_width(&self->component,
		state->item_size.width);
	psy_ui_component_set_align(&self->component, psy_ui_ALIGN_LEFT);
	psy_ui_component_set_style_type(&self->component,
		STYLE_SEQLISTVIEW_TRACK);
	psy_ui_component_set_style_type_select(&self->component,		
		STYLE_SEQLISTVIEW_TRACK_SELECT);	
	psy_ui_component_prevent_app_focus_out(&self->component);
	if (psy_audio_sequenceselection_first(
			&state->cmds->workspace->song->sequence_.selection).track ==
			track_index) {
		psy_ui_component_add_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	}
	if (workspace_song(state->cmds->workspace)) {
		psy_audio_Song* song;
		psy_audio_Sequence* sequence;
	
		song = workspace_song(state->cmds->workspace);
		sequence = psy_audio_song_sequence(song);	
		psy_signal_connect(&sequence->signal_insert,
			self, seqviewtrack_on_sequence_insert);
		psy_signal_connect(&sequence->signal_remove,
			self, seqviewtrack_on_sequence_remove);		
	}
	seqviewtrack_build(self);
}

void seqviewtrack_on_destroyed(SeqViewTrack* self)
{
	if (workspace_song(self->state->cmds->workspace)) {
		psy_audio_Song* song;
		psy_audio_Sequence* sequence;
	
		song = workspace_song(self->state->cmds->workspace);
		sequence = psy_audio_song_sequence(song);	
		psy_signal_disconnect(&sequence->signal_insert,
			self, seqviewtrack_on_sequence_insert);
		psy_signal_disconnect(&sequence->signal_remove,
			self, seqviewtrack_on_sequence_remove);
	}
}		

SeqViewTrack* seqviewtrack_alloc(void)
{
	return (SeqViewTrack*)malloc(sizeof(SeqViewTrack));
}

SeqViewTrack* seqviewtrack_allocinit(psy_ui_Component* parent,
	uintptr_t track_index, SeqViewState* state)
{
	SeqViewTrack* rv;

	rv = seqviewtrack_alloc();
	if (rv) {
		seqviewtrack_init(rv, parent, track_index, state);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void seqviewtrack_build(SeqViewTrack* self)
{
	psy_List* p;
	uintptr_t row;
	
	assert(self);
		
	psy_ui_component_clear(&self->component);	
	for (p = self->track->nodes, row = 0; p != NULL; psy_list_next(&p), ++row) {		
		seqviewitem_allocinit(&self->component, 
			(psy_audio_SequenceEntry*)(p->entry),
			psy_audio_orderindex_make(self->trackindex, row),
			self->state);
	}
}

void seqviewtrack_on_sequence_insert(SeqViewTrack* self,
	psy_audio_Sequence* sender, psy_audio_OrderIndex* order_index)
{		
	seqviewtrack_build(self);
	sequencelistviewstate_realign_list(self->state);	
}

void seqviewtrack_on_sequence_remove(SeqViewTrack* self,
	psy_audio_Sequence* sender, psy_audio_OrderIndex* order_index)
{	
	seqviewtrack_build(self);
	sequencelistviewstate_realign_list(self->state);
}

void seqviewtrack_on_mouse_down(SeqViewTrack* self, psy_ui_MouseEvent* ev)
{
	psy_audio_SequenceCursor cursor;
	psy_audio_Sequence* sequence;

	sequence = &self->state->cmds->workspace->song->sequence_;
	if (psy_ui_mouseevent_button(ev) != 1) {
		psy_ui_mouseevent_stop_propagation(ev);
		return;
	}	
	cursor = psy_audio_sequencecursor_make(
		psy_audio_orderindex_make(self->trackindex, psy_INDEX_INVALID),
		0, psy_dsp_beatpos_zero());	
	psy_audio_sequence_set_cursor(sequence, cursor);
	psy_ui_mouseevent_stop_propagation(ev);	
}

/* SeqviewList */

/* prototypes */
static void seqviewlist_on_preferred_size(SeqviewList*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void seqviewlist_on_playline_changed(SeqviewList*,
	psy_audio_Sequencer* sender);
static void seqviewlist_on_playstatus_changed(SeqviewList*,
	psy_audio_Sequencer* sender);
static void seqviewlist_invalidate_item(SeqviewList*, psy_audio_OrderIndex);
static void seqviewlist_on_sequence_select(SeqviewList*,
	psy_audio_SequenceSelection*, psy_audio_OrderIndex*);
static void seqviewlist_on_sequence_deselect(SeqviewList*,
	psy_audio_SequenceSelection*, psy_audio_OrderIndex*);
static void seqviewlist_on_track_insert(SeqviewList*, psy_audio_Sequence*,
	uintptr_t trackidx);
static void seqviewlist_on_track_remove(SeqviewList*, psy_audio_Sequence*,
	uintptr_t trackidx);
static void seqviewlist_on_track_swap(SeqviewList*,
	psy_audio_Sequence* sender, uintptr_t first, uintptr_t second);
static void seqviewlist_set_player(SeqviewList*, psy_audio_Player*);
static void seqviewlist_on_follow(SeqviewList*);

/* vtable */
static psy_ui_ComponentVtable seqviewlist_vtable;
static psy_ui_ComponentVtable seqviewlist_super_vtable;
static bool seqviewlist_vtable_initialized = FALSE;

static void seqviewlist_vtable_init(SeqviewList* self)
{
	if (!seqviewlist_vtable_initialized) {
		seqviewlist_vtable = *(self->component.vtable);
		seqviewlist_super_vtable = *(self->component.vtable);		
		seqviewlist_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			seqviewlist_on_preferred_size;		
		seqviewlist_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &seqviewlist_vtable);
}

/* implementation */
void seqviewlist_init(SeqviewList* self, psy_ui_Component* parent,
	SeqViewState* state, Workspace* workspace)
{
	psy_ui_component_init(&self->component, parent, NULL);
	seqviewlist_vtable_init(self);	
	self->state = state;
	self->workspace = workspace;
	/* init component */
	psy_ui_component_set_wheel_scroll(&self->component, 1);	
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_set_overflow(&self->component, psy_ui_OVERFLOW_SCROLL);
	psy_ui_component_set_scroll_step(&self->component, self->state->item_size);	
	/* connect */
	seqviewlist_set_player(self, &self->state->cmds->workspace->player_);
	if (workspace_song(state->cmds->workspace)) {
		seqviewlist_set_song(self, workspace_song(state->cmds->workspace));
	}
	/* build */
	seqviewlist_build(self);	
}

void seqviewlist_set_player(SeqviewList* self, psy_audio_Player* player)
{
	assert(self);
	assert(player);
	
	psy_signal_connect(&player->sequencer.signal_play_line_changed,
		self, seqviewlist_on_playline_changed);
	psy_signal_connect(&player->sequencer.signal_play_status_changed,
		self, seqviewlist_on_playstatus_changed);
}

void seqviewlist_set_song(SeqviewList* self, psy_audio_Song* song)
{
	assert(self);
	
	if (song) {
		psy_audio_Sequence* sequence;
		psy_audio_Patterns* patterns;
		
		sequence = psy_audio_song_sequence(song);
		psy_signal_connect(&sequence->selection.signal_select,
			self, seqviewlist_on_sequence_select);
		psy_signal_connect(&sequence->selection.signal_deselect,
			self, seqviewlist_on_sequence_deselect);
		psy_signal_connect(&sequence->signal_track_insert, self,
			seqviewlist_on_track_insert);
		psy_signal_connect(&sequence->signal_track_swap, self,
			seqviewlist_on_track_swap);
		psy_signal_connect(&sequence->signal_track_remove, self,
			seqviewlist_on_track_remove);
		patterns = psy_audio_song_patterns(song);
		psy_signal_connect(&patterns->signal_name_changed,
			self, seqviewlist_on_pattern_name_changed);	
	}
}

void seqviewlist_build(SeqviewList* self)
{	
	uintptr_t track_index;
	
	assert(self);
	
	track_index = 0;		
	psy_ui_component_clear(&self->component);
	for (; track_index < psy_audio_sequence_width(self->state->cmds->sequence);
			++track_index) {		
		seqviewtrack_allocinit(&self->component, track_index, self->state);	
	}	
}

void seqviewlist_on_preferred_size(SeqviewList* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{	
	seqviewlist_super_vtable.onpreferredsize(&self->component, limit, rv);	
	psy_ui_value_add(&rv->width, &self->state->item_size.width,
		psy_ui_component_textmetric(&self->component), NULL);	
}

void seqviewlist_on_playline_changed(SeqviewList* self,
	psy_audio_Sequencer* sender)
{
	psy_audio_OrderIndex last;
	psy_audio_OrderIndex curr;	
	
	last = sender->hostseqtime.lastplaycursor.order_index;
	curr = sender->hostseqtime.currplaycursor.order_index;
	if (last.order != curr.order) {		
		seqviewlist_invalidate_item(self, last);
	}	
	seqviewlist_invalidate_item(self, curr);	
}

void seqviewlist_on_playstatus_changed(SeqviewList* self,
	psy_audio_Sequencer* sender)
{
	seqviewlist_on_playline_changed(self, sender);	
}

void seqviewlist_on_pattern_name_changed(SeqviewList* self,
	psy_audio_Patterns* patterns, uintptr_t slot)
{
	psy_ui_component_invalidate(&self->component);
}

void seqviewlist_invalidate_item(SeqviewList* self, psy_audio_OrderIndex index)
{	
	if (psy_audio_orderindex_valid(&index)) {		
		psy_ui_Component* track;
		
		track = psy_ui_component_at(&self->component, index.track);
		if (track) {		
			psy_ui_Component* item;
						
			item = psy_ui_component_at(track, index.order);
			if (item) {				
				psy_ui_component_invalidate(item);
			}
		}		
	}
}

void seqviewlist_on_sequence_select(SeqviewList* self,
	psy_audio_SequenceSelection* selection, psy_audio_OrderIndex* index)
{	
	psy_ui_Component* track;
	
	assert(self);
	
	track = psy_ui_component_at(&self->component, index->track);
	if (track) {				
		psy_ui_Component* row;
				
		psy_ui_component_add_style_state(track, psy_ui_STYLESTATE_SELECT);
		row = psy_ui_component_at(track, index->order);
		if (row) {			
			psy_ui_component_add_style_state(row, psy_ui_STYLESTATE_SELECT);
		}		
	}
}

void seqviewlist_on_sequence_deselect(SeqviewList* self,
	psy_audio_SequenceSelection* selection, psy_audio_OrderIndex* index)
{
	
	psy_ui_Component* track;
	
	assert(self);
	
	track = psy_ui_component_at(&self->component, index->track);
	if (track) {				
		psy_ui_Component* row;
				
		psy_ui_component_remove_style_state(track, psy_ui_STYLESTATE_SELECT);
		row = psy_ui_component_at(track, index->order);
		if (row) {			
			psy_ui_component_remove_style_state(row, psy_ui_STYLESTATE_SELECT);
		}		
	}
}

void seqviewlist_on_track_insert(SeqviewList* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	seqviewlist_build(self);	
}

void seqviewlist_on_track_remove(SeqviewList* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	seqviewlist_build(self);
}

void seqviewlist_on_track_swap(SeqviewList* self, psy_audio_Sequence* sender,
	uintptr_t first, uintptr_t second)
{
	seqviewlist_build(self);	
}

void seqlistview_on_sequence_select(SeqviewList* self,
	psy_audio_SequenceSelection* sender,
	psy_audio_OrderIndex* index)
{
	const psy_audio_HostSequencerTime* seqtime;
	
	assert(self);		
		
	seqtime = &self->workspace->player_.sequencer.hostseqtime;
	if (psy_audio_hostsequencertime_playing(seqtime) && 
			psy_audio_player_following_song(&self->workspace->player_)) {
		seqviewlist_on_follow(self);
	} else {		
		uintptr_t top;
		uintptr_t visible_lines;
		uintptr_t list_view_top;
		
		top = psy_audio_sequenceselection_first(sender).order;
		if (top == psy_INDEX_INVALID) {
			top = 0;
		}
		list_view_top = seqviewlist_top_order(self);	
		visible_lines = seqviewlist_visible_lines(self);
		if (top < list_view_top) {
			seqviewlist_set_top_order(self, top);		
		} else if (top > list_view_top + visible_lines - 1) {
			seqviewlist_set_top_order(self, top - visible_lines + 1);
		} else {
			sequencelistviewstate_repaint_list(self->state);
		}
	}	
}

void seqviewlist_on_follow(SeqviewList* self)
{	
	uintptr_t top;		
		
	assert(self);

	if (!self->state->cmds->sequence) {
		return;
	}
	top = psy_audio_sequenceselection_first(
		&self->state->cmds->sequence->selection).order;		
	if (top == psy_INDEX_INVALID) {
		top = 0;
	} else {
		uintptr_t visible_lines_2;
		
		visible_lines_2 = seqviewlist_visible_lines(self) / 2;
		if (top > visible_lines_2) {
			top -= visible_lines_2;
		} else {
			top = 0;
		}
	}
	seqviewlist_set_top_order(self, top);
}

uintptr_t seqviewlist_visible_lines(const SeqviewList* self)
{
	psy_ui_RealSize clientsize;
	
	assert(self);

	clientsize =  psy_ui_component_clientsize_px(&self->component);	
	return (uintptr_t)((clientsize.height) / psy_ui_value_px(&self->state->item_size.height,
		psy_ui_component_textmetric(&self->component), NULL));
}

void seqviewlist_set_top_order(SeqviewList* self, uintptr_t order)
{
	assert(self);

	psy_ui_component_set_scroll_top(&self->component,
		psy_ui_value_make_px(order * psy_ui_value_px(
			&self->state->item_size.height, psy_ui_component_textmetric(
			&self->component), NULL)));	
}

uintptr_t seqviewlist_top_order(const SeqviewList* self)
{
	assert(self);

	return (uintptr_t)(psy_ui_component_scroll_top_px(&self->component) /
		psy_ui_value_px(&self->state->item_size.height,
			psy_ui_component_textmetric(&self->component), NULL));
}

#endif /* PSYCLE_USE_SEQVIEW */
