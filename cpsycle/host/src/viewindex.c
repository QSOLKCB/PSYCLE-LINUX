/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "viewindex.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* implementation */
void viewindex_trace(const ViewIndex* self)
{
	char text[256];

	assert(self);
	
	psy_snprintf(text, 256, "view id %d section %d option %d seqpos %d\n",
		(int)self->id, (int)self->section, (int)self->option,
		(int)self->seqpos);
	TRACE(text);
}
