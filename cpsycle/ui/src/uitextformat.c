/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitextformat.h"
/* local */
#include "uiapp.h"
/* platform */
#include "../../detail/portable.h"



/* psy_ui_TextLine */

/* implementation */
void psy_ui_textline_init(psy_ui_TextLine* self)
{
	assert(self);
	
	self->index = psy_INDEX_INVALID;
	self->start = psy_INDEX_INVALID;
	self->size = 0;
}

void psy_ui_textline_init_all(psy_ui_TextLine* self,
	uintptr_t index, uintptr_t start)
{
	assert(self);
	
	self->index = index;
	self->start = start;
	self->size = 0;
}

void psy_ui_textline_dispose(psy_ui_TextLine* self)
{
	
}

psy_ui_TextLine* psy_ui_textline_alloc(void)
{
	return (psy_ui_TextLine*)malloc(sizeof(psy_ui_TextLine));
}

psy_ui_TextLine* psy_ui_textline_alloc_init(void)
{
	psy_ui_TextLine* rv;
	
	rv = psy_ui_textline_alloc();
	if (rv) {
		psy_ui_textline_init(rv);
	}
	return rv;
}

psy_ui_TextLine* psy_ui_textline_alloc_init_all(uintptr_t index,
	uintptr_t start)
{
	psy_ui_TextLine* rv;
	
	rv = psy_ui_textline_alloc();
	if (rv) {
		psy_ui_textline_init_all(rv, index, start);
	}
	return rv;
}

psy_ui_TextLine psy_ui_textline_make(void)
{
	psy_ui_TextLine rv;
	
	psy_ui_textline_init(&rv);
	return rv;
}

uintptr_t psy_ui_textline_line_end(const psy_ui_TextLine* self, const char* str)
{
	uintptr_t rv;
	
	assert(self);
		
	if (self->size == 0) {
		return self->start;
	}	
	rv = self->start + self->size;
	if ((str[rv - 1] == '\n')) {
		--rv;
	}
	if ((rv > self->start) && (str[rv - 1] == '\r')) {
		--rv;
	}
	return rv;
}


/* psy_ui_TextFormat */

/* prototypes */
static uintptr_t psy_ui_textformat_numchars(const psy_ui_TextFormat*,
	const char* text, double width, const psy_ui_Font* font,
	const psy_ui_TextMetric* tm);
static uintptr_t psy_ui_textformat_next_line(psy_ui_TextFormat*,
	const char* text);
static uintptr_t psy_ui_textformat_next_wrap(psy_ui_TextFormat*,
	const char* text, uintptr_t num, double width, const psy_ui_Font*,
	const psy_ui_TextMetric*);

/* implementation */
void psy_ui_textformat_init(psy_ui_TextFormat* self)
{
	assert(self);
		
	psy_table_init(&self->lines);
	self->word_wrap = TRUE;
	self->line_wrap = TRUE;
	self->width = 0.0;
	self->avgcharwidth = 0.0;
	self->nummaxchars = 0;
	self->textheight = 19;
	self->linespacing = 1.0;	
	self->alignment = psy_ui_ALIGNMENT_LEFT;
	self->numavgchars = 0;
	self->dbg = 0;
}

void psy_ui_textformat_dispose(psy_ui_TextFormat* self)
{
	psy_table_dispose_all(&self->lines, (psy_fp_disposefunc)
		psy_ui_textline_dispose);
	self->width = self->avgcharwidth = 0.0;
}

void psy_ui_textformat_clear(psy_ui_TextFormat* self)
{
	psy_table_clear_all(&self->lines, (psy_fp_disposefunc)
		psy_ui_textline_dispose);	
	self->numavgchars = 0;	
}

void psy_ui_textformat_update(psy_ui_TextFormat* self, const char* text,
	const psy_ui_Font* font)
{
	psy_ui_textformat_clear(self);
	psy_ui_textformat_calc(self, text, self->width, font);
}

