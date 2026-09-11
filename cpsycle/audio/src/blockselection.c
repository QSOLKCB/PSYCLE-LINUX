/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "blockselection.h"
/* local */
#include "sequence.h"

/* implementation */
void psy_audio_blockselection_init(psy_audio_BlockSelection* self)
{
	assert(self);

	psy_audio_sequencecursor_init(&self->topleft);
	psy_audio_sequencecursor_init(&self->bottomright);
	psy_audio_sequencecursor_init(&self->drag_base_);
	self->valid_ = FALSE;
}

void psy_audio_blockselection_init_all(psy_audio_BlockSelection* self,
	psy_audio_SequenceCursor topleft, psy_audio_SequenceCursor bottomright)
{
	assert(self);

	self->topleft = self->drag_base_ = topleft;
	self->bottomright = bottomright;
	self->valid_ = TRUE;
}

psy_audio_BlockSelection psy_audio_blockselection_make(
	psy_audio_SequenceCursor topleft,
	psy_audio_SequenceCursor bottomright)
{
	psy_audio_BlockSelection rv;

	psy_audio_blockselection_init_all(&rv, topleft, bottomright);
	rv.topleft = rv.drag_base_ = topleft;
	rv.bottomright = bottomright;	
	return rv;
}

void psy_audio_blockselection_start_drag(psy_audio_BlockSelection* self,	
	psy_audio_SequenceCursor cursor)
{
	psy_dsp_beatpos_t bpl;	

	assert(self);
	
	psy_audio_blockselection_enable(self);
	bpl = psy_audio_sequencecursor_bpl(&cursor);
	self->drag_base_ = self->topleft;
	self->topleft = cursor;
	self->bottomright = cursor;	
	if (cursor.channel_ >= self->drag_base_.channel_) {
		self->topleft.channel_ = self->drag_base_.channel_;
		self->bottomright.channel_ = cursor.channel_;
	} else {
		self->topleft.channel_ = cursor.channel_;
		self->bottomright.channel_ = self->drag_base_.channel_;
	}	
	if (psy_dsp_beatpos_greater_equal(cursor.offset, self->drag_base_.offset)) {
		psy_audio_sequencecursor_set_offset(&self->topleft,
			self->drag_base_.offset);
		psy_audio_sequencecursor_set_offset(&self->bottomright,
			psy_dsp_beatpos_add(cursor.offset, bpl));
	} else {
		psy_audio_sequencecursor_set_offset(&self->topleft, cursor.offset);
		psy_audio_sequencecursor_set_offset(&self->bottomright,
			psy_dsp_beatpos_add(self->drag_base_.offset, bpl));
	}
	if (cursor.key >= self->drag_base_.key) {
		self->topleft.key = self->drag_base_.key;
		self->bottomright.key = cursor.key;
	} else {
		self->topleft.key = cursor.key;
		self->bottomright.key = self->drag_base_.key;
	}	
	self->bottomright.channel_ += 1;
	self->topleft.key = 100;
	self->bottomright.key = 0;	
}

void psy_audio_blockselection_drag(psy_audio_BlockSelection* self,
	psy_audio_SequenceCursor cursor, psy_audio_Sequence* sequence)
{
	psy_dsp_beatpos_t bpl;

	assert(self);

	bpl = psy_audio_sequencecursor_bpl(&cursor);
	/* channel */
	if (cursor.channel_ >= self->drag_base_.channel_) {
		self->topleft.channel_ = self->drag_base_.channel_;
		self->bottomright.channel_ = cursor.channel_ + 1;
	} else {
		self->topleft.channel_ = cursor.channel_;
		self->bottomright.channel_ = self->drag_base_.channel_ + 1;
	}
	/* offset */
	if (psy_dsp_beatpos_greater_equal(	
			psy_audio_sequencecursor_offset_abs(&cursor, sequence),
			psy_audio_sequencecursor_offset_abs(&self->drag_base_, sequence))) {
		psy_audio_sequencecursor_set_order_index(&self->topleft,
			self->drag_base_.order_index);
		psy_audio_sequencecursor_set_offset(&self->topleft,
			self->drag_base_.offset);
		psy_audio_sequencecursor_set_order_index(&self->bottomright,
			cursor.order_index);
		psy_audio_sequencecursor_set_offset(&self->bottomright,
			psy_dsp_beatpos_add(cursor.offset, bpl));
	} else {
		psy_audio_sequencecursor_set_order_index(&self->topleft,
			cursor.order_index);
		psy_audio_sequencecursor_set_offset(&self->topleft,
			cursor.offset);			
		psy_audio_sequencecursor_set_order_index(&self->bottomright,
			self->drag_base_.order_index);			
		psy_audio_sequencecursor_set_offset(&self->bottomright,
			psy_dsp_beatpos_add(self->drag_base_.offset, bpl));
	}
	/* key */
	if (cursor.key >= self->drag_base_.key) {
		self->topleft.key = self->drag_base_.key;
		self->bottomright.key = cursor.key + 1;
	} else {
		self->topleft.key = cursor.key;
		self->bottomright.key = self->drag_base_.key + 1;
	}	
}
