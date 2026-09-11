/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_FILEREADER_H
#define psy_FILEREADER_H


#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_FileReader {	
	psy_Encoding encoding;
	bool norm_eol;
	bool dos_to_utf8_;
} psy_FileReader;

void psy_filereader_init(psy_FileReader*);
void psy_filereader_dispose(psy_FileReader*);

intptr_t psy_filereader_load(psy_FileReader*, const char* path, char** rv);
void psy_filereader_set_encoding(psy_FileReader*, psy_Encoding);



#ifdef __cplusplus
}
#endif

#endif /* psy_FILEREADER_H */