void psy_ui_textformat_calc(psy_ui_TextFormat* self, const char* text,
	double width, const psy_ui_Font* font)
{
	const psy_ui_TextMetric* tm;

	tm = psy_ui_font_textmetric(font);
	if (psy_table_size(&self->lines) > 0 && (self->width != width ||
			self->avgcharwidth != tm->tmAveCharWidth)) {
		psy_ui_textformat_clear(self);
	}
	self->width = width;
	self->textheight = tm->tmHeight;	
	self->avgcharwidth = tm->tmAveCharWidth;	
	if (psy_table_size(&self->lines) == 0) {		
		uintptr_t next_line;
		const char* str;
		uintptr_t cp;
		uintptr_t line;
						
		str = text;
		cp = 0;
		line = 0;
		do {
			psy_ui_TextLine* curr_line;
			uintptr_t len;			
			
			curr_line = psy_ui_textline_alloc_init_all(line, cp);
			psy_table_insert(&self->lines, line, curr_line);
			++line;
			if (self->line_wrap) {
				next_line = psy_ui_textformat_next_line(self, str);
			} else {
				next_line = psy_INDEX_INVALID;
			}
			if (next_line != psy_INDEX_INVALID) {
				len = next_line;
			} else {
				len = psy_strlen(str);
			}
			curr_line->size = len;
			if (self->word_wrap) {
				uintptr_t next_wrap;
				uintptr_t line_cp;
				const char* line_str;				
				
				line_str = str;
				line_cp = cp;
				do {
					next_wrap = psy_ui_textformat_next_wrap(self,
						line_str, len, width, font, tm);
					if (next_wrap != psy_INDEX_INVALID) {
						curr_line->size = next_wrap;
						line_str = line_str + next_wrap;
						line_cp += next_wrap;
						len = len - next_wrap;						
						curr_line = psy_ui_textline_alloc_init_all(line,
							line_cp);
						psy_table_insert(&self->lines, line, curr_line);
						++line;
					} else {
						curr_line->size = len;
					}
				} while (next_wrap != psy_INDEX_INVALID);
			}
			if (next_line != psy_INDEX_INVALID) {				
				str += next_line;
				cp += next_line;
			}
		} while (next_line != psy_INDEX_INVALID);
	}
}

uintptr_t psy_ui_textformat_next_line(psy_ui_TextFormat* self, const char* text)
{
	uintptr_t p;
	uintptr_t rv;
	
	p = 0;
	rv = psy_INDEX_INVALID;
	while (text[p] != '\0') {
		if (text[p] == '\n') {
			rv = p + 1;
			break;
		}
		++p;
	}
	return rv;
}

uintptr_t psy_ui_textformat_next_wrap(psy_ui_TextFormat* self, const char* text,
	uintptr_t num, double width, const psy_ui_Font* font,
	const psy_ui_TextMetric* tm)
{
	uintptr_t rv;	
	uintptr_t cp;	
	
	cp = 0;
	rv = psy_INDEX_INVALID;	
	if (cp < num) {
		uintptr_t linestart;
		uintptr_t numchars;
		linestart = cp;

		numchars = psy_ui_textformat_numchars(self, text + linestart, width,
			font, tm);		
		if (numchars == 0) {
			return rv;
		}
		cp += numchars;
		if (cp >= num) {			
			return psy_INDEX_INVALID;
		}		
		while (cp > linestart + 1) {
			char c;

			c = text[cp - 1];		
			if (c == ' ') {
				break;
			}
			--cp;
		}
		if (cp < linestart) {
			cp += numchars;
			cp = psy_min(num, cp);						
		}
		rv = cp;		
	}
	return rv;
}

uintptr_t psy_ui_textformat_line_at(const psy_ui_TextFormat* self,
	uintptr_t index)
{
	if (psy_table_exists(&self->lines, index)) {
		psy_ui_TextLine* line;
		
		line = (psy_ui_TextLine*)psy_table_at_const(&self->lines, index);
		if (line) {
			return line->start;
		} else {
			assert(0);
		}
	}
	return psy_INDEX_INVALID;
}

const psy_ui_TextLine* psy_ui_textformat_line(const psy_ui_TextFormat* self,
    uintptr_t index)
{
	return (psy_ui_TextLine*)psy_table_at_const(&self->lines, index);
}

uintptr_t psy_ui_textformat_num_lines(const psy_ui_TextFormat* self)
{
	return psy_table_size(&self->lines);
}

