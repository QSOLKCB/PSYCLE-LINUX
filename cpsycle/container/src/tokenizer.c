/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "tokenizer.h"
/* platform */
#include "../../detail/portable.h"


#define LEX_STATE_START 0
#define LEX_STATE_INT 1
#define LEX_STATE_FLOAT 2
#define LEX_STATE_IDENT 3
#define LEX_STATE_DELIM 7

#define MAX_DELIM_COUNT 24
char delims[MAX_DELIM_COUNT] =
{ ',', '(', ')', '[', ']', '{', '}', ';', ':', '\n', '\t', '\0', '#'};

/* prototypes */
static bool psy_tokenizer_is_char_whitespace(char ch);
static bool psy_tokenizer_is_char_ident(psy_Tokenizer*, char ch);
static bool psy_tokenizer_is_char_delimiter(char ch);
static bool psy_tokenizer_is_char_numeric(char ch);

/* implementation */
void psy_tokenizer_init(psy_Tokenizer* self, const char* buffer)
{
	assert(self);

	self->buffer = buffer;
	self->curr_lexeme_start = 0;
	self->next_lexeme_char_index = 0;
	self->curr_lexeme_end = 0;
	self->curr_lexeme[0] = '\0';
	self->prevent_ident_numbers = FALSE;
}

void psy_tokenizer_dispose(psy_Tokenizer* self)
{
	assert(self);
	
}

void psy_tokenizer_start(psy_Tokenizer* self)
{
	assert(self);

	self->curr_lexeme_start = 0;
	self->next_lexeme_char_index = 0;
	self->curr_lexeme_end = 0;
	self->curr_lexeme[0] = '\0';
	self->curr_token = psy_TOKEN_TYPE_END; 
}

void psy_tokenizer_set_buffer(psy_Tokenizer* self, const char* buffer)
{
	assert(self);

	self->buffer = buffer;
}

intptr_t psy_tokenizer_next(psy_Tokenizer* self)
{
	char ch;
	intptr_t state;
	bool done;
	intptr_t token;
	bool add_curr_char;

	assert(self);

	state = LEX_STATE_START;
	done = FALSE;
	self->curr_lexeme_start = self->curr_lexeme_end;
	self->next_lexeme_char_index = 0;
	while (TRUE) {
		ch = psy_tokenizer_next_char(self);
		if (ch == '\0' || ch == EOF) {
			break;
		}
		add_curr_char = TRUE;
		switch (state) {
		case LEX_STATE_START:
			if (psy_tokenizer_is_char_whitespace(ch)) {
				++self->curr_lexeme_start;
				add_curr_char = FALSE;
			} else if (psy_tokenizer_is_char_delimiter(ch)) {
				state = LEX_STATE_DELIM;
			} else if (psy_tokenizer_is_char_numeric(ch)) {
				state = LEX_STATE_INT;
			} else if (ch == '.') {
				state = LEX_STATE_FLOAT;
			} else if (psy_tokenizer_is_char_ident(self, ch)) {
				state = LEX_STATE_IDENT;
			}
			break;		
		case LEX_STATE_INT:
			if (psy_tokenizer_is_char_numeric(ch)) {
				state = LEX_STATE_INT;
			} else if (ch == '.') {
				state = LEX_STATE_FLOAT;
			} else if (psy_tokenizer_is_char_whitespace(ch) ||
					psy_tokenizer_is_char_delimiter(ch) ||
					psy_tokenizer_is_char_ident(self, ch)) {
				add_curr_char = FALSE;
				done = TRUE;
			}
			break;
		case LEX_STATE_FLOAT:			
			if (psy_tokenizer_is_char_numeric(ch)) {				
					state = LEX_STATE_FLOAT;
			} else if (psy_tokenizer_is_char_whitespace(ch) ||
				psy_tokenizer_is_char_delimiter(ch))
			{
				done = TRUE;
				add_curr_char = FALSE;
			}
			break;
		case LEX_STATE_IDENT:			
			if (psy_tokenizer_is_char_ident(self, ch)) {
				state = LEX_STATE_IDENT;
			} else if (psy_tokenizer_is_char_numeric(ch)) {
				add_curr_char = FALSE;
				done = TRUE;
			} else if (ch == '.') {
				add_curr_char = FALSE;
				done = TRUE;
			} else if (psy_tokenizer_is_char_whitespace(ch)) {
				add_curr_char = FALSE;
				done = TRUE;
			} else if (psy_tokenizer_is_char_delimiter(ch)) {
				add_curr_char = FALSE;
				done = TRUE;
			}
			break;
		case LEX_STATE_DELIM:
			add_curr_char = FALSE;
			done = TRUE;
			break;
		default:
			break;
		}
		if (add_curr_char)
		{
			if (self->next_lexeme_char_index < 4096) {
				self->curr_lexeme[self->next_lexeme_char_index] = ch;
			}
			else {
				self->curr_lexeme[4095] = '\0';
			}
			++self->next_lexeme_char_index;
		}
		if (done) {
			break;
		}
	}	
	self->curr_lexeme[self->next_lexeme_char_index] = '\0';
	--self->curr_lexeme_end;

	switch (state)
	{
	case LEX_STATE_INT:
		token = psy_TOKEN_TYPE_INT;
		break;
	case LEX_STATE_FLOAT:
		token = psy_TOKEN_TYPE_FLOAT;
		break;
	case LEX_STATE_DELIM:		
		token = psy_TOKEN_TYPE_DELIM;
		break;	
	case LEX_STATE_IDENT:
		token = psy_TOKEN_TYPE_IDENT;		
		break;
	default:
		token = psy_TOKEN_TYPE_END;
	}
	self->curr_token = token;
	return token;
}

