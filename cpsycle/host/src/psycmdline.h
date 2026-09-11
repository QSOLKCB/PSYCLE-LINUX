/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PSYCMDLINE_H)
#define PSYCMDLINE_H

/* portable */
#include "../../detail/portable.h"

#ifdef __cplusplus
extern "C" {
#endif


/*!
** @struct PsycleCmdLine
** @brief Parses the command line argmunets
*/
typedef struct PsycleCmdLine {
	bool skip_scan_;
	char song_name_[psy_MAX_PATH];
} PsycleCmdLine;

void psyclecmdline_init(PsycleCmdLine*);
void psyclecmdline_dispose(PsycleCmdLine*);

void psyclecmdline_parse(PsycleCmdLine*, int argc, char** argv);

INLINE bool psyclecmdline_skip_scan(const PsycleCmdLine* self)
{
	assert(self);

	return self->skip_scan_;
}

INLINE const char* psyclecmdline_song_name(const PsycleCmdLine* self)
{
	assert(self);

	return self->song_name_;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCMDLINE_H */
