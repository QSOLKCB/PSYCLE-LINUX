/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitextarea.h"
/* local */
#include "uiapp.h"
#include "uiimpfactory.h"
#include "uitextformat.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_TextAreaPane */

/* prototypes */
static void psy_ui_textareapane_on_destroyed(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_on_preferred_size(psy_ui_TextAreaPane*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static void psy_ui_textareapane_on_key_down(psy_ui_TextAreaPane*,
	psy_ui_KeyboardEvent*);
static void psy_ui_textareapane_update_format(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_on_focus_lost(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_on_draw(psy_ui_TextAreaPane*,
	psy_ui_Graphics*);
static void psy_ui_textareapane_on_mouse_down(psy_ui_TextAreaPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textareapane_on_mouse_move(psy_ui_TextAreaPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textareapane_on_mouse_up(psy_ui_TextAreaPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textareapane_on_mouse_double_click(psy_ui_TextAreaPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textareapane_on_enable_input(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_on_prevent_input(psy_ui_TextAreaPane*);
static void psy_ui_textarea_on_edit_accept(psy_ui_TextArea*,
	psy_ui_TextAreaPane* sender);
static void psy_ui_textarea_on_edit_reject(psy_ui_TextArea*,
	psy_ui_TextAreaPane* sender);
static void psy_ui_textareapane_prev_col(psy_ui_TextAreaPane*,
	uintptr_t step);
static void psy_ui_textareapane_next_col(psy_ui_TextAreaPane*,
	uintptr_t step);
static void psy_ui_textareapane_prev_lines(psy_ui_TextAreaPane*,
	uintptr_t step);
static void psy_ui_textareapane_advance_lines(psy_ui_TextAreaPane*,
	uintptr_t step);
static void psy_ui_textareapane_scroll_up(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_scroll_down(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_scroll_left(psy_ui_TextAreaPane*);
static void psy_ui_textareapane_scroll_right(psy_ui_TextAreaPane*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_TextAreaPane* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super_vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_textareapane_on_destroyed;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_textareapane_on_draw;
		vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_textareapane_on_preferred_size;		
		vtable.on_focus_lost =
			(psy_ui_fp_component)
			psy_ui_textareapane_on_focus_lost;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			psy_ui_textareapane_on_key_down;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textareapane_on_mouse_down;
		vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textareapane_on_mouse_move;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textareapane_on_mouse_up;
		vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textareapane_on_mouse_double_click;
		vtable.enableinput =
			(psy_ui_fp_component)
			psy_ui_textareapane_on_enable_input;
		vtable.preventinput =
			(psy_ui_fp_component)
			psy_ui_textareapane_on_prevent_input;
		vtable_initialized = TRUE;
	}	
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void psy_ui_textareapane_init(psy_ui_TextAreaPane* self,
	psy_ui_Component* parent)
{ 
	psy_ui_component_init(psy_ui_textareapane_base(self), parent, NULL);
	vtable_init(self);	
	psy_ui_component_set_aligner(&self->component, NULL);
	psy_ui_component_set_tab_index(&self->component, 0);
	self->charnumber = 0.0;
	self->linenumber = psy_INDEX_INVALID;
	self->isinputfield = FALSE;
	psy_ui_textsource_init(&self->source);	
	self->prevent_input = FALSE;
	self->down = FALSE;
	psy_ui_textformat_init(&self->format);
	psy_signal_init(&self->signal_change);
	psy_signal_init(&self->signal_accept);
	psy_signal_init(&self->signal_reject);	
	psy_ui_component_set_style_type(&self->component,
		psy_ui_STYLE_EDIT);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_EDIT_FOCUS);
	psy_ui_component_set_scroll_step(psy_ui_textareapane_base(self),
		psy_ui_size_make_em(1.0, 1.0));	
}

void psy_ui_textareapane_on_destroyed(psy_ui_TextAreaPane* self)
{	
	psy_signal_dispose(&self->signal_change);
	psy_signal_dispose(&self->signal_accept);
	psy_signal_dispose(&self->signal_reject);	
	psy_ui_textsource_dispose(&self->source);
	psy_ui_textformat_dispose(&self->format);
}

void psy_ui_textareapane_enable_inputfield(psy_ui_TextAreaPane* self)
{
	self->isinputfield = TRUE;	
}

void psy_ui_textareapane_set_text(psy_ui_TextAreaPane* self,
	const char* text)
{		
	psy_ui_textsource_set_text(&self->source, text);
	psy_ui_textformat_clear(&self->format);	
	psy_ui_component_invalidate(&self->component);
}

void psy_ui_textareapane_add_text(psy_ui_TextAreaPane* self,
	const char* text)
{
	psy_ui_textsource_add_text(&self->source, text);
	psy_ui_textformat_clear(&self->format);	
	psy_ui_component_invalidate(&self->component);
}

 const char* psy_ui_textareapane_text(const psy_ui_TextAreaPane* self)
{	
	return psy_ui_textsource_text(&self->source);
}

 uintptr_t psy_ui_textarea_length(const psy_ui_TextArea* self)
 {
	 return psy_ui_textsource_len(&self->pane.source);
 }

 void psy_ui_textarea_range(psy_ui_TextArea* self, intptr_t start,
	intptr_t end, char* rv)
 {
	 if (end > start) {
		 psy_snprintf(rv, end - start, self->pane.source.text);
	 }
 }

void psy_ui_textareapane_set_char_number(psy_ui_TextAreaPane* self,
	double number)
{
	self->charnumber = number;
	psy_ui_textformat_clear(&self->format);
}

void psy_ui_textareapane_set_line_number(psy_ui_TextAreaPane* self,
	uintptr_t number)
{
	self->linenumber = number;
}

void psy_ui_textareapane_on_preferred_size(psy_ui_TextAreaPane* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{			
	const psy_ui_TextMetric* tm;	
	double width;
	const psy_ui_Font* font;
	uintptr_t lines;
	
	font = psy_ui_component_font(psy_ui_textareapane_base(self));
	tm = psy_ui_component_textmetric(psy_ui_textareapane_base(self));	
	width = (self->charnumber == 0 && limit)
		? psy_ui_value_px(&limit->width, tm, limit)
		: (self->charnumber) * tm->tmAveCharWidth;
	psy_ui_textformat_calc(&self->format, self->source.text, width, font);
	if (self->format.line_wrap && !self->format.word_wrap &&
			self->charnumber == 0.0) {
		width = psy_max(psy_ui_value_px(&limit->width, tm, limit),
			self->format.nummaxchars * tm->tmAveCharWidth);
	}
	if (self->linenumber == psy_INDEX_INVALID) {
		lines = psy_ui_textformat_num_lines(&self->format);
	} else {
		lines = self->linenumber;
	}
	*rv = psy_ui_size_make_px(width, lines * (tm->tmHeight * 1.0));	
}

void psy_ui_textareapane_enable_edit(psy_ui_TextAreaPane* self)
{
	psy_ui_component_enable_input(&self->component, psy_ui_NONE_RECURSIVE);
	self->prevent_input = FALSE;
}

void psy_ui_textareapane_prevent_edit(psy_ui_TextAreaPane* self)
{
	psy_ui_component_prevent_input(&self->component, psy_ui_NONE_RECURSIVE);
	self->prevent_input = TRUE;
}

void psy_ui_textareapane_on_enable_input(psy_ui_TextAreaPane* self)
{
	self->prevent_input = FALSE;
}

void psy_ui_textareapane_on_prevent_input(psy_ui_TextAreaPane* self)
{
	self->prevent_input = TRUE;
}

void psy_ui_textareapane_set_sel(psy_ui_TextAreaPane* self,
	uintptr_t cpmin, uintptr_t cpmax)
{

}

/* signal_accept event handling */
void psy_ui_textareapane_on_key_down(psy_ui_TextAreaPane* self,
	psy_ui_KeyboardEvent* ev)
{	
	bool reset_selection;
	
	assert(self);

	if (self->prevent_input) {
		return;
	}
	if ((psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_SHIFT) ||
		(psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_CONTROL)) {
		return;
	}
	reset_selection = TRUE;
	if (psy_ui_keyboardevent_ctrl_key(ev)) {
		switch (psy_ui_keyboardevent_keycode(ev)) {
		case psy_ui_KEY_C:		    
			if (self->source.position.selection != psy_INDEX_INVALID) {
				uintptr_t sel_start;
				uintptr_t sel_end;				
		
				psy_ui_textposition_selection(&self->source.position, &sel_start,
					&sel_end);
				if (sel_end > sel_start) {
					char* seltext;
										
					seltext = (char*)malloc((sel_end - sel_start) + 2);
					psy_snprintf(seltext, sel_end - sel_start + 1, "%s",
						self->source.text + sel_start);
					psy_ui_clipboard_set_text(&psy_ui_app()->clipboard,
						seltext);
					free(seltext);
					seltext = NULL;
				}
			}			
			break;
		case psy_ui_KEY_V:
			if (psy_strlen(psy_ui_clipboard_text(&psy_ui_app()->clipboard))
					> 0) {
				psy_ui_textsource_inserttext(&self->source, psy_ui_clipboard_text(
					&psy_ui_app()->clipboard));
				psy_ui_textareapane_update_format(self);
				psy_signal_emit(&self->signal_change, self, 0);
			}
			break;
		default:
			break;
		}
		psy_ui_keyboardevent_stop_propagation(ev);
		psy_ui_component_invalidate(&self->component);
		return;
	}
	switch (psy_ui_keyboardevent_keycode(ev)) {
	case psy_ui_KEY_ESCAPE:
		if (self->isinputfield) {
			psy_ui_keyboardevent_prevent_default(ev);						
			psy_signal_emit(&self->signal_reject, self, 0);
			psy_ui_keyboardevent_prevent_default(ev);
		}
		break;	
	case psy_ui_KEY_LEFT:
		if (psy_ui_keyboardevent_shift_key(ev)) {
			if (!psy_ui_textposition_has_selection(&self->source.position)) {
				self->source.position.selection = self->source.position.caret;
			}
			reset_selection = FALSE;
		}
		psy_ui_textareapane_prev_col(self, 1);		
		break;
	case psy_ui_KEY_RIGHT:
		if (psy_ui_keyboardevent_shift_key(ev)) {
			if (!psy_ui_textposition_has_selection(&self->source.position)) {
				self->source.position.selection = self->source.position.caret;
			}
			reset_selection = FALSE;
		}
		psy_ui_textareapane_next_col(self, 1);
		break;
	case psy_ui_KEY_UP:
		psy_ui_textareapane_prev_lines(self, 1);
		break;
	case psy_ui_KEY_DOWN:
		psy_ui_textareapane_advance_lines(self, 1);		
		break;
	case psy_ui_KEY_PRIOR:
		psy_ui_textareapane_prev_lines(self, 16);
		break;
	case psy_ui_KEY_NEXT:
		psy_ui_textareapane_advance_lines(self, 16);
		break;
	case psy_ui_KEY_HOME: {
		const psy_ui_TextLine* curr;

		curr = psy_ui_textformat_line_from_position(&self->format,
			self->source.position.caret, self->source.text);
		if (curr) {
			if (psy_ui_keyboardevent_shift_key(ev)) {
				self->source.position.selection = self->source.position.caret;
				self->source.position.caret = curr->start;
				reset_selection = FALSE;
			} else {
				self->source.position.caret = curr->start;
			}
			psy_ui_textareapane_scroll_left(self);
		}
		else {
			self->source.position.caret = 0;
		}
		break; }
	case psy_ui_KEY_END: {
		const psy_ui_TextLine* curr;

		curr = psy_ui_textformat_line_from_position(&self->format,
			self->source.position.caret, self->source.text);
		if (curr) {
			if (psy_ui_keyboardevent_shift_key(ev)) {
				self->source.position.selection = self->source.position.caret;
				self->source.position.caret = psy_ui_textline_line_end(curr, self->source.text);
				reset_selection = FALSE;
			} else {
				self->source.position.caret = psy_ui_textline_line_end(curr, self->source.text);
			}
			psy_ui_textareapane_scroll_right(self);
		}
		else {
			self->source.position.caret = 0;
		}
		break; }
	case psy_ui_KEY_BACK:
		if (psy_ui_textposition_has_selection(&self->source.position)) {
			psy_ui_textsource_delete_selection(&self->source);
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		} else {
			psy_ui_textsource_delete_char(&self->source);
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);			
		}
		break;
	case psy_ui_KEY_DELETE:	
		if (psy_ui_textposition_has_selection(&self->source.position)) {
			psy_ui_textsource_delete_selection(&self->source);
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		} else {
			psy_ui_textsource_removechar(&self->source);
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		}
		break;
	case psy_ui_KEY_SPACE:
		psy_ui_textsource_insert_char(&self->source, ' ');
		psy_ui_textareapane_update_format(self);
		psy_signal_emit(&self->signal_change, self, 0);
		break;
	case psy_ui_KEY_TAB:
		psy_ui_textsource_insert_char(&self->source, '\t');
		psy_ui_textareapane_update_format(self);
		psy_signal_emit(&self->signal_change, self, 0);
		break;
	case psy_ui_KEY_SHIFT:
	case psy_ui_KEY_CONTROL:
	case psy_ui_KEY_MENU:
		break;
	case psy_ui_KEY_RETURN:
		if (self->format.line_wrap) {
			psy_ui_textsource_insert_char(&self->source, '\n');
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
			psy_ui_component_align(psy_ui_component_parent(
				&self->component));
		} else if (self->isinputfield) {
			psy_signal_emit(&self->signal_accept, self, 0);
		}		
		break;
	default: {
		unsigned char printable;
		
		printable = psy_ui_keyboardevent_printable_char(ev);		
		if (printable != '\0') {
			psy_ui_textsource_insert_char(&self->source, printable);
			psy_ui_textareapane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		} else if (psy_ui_keyboardevent_keycode(ev) ==
				psy_ui_KEY_RETURN) {
			
		}
		break; }
	}
	if (reset_selection) {
		self->source.position.selection = psy_INDEX_INVALID;
	}
	psy_ui_component_invalidate(&self->component);
	psy_ui_keyboardevent_stop_propagation(ev);	
}

void psy_ui_textareapane_update_format(psy_ui_TextAreaPane* self)
{
	assert(self);

	psy_ui_textformat_update(&self->format, self->source.text,
		psy_ui_component_font(&self->component));
}

char psy_ui_textareapane_char_from_keycode(psy_ui_TextAreaPane* self,
	int keycode, bool shift_state)
{
	switch (keycode) {
		default:
		break;
	}
	return keycode;
}

void psy_ui_textareapane_prev_col(psy_ui_TextAreaPane* self,
	uintptr_t step)
{
	if (self->source.position.caret > step) {
		self->source.position.caret -= step;
	} else {
		self->source.position.caret = 0;
	}
	psy_ui_textareapane_scroll_left(self);
}

void psy_ui_textareapane_next_col(psy_ui_TextAreaPane* self, uintptr_t step)
{	
	self->source.position.caret = psy_min(self->source.position.caret + step,
		psy_strlen(self->source.text));
}

void psy_ui_textareapane_prev_lines(psy_ui_TextAreaPane* self, uintptr_t step)
{
	const psy_ui_TextLine* curr;
	uintptr_t currline;	
	/* uintptr_t offset; */

	curr = psy_ui_textformat_line_from_position(&self->format,
		self->source.position.caret, self->source.text);
	if (!curr) {
		return;
	}
	/* offset = self->position.caret - curr->start; */
	currline = curr->index;	
	if (currline > step) {
		currline -= step;
	} else {
		currline = 0;
	}
	curr = psy_ui_textformat_line(&self->format, currline);
	if (curr) {		
		self->source.position.caret = curr->start;
		/*if (offset < curr->size) {
			self->position.caret += offset;
		} else {
			self->position.caret += curr->size;
		}*/
	} else {
		self->source.position.caret = 0;
	}	
	psy_ui_textareapane_scroll_up(self);
}

void psy_ui_textareapane_advance_lines(psy_ui_TextAreaPane* self,
	uintptr_t step)
{
	const psy_ui_TextLine* curr;
	uintptr_t currline;
	uintptr_t numlines;
	/* uintptr_t offset; */

	numlines = psy_ui_textformat_num_lines(&self->format);
	if (numlines == 0) {
		return;
	}
	curr = psy_ui_textformat_line_from_position(&self->format,
		self->source.position.caret, self->source.text);
	if (!curr) {
		return;
	}
/* 	offset = self->position.caret - curr->start; */
	currline = curr->index;	
	if (currline + step < numlines) {
		currline += step;
	} else {
		currline = numlines - 1;
	}
	curr = psy_ui_textformat_line(&self->format, currline);
	if (curr) {		
		self->source.position.caret = curr->start;
		/*if (offset < curr->size) {
			self->position.caret += offset;
		} else {
			self->position.caret += curr->size;
		}*/
	} else {
		self->source.position.caret = 0;
	}	
	psy_ui_textareapane_scroll_down(self);
}

uintptr_t psy_ui_textareapane_cursor_line(const psy_ui_TextAreaPane*
	self)
{
	const psy_ui_TextLine* curr;	

	assert(self);
		
	curr = psy_ui_textformat_line_from_position(&self->format,
		self->source.position.caret, self->source.text);
	if (!curr) {
		return 0;
	}
	return curr->index;
}

uintptr_t psy_ui_textareapane_cursor_column(const psy_ui_TextAreaPane*
	self)
{
	const psy_ui_TextLine* curr;	
	uintptr_t numlines;	

	numlines = psy_ui_textformat_num_lines(&self->format);
	if (numlines == 0) {
		return 0;
	}
	curr = psy_ui_textformat_line_from_position(&self->format,
		self->source.position.caret, self->source.text);
	if (!curr) {
		return 0;
	}
	return self->source.position.caret - curr->start;
}

uintptr_t psy_ui_textareapane_num_lines(const psy_ui_TextAreaPane* self)
{
	assert(self);
	
	return psy_ui_textformat_num_lines(&self->format);
}

void psy_ui_textareapane_on_focus_lost(psy_ui_TextAreaPane* self)
{
	assert(self);
	
	self->source.position.selection = psy_INDEX_INVALID;
	super_vtable.on_focus_lost(&self->component);
	if (self->isinputfield) {		
		psy_signal_emit(&self->signal_accept, self, 0);		
	}	
}

void psy_ui_textareapane_on_draw(psy_ui_TextAreaPane* self,
	psy_ui_Graphics* g)
{		
	psy_ui_TextDraw textdraw;	
	
	psy_ui_textdraw_init(&textdraw, &self->format,
		psy_ui_component_size_px(psy_ui_textareapane_base(self)),
		self->source.text);
	psy_ui_textdraw_draw(&textdraw, g,		
		self->source.position, (psy_ui_component_has_focus(&self->component) &&
		!(self->prevent_input)));		
	psy_ui_textdraw_dispose(&textdraw);
}

void psy_ui_textareapane_on_mouse_down(psy_ui_TextAreaPane* self,
	psy_ui_MouseEvent* ev)
{	
	self->down = TRUE;
	self->sel_starting = TRUE;
	self->source.position.selection = psy_INDEX_INVALID;
	self->source.position.caret = psy_min(
		psy_ui_textformat_cursor_position(&self->format, self->source.text,
			psy_ui_mouseevent_offset(ev),
			psy_ui_component_textmetric(&self->component),
			psy_ui_component_font(&self->component)),
		psy_strlen(self->source.text));
	psy_ui_mouseevent_stop_propagation(ev);	
	psy_ui_component_set_focus(&self->component);	
	psy_ui_component_invalidate(&self->component);
	psy_ui_component_capture(&self->component);
}

void psy_ui_textareapane_on_mouse_move(psy_ui_TextAreaPane* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_component_set_cursor(&self->component,
		psy_ui_CURSORSTYLE_DEFAULT_TEXT);
	if (self->down) {
		uintptr_t pos;
		
		pos = psy_min(
			psy_ui_textformat_cursor_position(&self->format, self->source.text,
				psy_ui_mouseevent_offset(ev),
				psy_ui_component_textmetric(&self->component),
				psy_ui_component_font(&self->component)),
				psy_strlen(self->source.text));
		if (self->sel_starting) {
			self->source.position.selection = pos;
			self->sel_starting = FALSE;
		}
		self->source.position.caret = pos;
		psy_ui_component_invalidate(&self->component);
		psy_ui_mouseevent_stop_propagation(ev);	
	}	
}

void psy_ui_textareapane_on_mouse_up(psy_ui_TextAreaPane* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);
	self->down = FALSE;
}

void psy_ui_textareapane_on_mouse_double_click(psy_ui_TextAreaPane* self,
	psy_ui_MouseEvent* ev)
{
	self->source.position.caret = psy_strlen(self->source.text);
	self->source.position.selection = 0;
	psy_ui_component_invalidate(&self->component);
	psy_ui_mouseevent_stop_propagation(ev);	
}

/* psy_ui_TextArea */

/* prototypes */
static void psy_ui_textarea_init_internal(psy_ui_TextArea*,
	psy_ui_Component* parent);
static void psy_ui_textarea_on_destroyed(psy_ui_TextArea*);
static void psy_ui_textarea_on_property_changed(psy_ui_TextArea*,
	psy_Property* sender);
static void psy_ui_textarea_before_property_destroyed(psy_ui_TextArea*,
	psy_Property* sender);
static void psy_ui_textarea_on_edit_accept(psy_ui_TextArea*,
	psy_ui_TextAreaPane* sender);
static void psy_ui_textarea_on_edit_reject(psy_ui_TextArea*,
	psy_ui_TextAreaPane* sender);
static void psy_ui_textarea_on_edit_change(psy_ui_TextArea*,
	psy_ui_TextAreaPane* sender);

/* psy_ui_textarea_vtable */
static psy_ui_ComponentVtable psy_ui_textarea_vtable;
static psy_ui_ComponentVtable psy_ui_textarea_super_vtable;
static bool psy_ui_textarea_vtable_initialized = FALSE;

static void psy_ui_textarea_vtable_init(psy_ui_TextArea* self)
{
	if (!psy_ui_textarea_vtable_initialized) {
		psy_ui_textarea_vtable = *(self->component.vtable);
		psy_ui_textarea_super_vtable = *(self->component.vtable);
		psy_ui_textarea_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_textarea_on_destroyed;		
		psy_ui_textarea_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_textarea_vtable);
}

/* implementation */
void psy_ui_textarea_init(psy_ui_TextArea* self, psy_ui_Component* parent)
{
	psy_ui_textarea_init_internal(self, parent);
	psy_ui_component_set_wheel_scroll(&self->pane.component, 4);
	psy_ui_component_set_overflow(&self->pane.component,
		psy_ui_OVERFLOW_SCROLL);
}

void psy_ui_textarea_init_internal(psy_ui_TextArea* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(psy_ui_textarea_base(self), parent, NULL);
	psy_ui_textarea_vtable_init(self);
	self->property = NULL;
	psy_ui_textareapane_init(&self->pane, psy_ui_textarea_base(self));
	psy_ui_scroller_init(&self->scroller, psy_ui_textarea_base(self),
		NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->pane.component);
	psy_ui_component_set_align(&self->scroller.component,
		psy_ui_ALIGN_CLIENT);
	psy_signal_init(&self->signal_change);
	psy_signal_init(&self->signal_accept);
	psy_signal_init(&self->signal_reject);
	psy_signal_connect(&self->pane.signal_accept, self,
		psy_ui_textarea_on_edit_accept);
	psy_signal_connect(&self->pane.signal_reject, self,
		psy_ui_textarea_on_edit_reject);
	psy_signal_connect(&self->pane.signal_change, self,
		psy_ui_textarea_on_edit_change);
}

void psy_ui_textarea_on_destroyed(psy_ui_TextArea* self)
{
	assert(self);
	
	if (self->property) {
		psy_property_disconnect(self->property, self);
	}
	psy_signal_dispose(&self->signal_change);
	psy_signal_dispose(&self->signal_accept);
	psy_signal_dispose(&self->signal_reject);
}

psy_ui_TextArea* psy_ui_textarea_alloc(void)
{
	return (psy_ui_TextArea*)malloc(sizeof(psy_ui_TextArea));
}

psy_ui_TextArea* psy_ui_textarea_allocinit(psy_ui_Component* parent)
{
	psy_ui_TextArea* rv;

	rv = psy_ui_textarea_alloc();
	if (rv) {
		psy_ui_textarea_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_textarea_exchange(psy_ui_TextArea* self,
	psy_Property* property)
{
	assert(self);
	
	self->property = property;
	if (self->property) {
		psy_ui_textarea_on_property_changed(self, self->property);
		psy_property_connect(self->property, self,
			psy_ui_textarea_on_property_changed);
		psy_signal_connect(&self->property->before_destroyed, self,
			psy_ui_textarea_before_property_destroyed);
		psy_ui_textarea_enable_input_field(self);
	}
}

void psy_ui_textarea_on_property_changed(psy_ui_TextArea* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_is_int(sender)) {
		char text[64];

		psy_snprintf(text, 40,
			(psy_property_is_hex(self->property)) ? "%X" : "%d",
			(int)psy_property_item_int(self->property));
		psy_ui_textarea_set_text(self, text);
	} else if (psy_property_is_double(sender)) {
		char text[64];

		psy_snprintf(text, 40, "%f", psy_property_item_double(sender));
		psy_ui_textarea_set_text(self, text);
	} else if (psy_property_is_string(sender)) {
		psy_ui_textarea_set_text(self, psy_property_item_str(sender));
	}
}

void psy_ui_textarea_before_property_destroyed(psy_ui_TextArea* self,
	psy_Property* sender)
{
	assert(self);

	self->property = NULL;
}

void psy_ui_textarea_prevent_wrap(psy_ui_TextArea* self)
{
	assert(self);
	
	psy_ui_textformat_prevent_word_wrap(&self->pane.format);
	psy_ui_textformat_prevent_line_wrap(&self->pane.format);
}

void psy_ui_textarea_word_wrap(psy_ui_TextArea* self)
{
	assert(self);
	
	psy_ui_textformat_word_wrap(&self->pane.format);
}

void psy_ui_textarea_line_wrap(psy_ui_TextArea* self)
{
	assert(self);
	
	psy_ui_textformat_line_wrap(&self->pane.format);
}

void psy_ui_textarea_on_edit_accept(psy_ui_TextArea* self,
	psy_ui_TextAreaPane* sender)
{	
	assert(self);
	
	if (self->property) {
		if (psy_property_is_double(self->property)) {
			psy_property_set_item_double(self->property,
				strtof(psy_ui_textarea_text(self), NULL));
		} else if (psy_property_is_int(self->property)) {
			psy_property_set_item_int(self->property,
				(psy_property_is_hex(self->property))
				? strtol(psy_ui_textarea_text(self), NULL, 16)
				: atoi(psy_ui_textarea_text(self)));
		} else if (psy_property_is_string(self->property)) {
			psy_property_set_item_str(self->property,
				psy_ui_textarea_text(self));
		}
	}
	psy_signal_emit(&self->signal_accept, self, 0);
}

void psy_ui_textarea_on_edit_reject(psy_ui_TextArea* self,
	psy_ui_TextAreaPane* sender)
{
	assert(self);
	
	psy_signal_emit(&self->signal_reject, self, 0);
}

void psy_ui_textarea_on_edit_change(psy_ui_TextArea* self,
	psy_ui_TextAreaPane* sender)
{
	assert(self);
	
	psy_signal_emit(&self->signal_change, self, 0);
}

void psy_ui_textareapane_scroll_up(psy_ui_TextAreaPane* self)
{
	intptr_t line;
	intptr_t topline;
	double top;
	double line_height;
	const psy_ui_TextMetric* tm;

	assert(self);
	
	line = psy_ui_textareapane_cursor_line(self);
	tm = psy_ui_component_textmetric(&self->component);
	line_height = tm->tmHeight * self->format.linespacing;
	top = line_height * line;
	topline = 0;	
	if (psy_ui_component_scroll_top_px(&self->component) +
			topline * line_height > top) {
		intptr_t dlines;

		dlines = (intptr_t)((psy_ui_component_scroll_top_px(
			&self->component) + topline * line_height - top) /
			(line_height));
		psy_ui_component_set_scroll_top_px(&self->component,
			psy_ui_component_scroll_top_px(&self->component) -
			psy_ui_component_scroll_step_height_px(&self->component) *
				dlines);
	}
}

void psy_ui_textareapane_scroll_down(psy_ui_TextAreaPane* self)
{
	intptr_t line;
	intptr_t visilines;
	psy_ui_RealSize clientsize;
	double line_height;
	const psy_ui_TextMetric* tm;
	
	assert(self);

	line = psy_ui_textareapane_cursor_line(self);
	tm = psy_ui_component_textmetric(&self->component);
	line_height = tm->tmHeight * self->format.linespacing;
	clientsize = psy_ui_component_clientsize_px(&self->component);
	visilines = (uintptr_t)(clientsize.height / line_height);		
	if (visilines < line - psy_ui_component_scroll_top_px(
			&self->component) / line_height) {
		intptr_t dlines;

		dlines = (intptr_t)
			(line - psy_ui_component_scroll_top_px(&self->component) /
				line_height - visilines);
		psy_ui_component_set_scroll_top_px(&self->component,
			psy_ui_component_scroll_top_px(&self->component) +
			psy_ui_component_scroll_step_height_px(&self->component) *
				dlines);
	}
}

void psy_ui_textareapane_scroll_left(psy_ui_TextAreaPane* self)
{
	intptr_t column;	
	intptr_t linestart;
	double screen_offset;	
	const psy_ui_TextMetric* tm;
	const psy_ui_TextLine* curr;
	
	assert(self);

	tm = psy_ui_component_textmetric(&self->component);		
	curr = psy_ui_textformat_line_from_position(&self->format, 
		self->source.position.caret, self->source.text);
	if (curr) {
		linestart = curr->start;
	} else {
		return;
	}	
	column = psy_ui_textareapane_cursor_column(self);
	screen_offset = psy_ui_textformat_screen_offset(&self->format,
		self->source.text + linestart, psy_max(0, column),
		psy_ui_component_font(&self->component),
		tm);				
	if (psy_ui_component_scroll_left_px(&self->component) >
			screen_offset) {
		intptr_t chars;
		
		chars = (intptr_t)(screen_offset / tm->tmAveCharWidth);
		psy_ui_component_set_scroll_left(&self->component,
			psy_ui_value_make_px((double)(chars *
				(intptr_t)tm->tmAveCharWidth)));
	}
}

void psy_ui_textareapane_scroll_right(psy_ui_TextAreaPane* self)
{
	intptr_t column;	
	intptr_t linestart;	
	psy_ui_RealSize client_size;
	const psy_ui_TextMetric* tm;
	double screen_offset;
	const psy_ui_TextLine* curr;

	tm = psy_ui_component_textmetric(&self->component);
	client_size = psy_ui_component_clientsize_px(&self->component);
	curr = psy_ui_textformat_line_from_position(&self->format, 
		self->source.position.caret, self->source.text);
	if (curr) {
		linestart = curr->start;
	} else {
		return;
	}
	column = psy_ui_textareapane_cursor_column(self);
	screen_offset = psy_ui_textformat_screen_offset(&self->format,
		self->source.text + linestart, psy_max(0, column),
		psy_ui_component_font(&self->component),
		tm);	
	if (screen_offset >
			psy_ui_component_scroll_left_px(&self->component) +
			client_size.width) {				
		psy_ui_component_set_scroll_left(&self->component,
			psy_ui_value_make_px(psy_ui_component_scroll_left_px(
				&self->component) + tm->tmAveCharWidth));
	}
}

uintptr_t psy_ui_textarea_cursor_line(const psy_ui_TextArea* self)
{
	return psy_ui_textareapane_cursor_line(&self->pane);
}

uintptr_t psy_ui_textarea_cursor_column(const psy_ui_TextArea* self)
{
	return psy_ui_textareapane_cursor_column(&self->pane);
}

uintptr_t psy_ui_textarea_num_lines(const psy_ui_TextArea* self)
{
	return psy_ui_textareapane_num_lines(&self->pane);
}
