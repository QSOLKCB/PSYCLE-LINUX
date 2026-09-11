/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "inireader.h"
/* local */
#include "encoding.h"
/* std */
#include <errno.h>
/* platform */
#include "../../detail/portable.h"

#define MAXSTRINGSIZE 4096

typedef struct psy_char_utf8 { int v[4]; } psy_char_utf8;

static psy_char_utf8 psy_fgetc(FILE* fp, int* len, bool utf8);
static int fgetc_utf8(FILE* fp, int *len, psy_char_utf8* rv);

typedef enum {
	INIREADER_STATE_READKEY = 0,
	INIREADER_STATE_READVAL = 1,
	INIREADER_STATE_ADDVAL = 2,
	INIREADER_STATE_READSECTION = 3,
	INIREADER_STATE_ADDSECTION = 4,
	INIREADER_STATE_READCOMMENT = 5,
} PropertiesIOState;

static int reallocstr(char** str, size_t size, size_t* cap);

static char_dyn_t* lastkey(const char* key)
{
	char_dyn_t* rv;
	char* text;
	char seps[] = ".";
	char* token;
	char* lasttoken;

	text = psy_strdup(key);
	token = strtok(text, seps);
	lasttoken = NULL;	
	while (token != 0) {
		lasttoken = token;
		token = strtok(0, seps);
	}
	rv = psy_strdup(lasttoken);
	free(text);
	return rv;
}

void psy_inireader_init(psy_IniReader* self)
{
	assert(self);

	psy_signal_init(&self->signal_comment);
	psy_signal_init(&self->signal_read);
	psy_signal_init(&self->signal_section);
	self->section = NULL;
	self->curr_key_ = self->curr_value_ = NULL;
	self->cpp_comment = FALSE;	
	self->encoding = PSY_ENCODING_UTF8;
}

void psy_inireader_dispose(psy_IniReader* self)
{
	assert(self);

	psy_signal_dispose(&self->signal_comment);
	psy_signal_dispose(&self->signal_read);
	psy_signal_dispose(&self->signal_section);
	free(self->section);
	self->section = NULL;
}

void psy_inireader_set_encoding(psy_IniReader* self, psy_Encoding encoding)
{
	assert(self);
	
	self->encoding = encoding;
}

int psy_inireader_load(psy_IniReader* self, const char* path)
{
	FILE* fp;
	
	assert(self);
				
	if (!path) {
		return PSY_ERRFILE;
	}
	fp = fopen(path, "rb");
	if (fp) {
		psy_char_utf8 c;
		uintptr_t cp = 0;		
		PropertiesIOState state;
		char* key = 0;
		uintptr_t key_byte_size = 0;
		size_t keycap = 0;
		char* value = 0;
		size_t valcap = 0;
		bool testcppcomment;

		state = INIREADER_STATE_READKEY;
		reallocstr(&key, 256, &keycap);
		reallocstr(&value, 256, &valcap);
		free(self->section);
		self->section = NULL;
		testcppcomment = FALSE;
		while (TRUE) {
			int len;
											
			c = psy_fgetc(fp, &len, (self->encoding == PSY_ENCODING_UTF8));						
			if (c.v[0] == EOF) {
				break;
			}
			switch (state) {
				case INIREADER_STATE_READKEY:			
				if (c.v[0] == '\r') {
					state = INIREADER_STATE_READKEY;
				} else if (c.v[0] == '\n') {
					state = INIREADER_STATE_READKEY;
				} else if (c.v[0] == ';') {
					state = INIREADER_STATE_READCOMMENT;
				} else if (c.v[0] == '[') {
					state = INIREADER_STATE_READSECTION;
					key[cp] = '\0';
					key_byte_size = cp;
					cp = 0;
				} else if (c.v[0] == '=') {
					state = INIREADER_STATE_READVAL;
					key[cp] = '\0';
					key_byte_size = cp;
					cp = 0;					
				} else { /* read key*/
					if (testcppcomment && c.v[0] == '/') {
						--cp;
						key[cp] = '\0';
						key_byte_size = cp;
						state = INIREADER_STATE_READCOMMENT;
						testcppcomment = FALSE;
					} else if (self->cpp_comment && c.v[0] == '/') {
						testcppcomment = TRUE;
					}
					if (state != INIREADER_STATE_READCOMMENT &&
							c.v[0] != '\"' &&
							!reallocstr(&key, cp * 4, &keycap)) {
						int i;
												
						for (i = 0; i < len; ++i) {
							key[cp + i] = c.v[i];
						}
						cp += len;
					}
				}
				break;
			case INIREADER_STATE_READVAL:
				if ((c.v[0] == '\n') || (c.v[0] == '\r')) {
					state = INIREADER_STATE_ADDVAL;
					value[cp] = '\0';
					cp = 0;
				} else if (!reallocstr(&value, cp * 4, &valcap)) {
					int i;
												
					for (i = 0; i < len; ++i) {
						value[cp + i] = c.v[i];
					}					
				}
				cp += len;			
				break;			
			case INIREADER_STATE_READSECTION:
				if (c.v[0] == ']') {
					state = INIREADER_STATE_ADDSECTION;
					key[cp] = '\0';
					key_byte_size = cp;
					cp = 0;					
				} else if (!reallocstr(&key, cp * 4, &keycap)) {
					int i;
												
					for (i = 0; i < len; ++i) {
						key[cp + i] = c.v[i];
					}
				}
				cp += len;
				break;			
			case INIREADER_STATE_READCOMMENT:
				if (c.v[0] == '\r' || c.v[0] == '\n') {
					state = INIREADER_STATE_READKEY;
				}				
			break;
			default:
				break;
			}
			if (state == INIREADER_STATE_ADDVAL) {
				uintptr_t bytesize;

				bytesize = psy_strlen(value);
				if (self->encoding != PSY_ENCODING_UTF8 && bytesize > 0) {
					char* out;
					intptr_t outlen;
					intptr_t inlen;
					
					inlen = bytesize + 1;
					outlen = (inlen ) * 4 + 5;
					out = (char*)malloc((outlen) * sizeof(char) + 1);	
					isolat1ToUTF8(out, &outlen, value, &inlen);												
					self->curr_key_ = key;
					self->curr_value_ = out;
					psy_signal_emit(&self->signal_read, self, 0);
					free(out);
					out = NULL;
				} else {
					self->curr_key_ = key;
					self->curr_value_ = value;
					psy_signal_emit(&self->signal_read, self, 0);
				}
				cp = 0;
				state = INIREADER_STATE_READKEY;
			} else if (state == INIREADER_STATE_ADDSECTION) {
				psy_strreset(&self->section, key);
				self->curr_key_ = key;
				psy_signal_emit(&self->signal_section, self, 0);			
				cp = 0;
				state = INIREADER_STATE_READKEY;
			}
		}		
		fclose(fp);
		if (state == INIREADER_STATE_READVAL) {
			uintptr_t bytesize;
			if (!reallocstr(&value, cp * 4, &valcap)) {
				value[cp] = '\0';
			} else {
				value[MAXSTRINGSIZE - 1] = '\0';
			}
			bytesize = psy_strlen(value);
			if (self->encoding != PSY_ENCODING_UTF8 && bytesize > 0) {
				char* out;
										
				out = psy_dos_to_utf8(value, NULL);
				self->curr_key_ = key;
				self->curr_value_ = out;
				psy_signal_emit(&self->signal_read, self, 0);
				free(out);
				out = NULL;
			} else {
				self->curr_key_ = key;
				self->curr_value_ = value;
				psy_signal_emit(&self->signal_read, self, 0);
			}
		}
		free(key);
		free(value);
		free(self->section);
		self->section = NULL;
		return PSY_OK;
	}	
	return PSY_ERRFILE;
}

