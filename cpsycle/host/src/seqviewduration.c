/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqviewduration.h"

#ifdef PSYCLE_USE_SEQVIEW

/* host */
#include "styles.h"
#include "workspace.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* prototypes */
static void seqviewduration_on_destroyed(SeqviewDuration*);	
static psy_audio_Sequence* seqviewduration_sequence(SeqviewDuration*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(SeqviewDuration* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			seqviewduration_on_destroyed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void seqviewduration_init(SeqviewDuration* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->workspace_ = workspace;
	self->duration_ms_ = 0;
	self->duration_bts_ = psy_dsp_beatpos_zero();
	self->calculating_ = FALSE;
	psy_ui_component_set_padding(&self->component, psy_ui_margin_make_em(
		0.5, 0.0, 0.5, 0.5));
	psy_ui_label_init_text(&self->desc_, &self->component, "seqview.duration");
	psy_ui_component_set_padding(&self->desc_.component,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));	
	psy_ui_component_set_align(&self->desc_.component, psy_ui_ALIGN_LEFT);
	psy_ui_label_init(&self->duration_, &self->component);	
	psy_ui_component_set_align(&self->duration_.component, psy_ui_ALIGN_LEFT);	
	psy_ui_label_set_char_number(&self->duration_, 18.0);
	psy_ui_label_prevent_translation(&self->duration_);
	psy_ui_component_set_style_type(psy_ui_label_base(&self->duration_),
		STYLE_DURATION_TIME);	
	seqviewduration_update(self, FALSE);
}

void seqviewduration_on_destroyed(SeqviewDuration* self)
{		
	assert(self);
	
	seqviewduration_stop_duration_calc(self);
}

void seqviewduration_stop_duration_calc(SeqviewDuration* self)
{
	if (workspace_song(self->workspace_) && self->calculating_) {
		psy_audio_sequence_endcalcdurationinmsresult(
			&workspace_song(self->workspace_)->sequence_);
		self->calculating_ = FALSE;
	}
}

void seqviewduration_update(SeqviewDuration* self, bool force)
{		
	psy_dsp_beatpos_t duration_bts;
	psy_audio_Sequence* sequence;
	
	assert(self);
	
	sequence = seqviewduration_sequence(self);
	if (!sequence) {
		return;
	}
	duration_bts = psy_audio_sequence_duration(sequence);
	if (psy_dsp_beatpos_not_equal(self->duration_bts_, duration_bts) || force) {
		char text[64];

		self->duration_bts_ = duration_bts;
		if (self->calculating_) {
			self->duration_ms_ = psy_audio_sequence_endcalcdurationinmsresult(
				sequence);
			self->calculating_ = FALSE;
		}
		if (!self->calculating_) {
			self->calculating_ = TRUE;
			psy_audio_sequence_start_calc_duration_in_ms(sequence);
			seqviewduration_idle(self);
		}
		psy_snprintf(text, 64, "--m--s %.2fb",
			psy_dsp_beatpos_real(self->duration_bts_));
		psy_ui_label_set_text(&self->duration_, text);
	}
}

void seqviewduration_idle(SeqviewDuration* self)
{
	psy_audio_Sequence* sequence;
	uintptr_t i;
	
	assert(self);
		
	if (!self->calculating_) {
		return;
	}
	sequence = seqviewduration_sequence(self);
	if (!sequence) {
		return;
	}
	for (i = 0; i < 10; ++i) {
		if (!psy_audio_sequence_calc_duration_in_ms(sequence)) {
			char text[64];

			self->duration_ms_ = psy_audio_sequence_endcalcdurationinmsresult(
				sequence);
			psy_snprintf(text, 64, " %02dm%02ds %.2fb",
				(int)(self->duration_ms_ / 60),
				((int)self->duration_ms_ % 60),
				(float)psy_dsp_beatpos_real(self->duration_bts_));
			psy_ui_label_set_text(&self->duration_, text);
			self->calculating_ = FALSE;
			break;
		}
	}
}

psy_audio_Sequence* seqviewduration_sequence(SeqviewDuration* self)
{
	assert(self);
	
	if (!workspace_song(self->workspace_)) {
		return NULL;
	}
	return psy_audio_song_sequence(workspace_song(self->workspace_));
}

#endif /* PSYCLE_USE_SEQVIEW */
