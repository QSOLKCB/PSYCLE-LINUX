/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequenceview.h"

#ifdef PSYCLE_USE_SEQVIEW

/* host */
#include "styles.h"


/* prototypes */
static void seqview_init_list_view(SeqView*);
static void seqview_init_button_bar(SeqView*);
static void seqview_init_spacer(SeqView*);
static void seqview_init_header(SeqView*);
static void seqview_init_duration(SeqView*);
static void seqview_on_song_changed(SeqView*, psy_audio_Player* sender);
static void seqview_on_sequence_select(SeqView*,
	psy_audio_SequenceSelection*, psy_audio_OrderIndex*);
static void seqview_on_track_reposition(SeqView*,
	psy_audio_Sequence* sender, uintptr_t trackidx);
static void seqview_show_pattern_names(SeqView*);
static void seqview_hide_pattern_names(SeqView*);
static void seqview_on_pattern_names(SeqView*, psy_Property*);
static void seqview_on_scroll(SeqView*, psy_ui_Component* sender,
	double dx, double dy);
static void seqview_rebuild(SeqView*);
static void seqview_on_edit_seqlist(SeqView*, psy_ui_Button* sender);
static void seqview_connect_song(SeqView*, psy_audio_Song*);
static void seqview_connect_cfg(SeqView*);
static void seqview_on_track_insert(SeqView*, psy_audio_Sequence* sender,
	uintptr_t trackidx);
static void seqview_on_track_remove(SeqView*, psy_audio_Sequence* sender,
	uintptr_t trackidx);
static void seqview_on_track_swap(SeqView*, psy_audio_Sequence* sender,
	uintptr_t first, uintptr_t second);	

/* vtable */
static psy_ui_ComponentVtable seqview_vtable;
static bool seqview_vtable_initialized = FALSE;

static void seqview_vtable_init(SeqView* self)
{
	if (!seqview_vtable_initialized) {
		seqview_vtable = *(self->component.vtable);		
		seqview_vtable_initialized = TRUE;
	}
	self->component.vtable = &seqview_vtable;
}

/* implementation */
void seqview_init(SeqView* self, psy_ui_Component* parent, Workspace* workspace)
{	
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	seqview_vtable_init(self);
	self->workspace = workspace;	
	psy_ui_component_prevent_app_focus_out(&self->component);
	sequencecmds_init(&self->cmds, workspace);	
	seqviewstate_init(&self->state, &self->cmds, self);	
	seqview_init_list_view(self);		
	seqview_init_button_bar(self);	
	seqview_init_spacer(self);	
	seqview_init_header(self);
	seqview_init_duration(self);		
	seqview_connect_song(self, workspace_song(workspace));
	seqview_connect_cfg(self);	
}

