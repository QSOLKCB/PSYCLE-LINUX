/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "psycmdline.h"


void psyclecmdline_init(PsycleCmdLine* self)
{
	assert(self);
	
	self->skip_scan_ = FALSE;
	self->song_name_[0] = '\0';
}

void psyclecmdline_dispose(PsycleCmdLine* self)
{
	assert(self);
	
}

void psyclecmdline_parse(PsycleCmdLine* self, int argc, char** argv)
{
	assert(self);
	
	if (argc > 1) {
		if (strcmp(argv[1], "/skipscan") == 0) {			
			self->skip_scan_ = TRUE;
		} else {
			psy_snprintf(self->song_name_, psy_MAX_PATH, "%s", argv[1]);
		}
	}
}
