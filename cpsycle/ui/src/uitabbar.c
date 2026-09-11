/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uitabbar.h"
/* local */
#include "uiapp.h"
#include "trackercmds.h"
/* platform */
#include "../../detail/portable.h"


/* protoypes */
static void psy_ui_tab_on_destroyed(psy_ui_Tab*);
static void psy_ui_tab_on_mouse_down(psy_ui_Tab*, psy_ui_MouseEvent*);
static void psy_ui_tab_on_draw(psy_ui_Tab*, psy_ui_Graphics*);
static void psy_ui_tab_on_preferred_size(psy_ui_Tab*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void psy_ui_tab_on_key_down(psy_ui_Tab*, psy_ui_KeyboardEvent*);
static void psy_ui_tab_on_language_changed(psy_ui_Tab*);
static uintptr_t psy_ui_tabbar_index_by_id(psy_ui_TabBar*, uintptr_t id);
static bool psy_ui_tab_on_input(psy_ui_Tab*, InputHandler* sender);

/* vtable */
static psy_ui_ComponentVtable psy_ui_tab_vtable;
static bool psy_ui_tab_vtable_initialized = FALSE;

static void psy_ui_tab_vtable_init(psy_ui_Tab* self)
{
	if (!psy_ui_tab_vtable_initialized) {
		psy_ui_tab_vtable = *(self->component.vtable);
		psy_ui_tab_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_tab_on_destroyed;
		psy_ui_tab_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_tab_on_mouse_down;
		psy_ui_tab_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			psy_ui_tab_on_key_down;
		psy_ui_tab_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_tab_on_draw;
		psy_ui_tab_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_tab_on_preferred_size;
		psy_ui_tab_vtable.on_language_changed =
			(psy_ui_fp_component)
			psy_ui_tab_on_language_changed;
		psy_ui_tab_vtable_initialized = TRUE;
	}
	self->component.vtable = &psy_ui_tab_vtable;
}

/* implementation */
void psy_ui_tab_init(psy_ui_Tab* self, psy_ui_Component* parent,
	const char* text, uintptr_t index)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_tab_vtable_init(self);
	psy_ui_component_set_aligner(&self->component, NULL);
	psy_ui_component_set_style_types(&self->component,
		psy_ui_STYLE_TAB, psy_ui_STYLE_TAB_HOVER, psy_ui_STYLE_TAB_SELECT,
		psy_INDEX_INVALID);
	psy_ui_component_set_tab_index(&self->component, 0);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_TAB_FOCUS);
	psy_signal_init(&self->signal_clicked);	
	psy_ui_svg_init(&self->svg);
	self->text = psy_strdup(text);
	self->translation = NULL;
	self->prevent_translation = FALSE;
	self->key = psy_strdup("");
	self->vertical_icon = FALSE;
	psy_strreset(&self->translation, psy_ui_translate(text));
	self->istoggle = FALSE;
	self->mode = psy_ui_TABMODE_SINGLESEL;
	self->checkstate = 0;	
	self->index = index;	
	self->bitmapident = 1.0;	
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS, psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_tab_on_input);
}

psy_ui_Tab* psy_ui_tab_alloc(void)
{
	return (psy_ui_Tab*)malloc(sizeof(psy_ui_Tab));
}

