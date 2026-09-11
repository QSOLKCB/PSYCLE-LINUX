/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiviewframe.h"
/* host */
// #include "viewindex.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_FrameDrag */

/* prototypes */
static void psy_ui_framedrag_on_mouse_down(psy_ui_FrameDrag*, psy_ui_Component* sender,
	psy_ui_MouseEvent*);
static void psy_ui_framedrag_on_mouse_double_click(psy_ui_FrameDrag*,
	psy_ui_Component* sender, psy_ui_MouseEvent*);
static void psy_ui_framedrag_on_mouse_move(psy_ui_FrameDrag*, psy_ui_Component* sender,
	psy_ui_MouseEvent*);
static void psy_ui_framedrag_on_mouse_up(psy_ui_FrameDrag*, psy_ui_Component* sender,
	psy_ui_MouseEvent*);
static bool psy_ui_framedrag_accept_frame_move(const psy_ui_FrameDrag*,
	psy_ui_Component* target);

/* implementation */
void psy_ui_framedrag_init(psy_ui_FrameDrag* self, psy_ui_Component* frame)
{
	assert(self);

	self->frame = frame;
	self->accept = NULL;
	psy_signal_connect(&self->frame->signal_mouse_down, self,
		psy_ui_framedrag_on_mouse_down);
	psy_signal_connect(&self->frame->signal_mouse_up, self,
		psy_ui_framedrag_on_mouse_up);
	psy_signal_connect(&self->frame->signal_mouse_move, self,
		psy_ui_framedrag_on_mouse_move);
	psy_signal_connect(&self->frame->signal_mouse_double_click, self,
		psy_ui_framedrag_on_mouse_double_click);
}

void psy_ui_framedrag_dispose(psy_ui_FrameDrag* self)
{
	assert(self);

	psy_list_free(self->accept);
	self->accept = NULL;
}

void psy_ui_framedrag_add(psy_ui_FrameDrag* self,
	const psy_ui_Component* component)
{
	assert(self);

	psy_list_append(&self->accept, (void*)component);
}

void psy_ui_framedrag_on_mouse_down(psy_ui_FrameDrag* self,
		psy_ui_Component* sender,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	self->frame_drag_offset = psy_ui_mouseevent_offset(ev);
	self->allow_frame_move = psy_ui_framedrag_accept_frame_move(self,
		psy_ui_mouseevent_target(ev));
}

void psy_ui_framedrag_on_mouse_double_click(psy_ui_FrameDrag* self,
	psy_ui_Component* sender, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_framedrag_accept_frame_move(self,
			psy_ui_mouseevent_target(ev))) {
		switch (psy_ui_component_state(self->frame)) {
		case psy_ui_COMPONENTSTATE_NORMAL:
			psy_ui_component_set_state(self->frame,
				psy_ui_COMPONENTSTATE_MAXIMIZED);
			break;
		case psy_ui_COMPONENTSTATE_MAXIMIZED:
			psy_ui_component_set_state(self->frame,
				psy_ui_COMPONENTSTATE_NORMAL);
			break;
		case psy_ui_COMPONENTSTATE_FULLSCREEN:
			psy_ui_component_set_state(self->frame,
				psy_ui_COMPONENTSTATE_NORMAL);
			break;
		default:
			break;
		}
	}
}

bool psy_ui_framedrag_accept_frame_move(const psy_ui_FrameDrag* self,
	psy_ui_Component* target)
{
	const psy_List* p;
	bool rv;

	assert(self);

	rv = FALSE;
	for (p = self->accept; p != NULL; p = p->next) {
		const psy_ui_Component* curr;

		curr = (const psy_ui_Component*)p->entry;
		if (curr == target) {
			rv = TRUE;
			break;
		}
	}
	return rv;
}

