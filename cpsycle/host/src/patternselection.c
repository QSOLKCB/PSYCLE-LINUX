/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternselection.h"
#include "sequence.h"


/* implementation */
void patternselection_init(PatternSelection* self)
{
	assert(self);
	
	psy_audio_blockselection_init(&self->block);
	psy_signal_init(&self->signal_changed);
	self->selecting = FALSE;
}

void patternselection_dispose(PatternSelection* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_changed);
}

void patternselection_notify(PatternSelection* self)
{
	assert(self);

	psy_signal_emit(&self->signal_changed, self, 0);
}

void patternselection_set(PatternSelection* self, psy_audio_SequenceCursor
	topleft, psy_audio_SequenceCursor bottomright)
{
	self->block.topleft = self->block.drag_base_ = topleft; 
	self->block.bottomright = bottomright;
	patternselection_notify(self);
}

void patternselection_enable(PatternSelection* self)
{
	assert(self);
	
	if (!psy_audio_blockselection_valid(&self->block)) {
		psy_audio_blockselection_enable(&self->block);
		patternselection_notify(self);
	}
}

void patternselection_disable(PatternSelection* self)
{
	assert(self);
	
	if (psy_audio_blockselection_valid(&self->block)) {
		psy_audio_blockselection_disable(&self->block);
		patternselection_notify(self);
	}
}

void patternselection_start_drag(PatternSelection* self,
	psy_audio_SequenceCursor cursor)
{
	assert(self);
	
	self->selecting = TRUE;
	psy_audio_blockselection_start_drag(&self->block, cursor);	
	patternselection_notify(self);
}

void patternselection_drag(PatternSelection* self, psy_audio_SequenceCursor
	cursor, struct psy_audio_Sequence* sequence)
{
	assert(self);
	
	psy_audio_blockselection_drag(&self->block, cursor, sequence);
	patternselection_notify(self);
}

void patternselection_stop_drag(PatternSelection* self)
{
	assert(self);
	
	self->selecting = FALSE;
}

void patternselection_select_col(PatternSelection* self,
	psy_audio_Sequence* sequence, bool single)
{
	assert(self);
	
	if (!sequence) {
		return;
	}
	if (single) {
		self->block = psy_audio_sequence_block_selection_order(
			sequence, sequence->cursor.order_index);
	} else {
		self->block = psy_audio_sequence_block_selection_track(
			sequence, sequence->cursor.order_index.track);		
	}	
	if (psy_audio_blockselection_valid(&self->block)) {
		self->block.topleft.channel_ = sequence->cursor.channel_;
		self->block.bottomright.channel_ = self->block.topleft.channel_ + 1;
	}
	patternselection_notify(self);	
}

void patternselection_select_bar(PatternSelection* self,
	psy_audio_Sequence* sequence, bool single)
{	
	const psy_audio_SequenceEntry* seq_entry;
	
	assert(self);
	
	if (!sequence) {
		return;
	}		
	seq_entry = psy_audio_sequence_entry(sequence,
		psy_audio_sequencecursor_order_index(&sequence->cursor));
	if (!seq_entry) {
		return;
	}	
	self->block = psy_audio_blockselection_make(sequence->cursor,
		psy_audio_sequencecursor_make(sequence->cursor.order_index,
		sequence->cursor.channel_ + 1,
		psy_dsp_beatpos_min(
			psy_dsp_beatpos_add(
				psy_audio_sequencecursor_offset(&sequence->cursor),
				psy_dsp_beatpos_make_real(4.0, psy_dsp_DEFAULT_PPQ)),
			psy_audio_sequenceentry_length(seq_entry))));
	patternselection_notify(self);	
}

void patternselection_select_all(PatternSelection* self,
	psy_audio_Sequence* sequence, bool single)
{	
	assert(self);	
	
	if (!sequence) {
		return;
	}
	if (single) {
		self->block = psy_audio_sequence_block_selection_order(
			sequence, sequence->cursor.order_index);
	} else {
		self->block = psy_audio_sequence_block_selection_track(
			sequence, sequence->cursor.order_index.track);		
	}
	patternselection_notify(self);
}