psy_ui_Tab* psy_ui_tab_alloc_init(psy_ui_Component* parent,
	const char* text, uintptr_t index)
{
	psy_ui_Tab* rv;

	rv = psy_ui_tab_alloc();
	if (rv) {
		psy_ui_tab_init(rv, parent, text, index);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_tab_on_destroyed(psy_ui_Tab* self)
{
	assert(self);

	free(self->text);
	self->text = NULL;
	free(self->translation);
	self->translation = NULL;
	free(self->key);
	self->key = NULL;	
	psy_signal_dispose(&self->signal_clicked);
	inputhandler_disconnect(
		psy_ui_app_input_handler(psy_ui_app()),
		self, (fp_inputhandler_input)psy_ui_tab_on_input);
}

void psy_ui_tab_set_text(psy_ui_Tab* self, const char* text)
{
	assert(self);

	psy_strreset(&self->text, text);	
	if (self->prevent_translation) {
		free(self->translation);
		self->translation = NULL;
	} else {
		psy_strreset(&self->translation, psy_ui_translate(text));
	}
}

void psy_ui_tab_set_icon(psy_ui_Tab* self, const psy_ui_SVG* svg)
{
	assert(self);

	psy_ui_svg_copy(&self->svg, svg);
}

void psy_ui_tab_set_key(psy_ui_Tab* self, const char* text)
{
	assert(self);

	psy_strreset(&self->key, text);	
}

void psy_ui_tab_set_mode(psy_ui_Tab* self, TabMode mode)
{
	self->mode = mode;	
	if (mode == psy_ui_TABMODE_LABEL) {
		psy_ui_component_set_style_types(&self->component,
			psy_ui_STYLE_TAB_LABEL, psy_INDEX_INVALID, psy_INDEX_INVALID,
			psy_INDEX_INVALID);		
	} else {
		psy_ui_component_set_style_types(&self->component,
			psy_ui_STYLE_TAB, psy_ui_STYLE_TAB_HOVER, psy_ui_STYLE_TAB_SELECT,
			psy_INDEX_INVALID);
	}
}

void psy_ui_tab_prevent_translation(psy_ui_Tab* self)
{
	self->prevent_translation = TRUE;
	free(self->translation);
	self->translation = NULL;
}

void psy_ui_tab_on_draw(psy_ui_Tab* self, psy_ui_Graphics* g)
{
	char* text;
	psy_ui_RealSize size;
	const psy_ui_TextMetric* tm;
	psy_ui_RealPoint center;	
	double textident;

	assert(self);	
		
	textident = 0.0;
	tm = psy_ui_component_textmetric(&self->component);
	size = psy_ui_component_scroll_size_px(&self->component);
	center = psy_ui_realpoint_make(textident, 0.0);
	if (self->translation) {
		text = self->translation;
	}
	else {
		text = self->text;
	}
	if (self->vertical_icon) {		
		if (!psy_ui_svg_empty(&self->svg)) {
			psy_ui_SVGDraw draw;
			psy_ui_RealSize svgsize;
			double hcenter;

			svgsize = psy_ui_svg_viewport(&self->svg);
			hcenter = (size.width - svgsize.width) / 2.0;
			psy_ui_svgdraw_init(&draw, &self->svg, svgsize);
			psy_ui_svgdraw_draw(&draw, g, psy_ui_realpoint_make(
				hcenter, tm->tmHeight * 0.25));			
			center.y = svgsize.height + tm->tmHeight * 0.75;
		}
		if (psy_strlen(text) > 0) {			
			center.x = (size.width - tm->tmAveCharWidth * psy_strlen(text)) / 2.0;
			psy_ui_graphics_textout(g, center, text, psy_strlen(text));
		}
	} else {		
		if (!psy_ui_svg_empty(&self->svg)) {
			psy_ui_SVGDraw draw;
			psy_ui_RealSize svgsize;
			double vcenter;

			svgsize = psy_ui_svg_viewport(&self->svg);
			vcenter = (size.height - svgsize.height) / 2.0;
			psy_ui_svgdraw_init(&draw, &self->svg, svgsize);
			psy_ui_svgdraw_draw(&draw, g, psy_ui_realpoint_make(0.0, vcenter));			
			textident += svgsize.width + tm->tmAveCharWidth * self->bitmapident;
		}
		if (psy_strlen(text) > 0) {
			center.x = textident;
			center.y = (size.height - tm->tmHeight) / 2.0;
			psy_ui_graphics_textout(g, center, text, psy_strlen(text));
		}
	}
}

void psy_ui_tab_on_preferred_size(psy_ui_Tab* self, const psy_ui_Size* limit,
	psy_ui_Size* rv)
{		
	const psy_ui_TextMetric* tm;
	char* text;	
	const psy_ui_Font* font;	
	
	if (self->translation) {
		text = self->translation;
	} else {
		text = self->text;
	}
	font = psy_ui_component_font(&self->component);
	if (font) {
		*rv = psy_ui_font_textsize(font, text, psy_strlen(text));
	} else {
		*rv = psy_ui_size_zero();
	}	
	rv->height = psy_ui_value_make_eh(1.8);
	tm = psy_ui_component_textmetric(psy_ui_tab_base(self));		
	if (!psy_ui_svg_empty(&self->svg)) {
		psy_ui_RealSize svgsize;
		psy_ui_RealSize textsizepx;

		svgsize = psy_ui_svg_viewport(&self->svg);
		textsizepx = psy_ui_size_px(rv, tm, NULL);
		if (self->vertical_icon) {
			rv->width = psy_ui_value_make_px(psy_max(textsizepx.width,
				svgsize.width) + tm->tmAveCharWidth * self->bitmapident);
			rv->height = psy_ui_value_make_px(tm->tmHeight * 2.0 + svgsize.height);
		}
		else {
			rv->width = psy_ui_value_make_px(textsizepx.width + svgsize.width
				+ tm->tmAveCharWidth * self->bitmapident);
		}
	}
}

void psy_ui_tab_on_mouse_down(psy_ui_Tab* self, psy_ui_MouseEvent* ev)
{	
	if (self->mode != psy_ui_TABMODE_LABEL) {
		if (self->istoggle) {
			if (self->checkstate != TABCHECKSTATE_OFF) {
				self->checkstate = TABCHECKSTATE_OFF;
				psy_ui_component_remove_style_state(&self->component,
					psy_ui_STYLE_TAB_SELECT);
			} else {
				self->checkstate = TABCHECKSTATE_ON;
				psy_ui_component_add_style_state(&self->component,
					psy_ui_STYLE_TAB_SELECT);
			}
		}		
		psy_signal_emit(&self->signal_clicked, self, 0);
	}
}

bool psy_ui_tab_on_input(psy_ui_Tab* self, InputHandler* sender)
{
	assert(self);

	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVSELECT: {		
		psy_signal_emit(&self->signal_clicked, self, 0);
		return TRUE; }
	default:
		return FALSE;
	}
}

void psy_ui_tab_on_key_down(psy_ui_Tab* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN &&
			!psy_ui_component_input_prevented(&self->component)) {
		psy_signal_emit(&self->signal_clicked, self, 0);
		psy_ui_keyboardevent_stop_propagation(ev);
	}
}