void psy_ui_framedrag_on_mouse_move(psy_ui_FrameDrag* self,
	psy_ui_Component* sender, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (psy_ui_mouseevent_button(ev) == 1 && self->allow_frame_move) {
		psy_ui_RealRectangle position;
		psy_ui_ComponentState state;

		position = psy_ui_component_screenposition(self->frame);
		state = psy_ui_component_state(self->frame);
		if (state == psy_ui_COMPONENTSTATE_FULLSCREEN ||
			state == psy_ui_COMPONENTSTATE_MAXIMIZED) {
			psy_ui_RealRectangle normal;
			double left;
			double width;

			normal = psy_ui_component_restore_position(self->frame);
			psy_ui_component_set_state(self->frame,
				psy_ui_COMPONENTSTATE_NORMAL);
			width = position.right - position.left;
			if (self->frame_drag_offset.x > width / 2.0) {
				double margin_right;

				margin_right = width - psy_ui_mouseevent_offset(ev).x;
				width = normal.right - normal.left;
				left = psy_ui_mouseevent_offset(ev).x - (width - margin_right);
				self->frame_drag_offset.x = (width - margin_right);
			}
			else {
				left = 0.0;
			}
			psy_ui_component_move(self->frame, psy_ui_point_make_px(left,
				0.0));
		}
		else {
			psy_ui_Point pt;

			pt = psy_ui_point_make_px(
				position.left + (psy_ui_mouseevent_offset(ev).x -
					self->frame_drag_offset.x),
				position.top + (psy_ui_mouseevent_offset(ev).y -
					self->frame_drag_offset.y));
			psy_ui_component_move(self->frame, pt);
		}
	}
}

void psy_ui_framedrag_on_mouse_up(psy_ui_FrameDrag* self,
	psy_ui_Component* sender, psy_ui_MouseEvent* ev)
{
	assert(self);

	self->allow_frame_move = FALSE;
}


/* psy_ui_EmptyViewPage */

void psy_ui_emptyviewpage_init(psy_ui_EmptyViewPage* self,
	psy_ui_Component* parent)
{
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_id(&self->component, VIEW_ID_FLOATED);
	psy_ui_label_init_text(&self->label, &self->component, "main.floated");
	psy_ui_component_set_align(psy_ui_label_base(&self->label),
		psy_ui_ALIGN_CENTER);
}


/* psy_ui_ViewFrame */

/* prototypes */
static bool psy_ui_viewframe_on_close(psy_ui_ViewFrame*);
static void psy_ui_viewframe_on_key_down(psy_ui_ViewFrame*, psy_ui_KeyboardEvent*);
static void psy_ui_viewframe_on_key_up(psy_ui_ViewFrame*, psy_ui_KeyboardEvent*);
static void psy_ui_viewframe_delegate_keyboard(psy_ui_ViewFrame*, intptr_t message,
	psy_ui_KeyboardEvent*);
static void psy_ui_viewframe_on_language_changed(psy_ui_ViewFrame*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_ViewFrame* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.onclose =
			(psy_ui_fp_component_onclose)
			psy_ui_viewframe_on_close;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			psy_ui_viewframe_on_key_down;
		vtable.onkeyup =
			(psy_ui_fp_component_on_key_event)
			psy_ui_viewframe_on_key_up;
		vtable.on_language_changed =
			(psy_ui_fp_component)
			psy_ui_viewframe_on_language_changed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(psy_ui_viewframe_base(self), &vtable);
}

/* implementation */
void psy_ui_viewframe_init(psy_ui_ViewFrame* self, psy_ui_Component* parent,
	psy_ui_Component* page, psy_ui_Component* splitter,
	psy_ui_Component* icons, psy_ui_Component* close,
	psy_ui_Sizer* resizer, psy_EventDriver* kbd,
	psy_ui_Size frame_size, bool center)
{
	assert(page);

	psy_ui_toolframe_init(&self->component, parent);
	vtable_init(self);
	psy_ui_component_doublebuffer(psy_ui_viewframe_base(self));
	self->splitter = splitter;
	self->icons = icons;
	self->close = close;
	self->resizer = resizer;
	self->restore_parent = psy_ui_component_parent(page);
	self->restore_align = page->align;
	self->restore_section = psy_ui_component_section(self->restore_parent);	
	self->kbd = kbd;
	self->frame_size = frame_size;
	self->center = center;
	psy_ui_component_init(&self->pane, &self->component, &self->component);
	psy_ui_component_set_align(&self->pane, psy_ui_ALIGN_CLIENT);	
	psy_ui_viewframe_float(self, page);
}

psy_ui_ViewFrame* psy_ui_viewframe_alloc(void)
{
	return (psy_ui_ViewFrame*)malloc(sizeof(psy_ui_ViewFrame));
}