const psy_ui_TextLine* psy_ui_textformat_line_from_position(
	const psy_ui_TextFormat* self, uintptr_t position,
	const char* text)
{
	const psy_ui_TextLine* rv;
	uintptr_t line;
	uintptr_t num_lines;	

	assert(self);
	
	rv = NULL;
	num_lines = psy_ui_textformat_num_lines(self);
	if (num_lines == 0) {
		return rv;
	}	
	for (line = 0; line < num_lines; ++line) {
		const psy_ui_TextLine* curr;

		curr = psy_ui_textformat_line(self, line);
		if (curr) {
			uintptr_t line_end;
			
			line_end = psy_ui_textline_line_end(curr, text);
			if (position >= curr->start && position <= line_end) {
				rv = curr;
				break;		
			}
		}		
	}
	return rv;
}

uintptr_t psy_ui_textformat_numchars(const psy_ui_TextFormat* self,
	const char* text, double width, const psy_ui_Font* font,
	const psy_ui_TextMetric* tm)
{	
	uintptr_t i;
	uintptr_t len;
	double curr_width;
	double avg_char_width;	
	
	len = psy_strlen(text);
	avg_char_width = (tm->tmAveCharWidth * 1.3);	
	for (i = 0, curr_width = 0.0; text[i] != '\0' && curr_width < width; ++i) {		
		if (text[i] == '\t') {			
			curr_width += (avg_char_width * 4);
		} else {
			curr_width += avg_char_width;
		}
	}
	((psy_ui_TextFormat*)self)->numavgchars = psy_min(
		(uintptr_t)(width / (avg_char_width)), len);
	return i;
}

uintptr_t psy_ui_textformat_cursor_position(const psy_ui_TextFormat* self,
	const char* text, psy_ui_RealPoint pt, const psy_ui_TextMetric* tm,
	const psy_ui_Font* font)
{		
	uintptr_t cp;
	uintptr_t line;
	uintptr_t linestart;
	uintptr_t lineend;
	double x;
	const psy_ui_TextLine* curr;

	if (!text) {
		return 0;
	}	
	linestart = 0;
	line = (uintptr_t)(pt.y / tm->tmHeight);	
	curr = psy_ui_textformat_line(self, line);
	if (curr) {		
		linestart = curr->start;
		lineend = curr->start + curr->size;		
	} else {
		return psy_strlen(text);
	}	
	x = 0.0;
	cp = linestart;	
	while (x < pt.x && cp < lineend) {
		psy_ui_Size textsize;
		psy_ui_RealSize textsizepx;
		uintptr_t len;
		char_dyn_t* str;

		str = psy_ui_textformat_fill_tabs(text + linestart, cp - linestart, &len);
		textsize = psy_ui_font_textsize(font, str, len);
		free(str);
		textsizepx = psy_ui_size_px(&textsize, tm, NULL);
		x = textsizepx.width;
		if (x >= pt.x - (tm->tmAveCharWidth)) {
			break;
		}
		++cp;
	}
	return cp;
}

double psy_ui_textformat_screen_offset(const psy_ui_TextFormat* self,
	const char* text, uintptr_t count, const psy_ui_Font* font,
	const psy_ui_TextMetric* tm)
{	
	psy_ui_Size textsize;
	psy_ui_RealSize textsizepx;

	textsize = psy_ui_font_textsize(font, text, count);
	textsizepx = psy_ui_size_px(&textsize, tm, NULL);
	return textsizepx.width;
}

char_dyn_t* psy_ui_textformat_fill_tabs(const char* text, uintptr_t len,
	uintptr_t* rv_len)
{
	char_dyn_t* str;

	str = 0;
	*rv_len = 0;
	if (len) {
		uintptr_t src_cp;
		uintptr_t cp;

		str = (char*)malloc(len * 4);
		src_cp = 0;
		cp = 0;
		while (src_cp < len && text[src_cp] != '\0') {
			if (text[src_cp] == '\t') {
				uintptr_t i;

				for (i = 0; i < 4; ++i, ++cp) {
					str[cp] = ' ';
				}
			} else {
				str[cp] = text[src_cp];
				++cp;
			}
			++src_cp;
		}
		*rv_len = cp;
	}
	return str;
}

double psy_ui_textformat_center_y(const psy_ui_TextFormat* self, double height,
	uintptr_t numlines, const psy_ui_TextMetric* tm)
{
	assert(self);

	if ((self->alignment & psy_ui_ALIGNMENT_CENTER_VERTICAL) ==
			psy_ui_ALIGNMENT_CENTER_VERTICAL) {	
		return (height - self->linespacing * tm->tmHeight * numlines) /
			2.0;
	}
	return 0.0;
}
