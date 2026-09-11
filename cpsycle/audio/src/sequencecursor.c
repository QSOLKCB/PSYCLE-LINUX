/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequencecursor.h"
/* local */
#include "patternevent.h"
#include "sequence.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* implementation */
void psy_audio_sequencecursor_init(psy_audio_SequenceCursor* self)
{	
	assert(self);
	
	self->order_index = psy_audio_orderindex_make(0, 0);	
	self->key = psy_audio_NOTECOMMANDS_MIDDLEC;
	self->channel_ = 0;
	self->offset = psy_dsp_beatpos_zero();	
	self->lpb = 4;
	self->column = 0;
	self->digit = 0;
	self->noteindex = 0;
	self->abs_offset = psy_dsp_beatpos_zero();
	self->abs_line = 0;
}

void psy_audio_sequencecursor_init_all(psy_audio_SequenceCursor* self,
	psy_audio_OrderIndex order_index)
{
	assert(self);
	
	psy_audio_sequencecursor_init(self);
	self->order_index = order_index;
}

psy_audio_SequenceCursor psy_audio_sequencecursor_make_all(
	psy_audio_OrderIndex order_index,
	uintptr_t channel, psy_dsp_beatpos_t offset, uint8_t key)
{
	psy_audio_SequenceCursor rv;
	
	psy_audio_sequencecursor_init(&rv);
	rv.order_index = order_index;
	rv.channel_ = channel;
	rv.offset = offset;
	rv.key = key;	
	return rv;
}

psy_audio_SequenceCursor psy_audio_sequencecursor_make_invalid(void)
{
	return psy_audio_sequencecursor_make_all(
		psy_audio_orderindex_make_invalid(), 0, psy_dsp_beatpos_zero(), 0);
}

psy_audio_SequenceCursor psy_audio_sequencecursor_make(
	psy_audio_OrderIndex order_index, uintptr_t channel,
	psy_dsp_beatpos_t offset)
{
	psy_audio_SequenceCursor rv;

	psy_audio_sequencecursor_init(&rv);
	rv.order_index = order_index;
	rv.channel_ = channel;
	rv.offset = offset;	
	return rv;
}

bool psy_audio_sequencecursor_equal(psy_audio_SequenceCursor* lhs,
	psy_audio_SequenceCursor* rhs)
{
	assert(lhs);
	assert(rhs);

	return (psy_audio_orderindex_equal(&lhs->order_index, rhs->order_index) &&
		psy_dsp_beatpos_equal(rhs->offset, lhs->offset) &&
		rhs->column == lhs->column &&
		rhs->digit == lhs->digit &&
		rhs->channel_ == lhs->channel_ &&		
		rhs->noteindex == lhs->noteindex &&
		rhs->lpb == lhs->lpb &&
		rhs->key == lhs->key);		
}

uintptr_t psy_audio_sequencecursor_pattern_id(
	const psy_audio_SequenceCursor* self,
	const psy_audio_Sequence* sequence)
{	
	const psy_audio_SequenceEntry* entry;

	assert(self);	
		
	entry = psy_audio_sequence_entry_const(sequence, self->order_index);
	if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
		return ((psy_audio_SequencePatternEntry*)entry)->patternslot;
	}
	return psy_INDEX_INVALID;
}

psy_dsp_beatpos_t psy_audio_sequencecursor_seqoffset(
	const psy_audio_SequenceCursor* self,
	const psy_audio_Sequence* sequence)
{
	const psy_audio_SequenceEntry* entry;
	
	assert(self);
	assert(sequence);
		
	entry = psy_audio_sequence_entry_const(sequence, self->order_index);	
	if (entry) {
		return psy_audio_sequenceentry_offset(entry);
	}
	return psy_dsp_beatpos_zero();
}

void psy_audio_sequencecursor_update_abs(psy_audio_SequenceCursor* self,
	const struct psy_audio_Sequence* sequence)
{
	assert(self);
	
	if (sequence) {
		self->abs_offset = psy_audio_sequencecursor_offset_abs(self, sequence);
		self->abs_line = (uintptr_t)(psy_dsp_beatpos_real(self->abs_offset) *
			(double)self->lpb);
	} else {
		self->abs_offset = psy_dsp_beatpos_zero();
		self->abs_line = 0;
	}
}

void psy_audio_sequencecursor_trace(const psy_audio_SequenceCursor* self)
{
	char text[256];

	assert(self);
	
	psy_snprintf(text, 256, "trk: %u  order: %u pos %f\n",
		(unsigned int)self->order_index.track,
		(unsigned int)self->order_index.order,
		psy_dsp_beatpos_real(self->offset));
	TRACE(text);
}
