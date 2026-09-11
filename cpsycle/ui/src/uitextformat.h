/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_TEXTFORMAT_H
#define psy_ui_TEXTFORMAT_H

#include "uicomponent.h"

/*
** psy_ui_TextFormat
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum psy_ui_EolType {
	psy_ui_EOLTYPE_NONE = 0,
	psy_ui_EOLTYPE_CRLF = 1,
	psy_ui_EOLTYPE_CR   = 2,
	psy_ui_EOLTYPE_LF   = 3
} psy_ui_EolType;


typedef struct psy_ui_TextLine {
	uintptr_t index;
	uintptr_t start;
	uintptr_t size;
} psy_ui_TextLine;

void psy_ui_textline_init(psy_ui_TextLine*);
void psy_ui_textline_init_all(psy_ui_TextLine*, uintptr_t index,
	uintptr_t start);
void psy_ui_textline_dispose(psy_ui_TextLine*);

psy_ui_TextLine* psy_ui_textline_alloc(void);
psy_ui_TextLine* psy_ui_textline_alloc_init(void);
psy_ui_TextLine* psy_ui_textline_alloc_init_all(uintptr_t index,
	uintptr_t start);

psy_ui_TextLine psy_ui_textline_make(void);

uintptr_t psy_ui_textline_line_end(const psy_ui_TextLine*, const char* str);


typedef struct psy_ui_TextFormat {
    psy_Table lines;
    bool word_wrap;
    bool line_wrap;
    double width;
    double avgcharwidth;
    double textheight;
    double linespacing;
    uintptr_t numavgchars;
    uintptr_t nummaxchars;    
    psy_ui_Alignment alignment;
    int dbg;
} psy_ui_TextFormat;

void psy_ui_textformat_init(psy_ui_TextFormat*);
void psy_ui_textformat_dispose(psy_ui_TextFormat*);

void psy_ui_textformat_clear(psy_ui_TextFormat*);
void psy_ui_textformat_calc(psy_ui_TextFormat*, const char* text,
    double width, const psy_ui_Font*);
void psy_ui_textformat_update(psy_ui_TextFormat*, const char* text,
    const psy_ui_Font*);
uintptr_t psy_ui_textformat_num_lines(const psy_ui_TextFormat*);
const psy_ui_TextLine* psy_ui_textformat_line(const psy_ui_TextFormat*,
    uintptr_t index);
const psy_ui_TextLine* psy_ui_textformat_line_from_position(
	const psy_ui_TextFormat*, uintptr_t position,
	const char* text);

INLINE void psy_ui_textformat_set_alignment(psy_ui_TextFormat* self,
    psy_ui_Alignment alignment)
{
    self->alignment = alignment;
}

INLINE void psy_ui_textformat_set_line_spacing(psy_ui_TextFormat* self,
    double line_spacing)
{
    self->linespacing = line_spacing;
}

INLINE void psy_ui_textformat_word_wrap(psy_ui_TextFormat* self)
{
    self->word_wrap = TRUE;
    psy_ui_textformat_clear(self);
}

INLINE void psy_ui_textformat_prevent_word_wrap(psy_ui_TextFormat* self)
{
    self->word_wrap = FALSE;
    psy_ui_textformat_clear(self);
}

INLINE bool psy_ui_textformat_has_word_wrap(const psy_ui_TextFormat* self)
{
    return self->word_wrap;
}

INLINE void psy_ui_textformat_prevent_wrap(psy_ui_TextFormat* self)
{
    self->line_wrap = FALSE;
    self->word_wrap = FALSE;
    psy_ui_textformat_clear(self);
}

INLINE bool psy_ui_textformat_has_wrap(const psy_ui_TextFormat* self)
{
    return self->word_wrap || self->line_wrap;
}

INLINE void psy_ui_textformat_line_wrap(psy_ui_TextFormat* self)
{
    self->line_wrap = TRUE;
    psy_ui_textformat_clear(self);
}

INLINE void psy_ui_textformat_prevent_line_wrap(psy_ui_TextFormat* self)
{
    self->line_wrap = FALSE;
    psy_ui_textformat_clear(self);
}

INLINE bool psy_ui_textformat_has_line_wrap(const psy_ui_TextFormat* self)
{
    return self->line_wrap;
}

uintptr_t psy_ui_textformat_cursor_position(const psy_ui_TextFormat*,
    const char* text, psy_ui_RealPoint, const psy_ui_TextMetric*,
    const psy_ui_Font*);
double psy_ui_textformat_screen_offset(const psy_ui_TextFormat*,
    const char* text, uintptr_t count, const psy_ui_Font*,
    const psy_ui_TextMetric*);
char_dyn_t* psy_ui_textformat_fill_tabs(const char* text, uintptr_t len, uintptr_t* rv_len);

double psy_ui_textformat_center_y(const psy_ui_TextFormat*,
    double height, uintptr_t numlines, const psy_ui_TextMetric*);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_TEXTFORMAT_H */
