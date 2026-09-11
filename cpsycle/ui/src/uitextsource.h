/* 
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_TEXTSOURCE_H
#define psy_ui_TEXTSOURCE_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_TextPosition {
	uintptr_t caret;
	uintptr_t selection;
} psy_ui_TextPosition;

void psy_ui_textposition_init(psy_ui_TextPosition*);

bool psy_ui_textposition_has_selection(const psy_ui_TextPosition*);
void psy_ui_textposition_selection(const psy_ui_TextPosition*,
	uintptr_t* cp_start, uintptr_t* cp_end);

psy_ui_TextPosition psy_ui_textposition_make(uintptr_t caret,
	uintptr_t selection);

/* psy_ui_TextSource */

typedef struct psy_ui_TextSource {
    psy_ui_TextPosition position;
    char* text;    
} psy_ui_TextSource;

void psy_ui_textsource_init(psy_ui_TextSource*);
void psy_ui_textsource_dispose(psy_ui_TextSource*);
void psy_ui_textsource_set_text(psy_ui_TextSource*,
    const char* text);
void psy_ui_textsource_add_text(psy_ui_TextSource*,
	const char* text);
const char* psy_ui_textsource_text(const psy_ui_TextSource*);
uintptr_t psy_ui_textsource_len(const psy_ui_TextSource*);
void psy_ui_textsource_insert_char(psy_ui_TextSource*, char c);
void psy_ui_textsource_inserttext(psy_ui_TextSource*, const char* insert);
void psy_ui_textsource_delete_selection(psy_ui_TextSource*);
void psy_ui_textsource_delete_char(psy_ui_TextSource*);
void psy_ui_textsource_removechar(psy_ui_TextSource*);
char_dyn_t* psy_ui_textsource_lefttext(psy_ui_TextSource*, uintptr_t split);
char_dyn_t* psy_ui_textsource_righttext(psy_ui_TextSource*, uintptr_t split);



#ifdef __cplusplus
}
#endif

#endif /* psy_ui_TEXTSOURCE_H */
