/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitextdraw.h"
/* local */
#include "uiapp.h"
#include "uialignment.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_textdraw_draw_single_line(psy_ui_TextDraw*, psy_ui_Graphics*,
	psy_ui_TextPosition position, const psy_ui_TextMetric*);
static void psy_ui_textdraw_draw_cursor(psy_ui_TextDraw*, psy_ui_Graphics*,
	const psy_ui_TextMetric*, double cpy, uintptr_t linestart, uintptr_t cp);
static double psy_ui_textdraw_center_x(const psy_ui_TextDraw*,
	double width, const char* text, uintptr_t count, const psy_ui_Font*,
	const psy_ui_TextMetric*);
static void psy_ui_textdraw_wrap_textout(psy_ui_TextDraw*, psy_ui_Graphics*,
	psy_ui_RealPoint, const char* str, uintptr_t len, psy_ui_TextPosition);
static void psy_ui_textdraw_sel_textout(psy_ui_TextDraw*, psy_ui_Graphics* g,
	psy_ui_RealPoint pt, const char* text, uintptr_t len,
	psy_ui_TextPosition position);

/* implementation */
void psy_ui_textdraw_init(psy_ui_TextDraw* self, psy_ui_TextFormat* format,
	psy_ui_RealSize size, const char* text)
{	
	assert(self);
	
	self->format = format;
	self->text = text;
	self->size = size;
	self->dbg = 0;
}

void psy_ui_textdraw_dispose(psy_ui_TextDraw* self)
{
	assert(self);
	
}

void psy_ui_textdraw_draw(psy_ui_TextDraw* self, psy_ui_Graphics* g,
	psy_ui_TextPosition position, bool draw_caret)
{		
	psy_ui_RealPoint center;	
	intptr_t line;
	uintptr_t numlines;	
	double line_height;
	psy_ui_RealRectangle clip;	
	const psy_ui_TextMetric* tm;
	const psy_ui_Font* font;
	psy_ui_TextPosition curr_position;

	assert(self);
	
	font = psy_ui_graphics_font(g);
	if (!font) {
		return;
	}
	tm = psy_ui_font_textmetric(font);
	if (!psy_ui_textformat_has_wrap(self->format)) {
		psy_ui_textdraw_draw_single_line(self, g, position, tm);
		if (draw_caret && (position.caret <= psy_strlen(self->text))) {
			psy_ui_textdraw_draw_cursor(self, g, tm, psy_ui_textformat_center_y(
				self->format, self->size.height, 1, tm), 0, position.caret);
		}
		return;
	}	
	curr_position = position;
	center.y = psy_ui_textformat_center_y(self->format, self->size.height, 0, tm);
	clip = psy_ui_graphics_cliprect(g);
	line_height = (self->format->linespacing * tm->tmHeight);
	line = (intptr_t)(clip.top / line_height);
	if (psy_strlen(self->text) == 0) {
		if (draw_caret && (position.caret == 0)) {	
			psy_ui_textdraw_draw_cursor(self, g, tm, center.y, 0,
				position.caret);
		}
		return;
	}	
	self->format->dbg = self->dbg;	
	psy_ui_textformat_calc(self->format, self->text, self->size.width,
		psy_ui_graphics_font(g));
	numlines = psy_ui_textformat_num_lines(self->format);
	center.y = psy_ui_textformat_center_y(self->format, self->size.height,
		numlines, tm);	
	if (numlines == 0) {
		return;
	}			
	center.y += (line * line_height);
	numlines = psy_min(numlines,
		((uintptr_t)(floor(clip.bottom) / line_height)) + 1);	
	for (; line < (intptr_t)numlines; ++line) {
		const psy_ui_TextLine* curr;
		
		curr = psy_ui_textformat_line(self->format, line);
		if (curr) {		
			char* str;	
			uintptr_t line_end;
			uintptr_t len;
						
			str = psy_ui_textformat_fill_tabs(self->text + curr->start,
				curr->size, &len);	
			center.x = psy_ui_textdraw_center_x(self, self->size.width,
				str, len, psy_ui_graphics_font(g), tm);
			// psy_ui_textdraw_wrap_textout(g, center, str, len);
			curr_position = position;			
			if (curr_position.selection != psy_INDEX_INVALID) {
				if (curr_position.selection > curr->start) {
					curr_position.selection -= curr->start;
				} else {
					curr_position.selection = 0;
				}
			}
			if (curr_position.caret > curr->start) {
				curr_position.caret -= curr->start;
			} else {
				curr_position.caret = 0;
			}
			psy_ui_textdraw_wrap_textout(self, g, center, str, len,
				curr_position);			
			free(str);
			str = NULL;
			line_end = psy_ui_textline_line_end(curr, self->text);
			if (draw_caret && (position.caret >= curr->start) &&
					(position.caret <= line_end)) {
				psy_ui_textdraw_draw_cursor(self, g, tm, center.y, curr->start,
					position.caret);
			}
		}		
		center.y += line_height;		
	}	
}

void psy_ui_textdraw_wrap_textout(psy_ui_TextDraw* self,
	psy_ui_Graphics* g, psy_ui_RealPoint pt,
	const char* str, uintptr_t len, psy_ui_TextPosition position)
{
	if (len > 0) {
		uintptr_t p;
		
		p = len - 1;
		while (TRUE) {
			if ((str[p] == '\n') || (str[p] == '\r')) {				
				--len;				
			} else {
				break;
			}
			if (p == 0) {
				break;
			}
			--p;						
		}		
		if (len > 0) {
			psy_ui_textdraw_sel_textout(self, g, pt, str, len, position);
			psy_ui_graphics_textout(g, pt, str, len);
		}
	}
}

