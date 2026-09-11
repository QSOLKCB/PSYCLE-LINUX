/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqviewstate.h"

#ifdef PSYCLE_USE_SEQVIEW

#include "sequenceview.h"

/* SeqViewState */
void seqviewstate_init(SeqViewState* self, SequenceCmds* cmds, SeqView* seqview)
{	
	assert(self);
	assert(cmds);

	self->cmds = cmds;
	self->seqview = seqview;
	self->item_size = psy_ui_size_make_em(16.0, 1.2);	
	self->digit_width_em = 1.2;
	self->cmd = seqviewcmd_make(SEQLVCMD_NONE, psy_INDEX_INVALID);
	self->showpatternnames = FALSE;	
}

void sequencelistviewstate_realign(SeqViewState* self, SeqViewAlign align)
{
	assert(self);
	
	seqview_realign(self->seqview, align);
}

void sequencelistviewstate_realign_full(SeqViewState* self)
{
	assert(self);
	
	seqview_realign(self->seqview, SEQVIEW_ALIGN_FULL);
}

void sequencelistviewstate_realign_list(SeqViewState* self)
{
	assert(self);
	
	seqview_realign(self->seqview,  SEQVIEW_ALIGN_LIST);
}

void sequencelistviewstate_repaint_list(SeqViewState* self)
{
	assert(self);
	
	seqview_realign(self->seqview, SEQVIEW_ALIGN_REPAINT_LIST);
}

#endif /* PSYCLE_USE_SEQVIEW */