void psy_ui_tab_on_language_changed(psy_ui_Tab* self)
{	
	psy_strreset(&self->translation, psy_ui_translate(self->text));
	psy_ui_component_invalidate(&self->component);
}


/* psy_ui_TabBar */
/* prototypes */
static void tabbar_on_destroyed(psy_ui_TabBar*);
static void tabbar_on_tab_clicked(psy_ui_TabBar*, psy_ui_Tab* sender);
static void tabbar_on_mouse_wheel(psy_ui_TabBar*, psy_ui_MouseEvent*);
/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_TabBar* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			tabbar_on_destroyed;
		vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			tabbar_on_mouse_wheel;
		vtable_initialized = TRUE;
	}
	self->component.vtable = &vtable;
}

/* implementation */
void psy_ui_tabbar_init(psy_ui_TabBar* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(psy_ui_tabbar_base(self), parent, NULL);
	vtable_init(self);	
	psy_ui_component_set_style_type(&self->component, psy_ui_STYLE_TABBAR);
	psy_signal_init(&self->signal_change);
	self->num_tabs = 0;
	self->selected = 0;
	self->prevent_translation = FALSE;
	self->tab_alignment = psy_ui_ALIGN_LEFT;
	psy_ui_component_set_default_align(&self->component, self->tab_alignment,
		psy_ui_margin_zero());
}

void tabbar_on_destroyed(psy_ui_TabBar* self)
{	
	assert(self);
	
	psy_signal_dispose(&self->signal_change);
}

void psy_ui_tabbar_set_tab_align(psy_ui_TabBar* self, psy_ui_AlignType align)
{	
	psy_List* p;
	psy_List* q;

	assert(self);
	
	self->tab_alignment = align;
	if (self->component.aligner) {
		psy_ui_component_set_default_align(&self->component, align,
			self->component.aligner->insertmargin);
	}
	q = psy_ui_component_children(psy_ui_tabbar_base(self),
			psy_ui_NONE_RECURSIVE, FALSE);
	for (p = q; p != NULL; p = p->next) {
		psy_ui_component_set_align((psy_ui_Component*)psy_list_entry(p),
			align);
	}
	psy_list_free(q);	
}

void psy_ui_tabbar_prevent_translation(psy_ui_TabBar* self)
{
	self->prevent_translation = TRUE;
}

void tabbar_on_tab_clicked(psy_ui_TabBar* self, psy_ui_Tab* sender)
{	
	psy_ui_tabbar_select(self, sender->index);	
}

void psy_ui_tabbar_mark(psy_ui_TabBar* self, uintptr_t tabindex)
{
	psy_ui_Tab* tab;

	assert(self);

	tab = psy_ui_tabbar_tab(self, tabindex);
	if (tab) {
		if (tab->mode == psy_ui_TABMODE_LABEL) {
			return;
		}
		if (!tab->istoggle) {
			psy_ui_Tab* oldtab;

			oldtab = psy_ui_tabbar_tab(self, self->selected);
			if (oldtab) {
				psy_ui_component_remove_style_state(&oldtab->component,
					psy_ui_STYLESTATE_SELECT);
			}
			self->selected = tabindex;
			psy_ui_component_add_style_state(&tab->component,
				psy_ui_STYLESTATE_SELECT);
		}
	} else {
		psy_ui_Tab* oldtab;

		oldtab = psy_ui_tabbar_tab(self, self->selected);
		if (oldtab) {
			psy_ui_component_remove_style_state(&oldtab->component,
				psy_ui_STYLESTATE_SELECT);
		}
	}
	self->selected = tabindex;
}

