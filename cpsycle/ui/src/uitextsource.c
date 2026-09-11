/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitextsource.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_TextPosition */

/* implementation */
void psy_ui_textposition_init(psy_ui_TextPosition* self)
{
	assert(self);

	self->caret = 0;
	self->selection = psy_INDEX_INVALID;
}

psy_ui_TextPosition psy_ui_textposition_make(uintptr_t caret,
	uintptr_t selection)
{
	psy_ui_TextPosition	rv;

	rv.caret = caret;
	rv.selection = selection;
	return rv;
}

bool psy_ui_textposition_has_selection(const psy_ui_TextPosition* self)
{
	assert(self);

	return (self->selection != psy_INDEX_INVALID);
}

void psy_ui_textposition_selection(const psy_ui_TextPosition* self,
	uintptr_t* cp_start, uintptr_t* cp_end)
{
	assert(self);
	assert(cp_start);
	assert(cp_end);

	*cp_start = psy_min(self->caret, self->selection);
	*cp_end = psy_max(self->caret, self->selection);
}


/* psy_ui_TextSource */
void psy_ui_textsource_init(psy_ui_TextSource* self)
{
	assert(self);

	self->text = psy_strdup("");
	psy_ui_textposition_init(&self->position);
}

void psy_ui_textsource_dispose(psy_ui_TextSource* self)
{
	assert(self);

	free(self->text);
	self->text = NULL;
}

void psy_ui_textsource_set_text(psy_ui_TextSource* self,
	const char* text)
{
	assert(self);

	psy_strreset(&self->text, text);
	self->position.selection = psy_INDEX_INVALID;
	self->position.caret = psy_strlen(text);
}

void psy_ui_textsource_add_text(psy_ui_TextSource* self,
	const char* text)
{
	assert(self);

	self->text = psy_strcat_realloc(self->text, text);
}

const char* psy_ui_textsource_text(const psy_ui_TextSource* self)
{
	assert(self);

	return self->text;
}

uintptr_t psy_ui_textsource_len(const psy_ui_TextSource* self)
{
	assert(self);

	return psy_strlen(self->text);
}

void psy_ui_textsource_insert_char(psy_ui_TextSource* self, char c)
{
	char insert[2];
	char* left;
	char* right;

	insert[0] = c;
	insert[1] = '\0';
	left = psy_ui_textsource_lefttext(self, self->position.caret + 1);
	right = psy_ui_textsource_righttext(self, self->position.caret);
	left = psy_strcat_realloc(left, insert);
	if (psy_strlen(right) > 0) {
		left = psy_strcat_realloc(left, right);
	}
	psy_strreset(&self->text, left);
	free(left);
	left = NULL;
	free(right);
	right = NULL;
	++self->position.caret;	
}

void psy_ui_textsource_inserttext(psy_ui_TextSource* self, const char* insert)
{
	char* left;
	char* right;

	left = psy_ui_textsource_lefttext(self, self->position.caret + 1);
	right = psy_ui_textsource_righttext(self, self->position.caret);
	left = psy_strcat_realloc(left, insert);
	if (psy_strlen(right) > 0) {
		left = psy_strcat_realloc(left, right);
	}
	psy_strreset(&self->text, left);
	free(left);
	left = NULL;
	free(right);
	right = NULL;
	self->position.caret += psy_strlen(insert);	
}

void psy_ui_textsource_delete_selection(psy_ui_TextSource* self)
{
	char* left;
	char* right;
	uintptr_t sel_start;
	uintptr_t sel_end;

	psy_ui_textposition_selection(&self->position, &sel_start,
		&sel_end);
	if (sel_start < sel_end) {
		++sel_start;
	}
	left = psy_ui_textsource_lefttext(self, sel_start);
	right = psy_ui_textsource_righttext(self, sel_end);
	if (psy_strlen(right) > 0) {
		left = psy_strcat_realloc(left, right);
	}
	psy_strreset(&self->text, left);
	free(left);
	left = NULL;
	free(right);
	right = NULL;
	self->position.caret = sel_start;	
}

void psy_ui_textsource_delete_char(psy_ui_TextSource* self)
{
	char* left;
	char* right;

	left = psy_ui_textsource_lefttext(self, self->position.caret);
	right = psy_ui_textsource_righttext(self, self->position.caret);
	if (psy_strlen(right) > 0) {
		left = psy_strcat_realloc(left, right);
	}
	psy_strreset(&self->text, left);
	free(left);
	left = NULL;
	free(right);
	right = NULL;
	if (self->position.caret > 0) {
		--self->position.caret;
	}	
}

void psy_ui_textsource_removechar(psy_ui_TextSource* self)
{
	char* left;
	char* right;

	left = psy_ui_textsource_lefttext(self, self->position.caret + 1);
	right = psy_ui_textsource_righttext(self, self->position.caret + 1);
	if (psy_strlen(right) > 0) {
		left = psy_strcat_realloc(left, right);
	}
	psy_strreset(&self->text, left);
	free(left);
	left = NULL;
	free(right);
	right = NULL;	
}

char_dyn_t* psy_ui_textsource_lefttext(psy_ui_TextSource* self, uintptr_t split)
{
	char* rv;

	rv = malloc(split + 1);
	if (split > 0) {
		psy_snprintf(rv, split, "%s", self->text);
	}
	else {
		rv[0] = '\0';
	}
	return rv;
}

char_dyn_t* psy_ui_textsource_righttext(psy_ui_TextSource* self, uintptr_t split)
{
	char* rv;

	if (psy_strlen(self->text) > split) {
		uintptr_t num;

		num = psy_strlen(self->text) - split;
		rv = malloc(num + 1);
		psy_snprintf(rv, num + 1, "%s", self->text + split);
	}
	else {
		rv = NULL;
	}
	return rv;
}
