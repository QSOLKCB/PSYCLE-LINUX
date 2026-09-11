/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "hostsequencertime.h"


/* implementation */
void psy_audio_hostsequencertime_init(psy_audio_HostSequencerTime* self)
{
	psy_audio_sequencecursor_init(&self->currplaycursor);
	psy_audio_sequencecursor_init(&self->lastplaycursor);	
	self->playing = FALSE;
	self->playend = FALSE;
}

void psy_audio_hostsequencertime_set_play_cursor(
	psy_audio_HostSequencerTime* self, psy_audio_SequenceCursor cursor)
{
	self->currplaycursor = cursor;	
}

void psy_audio_hostsequencertime_update_last_play_cursor(
	psy_audio_HostSequencerTime* self)
{
	self->lastplaycursor = self->currplaycursor;	
}