int reallocstr(char** str, size_t size, size_t* cap)
{
	int err = 0;

	if (cap && *cap < size) {
		*cap = psy_min(size + 256, MAXSTRINGSIZE);
		*str = realloc(*str, *cap);
		if (*cap == MAXSTRINGSIZE || *str == 0) {			
			err = 1;			
		}
	}
	return err;
}

psy_char_utf8 psy_fgetc(FILE* fp, int* len, bool utf8)
{
	psy_char_utf8 rv;
	
	memset(&rv, 0, sizeof(rv));
	if (utf8) {		
		*len = 4;
		fgetc_utf8(fp, len, &rv);
	} else {
		*len = 1;
		rv.v[0] = fgetc(fp);
	}
	return rv;
}

int fgetc_utf8(FILE* fp, int *len, psy_char_utf8* rv) {
    unsigned int c;
    psy_char_utf8 utf;

    if (fp == NULL) {
		goto error;
	}	
    if (len == NULL) {
		goto error;
	}
    if (*len < 1) {
		goto error;
	}
	utf.v[0] = fgetc(fp);
	if (utf.v[0] == EOF) {
		goto error;
	}
    c = utf.v[0];
    if (c & 0x80) {
		if (*len < 2) {
			goto error;
		}
		utf.v[1] = fgetc(fp);
		if (utf.v[1] == EOF) {
			goto error;
		}
		if ((utf.v[1] & 0xc0) != 0x80) {
			goto error;
		}
		if ((c & 0xe0) == 0xe0) {
			if (*len < 3) {
				goto error;
			}
			utf.v[2] = fgetc(fp);
			if (utf.v[2] == EOF) {
				goto error;
			}
			if ((utf.v[2] & 0xc0) != 0x80) {
				goto error;
			}
			if ((c & 0xf0) == 0xf0) {
				if (*len < 4) {
					goto error;
				}
				utf.v[3] = fgetc(fp);
				if (utf.v[3] == EOF) {
					goto error;
				}
				if ((c & 0xf8) != 0xf0 || (utf.v[3] & 0xc0) != 0x80) {
					goto error;
				}
				*len = 4;
				/* 4-byte code */
				c = (utf.v[0] & 0x7) << 18;
				c |= (utf.v[1] & 0x3f) << 12;
				c |= (utf.v[2] & 0x3f) << 6;
				c |= utf.v[3] & 0x3f;
				} else {
				  /* 3-byte code */
				*len = 3;
				c = (utf.v[0] & 0xf) << 12;
				c |= (utf.v[1] & 0x3f) << 6;
				c |= utf.v[2] & 0x3f;
			}
		} else {
			/* 2-byte code */
			*len = 2;
			c = (utf.v[0] & 0x1f) << 6;
			c |= utf.v[1] & 0x3f;
		}
    } else {
	/* 1-byte code */
	*len = 1;
    }
    if (rv) {
		*rv = utf;
	}
    return(c);

error:
	if (rv) {
		*rv = utf;
	}
    *len = 0;
    return(-1);
}
