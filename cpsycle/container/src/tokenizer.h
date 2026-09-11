/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TOKENIZER_H)
#define TOKENIZER_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*! @struct psy_Tokenizer */

#define psy_TOKEN_TYPE_END 0
#define psy_TOKEN_TYPE_INT 1
#define psy_TOKEN_TYPE_FLOAT 2
#define psy_TOKEN_TYPE_IDENT 3
#define psy_TOKEN_TYPE_DELIM 4
#define psy_TOKEN_TYPE_UNKNOWN 99

typedef struct psy_Tokenizer {		
	const char* buffer;
	char curr_lexeme[4096];
	uintptr_t curr_lexeme_start;
	uintptr_t curr_lexeme_end;
	uintptr_t next_lexeme_char_index;
	intptr_t curr_token;
	bool prevent_ident_numbers;
} psy_Tokenizer;

void psy_tokenizer_init(psy_Tokenizer*, const char* buffer);
void psy_tokenizer_dispose(psy_Tokenizer*);

intptr_t psy_tokenizer_next(psy_Tokenizer*);
bool psy_tokenizer_has_next(const psy_Tokenizer*);

void psy_tokenizer_start(psy_Tokenizer*);
void psy_tokenizer_set_buffer(psy_Tokenizer*, const char * buffer);
int psy_tokenizer_next_char(psy_Tokenizer*);
int psy_tokenizer_curr_char(psy_Tokenizer*);
int psy_tokenizer_skip_line(psy_Tokenizer*);
const char* psy_tokenizer_lexeme(const psy_Tokenizer*);
bool psy_tokenizer_check_ident(const psy_Tokenizer*, const char* text);
bool psy_tokenizer_check_delim(const psy_Tokenizer*, const char* text);
bool psy_tokenizer_check_int(const psy_Tokenizer*);
bool psy_tokenizer_check_number(const psy_Tokenizer*);
bool psy_tokenizer_check(const psy_Tokenizer*, int token, const char* text);
void psy_tokenizer_read_line(psy_Tokenizer*);
void psy_tokenizer_prevent_ident_numbers(psy_Tokenizer*);

#ifdef __cplusplus
}
#endif

#endif /* TOKENIZER_H */
