/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_INIREADER_H
#define psy_INIREADER_H

/* container */
#include <properties.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_IniReader {
	psy_Signal signal_read;
	psy_Signal signal_comment;
	psy_Signal signal_section;
	char* section;
	bool cpp_comment;	
	psy_Encoding encoding;
	const char* curr_key_;
	const char* curr_value_;
} psy_IniReader;

void psy_inireader_init(psy_IniReader*);
void psy_inireader_dispose(psy_IniReader*);

int psy_inireader_load(psy_IniReader*, const char* path);
void psy_inireader_set_encoding(psy_IniReader*, psy_Encoding);

INLINE const char* psy_inireader_key(const psy_IniReader* self)
{
	return self->curr_key_;
}

INLINE const char* psy_inireader_value(const psy_IniReader* self)
{
	return self->curr_value_;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_INIREADER_H */
