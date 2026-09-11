/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uisplitbar.h"
/* std */
#include <stdlib.h>


/* prototypes */
static void psy_ui_splitter_set_align(psy_ui_Splitter*, psy_ui_AlignType);
static void psy_ui_splitter_on_draw(psy_ui_Splitter*, psy_ui_Graphics*);
static void psy_ui_splitter_on_mouse_down(psy_ui_Splitter*, psy_ui_MouseEvent*);
static void psy_ui_splitter_on_mouse_move(psy_ui_Splitter*, psy_ui_MouseEvent*);
static void psy_ui_splitter_on_mouse_up(psy_ui_Splitter*, psy_ui_MouseEvent*);
static void psy_ui_splitter_on_mouse_enter(psy_ui_Splitter*);
static void psy_ui_splitter_on_hide(psy_ui_Splitter*);
static void psy_ui_splitter_on_show(psy_ui_Splitter*);
static psy_ui_Component* psy_ui_splitter_prev(psy_ui_Splitter*);
static psy_ui_Component* psy_ui_splitter_next(psy_ui_Splitter*);
static void psy_ui_splitter_set_cursor(psy_ui_Splitter*);
static psy_ui_RealPoint psy_ui_splitter_vthumb_center(const psy_ui_Splitter*);
static psy_ui_RealPoint psy_ui_splitter_hthumb_center(const psy_ui_Splitter*);
static void psy_ui_splitter_on_button(psy_ui_Splitter*);
static void psy_ui_splitter_on_show_toggle(psy_ui_Splitter*, psy_ui_Component* sender);
static void psy_ui_splitter_on_hide_toggle(psy_ui_Splitter*, psy_ui_Component* sender);
static void psy_ui_splitter_update_positions(psy_ui_Splitter*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_Splitter* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super = *(self->component.vtable);
		vtable.setalign =
			(psy_ui_fp_component_setalign)
			psy_ui_splitter_set_align;
		vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_splitter_on_draw;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_splitter_on_mouse_down;
		vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_splitter_on_mouse_move;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_splitter_on_mouse_up;
		vtable.onmouseenter =
			(psy_ui_fp_component)
			psy_ui_splitter_on_mouse_enter;
		vtable.hide =
			(psy_ui_fp_component)
			psy_ui_splitter_on_hide;
		vtable.show =
			(psy_ui_fp_component)
			psy_ui_splitter_on_show;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void psy_ui_splitter_init(psy_ui_Splitter* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->resize_ = 0;
	self->has_restore_ = FALSE;
	self->thumb_size_ = 35.0;
	self->is_vertical_ = TRUE;
	self->toggle_ = NULL;
	self->button_ = NULL;
	self->direct_ = FALSE;
	psy_ui_size_init(&self->restore_size_);
	psy_ui_component_set_align(&self->component, psy_ui_ALIGN_LEFT);
	psy_ui_component_set_style_types(&self->component,
		psy_ui_STYLE_SPLITTER, psy_ui_STYLE_SPLITTER_HOVER,
		psy_ui_STYLE_SPLITTER_SELECT, psy_INDEX_INVALID);
	psy_ui_component_prevent_app_focus_out(&self->component);
}

psy_ui_Splitter* psy_ui_splitter_alloc(void)
{
	return (psy_ui_Splitter*)malloc(sizeof(psy_ui_Splitter));
}

psy_ui_Splitter* psy_ui_splitter_allocinit(psy_ui_Component* parent)
{
	psy_ui_Splitter* rv;

	rv = psy_ui_splitter_alloc();
	if (rv) {
		psy_ui_splitter_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_splitter_on_draw(psy_ui_Splitter* self, psy_ui_Graphics* g)
{
	assert(self);

	if (psy_ui_splitter_is_vertical(self)) {
		psy_ui_RealPoint center;

		center = psy_ui_splitter_vthumb_center(self);
		center.x -= 1;
		psy_ui_drawline(g, center, psy_ui_realpoint_make(center.x,
			center.y + self->thumb_size_));
		center.x += 2;
		psy_ui_drawline(g, center, psy_ui_realpoint_make(center.x,
			center.y + self->thumb_size_));
	}
	else {
		psy_ui_RealPoint center;

		center = psy_ui_splitter_hthumb_center(self);
		center.y -= 1;
		psy_ui_drawline(g, center,
			psy_ui_realpoint_make(center.x + self->thumb_size_, center.y));
		center.y += 2;
		psy_ui_drawline(g, center,
			psy_ui_realpoint_make(center.x + self->thumb_size_, center.y));
	}
}

psy_ui_RealPoint psy_ui_splitter_vthumb_center(const psy_ui_Splitter* self)
{
	psy_ui_RealSize size;

	size = psy_ui_component_size_px(&self->component);
	return psy_ui_realpoint_make(size.width / 2,
		size.height / 2 - self->thumb_size_ / 2);
}

psy_ui_RealPoint psy_ui_splitter_hthumb_center(const psy_ui_Splitter* self)
{
	psy_ui_RealSize size;

	size = psy_ui_component_size_px(&self->component);
	return psy_ui_realpoint_make(size.width / 2 - self->thumb_size_ / 2,
		size.height / 2);
}

void psy_ui_splitter_on_mouse_down(psy_ui_Splitter* self, psy_ui_MouseEvent* ev)
{
	if (psy_ui_mouseevent_button(ev) == 2) {
		psy_ui_Component* prev;

		psy_ui_component_capture(&self->component);
		prev = psy_ui_splitter_prev(self);
		if (prev) {
			if (prev->align == psy_ui_ALIGN_LEFT ||
				prev->align == psy_ui_ALIGN_RIGHT) {
				if (!self->has_restore_) {
					self->restore_size_ = psy_ui_component_scroll_size(prev);
					self->has_restore_ = TRUE;
					psy_ui_component_set_preferred_size(prev,
						psy_ui_size_make(psy_ui_value_make_ew(0),
							psy_ui_component_scroll_size(prev).height));
				}
				else {
					self->has_restore_ = FALSE;
					psy_ui_component_set_preferred_size(prev,
						psy_ui_size_make(self->restore_size_.width,
							psy_ui_component_scroll_size(prev).height));
				}
			}
			else
				if (prev->align == psy_ui_ALIGN_TOP ||
					prev->align == psy_ui_ALIGN_BOTTOM) {
					if (!self->has_restore_) {
						self->has_restore_ = TRUE;
						self->restore_size_ = psy_ui_component_scroll_size(prev);
						psy_ui_component_set_preferred_size(prev,
							psy_ui_size_make(
								psy_ui_component_scroll_size(prev).width,
								psy_ui_value_make_px(0)));
					}
					else {
						self->has_restore_ = FALSE;
						psy_ui_component_set_preferred_size(prev,
							psy_ui_size_make(
								psy_ui_component_scroll_size(prev).width,
								self->restore_size_.height));
					}
				}
			psy_ui_component_align_invalidate(psy_ui_component_parent(
				&self->component));
		}
	}
	else {
		psy_ui_component_capture(&self->component);
		self->resize_ = 1;
		if (self->component.align == psy_ui_ALIGN_LEFT ||
			self->component.align == psy_ui_ALIGN_RIGHT) {
			self->drag_offset_ = psy_ui_mouseevent_pt(ev).x;
		}
		else {
			self->drag_offset_ = psy_ui_mouseevent_pt(ev).y;
		}
		psy_ui_splitter_set_cursor(self);
	}
	if (self->resize_) {
		psy_ui_component_add_style_state(&self->component,
			psy_ui_STYLESTATE_SELECT);
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_splitter_on_mouse_move(psy_ui_Splitter* self, psy_ui_MouseEvent* ev)
{
	if (self->resize_ == 1) {
		psy_ui_RealRectangle position;
		psy_ui_RealRectangle new_position;
		psy_ui_RealRectangle parentposition;
		psy_ui_RealRectangle prevposition;
		psy_ui_RealRectangle nextposition;
		psy_ui_Component* parent;
		psy_ui_Component* prev;
		psy_ui_Component* next;

		position = psy_ui_component_position(&self->component);
		new_position = position;
		parent = psy_ui_component_parent(&self->component);
		if (parent) {
			parentposition = psy_ui_component_position(parent);
		}
		else {
			parentposition = position;
		}
		prev = psy_ui_splitter_prev(self);
		if (prev) {
			prevposition = psy_ui_component_position(prev);
		}
		else {
			prevposition = psy_ui_component_position(
				psy_ui_component_parent(&self->component));
		}
		next = psy_ui_splitter_next(self);
		if (next) {
			nextposition = psy_ui_component_position(next);
		}
		else {
			nextposition = psy_ui_component_position(
				psy_ui_component_parent(&self->component));
		}
		switch (self->component.align) {
		case psy_ui_ALIGN_LEFT:
			psy_ui_realrectangle_set_topleft(&new_position,
				psy_ui_realpoint_make(psy_max(prevposition.left, psy_min(
					nextposition.right - (position.right - position.left),
					position.left + psy_ui_mouseevent_pt(ev).x - self->drag_offset_)),
					position.top));
			break;
		case psy_ui_ALIGN_RIGHT:
			psy_ui_realrectangle_set_topleft(&new_position,
				psy_ui_realpoint_make(
					psy_max(nextposition.left,
						psy_min(prevposition.right -
							psy_ui_realrectangle_width(&position),
							position.left + psy_ui_mouseevent_pt(ev).x -
							self->drag_offset_)),
					position.top));
			break;
		case psy_ui_ALIGN_TOP:
			psy_ui_realrectangle_set_topleft(&new_position,
				psy_ui_realpoint_make(
					position.left, psy_max(prevposition.top, psy_min(
						nextposition.bottom - (position.bottom - position.top),
						position.top + psy_ui_mouseevent_pt(ev).y - self->drag_offset_))));
			break;
		case psy_ui_ALIGN_BOTTOM:
			psy_ui_realrectangle_set_topleft(&new_position,
				psy_ui_realpoint_make(
					position.left,
					psy_max(0.0, psy_min(prevposition.bottom -
						psy_ui_realrectangle_height(&position),
						position.top + psy_ui_mouseevent_pt(ev).y - self->drag_offset_))));
			break;
		default:
			break;
		}
		psy_ui_component_move(&self->component, psy_ui_point_make_px(
			new_position.left, new_position.top));
		if (self->direct_) {
			psy_ui_splitter_update_positions(self);
		}
		else {
			psy_ui_realrectangle_union(&new_position, &position);
			psy_ui_component_invalidate_rect(psy_ui_component_parent(
				&self->component), new_position);
		}
	}
	psy_ui_splitter_set_cursor(self);
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_splitter_on_mouse_up(psy_ui_Splitter* self, psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);
	if (psy_ui_mouseevent_button(ev) == 1) {
		psy_ui_splitter_update_positions(self);
	}
	self->resize_ = 0;
	psy_ui_component_remove_style_state(&self->component,
		psy_ui_STYLESTATE_SELECT);
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_splitter_update_positions(psy_ui_Splitter* self)
{
	psy_ui_RealRectangle position;
	psy_ui_Component* prev;
	psy_ui_Component* next;

	prev = psy_ui_splitter_prev(self);
	next = psy_ui_splitter_next(self);
	if (prev) {
		position = psy_ui_component_position(&self->component);
		if (prev->align == psy_ui_ALIGN_LEFT) {
			psy_ui_RealRectangle prev_position;

			prev_position = psy_ui_component_position(prev);
			psy_ui_component_set_preferred_size(prev,
				psy_ui_size_make(
					psy_ui_value_make_px(position.left -
						prev_position.left),
					psy_ui_component_scroll_size(prev).height));
		}
		else if (prev->align == psy_ui_ALIGN_RIGHT) {
			psy_ui_RealRectangle prev_position;

			prev_position = psy_ui_component_position(prev);
			psy_ui_component_set_preferred_size(prev,
				psy_ui_size_make(
					psy_ui_value_make_px(
						prev_position.right - position.right),
					psy_ui_component_scroll_size(prev).height));
		}
		else if (prev->align == psy_ui_ALIGN_TOP) {
			psy_ui_component_set_preferred_size(prev,
				psy_ui_size_make(
					psy_ui_component_scroll_size(prev).width,
					psy_ui_value_make_px(position.top)));
		}
		else if (prev->align == psy_ui_ALIGN_BOTTOM) {
			psy_ui_RealRectangle prev_position;

			prev_position = psy_ui_component_position(prev);
			psy_ui_component_set_preferred_size(prev,
				psy_ui_size_make(
					psy_ui_component_scroll_size(prev).width,
					psy_ui_value_make_px(prev_position.bottom -
						position.bottom)));
		}
		psy_ui_component_align_invalidate(psy_ui_component_parent(
			&self->component));
	}
	else {
		if (next) {
			psy_ui_component_invalidate(next);
		}
		psy_ui_component_invalidate(&self->component);
	}
	psy_ui_splitter_set_cursor(self);
}

void psy_ui_splitter_on_mouse_enter(psy_ui_Splitter* self)
{
	super.onmouseenter(psy_ui_splitter_base(self));
	psy_ui_splitter_set_cursor(self);
}

psy_ui_Component* psy_ui_splitter_prev(psy_ui_Splitter* self)
{
	psy_ui_Component* rv;
	psy_List* c;

	rv = NULL;
	c = psy_ui_component_children(psy_ui_component_parent(&self->component),
		psy_ui_NONE_RECURSIVE, FALSE);
	while (c) {
		if (c->entry == &self->component) {
			c = c->prev;
			while (c && (((psy_ui_Component*)c->entry)->align !=
				self->component.align ||
				!psy_ui_component_visible((psy_ui_Component*)c->entry))) {
				c = c->prev;
			}
			break;
		}
		c = c->next;
	}
	if (c && psy_ui_component_visible((psy_ui_Component*)c->entry)) {
		rv = (psy_ui_Component*)c->entry;
	}
	psy_list_free(c);
	return rv;
}

psy_ui_Component* psy_ui_splitter_next(psy_ui_Splitter* self)
{
	psy_ui_Component* rv;
	psy_List* c;

	rv = NULL;
	c = psy_ui_component_children(psy_ui_component_parent(&self->component),
		psy_ui_NONE_RECURSIVE, FALSE);
	while (c) {
		if (c->entry == &self->component) {
			c = c->next;
			while (c && (((psy_ui_Component*)c->entry)->align !=
				self->component.align ||
				!psy_ui_component_visible((psy_ui_Component*)c->entry))) {
				c = c->next;
			}
			break;
		}
		c = c->next;
	}
	if (c && psy_ui_component_visible((psy_ui_Component*)c->entry)) {
		rv = (psy_ui_Component*)c->entry;
	}
	psy_list_free(c);
	return rv;
}

void psy_ui_splitter_set_cursor(psy_ui_Splitter* self)
{
	psy_ui_component_set_cursor(psy_ui_splitter_base(self),
		(psy_ui_splitter_is_vertical(self))
		? psy_ui_CURSORSTYLE_COL_RESIZE
		: psy_ui_CURSORSTYLE_ROW_RESIZE);
}

void psy_ui_splitter_set_align(psy_ui_Splitter* self, psy_ui_AlignType align)
{
	self->is_vertical_ = (self->component.align == psy_ui_ALIGN_LEFT ||
		self->component.align == psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_preferred_size(&self->component,
		(psy_ui_splitter_is_vertical(self))
		? psy_ui_size_make_em(1.3, 1.5)
		: psy_ui_size_make_em(1.5, 0.5));
}

void psy_ui_splitter_on_hide(psy_ui_Splitter* self)
{
	super.hide(&self->component);
	if (self->toggle_ && psy_ui_component_visible(self->toggle_)) {
		psy_signal_prevent(&self->toggle_->signal_hide, self,
			psy_ui_splitter_on_hide_toggle);
		psy_ui_component_hide_align(self->toggle_);
		psy_signal_enable(&self->toggle_->signal_hide, self,
			psy_ui_splitter_on_hide_toggle);
	}
	if (self->button_) {
		psy_ui_button_disable_highlight(self->button_);
	}
}

void psy_ui_splitter_on_show(psy_ui_Splitter* self)
{
	super.show(&self->component);
	if (self->toggle_ && !psy_ui_component_visible(self->toggle_)) {
		psy_signal_prevent(&self->toggle_->signal_show, self,
			psy_ui_splitter_on_show_toggle);
		psy_ui_component_show_align(self->toggle_);
		psy_signal_enable(&self->toggle_->signal_show, self,
			psy_ui_splitter_on_show_toggle);
	}
	if (self->button_) {
		psy_ui_button_highlight(self->button_);
	}
}

void psy_ui_splitter_set_toggle(psy_ui_Splitter* self, psy_ui_Component* toggle)
{
	if (self->toggle_) {
		psy_signal_disconnect(&self->toggle_->signal_hide, self,
			psy_ui_splitter_on_hide_toggle);
		psy_signal_disconnect(&self->toggle_->signal_show, self,
			psy_ui_splitter_on_show_toggle);
	}
	self->toggle_ = toggle;
	psy_signal_connect(&self->toggle_->signal_hide, self,
		psy_ui_splitter_on_hide_toggle);
	psy_signal_connect(&self->toggle_->signal_show, self,
		psy_ui_splitter_on_show_toggle);
}

void psy_ui_splitter_set_button(psy_ui_Splitter* self, psy_ui_Button* button)
{
	if (self->button_) {
		psy_signal_disconnect(&self->button_->signal_clicked, self,
			psy_ui_splitter_on_button);
	}
	self->button_ = button;
	psy_signal_connect(&self->button_->signal_clicked, self,
		psy_ui_splitter_on_button);
}


void psy_ui_splitter_on_button(psy_ui_Splitter* self)
{
	if (psy_ui_component_visible(&self->component)) {
		psy_ui_component_hide(&self->component);
	}
	else {
		psy_ui_component_show(&self->component);
	}
}

void psy_ui_splitter_on_show_toggle(psy_ui_Splitter* self, psy_ui_Component* sender)
{
	psy_ui_component_show(&self->component);
}

void psy_ui_splitter_on_hide_toggle(psy_ui_Splitter* self, psy_ui_Component* sender)
{
	psy_ui_component_hide(&self->component);
}