void seqview_init_list_view(SeqView* self)
{
	assert(self);

	seqviewlist_init(&self->listview, &self->component, &self->state,
		self->workspace);
	psy_ui_scroller_init(&self->scroller, seqview_base(self), NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller,
		seqviewlist_base(&self->listview));
	psy_ui_component_set_style_type(psy_ui_scroller_base(&self->scroller),
		STYLE_BOX);
	psy_ui_component_set_padding(psy_ui_scroller_base(&self->scroller),
		psy_ui_margin_make_em(0.5, 0.5, 0.0, 0.5));
	psy_ui_component_set_align(psy_ui_scroller_base(&self->scroller),
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_align(seqviewlist_base(&self->listview),
		psy_ui_ALIGN_FIXED);
}

void seqview_init_button_bar(SeqView* self)
{
	assert(self);

	sequencebuttons_init(&self->buttons, seqview_base(self), &self->cmds);
	psy_ui_component_set_align(sequencebuttons_base(&self->buttons),
		psy_ui_ALIGN_TOP);
}

void seqview_init_spacer(SeqView* self)
{
	assert(self);

	psy_ui_component_init_align(&self->spacer, seqview_base(self), NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_preferred_size(&self->spacer,
		psy_ui_size_make_em(0.0, 0.3));
}

void seqview_init_header(SeqView* self)
{
	assert(self);

	seqviewtrackheaders_init(&self->trackheader, seqview_base(self),
		&self->state);
	psy_ui_component_set_align(&self->trackheader.component, psy_ui_ALIGN_TOP);
	psy_signal_connect(&self->listview.component.signal_scrolled, self,
		seqview_on_scroll);
}

void seqview_init_duration(SeqView* self)
{
	assert(self);

	seqviewduration_init(&self->duration, seqview_base(self), self->workspace);
	psy_ui_component_set_align(&self->duration.component, psy_ui_ALIGN_BOTTOM);
	psy_signal_connect(&self->workspace->player_.signal_song_changed, self,
		seqview_on_song_changed);
}

void seqview_on_scroll(SeqView* self, psy_ui_Component* sender,
	double dx, double dy)
{	
	assert(self);
	
	psy_ui_component_set_scroll_left(&self->trackheader.client,
		psy_ui_component_scroll_left(seqviewlist_base(&self->listview)));
}

void seqview_clear(SeqView* self)
{
	assert(self);
		
	sequencecmds_clear(&self->cmds);
}

void seqview_idle(SeqView* self)
{
	assert(self);
	
	seqviewduration_idle(&self->duration);
}

void seqview_on_song_changed(SeqView* self, psy_audio_Player* sender)
{
	assert(self);
	
	sequencecmds_update(&self->cmds);
	seqviewlist_set_song(&self->listview, psy_audio_player_song(sender));	
	seqview_connect_song(self, psy_audio_player_song(sender));	
	seqview_rebuild(self);
}

void seqview_connect_song(SeqView* self, psy_audio_Song* song)
{	
	assert(self);
	
	if (song) {
		psy_audio_Sequence* sequence;
		psy_audio_Patterns* patterns;
		
		sequence = psy_audio_song_sequence(song);
		patterns = psy_audio_song_patterns(song);
		psy_signal_connect(&patterns->signal_name_changed,
			&self->listview, seqviewlist_on_pattern_name_changed);		
		psy_signal_connect(&sequence->signal_track_reposition,
			self, seqview_on_track_reposition);
		psy_signal_connect(&sequence->selection.signal_select,
			self, seqview_on_sequence_select);		
		psy_signal_connect(&sequence->signal_track_insert, self,
			seqview_on_track_insert);
		psy_signal_connect(&sequence->signal_track_swap, self,
			seqview_on_track_swap);
		psy_signal_connect(&sequence->signal_track_remove, self,
			seqview_on_track_remove);				
	}
}

void seqview_connect_cfg(SeqView* self)
{
	assert(self);
	
	psy_configuration_connect(
		psycleconfig_general(workspace_cfg(self->workspace)),
		"showpatternnames", self, seqview_on_pattern_names);
	psy_configuration_configure(
		psycleconfig_general(workspace_cfg(self->workspace)),
		"showpatternnames");
}

void seqview_rebuild(SeqView* self)
{
	assert(self);
	
	seqviewduration_stop_duration_calc(&self->duration);
	seqviewtrackheaders_build(&self->trackheader);
	seqviewlist_build(&self->listview);			
	seqviewduration_update(&self->duration, TRUE);	
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_on_edit_seqlist(SeqView* self, psy_ui_Button* sender)
{
	assert(self);
	
	psy_ui_component_set_focus(&self->listview.component);
}

void seqview_on_sequence_select(SeqView* self,
	psy_audio_SequenceSelection* sender, psy_audio_OrderIndex* index)
{	
	assert(self);
	
	seqlistview_on_sequence_select(&self->listview, sender, index);
	psy_ui_component_invalidate(&self->trackheader.component);
}

void seqview_on_track_reposition(SeqView* self, psy_audio_Sequence* sender,
	uintptr_t trackindex)
{
	assert(self);
	
	seqviewduration_update(&self->duration, FALSE);	
	sequencelistviewstate_repaint_list(&self->state);
}

void seqview_on_track_insert(SeqView* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	assert(self);
	
	seqviewtrackheaders_build(&self->trackheader);	
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_on_track_remove(SeqView* self, psy_audio_Sequence* sender,
	uintptr_t trackidx)
{
	assert(self);
	
	seqviewtrackheaders_build(&self->trackheader);
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_on_track_swap(SeqView* self, psy_audio_Sequence* sender,
	uintptr_t first, uintptr_t second)
{
	assert(self);
	
	seqviewtrackheaders_build(&self->trackheader);
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_on_pattern_names(SeqView* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		seqview_show_pattern_names(self);
	} else {
		seqview_hide_pattern_names(self);
	}
}

void seqview_show_pattern_names(SeqView* self)
{
	assert(self);
		
	self->state.showpatternnames = TRUE;
	self->state.item_size.width = psy_ui_value_make_ew(26.0);
	seqviewtrackheaders_build(&self->trackheader);
	seqviewlist_build(&self->listview);
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_hide_pattern_names(SeqView* self)
{
	assert(self);
		
	self->state.showpatternnames = FALSE;
	self->state.item_size.width = psy_ui_value_make_ew(18.0);
	seqviewtrackheaders_build(&self->trackheader);
	seqviewlist_build(&self->listview);
	sequencelistviewstate_realign_full(&self->state);
}

void seqview_realign(SeqView* self, SeqViewAlign realign)
{
	assert(self);
		
	switch (realign) {
	case SEQVIEW_ALIGN_FULL:
		psy_ui_component_align(&self->trackheader.component);
		psy_ui_component_align(&self->scroller.pane);
		psy_ui_component_invalidate(&self->scroller.pane);
		psy_ui_component_invalidate(&self->trackheader.component);
		break;
	case SEQVIEW_ALIGN_LIST:
		psy_ui_component_align(&self->scroller.pane);
		psy_ui_component_invalidate(&self->scroller.pane);
		break;
	case SEQVIEW_ALIGN_REPAINT_LIST:
		psy_ui_component_invalidate(&self->scroller.pane);
		break;
	default:
		break;
	}
}

#endif /* PSYCLE_USE_SEQVIEW */