void psy_ui_textdraw_draw_cursor(psy_ui_TextDraw* self, psy_ui_Graphics* g,
	const psy_ui_TextMetric* tm, double cpy, uintptr_t linestart, uintptr_t cp)
{	
	psy_ui_Size textsize;
	double x;		
	uintptr_t len;
	char* str;
			
	assert(self);
	
	str = psy_ui_textformat_fill_tabs(self->text + linestart, cp - linestart,
		&len);
	textsize = psy_ui_graphics_text_size(g, str, len);
	free(str);
	str = NULL;
	x = psy_ui_value_px(&textsize.width, tm, NULL);
	psy_ui_drawline(g,
		psy_ui_realpoint_make(x, cpy),
		psy_ui_realpoint_make(x, cpy + tm->tmHeight));	
}

void psy_ui_textdraw_draw_single_line(psy_ui_TextDraw* self, psy_ui_Graphics* g,
	psy_ui_TextPosition position, const psy_ui_TextMetric* tm)
{	
	uintptr_t numchars;
	const psy_ui_Font* font;
	psy_ui_RealPoint cp;	
	
	assert(self);
	
	numchars = psy_strlen(self->text);
	if (numchars == 0) {
		return;
	}	
	font = psy_ui_graphics_font(g);
	if (!font) {
		return;
	}
	cp.x = psy_ui_textdraw_center_x(self, self->size.width,
		self->text, numchars, font, tm);
	cp.y = psy_ui_textformat_center_y(self->format, self->size.height, 1, tm);
	psy_ui_textdraw_sel_textout(self, g, cp, self->text, numchars, position);	
}

void psy_ui_textdraw_sel_textout(psy_ui_TextDraw* self, psy_ui_Graphics* g,
	psy_ui_RealPoint pt, const char* text, uintptr_t len,
	psy_ui_TextPosition position)
{
	uintptr_t numchars;
	const psy_ui_Font* font;	
	psy_ui_RealPoint cp;
	uintptr_t len_pre_sel;
	uintptr_t len_sel;
	uintptr_t len_post_sel;
	psy_ui_Size textsize;	
	const char* p;
	const psy_ui_TextMetric* tm;
	
	assert(self);
		
	numchars = psy_min(len, psy_strlen(text));
	if (numchars == 0) {
		return;
	}	
	font = psy_ui_graphics_font(g);
	if (!font) {
		return;
	}
	tm = psy_ui_font_textmetric(font);
	cp = pt;	
	p = text;	
	if (psy_ui_textposition_has_selection(&position)) {
		uintptr_t sel_start;
		uintptr_t sel_end;
		
		psy_ui_textposition_selection(&position, &sel_start, &sel_end);		
		sel_end = psy_min(sel_end, numchars);
		sel_start = psy_min(sel_start, sel_end);
		len_pre_sel = sel_start;
		len_sel = sel_end - sel_start;
		len_post_sel = numchars - sel_end;
	} else {		
		len_pre_sel = 0;
		len_sel = 0;
		len_post_sel = numchars;
	}
	if (len_pre_sel > 0) {
		psy_ui_graphics_textout(g, cp, p, len_pre_sel);
		textsize = psy_ui_font_textsize(font, p, len_pre_sel);			
		cp.x += psy_ui_size_px(&textsize, tm, NULL).width;
		p += len_pre_sel;
	}
	if (len_sel > 0) {
		psy_ui_Colour restore;
		psy_ui_Style* style;
		psy_ui_RealSize size;
		
		restore = psy_ui_graphics_text_colour(g);
		style = psy_ui_style(psy_ui_STYLE_LIST_ITEM_SELECT);
		textsize = psy_ui_font_textsize(font, p, len_sel);
		size = psy_ui_size_px(&textsize, tm, NULL);	
		size.height = tm->tmHeight;
		psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(style));
		psy_ui_graphics_draw_solid_rectangle(g, psy_ui_realrectangle_make(cp, size),
			psy_ui_style_background_colour(style));
		psy_ui_graphics_textout(g, cp, p, len_sel);
		psy_ui_graphics_set_text_colour(g, restore);		
		cp.x += size.width;
		p += len_sel;
	}
	if (len_post_sel > 0) {
		psy_ui_graphics_textout(g, cp, p, len_post_sel);
	}	
}


double psy_ui_textdraw_center_x(const psy_ui_TextDraw* self,
	double width, const char* text, uintptr_t count, const psy_ui_Font* font,
	const psy_ui_TextMetric* tm)
{
	assert(self);
		
	if (count > 0 && font) {
		psy_ui_Size textsize;
		psy_ui_RealSize textsizepx;
		psy_ui_RealPoint offset;

		textsize = psy_ui_font_textsize(font, text, count);
		textsizepx = psy_ui_size_px(&textsize, tm, NULL);
		offset = psy_ui_alignment_offset(self->format->alignment,
			psy_ui_realsize_make(width, tm->tmHeight), textsizepx);
		return offset.x;		
	}
	return 0.0;
}