bool psy_tokenizer_has_next(const psy_Tokenizer* self)
{
	assert(self);

	return (self->curr_token != psy_TOKEN_TYPE_END);
}

int psy_tokenizer_next_char(psy_Tokenizer* self)
{
	assert(self);

	return self->buffer[self->curr_lexeme_end++];
}

int psy_tokenizer_curr_char(psy_Tokenizer* self)
{
	assert(self);

	return self->buffer[self->curr_lexeme_end];
}

bool psy_tokenizer_is_char_whitespace(char ch)
{
	return ((ch == ' ') || (ch == '\t') || (ch == '\n'));
}

bool psy_tokenizer_is_char_ident(psy_Tokenizer* self, char ch)
{	
	assert(self);

	return ((!self->prevent_ident_numbers && (ch >= '0' && ch <= '9')) ||
		(ch >= 'A' && ch <= 'Z') ||
		(ch >= 'a' && ch <= 'z') ||
		ch == '_');
}

bool psy_tokenizer_is_char_delimiter(char ch)
{
	uintptr_t i;

	for (i = 0; i < MAX_DELIM_COUNT; ++i) {
		if (ch == delims[i]) {
			return TRUE;
		}
	}
	return FALSE;
}

bool psy_tokenizer_is_char_numeric(char ch)
{
	return ((ch >= '0') && (ch <= '9'));
}

const char* psy_tokenizer_lexeme(const psy_Tokenizer* self)
{
	assert(self);

	return self->curr_lexeme;
}

bool psy_tokenizer_check_delim(const psy_Tokenizer* self, const char* text)
{
	assert(self);

	return psy_tokenizer_check(self, psy_TOKEN_TYPE_DELIM, text);
}

bool psy_tokenizer_check_int(const psy_Tokenizer* self)
{
	assert(self);

	return (psy_tokenizer_check(self, psy_TOKEN_TYPE_INT, NULL));
}

bool psy_tokenizer_check_number(const psy_Tokenizer* self)
{
	assert(self);

	return (psy_tokenizer_check(self, psy_TOKEN_TYPE_INT, NULL) ||
		psy_tokenizer_check(self, psy_TOKEN_TYPE_FLOAT, NULL));
}


bool psy_tokenizer_check_ident(const psy_Tokenizer* self, const char* text)
{
	assert(self);

	return psy_tokenizer_check(self, psy_TOKEN_TYPE_IDENT, text);
}

bool psy_tokenizer_check(const psy_Tokenizer* self, int token, const char* text)
{
	assert(self);

	if (token != self->curr_token) {
		return FALSE;
	}
	if (text == NULL) {
		return TRUE;
	}
	return (stricmp(self->curr_lexeme, text) == 0);
}

void psy_tokenizer_read_line(psy_Tokenizer* self)
{	
	int ch;

	assert(self);

	self->curr_lexeme_start = self->curr_lexeme_end;
	self->next_lexeme_char_index = 0;
	while (TRUE) {		
		ch = psy_tokenizer_next_char(self);
		if (ch == '\0' || ch == EOF || ch == '\n') {
			break;
		}				
		if (self->next_lexeme_char_index < 4096) {
			self->curr_lexeme[self->next_lexeme_char_index] = ch;
		} else {
			self->curr_lexeme[4095] = '\0';
		}
		++self->next_lexeme_char_index;		
	}	
	self->curr_lexeme[self->next_lexeme_char_index] = '\0';
	--self->curr_lexeme_end;
	if (ch == '\n') {
		psy_tokenizer_next_char(self);
	}
}

int psy_tokenizer_skip_line(psy_Tokenizer* self)
{
	int ch;

	assert(self);

	while (TRUE) {
		ch = psy_tokenizer_next_char(self);
		if (ch == '\0' || ch == EOF || ch == '\n') {
			break;
		}
	}
	if (ch == '\n') {
		psy_tokenizer_next_char(self);
	}
	return ch;
}

void psy_tokenizer_prevent_ident_numbers(psy_Tokenizer* self)
{
	assert(self);

	self->prevent_ident_numbers = TRUE;
}
