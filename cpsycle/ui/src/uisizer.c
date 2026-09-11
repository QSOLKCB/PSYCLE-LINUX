/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uisizer.h"
/* local */
#include "uiapp.h"
#include "uisvg.h"

/* prototypes */
static void psy_ui_sizer_on_mouse_down(psy_ui_Sizer*,
	psy_ui_MouseEvent*);
static void psy_ui_sizer_on_mouse_move(psy_ui_Sizer*,
	psy_ui_MouseEvent*);
static void psy_ui_sizer_on_mouse_up(psy_ui_Sizer*,
	psy_ui_MouseEvent*);

/* vtable */
static psy_ui_ComponentVtable psy_ui_sizer_vtable;
static bool psy_ui_sizer_vtable_initialized = FALSE;

static void psy_ui_sizer_vtable_init(psy_ui_Sizer* self)
{
	assert(self);

	if (!psy_ui_sizer_vtable_initialized) {
		psy_ui_sizer_vtable = *(self->img.component.vtable);		
		psy_ui_sizer_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sizer_on_mouse_down;
		psy_ui_sizer_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sizer_on_mouse_move;
		psy_ui_sizer_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_sizer_on_mouse_up;
		psy_ui_sizer_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(psy_ui_image_base(&self->img),
		&psy_ui_sizer_vtable);
}

/* implementation */
void psy_ui_sizer_init(psy_ui_Sizer* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_image_init_svg(&self->img, parent, psy_ui_app_svg(psy_ui_app(),
		"img.icon-grip"));
	psy_ui_sizer_vtable_init(self);
	psy_ui_component_set_align(psy_ui_sizer_base(self), psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_preferred_size(psy_ui_sizer_base(self),
		psy_ui_size_make_em(3.0, 1.0));
	psy_ui_image_set_fixed_icon_size(&self->img,
		psy_ui_size_make_em(3.0, 1.0));
	self->resize_component_ = NULL;
	self->dragging_ = FALSE;
}


void psy_ui_sizer_set_resize_component(psy_ui_Sizer* self,
	psy_ui_Component* resize_component)
{
	assert(self);

	self->resize_component_ = resize_component;
	psy_ui_component_invalidate(psy_ui_image_base(&self->img));
}

void psy_ui_sizer_on_mouse_down(psy_ui_Sizer* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (!self->resize_component_) {
		return;
	}
	if (psy_ui_mouseevent_button(ev) == 1) {
		self->frame_drag_offset_ = psy_ui_mouseevent_offset(ev);
		self->dragging_ = TRUE;
		psy_ui_component_capture(psy_ui_image_base(&self->img));
	}
}

void psy_ui_sizer_on_mouse_move(psy_ui_Sizer* self, psy_ui_MouseEvent* ev)
{
	psy_ui_RealRectangle position;

	assert(self);

	if ((!self->dragging_) || (!self->resize_component_)) {
		return;
	}
	position = psy_ui_component_screenposition(self->resize_component_);
	{
		psy_ui_Size size;

		size = psy_ui_size_make_px(
			position.right - position.left + (psy_ui_mouseevent_offset(ev).x -
				self->frame_drag_offset_.x),
			position.bottom - position.top + (psy_ui_mouseevent_offset(ev).y -
				self->frame_drag_offset_.y));
		psy_ui_component_resize(self->resize_component_, size);
	}
}

void psy_ui_sizer_on_mouse_up(psy_ui_Sizer* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	if (!self->resize_component_) {
		return;
	}
	psy_ui_component_release_capture(psy_ui_image_base(&self->img));
	self->dragging_ = FALSE;
}