void psy_ui_tabbar_unmark(psy_ui_TabBar* self)
{
	psy_ui_component_removestylestate_children(&self->component,
		psy_ui_STYLESTATE_SELECT);
}

bool psy_ui_tabbar_select_id(psy_ui_TabBar* self, uintptr_t id)
{
	uintptr_t i;

	assert(self);

	i = psy_ui_tabbar_index_by_id(self, id);
	if (i != psy_INDEX_INVALID) {
		psy_ui_tabbar_select(self, i);
		return TRUE;
	}
	return FALSE;
}

bool psy_ui_tabbar_mark_id(psy_ui_TabBar* self, uintptr_t id)
{	
	uintptr_t i;

	assert(self);

	i = psy_ui_tabbar_index_by_id(self, id);
	psy_ui_tabbar_mark(self, i);
	return (i != psy_INDEX_INVALID);
}

uintptr_t psy_ui_tabbar_index_by_id(psy_ui_TabBar* self, uintptr_t id)
{
	uintptr_t rv;
	psy_List* p;
	psy_List* q;
	uintptr_t i;

	assert(self);

	rv = psy_INDEX_INVALID;
	if (id == psy_INDEX_INVALID) {
		return rv;
	}
	for (i = 0, p = q = psy_ui_component_children(&self->component,
		psy_ui_NONE_RECURSIVE, FALSE); p != NULL; ++i, p = p->next) {
		psy_ui_Component* component;

		component = (psy_ui_Component*)p->entry;
		if (psy_ui_component_id(component) == id) {
			rv = i;			
			break;
		}
	}
	psy_list_free(q);
	q = NULL;
	return rv;
}

void psy_ui_tabbar_select(psy_ui_TabBar* self, uintptr_t tabindex)
{
	assert(self);
	
	psy_ui_tabbar_mark(self, tabindex);	
	psy_signal_emit(&self->signal_change, self, 1, tabindex);	
}

psy_ui_Tab* psy_ui_tabbar_append(psy_ui_TabBar* self, const char* label,
	uintptr_t target_id)
{
	psy_ui_Tab* rv;

	assert(self);

	rv = psy_ui_tab_alloc_init(&self->component, label, self->num_tabs);
	if (rv) {
		psy_ui_component_set_id(&rv->component, target_id);
		if (self->prevent_translation) {
			psy_ui_tab_prevent_translation(rv);
		}		
		++self->num_tabs;
		psy_signal_connect(&rv->signal_clicked, self, tabbar_on_tab_clicked);
	}
	return rv;
}

void psy_ui_tabbar_append_tabs(psy_ui_TabBar* self, const char* label, ...)
{
	const char* curr;
	va_list ap;

	assert(self);
	
	va_start(ap, label);
	for (curr = label; curr != NULL; curr = va_arg(ap, const char*)) {
		psy_ui_tabbar_append(self, curr, psy_INDEX_INVALID);
	}
	va_end(ap);
}

void psy_ui_tabbar_clear(psy_ui_TabBar* self)
{
	assert(self);

	self->selected = 0;	
	psy_ui_component_clear(&self->component);
	self->num_tabs = 0;
}

void psy_ui_tabbar_set_tab_mode(psy_ui_TabBar* self, uintptr_t tabindex,
	TabMode mode)
{
	psy_ui_Tab* tab;

	assert(self);

	tab = psy_ui_tabbar_tab(self, tabindex);
	if (tab) {
		psy_ui_tab_set_mode(tab, mode);
	}
}

psy_ui_Tab* psy_ui_tabbar_tab(psy_ui_TabBar* self, uintptr_t tabindex)
{	
	assert(self);

	return (psy_ui_Tab*)psy_ui_component_at(&self->component, tabindex);	
}

const psy_ui_Tab* psy_ui_tabbar_tab_const(const psy_ui_TabBar* self,
	uintptr_t tabindex)
{
	assert(self);

	return psy_ui_tabbar_tab((psy_ui_TabBar*)self, tabindex);
}

void tabbar_on_mouse_wheel(psy_ui_TabBar* self, psy_ui_MouseEvent* ev)
{
	intptr_t delta;

	assert(self);

	delta = psy_ui_mouseevent_delta(ev);
	if (self->tab_alignment == psy_ui_ALIGN_TOP ||
		self->tab_alignment == psy_ui_ALIGN_BOTTOM) {
		delta *= -1;
	}
	if (delta > 0) {
		if (self->selected + 1 < self->num_tabs) {
			psy_ui_tabbar_select(self, self->selected + 1);
		}
	} else if (delta < 0) {
		if (self->selected > 0) {
			psy_ui_tabbar_select(self, self->selected - 1);
		}
	}		
	psy_ui_mouseevent_prevent_default(ev);
}