psy_ui_ViewFrame* psy_ui_viewframe_alloc_init(psy_ui_Component* parent,
	psy_ui_Component* page, psy_ui_Component* splitter,
	psy_ui_Component* icons, psy_ui_Component* close,
	psy_ui_Sizer* resizer, psy_EventDriver* kbd,
	psy_ui_Size frame_size, bool center)
{
	psy_ui_ViewFrame* rv;

	rv = psy_ui_viewframe_alloc();
	if (rv) {
		psy_ui_viewframe_init(rv, parent, page, splitter, icons, close, resizer,
			kbd, frame_size, center);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

bool psy_ui_viewframe_on_close(psy_ui_ViewFrame* self)
{
	psy_ui_viewframe_dock(self);
	return TRUE;
}

void psy_ui_viewframe_float(psy_ui_ViewFrame* self,
	psy_ui_Component* component)
{
	if (!component) {
		return;
	}
	self->restore_parent = psy_ui_component_parent(component);
	self->restore_align = component->align;
	self->restore_section = psy_ui_component_section(self->restore_parent);
	psy_ui_component_set_parent(component, &self->pane);
	psy_ui_component_set_align(component, psy_ui_ALIGN_CLIENT);
	if (self->icons) {
		psy_ui_component_hide(self->icons);
	}
	if (self->close) {
		psy_ui_component_hide(self->close);
	}
	if (self->resizer) {
		psy_ui_component_show(psy_ui_sizer_base(self->resizer));
		psy_ui_sizer_set_resize_component(self->resizer, &self->component);
	}
	psy_ui_component_set_title(&self->component,
		psy_ui_translate(psy_ui_component_title(component)));
	psy_ui_component_resize(&self->component, self->frame_size);
	if (self->center) {
		psy_ui_component_center(&self->component);
	}
	psy_ui_component_show(&self->component);
	psy_ui_component_select_section(self->restore_parent, VIEW_ID_FLOATED,
		psy_INDEX_INVALID);
	if (self->splitter) {
		psy_ui_component_hide_align(self->splitter);
		psy_ui_component_invalidate(self->restore_parent);
	}
}

void psy_ui_viewframe_dock(psy_ui_ViewFrame* self)
{
	psy_ui_Component* page;

	if (!self->restore_parent) {
		return;
	}
	page = psy_ui_component_at(&self->pane, 0);
	if (page) {
		if (self->splitter) {
			psy_ui_component_set_parent(self->splitter, &self->pane);
		}
		if (self->icons) {
			psy_ui_component_show(self->icons);
		}
		if (self->close) {
			psy_ui_component_show(self->close);
		}
		if (self->resizer) {
			psy_ui_component_hide(psy_ui_sizer_base(self->resizer));
		}
		psy_ui_component_set_parent(page, self->restore_parent);
		psy_ui_component_set_align(page, self->restore_align);
		psy_ui_component_select_section(self->restore_parent,
			self->restore_section, psy_INDEX_INVALID);
		psy_ui_component_align(self->restore_parent);
		psy_ui_component_invalidate(self->restore_parent);
	}
	if (self->splitter) {
		psy_ui_component_set_parent(self->splitter, self->restore_parent);
		psy_ui_component_show_align(self->splitter);
		psy_ui_component_invalidate(self->restore_parent);
	}
}

void psy_ui_viewframe_on_key_down(psy_ui_ViewFrame* self, psy_ui_KeyboardEvent* ev)
{
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		return;
	}
	psy_ui_viewframe_delegate_keyboard(self, psy_EVENTDRIVER_PRESS, ev);
}

void psy_ui_viewframe_on_key_up(psy_ui_ViewFrame* self, psy_ui_KeyboardEvent* ev)
{
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		return;
	}
	psy_ui_viewframe_delegate_keyboard(self, psy_EVENTDRIVER_RELEASE, ev);
}

/* delegate keyboard events to the keyboard driver */
void psy_ui_viewframe_delegate_keyboard(psy_ui_ViewFrame* self,
	intptr_t message, psy_ui_KeyboardEvent* ev)
{
	if (!self->kbd) {
		return;
	}
	psy_eventdriver_write(self->kbd,
		psy_eventdriverinput_make(message, psy_ui_keyboardevent_encode(ev,
			message == psy_EVENTDRIVER_RELEASE),
			psy_ui_keyboardevent_repeat(ev)));
}

void psy_ui_viewframe_on_language_changed(psy_ui_ViewFrame* self)
{
	psy_ui_Component* page;

	page = psy_ui_component_at(&self->pane, 0);
	if (page) {
		psy_ui_component_set_title(&self->component,
			psy_ui_translate(psy_ui_component_title(page)));
	}
}
