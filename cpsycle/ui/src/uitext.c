/*
** This source_ is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitext.h"
/* local */
#include "uiapp.h"
#include "uiimpfactory.h"
#include "uitextformat.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_TextPane */

/* prototypes */
static void psy_ui_textpane_on_destroyed(psy_ui_TextPane*);
static void psy_ui_textpane_on_preferred_size(psy_ui_TextPane*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static void psy_ui_textpane_on_key_down(psy_ui_TextPane*,
	psy_ui_KeyboardEvent*);
static void psy_ui_textpane_on_focus_lost(psy_ui_TextPane*);
static void psy_ui_textpane_on_draw(psy_ui_TextPane*,
	psy_ui_Graphics*);
static void psy_ui_textpane_on_mouse_down(psy_ui_TextPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textpane_on_mouse_move(psy_ui_TextPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textpane_on_mouse_up(psy_ui_TextPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textpane_on_mouse_double_click(psy_ui_TextPane*,
	psy_ui_MouseEvent*);
static void psy_ui_textpane_on_enable_input(psy_ui_TextPane*);
static void psy_ui_textpane_on_prevent_input(psy_ui_TextPane*);
static void psy_ui_edit_on_edit_accept(psy_ui_Edit*,
	psy_ui_TextPane* sender);
static void psy_ui_edit_on_edit_reject(psy_ui_Edit*,
	psy_ui_TextPane* sender);
static void psy_ui_textpane_prev_col(psy_ui_TextPane*,
	uintptr_t step);
static void psy_ui_textpane_next_col(psy_ui_TextPane*,
	uintptr_t step);
static void psy_ui_textpane_scroll_left(psy_ui_TextPane*);
static void psy_ui_textpane_scroll_right(psy_ui_TextPane*);
static void psy_ui_textpane_update_format(psy_ui_TextPane*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_TextPane* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super_vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_textpane_on_destroyed;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_textpane_on_draw;
		vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_textpane_on_preferred_size;		
		vtable.on_focus_lost =
			(psy_ui_fp_component)
			psy_ui_textpane_on_focus_lost;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			psy_ui_textpane_on_key_down;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textpane_on_mouse_down;
		vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textpane_on_mouse_move;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textpane_on_mouse_up;
		vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_textpane_on_mouse_double_click;
		vtable.enableinput =
			(psy_ui_fp_component)
			psy_ui_textpane_on_enable_input;
		vtable.preventinput =
			(psy_ui_fp_component)
			psy_ui_textpane_on_prevent_input;
		vtable_initialized = TRUE;
	}	
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void psy_ui_textpane_init(psy_ui_TextPane* self,
	psy_ui_Component* parent)
{ 
	psy_ui_component_init(psy_ui_textpane_base(self), parent, NULL);
	vtable_init(self);	
	psy_ui_component_set_aligner(&self->component, NULL);
	self->char_number_ = 0.0;	
	self->is_input_field_ = FALSE;		
	self->prevent_input_ = FALSE;
	self->down_ = FALSE;
	psy_ui_textformat_init(&self->format_);
	psy_ui_textsource_init(&self->source_);
	psy_signal_init(&self->signal_change);
	psy_signal_init(&self->signal_accept);
	psy_signal_init(&self->signal_reject);	
	psy_ui_component_set_style_type(&self->component,
		psy_ui_STYLE_EDIT);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_EDIT_FOCUS);
	psy_ui_component_set_scroll_step(psy_ui_textpane_base(self),
		psy_ui_size_make_em(1.0, 1.0));	
}

void psy_ui_textpane_on_destroyed(psy_ui_TextPane* self)
{	
	psy_signal_dispose(&self->signal_change);
	psy_signal_dispose(&self->signal_accept);
	psy_signal_dispose(&self->signal_reject);	
	psy_ui_textsource_dispose(&self->source_);
	psy_ui_textformat_dispose(&self->format_);
}

void psy_ui_textpane_enable_input_field(psy_ui_TextPane* self)
{
	self->is_input_field_ = TRUE;	
}

void psy_ui_textpane_set_text(psy_ui_TextPane* self,
	const char* text)
{	
	double width;
	const psy_ui_TextMetric* tm;
	const psy_ui_Font* font;
	
	assert(self);

	font = psy_ui_component_font(psy_ui_textpane_base(self));
	tm = psy_ui_font_textmetric(font);
	if ((self->char_number_ == 0.0)) {
		width = (self->char_number_) * tm->tmAveCharWidth;
	} else {
		psy_ui_RealSize size_px;

		size_px = psy_ui_component_scroll_size_px(&self->component);
		width = size_px.width;
	}
	psy_ui_textsource_set_text(&self->source_, text);
	psy_ui_textformat_clear(&self->format_);
	psy_ui_textformat_calc(&self->format_, psy_ui_textsource_text(&self->source_),
		width, font);
	psy_ui_component_invalidate(&self->component);
}

void psy_ui_textpane_add_text(psy_ui_TextPane* self,
	const char* text)
{
	assert(self);

	psy_ui_textsource_add_text(&self->source_, text);
	psy_ui_textformat_clear(&self->format_);	
	psy_ui_component_invalidate(&self->component);
}

 const char* psy_ui_textpane_text(const psy_ui_TextPane* self)
{	
	assert(self);

	return psy_ui_textsource_text(&self->source_);
}

 uintptr_t psy_ui_edit_length(const psy_ui_Edit* self)
 {
	 assert(self);

	 return psy_ui_textsource_len(&self->pane.source_);
 }

 void psy_ui_edit_range(psy_ui_Edit* self, intptr_t start,
	intptr_t end, char* rv)
 {
	 if (end > start) {
		 psy_snprintf(rv, end - start, self->pane.source_.text);
	 }
 }

void psy_ui_textpane_set_char_number(psy_ui_TextPane* self,
	double number)
{
	self->char_number_ = number;
	psy_ui_textformat_clear(&self->format_);
}

void psy_ui_textpane_on_preferred_size(psy_ui_TextPane* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{			
	const psy_ui_TextMetric* tm;	
	double width;
	const psy_ui_Font* font;	
	
	font = psy_ui_component_font(psy_ui_textpane_base(self));
	tm = psy_ui_component_textmetric(psy_ui_textpane_base(self));	
	width = (self->char_number_ == 0 && limit)
		? psy_ui_value_px(&limit->width, tm, limit)
		: (self->char_number_) * tm->tmAveCharWidth;
	psy_ui_textformat_calc(&self->format_, psy_ui_textsource_text(&self->source_), width, font);
	if (self->format_.line_wrap && !self->format_.word_wrap &&
			self->char_number_ == 0.0) {
		width = psy_max(psy_ui_value_px(&limit->width, tm, limit),
			self->format_.nummaxchars * tm->tmAveCharWidth);
	}	
	*rv = psy_ui_size_make_px(width, tm->tmHeight * 1.0);	
}

void psy_ui_textpane_enable_edit(psy_ui_TextPane* self)
{
	psy_ui_component_enable_input(&self->component, psy_ui_NONE_RECURSIVE);
	self->prevent_input_ = FALSE;
}

void psy_ui_textpane_prevent_edit(psy_ui_TextPane* self)
{
	psy_ui_component_prevent_input(&self->component, psy_ui_NONE_RECURSIVE);
	self->prevent_input_ = TRUE;
}

void psy_ui_textpane_on_enable_input(psy_ui_TextPane* self)
{
	self->prevent_input_ = FALSE;
}

void psy_ui_textpane_on_prevent_input(psy_ui_TextPane* self)
{
	self->prevent_input_ = TRUE;
}

void psy_ui_textpane_set_selection(psy_ui_TextPane* self,
	uintptr_t cpmin, uintptr_t cpmax)
{

}

/* signal_accept event handling */
void psy_ui_textpane_on_key_down(psy_ui_TextPane* self,
	psy_ui_KeyboardEvent* ev)
{	
	bool reset_selection;
	
	assert(self);

	if (self->prevent_input_) {
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
			if (self->source_.position.selection != psy_INDEX_INVALID) {				
				uintptr_t sel_start;
				uintptr_t sel_end;				
		
				psy_ui_textposition_selection(&self->source_.position, &sel_start,
					&sel_end);
				if (sel_end > sel_start) {
					char* seltext;
										
					seltext = (char*)malloc((sel_end - sel_start) + 2);
					psy_snprintf(seltext, sel_end - sel_start + 1, "%s",
						self->source_.text + sel_start);
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
				psy_ui_textsource_inserttext(&self->source_, psy_ui_clipboard_text(
					&psy_ui_app()->clipboard));
				psy_ui_textpane_update_format(self);
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
		if (self->is_input_field_) {
			psy_ui_keyboardevent_prevent_default(ev);						
			psy_signal_emit(&self->signal_reject, self, 0);
			psy_ui_keyboardevent_prevent_default(ev);
		}
		break;	
	case psy_ui_KEY_LEFT:
		if (psy_ui_keyboardevent_shift_key(ev)) {
			if (!psy_ui_textposition_has_selection(&self->source_.position)) {
				self->source_.position.selection = self->source_.position.caret;
			}
			reset_selection = FALSE;
		}
		psy_ui_textpane_prev_col(self, 1);		
		break;
	case psy_ui_KEY_RIGHT:
		if (psy_ui_keyboardevent_shift_key(ev)) {
			if (!psy_ui_textposition_has_selection(&self->source_.position)) {
				self->source_.position.selection = self->source_.position.caret;
			}
			reset_selection = FALSE;
		}
		psy_ui_textpane_next_col(self, 1);
		break;
	case psy_ui_KEY_UP:		
		break;
	case psy_ui_KEY_DOWN:		
		break;
	case psy_ui_KEY_PRIOR:		
		break;
	case psy_ui_KEY_NEXT:		
		break;
	case psy_ui_KEY_HOME: {
		const psy_ui_TextLine* curr;
		
		curr = psy_ui_textformat_line_from_position(&self->format_,
			self->source_.position.caret, self->source_.text);		
		if (curr) {
			if (psy_ui_keyboardevent_shift_key(ev)) {
				self->source_.position.selection = self->source_.position.caret;
				self->source_.position.caret = curr->start;
				reset_selection = FALSE;
			} else {
				self->source_.position.caret = curr->start;
			}
			psy_ui_textpane_scroll_left(self);
		} else {
			self->source_.position.caret = 0;
		}		
		break; }
	case psy_ui_KEY_END: {
		const psy_ui_TextLine* curr;
		
		curr = psy_ui_textformat_line_from_position(&self->format_,
			self->source_.position.caret, self->source_.text);
		if (curr) {
			if (psy_ui_keyboardevent_shift_key(ev)) {
				self->source_.position.selection = self->source_.position.caret;
				self->source_.position.caret = psy_ui_textline_line_end(curr, self->source_.text);
				reset_selection = FALSE;
			} else {
				self->source_.position.caret = psy_ui_textline_line_end(curr, self->source_.text);
			}
			psy_ui_textpane_scroll_right(self);
		} else {
			self->source_.position.caret = 0;			
		}
		break; }
	case psy_ui_KEY_BACK:
		if (psy_ui_textposition_has_selection(&self->source_.position)) {
			psy_ui_textsource_delete_selection(&self->source_);
			psy_ui_textpane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		} else {
			psy_ui_textsource_delete_char(&self->source_);
			psy_ui_textpane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
			
		}
		break;
	case psy_ui_KEY_DELETE:	
		if (psy_ui_textposition_has_selection(&self->source_.position)) {
			psy_ui_textsource_delete_selection(&self->source_);
			psy_ui_textpane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		} else {
			psy_ui_textsource_removechar(&self->source_);
			psy_ui_textpane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
		}
		break;
	case psy_ui_KEY_SPACE:		
		psy_ui_textsource_insert_char(&self->source_, ' ');
		psy_ui_textpane_update_format(self);
		psy_signal_emit(&self->signal_change, self, 0);
		break;
	case psy_ui_KEY_TAB:
		psy_ui_textsource_insert_char(&self->source_, '\t');
		psy_ui_textpane_update_format(self);
		psy_signal_emit(&self->signal_change, self, 0);
		break;
	case psy_ui_KEY_SHIFT:
	case psy_ui_KEY_CONTROL:
	case psy_ui_KEY_MENU:
		break;
	case psy_ui_KEY_RETURN:
		if (self->format_.line_wrap) {
			psy_ui_textsource_insert_char(&self->source_, '\n');
			psy_ui_textpane_update_format(self);
			psy_signal_emit(&self->signal_change, self, 0);
			psy_ui_component_align(psy_ui_component_parent(
				&self->component));
		} else if (self->is_input_field_) {
			psy_signal_emit(&self->signal_accept, self, 0);
		}		
		break;
	default: {
		unsigned char printable;
		
		printable = psy_ui_keyboardevent_printable_char(ev);		
		if (printable != '\0') {
			psy_ui_textsource_insert_char(&self->source_, printable);
			psy_ui_textpane_update_format(self);			
			psy_signal_emit(&self->signal_change, self, 0);
		} else if (psy_ui_keyboardevent_keycode(ev) ==
				psy_ui_KEY_RETURN) {
			
		}
		break; }
	}
	if (reset_selection) {
		self->source_.position.selection = psy_INDEX_INVALID;
	}
	psy_ui_component_invalidate(&self->component);
	psy_ui_keyboardevent_stop_propagation(ev);	
}

void psy_ui_textpane_update_format(psy_ui_TextPane* self)
{
	assert(self);

	psy_ui_textformat_update(&self->format_, self->source_.text,
		psy_ui_component_font(&self->component));
}

char psy_ui_textpane_char_from_keycode(psy_ui_TextPane* self,
	int keycode, bool shift_state)
{
	switch (keycode) {
		default:
		break;
	}
	return keycode;
}

void psy_ui_textpane_prev_col(psy_ui_TextPane* self,
	uintptr_t step)
{
	if (self->source_.position.caret > step) {
		self->source_.position.caret -= step;
	} else {
		self->source_.position.caret = 0;
	}
	psy_ui_textpane_scroll_left(self);
}

void psy_ui_textpane_next_col(psy_ui_TextPane* self, uintptr_t step)
{	
	self->source_.position.caret = psy_min(self->source_.position.caret + step,
		psy_strlen(self->source_.text));
}

uintptr_t psy_ui_textpane_cursor_column(const psy_ui_TextPane*
	self)
{
	return self->source_.position.caret;
}

void psy_ui_textpane_on_focus_lost(psy_ui_TextPane* self)
{
	assert(self);
	
	self->source_.position.selection = psy_INDEX_INVALID;	
	super_vtable.on_focus_lost(&self->component);
	psy_ui_component_set_tab_index(&self->component,
		psy_INDEX_INVALID);
	if (self->is_input_field_) {		
		psy_signal_emit(&self->signal_accept, self, 0);		
	}	
}

void psy_ui_textpane_on_draw(psy_ui_TextPane* self,
	psy_ui_Graphics* g)
{		
	psy_ui_TextDraw textdraw;	
	
	psy_ui_textdraw_init(&textdraw, &self->format_,
		psy_ui_component_size_px(psy_ui_textpane_base(self)),
		self->source_.text);
	psy_ui_textdraw_draw(&textdraw, g,		
		self->source_.position, (psy_ui_component_has_focus(&self->component) &&
		!(self->prevent_input_)));		
	psy_ui_textdraw_dispose(&textdraw);
}

void psy_ui_textpane_on_mouse_down(psy_ui_TextPane* self,
	psy_ui_MouseEvent* ev)
{	
	psy_ui_RealPoint pt;

	self->down_ = TRUE;
	self->sel_starting_ = TRUE;
	self->source_.position.selection = psy_INDEX_INVALID;
	pt = psy_ui_mouseevent_offset(ev);
	pt.y = 0;
	self->source_.position.caret = psy_min(
		psy_ui_textformat_cursor_position(&self->format_, self->source_.text,
			pt,
			psy_ui_component_textmetric(&self->component),
			psy_ui_component_font(&self->component)),
		psy_strlen(self->source_.text));
	psy_ui_mouseevent_stop_propagation(ev);	
	psy_ui_component_set_tab_index(&self->component, 0);
	psy_ui_component_set_focus(&self->component);	
	psy_ui_component_invalidate(&self->component);
	psy_ui_component_capture(&self->component);
}

void psy_ui_textpane_on_mouse_move(psy_ui_TextPane* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_component_set_cursor(&self->component,
		psy_ui_CURSORSTYLE_DEFAULT_TEXT);
	if (self->down_) {
		psy_ui_RealPoint pt;
		uintptr_t pos;		
		
		pt = psy_ui_mouseevent_offset(ev);
		pt.y = 0;
		pos = psy_min(
			psy_ui_textformat_cursor_position(&self->format_,
				self->source_.text, pt,
				psy_ui_component_textmetric(&self->component),
				psy_ui_component_font(&self->component)),
				psy_strlen(self->source_.text));
		if (self->sel_starting_) {
			self->source_.position.selection = pos;
			self->sel_starting_ = FALSE;
		}
		self->source_.position.caret = pos;
		psy_ui_component_invalidate(&self->component);
		psy_ui_mouseevent_stop_propagation(ev);	
	}	
}

void psy_ui_textpane_on_mouse_up(psy_ui_TextPane* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);
	self->down_ = FALSE;
}

void psy_ui_textpane_on_mouse_double_click(psy_ui_TextPane* self,
	psy_ui_MouseEvent* ev)
{
	self->source_.position.caret = psy_strlen(self->source_.text);
	self->source_.position.selection = 0;
	psy_ui_component_invalidate(&self->component);
	psy_ui_mouseevent_stop_propagation(ev);	
}


/* psy_ui_Edit */

/* prototypes */
static void psy_ui_edit_on_destroyed(psy_ui_Edit*);
static void psy_ui_edit_on_property_changed(psy_ui_Edit*,
	psy_Property* sender);
static void psy_ui_edit_before_property_destroyed(psy_ui_Edit*,
	psy_Property* sender);
static void psy_ui_edit_on_edit_accept(psy_ui_Edit*,
	psy_ui_TextPane* sender);
static void psy_ui_edit_on_edit_reject(psy_ui_Edit*,
	psy_ui_TextPane* sender);
static void psy_ui_edit_on_edit_change(psy_ui_Edit*,
	psy_ui_TextPane* sender);
static void psy_ui_edit_on_focus(psy_ui_Edit*);
static void psy_ui_edit_on_focus_lost(psy_ui_Edit*);
static void psy_ui_edit_on_key_down(psy_ui_Edit*,
	psy_ui_KeyboardEvent*);

/* psy_ui_edit_vtable */
static psy_ui_ComponentVtable psy_ui_edit_vtable;
static psy_ui_ComponentVtable psy_ui_edit_super_vtable;
static bool psy_ui_edit_vtable_initialized = FALSE;

static void psy_ui_edit_vtable_init(psy_ui_Edit* self)
{
	if (!psy_ui_edit_vtable_initialized) {
		psy_ui_edit_vtable = *(self->component.vtable);
		psy_ui_edit_super_vtable = *(self->component.vtable);
		psy_ui_edit_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_edit_on_destroyed;
		psy_ui_edit_vtable.on_focus =
			(psy_ui_fp_component)
			psy_ui_edit_on_focus;
		psy_ui_edit_vtable.on_focus_lost =
			(psy_ui_fp_component)
			psy_ui_edit_on_focus_lost;
		psy_ui_edit_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			psy_ui_edit_on_key_down;
		psy_ui_edit_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_edit_vtable);
}

/* implementation */
void psy_ui_edit_init(psy_ui_Edit* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(psy_ui_edit_base(self), parent, NULL);
	psy_ui_edit_vtable_init(self);
	self->property_ = NULL;
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_FOCUS);
	psy_ui_textpane_init(&self->pane, psy_ui_edit_base(self));
	psy_ui_component_set_align(&self->pane.component,
		psy_ui_ALIGN_CLIENT);	
	psy_signal_init(&self->signal_change);
	psy_signal_init(&self->signal_accept);
	psy_signal_init(&self->signal_reject);
	psy_signal_connect(&self->pane.signal_accept, self,
		psy_ui_edit_on_edit_accept);
	psy_signal_connect(&self->pane.signal_reject, self,
		psy_ui_edit_on_edit_reject);
	psy_signal_connect(&self->pane.signal_change, self,
		psy_ui_edit_on_edit_change);
	psy_ui_textformat_set_alignment(&self->pane.format_,
		psy_ui_ALIGNMENT_LEFT | psy_ui_ALIGNMENT_CENTER_VERTICAL);
	psy_ui_textformat_prevent_word_wrap(&self->pane.format_);
	psy_ui_textformat_prevent_line_wrap(&self->pane.format_);
}

void psy_ui_edit_on_destroyed(psy_ui_Edit* self)
{
	assert(self);
	
	if (self->property_) {
		psy_property_disconnect(self->property_, self);
	}
	psy_signal_dispose(&self->signal_change);
	psy_signal_dispose(&self->signal_accept);
	psy_signal_dispose(&self->signal_reject);
}

psy_ui_Edit* psy_ui_edit_alloc(void)
{
	return (psy_ui_Edit*)malloc(sizeof(psy_ui_Edit));
}

psy_ui_Edit* psy_ui_edit_allocinit(psy_ui_Component* parent)
{
	psy_ui_Edit* rv;

	rv = psy_ui_edit_alloc();
	if (rv) {
		psy_ui_edit_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_edit_exchange(psy_ui_Edit* self,
	psy_Property* property_)
{
	assert(self);
	
	self->property_ = property_;
	if (self->property_) {
		psy_ui_edit_on_property_changed(self, self->property_);
		psy_property_connect(self->property_, self,
			psy_ui_edit_on_property_changed);
		psy_signal_connect(&self->property_->before_destroyed, self,
			psy_ui_edit_before_property_destroyed);
		psy_ui_edit_enable_input_field(self);
	}
}

void psy_ui_edit_on_property_changed(psy_ui_Edit* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_is_int(sender)) {
		char text[64];

		psy_snprintf(text, 40,
			(psy_property_is_hex(self->property_)) ? "%X" : "%d",
			(int)psy_property_item_int(self->property_));
		psy_ui_edit_set_text(self, text);
	} else if (psy_property_is_double(sender)) {
		char text[64];

		psy_snprintf(text, 40, "%f", psy_property_item_double(sender));
		psy_ui_edit_set_text(self, text);
	} else if (psy_property_is_string(sender)) {
		psy_ui_edit_set_text(self, psy_property_item_str(sender));
	}
}

void psy_ui_edit_before_property_destroyed(psy_ui_Edit* self,
	psy_Property* sender)
{
	assert(self);

	self->property_ = NULL;
}

void psy_ui_edit_on_edit_accept(psy_ui_Edit* self,
	psy_ui_TextPane* sender)
{	
	assert(self);
	
	if (self->property_) {
		if (psy_property_is_double(self->property_)) {
			psy_property_set_item_double(self->property_,
				strtof(psy_ui_edit_text(self), NULL));
		} else if (psy_property_is_int(self->property_)) {
			psy_property_set_item_int(self->property_,
				(psy_property_is_hex(self->property_))
				? strtol(psy_ui_edit_text(self), NULL, 16)
				: atoi(psy_ui_edit_text(self)));
		} else if (psy_property_is_string(self->property_)) {
			psy_property_set_item_str(self->property_,
				psy_ui_edit_text(self));
		}
	}
	psy_signal_emit(&self->signal_accept, self, 0);
}

void psy_ui_edit_on_edit_reject(psy_ui_Edit* self,
	psy_ui_TextPane* sender)
{
	assert(self);
	
	psy_signal_emit(&self->signal_reject, self, 0);
}

void psy_ui_edit_on_edit_change(psy_ui_Edit* self,
	psy_ui_TextPane* sender)
{
	assert(self);
	
	psy_signal_emit(&self->signal_change, self, 0);
}

void psy_ui_textpane_scroll_left(psy_ui_TextPane* self)
{
	intptr_t column;	
	double screen_offset;	
	const psy_ui_TextMetric* tm;	
	
	assert(self);

	tm = psy_ui_component_textmetric(&self->component);	
	column = psy_ui_textpane_cursor_column(self);
	screen_offset = psy_ui_textformat_screen_offset(&self->format_,
		self->source_.text, psy_max(0, column),
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

void psy_ui_textpane_scroll_right(psy_ui_TextPane* self)
{
	intptr_t column;	
	psy_ui_RealSize client_size;
	const psy_ui_TextMetric* tm;
	double screen_offset;	

	tm = psy_ui_component_textmetric(&self->component);
	client_size = psy_ui_component_clientsize_px(&self->component);	
	column = psy_ui_textpane_cursor_column(self);
	screen_offset = psy_ui_textformat_screen_offset(&self->format_,
		self->source_.text, psy_max(0, column),
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

uintptr_t psy_ui_edit_cursor_column(const psy_ui_Edit* self)
{
	assert(self);

	return psy_ui_textpane_cursor_column(&self->pane);
}

void psy_ui_edit_on_focus(psy_ui_Edit* self)
{
	assert(self);

	super_vtable.on_focus(&self->component);	
}

void psy_ui_edit_on_focus_lost(psy_ui_Edit* self)
{
	assert(self);
	
	super_vtable.on_focus_lost(&self->component);
}

void psy_ui_edit_on_key_down(psy_ui_Edit* self,
	psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN) {
		psy_ui_component_set_tab_index(&self->pane.component, 0);
		psy_ui_component_set_focus(&self->pane.component);
		psy_ui_keyboardevent_stop_propagation(ev);
	}
}
