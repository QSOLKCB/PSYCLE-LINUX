/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "filereader.h"
/* local */
#include "encoding.h"
/* std */
#include <errno.h>
/* platform */
#include "../../detail/portable.h"

#define BLOCKSIZE 128 * 1024


void psy_filereader_init(psy_FileReader* self)
{
	assert(self);

	self->encoding = PSY_ENCODING_UTF8;
	self->dos_to_utf8_ = TRUE;
	self->norm_eol = TRUE;
}

void psy_filereader_dispose(psy_FileReader* self)
{
	assert(self);
	
}

void psy_filereader_set_encoding(psy_FileReader* self, psy_Encoding encoding)
{
	assert(self);
	
	self->encoding = encoding;
}

intptr_t psy_filereader_load(psy_FileReader* self, const char* path, char** rv)
{	
	FILE* fp;
	char* buffer;

	assert(self);
	assert(rv);

	*rv = NULL;
	buffer = NULL;
	fp = fopen(path, "r");
	if (fp) {
		int size;
		int ch;
		int i;

		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		buffer = (char*)malloc(size + 1);
		if (!buffer) {
			fclose(fp);
			return PSY_ERRRUN;
		}
		/* read, detect and convert its native two-character line breaks */
		for (i = 0; i < size; ++i) {
			/* Analyze the current character */
			ch = fgetc(fp);
			if (ch == EOF) {
				buffer[i] = '\0';
				break;
			}
			if (self->norm_eol && ch == 13) {
				/*
				** If a two-character line break is found, replace it with a
				** single newline
				*/
				fgetc(fp);
				--size;
				buffer[i] = '\n';
			} else {
				/* Otheriwse use it as-is */
				buffer[i] = ch;
			}
		}
		buffer[size] = '\0';
		fclose(fp);
		if (self->dos_to_utf8_) {
			char* out;

			out = psy_dos_to_utf8(buffer, NULL);
			buffer = out;			
			out = NULL;
		}
		*rv = buffer;
		return PSY_OK;
	}
	free(buffer);
	*rv = NULL;
	return PSY_ERRFILE;	
}
