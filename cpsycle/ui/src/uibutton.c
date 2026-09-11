/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uibutton.h"
/* local */
#include "uiapp.h"
#include "trackercmds.h"
/* platform */
#include "../../detail/trace.h"
#include "../../detail/portable.h"


psy_ui_ButtonRepeat psy_ui_buttonrepeat_make(uintptr_t rate, uintptr_t first_rate)
{
	psy_ui_ButtonRepeat rv;

	rv.repeat_rate = rate;
	rv.first_repeat_rate = first_rate;
	rv.first_repeat = TRUE;
	psy_ui_mouseevent_init(&rv.repeat_event);
	return rv;
}


/* psy_ui_Button */

/* prototypes */
static void psy_ui_button_on_destroyed(psy_ui_Button*);
static void psy_ui_button_on_mouse_down(psy_ui_Button*, psy_ui_MouseEvent*);
static void psy_ui_button_on_mouse_up(psy_ui_Button*, psy_ui_MouseEvent*);
static void psy_ui_button_emit(psy_ui_Button*, psy_ui_MouseEvent*);
static void psy_ui_button_on_preferred_size(psy_ui_Button*, psy_ui_Size* limit,
	psy_ui_Size* rv);
static void button_on_key_down(psy_ui_Button*, psy_ui_KeyboardEvent*);
static void psy_ui_button_on_property_changed(psy_ui_Button*,
	psy_Property* sender);
static void psy_ui_button_before_property_destroyed(psy_ui_Button*,
	psy_Property* sender);
static void psy_ui_button_on_repeat_timer(psy_ui_Button*, uintptr_t id);
static bool psy_ui_button_on_input(psy_ui_Button*, InputHandler*);
static void psy_ui_button_set_input_handler(psy_ui_Button*, struct InputHandler*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_Button* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(psy_ui_button_base(self)->vtable);
		super_vtable = *(psy_ui_button_base(self)->vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_button_on_destroyed;		
		vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			psy_ui_button_on_preferred_size;
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_button_on_mouse_down;
		vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_button_on_mouse_up;
		vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			button_on_key_down;		
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			psy_ui_button_on_repeat_timer;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(psy_ui_button_base(self), &vtable);
}

/* implementation */
void psy_ui_button_init(psy_ui_Button* self, psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(psy_ui_button_base(self), parent, NULL);
	vtable_init(self);
	psy_ui_component_set_align_expand(psy_ui_button_base(self),
		psy_ui_HEXPAND);
	self->property = NULL;
	self->charnumber = 0.0;
	self->linespacing = 1.0;
	self->data = psy_INDEX_INVALID;	
	self->shiftstate = FALSE;
	self->ctrlstate = FALSE;
	self->buttonstate = 1;
	self->allowrightclick = FALSE;
	self->stoppropagation = TRUE;	
	self->click_mode = psy_ui_CLICK_MODE_RELEASE;
	self->repeat = psy_ui_buttonrepeat_make(0, 0);
	self->prevent_property_text = FALSE;
	psy_ui_image_init(&self->img, &self->component);
	self->img.component.flags_ |= psy_ui_COMPONENTFLAGS_PREVENT_MOUSE_INPUT;
	psy_ui_component_set_align(psy_ui_image_base(&self->img),
		psy_ui_ALIGN_LEFT);	
	psy_ui_image_init(&self->img_selected, &self->component);
	self->img_selected.component.flags_ |= psy_ui_COMPONENTFLAGS_PREVENT_MOUSE_INPUT;
	psy_ui_component_set_align(psy_ui_image_base(&self->img_selected),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_hide(psy_ui_image_base(&self->img_selected));
	psy_ui_label_init(&self->label, &self->component);
	psy_ui_component_set_align(psy_ui_label_base(&self->label),
		psy_ui_ALIGN_CLIENT);
	psy_ui_label_set_text_alignment(&self->label, psy_ui_ALIGNMENT_CENTER);
	self->label.component.flags_ |= psy_ui_COMPONENTFLAGS_PREVENT_MOUSE_INPUT;
	psy_signal_init(&self->signal_clicked);
	psy_ui_component_set_style_types(psy_ui_button_base(self),
		psy_ui_STYLE_BUTTON, psy_ui_STYLE_BUTTON_HOVER,
		psy_ui_STYLE_BUTTON_SELECT, psy_INDEX_INVALID);
	psy_ui_component_set_style_type_active(psy_ui_button_base(self),
		psy_ui_STYLE_BUTTON_ACTIVE);
	psy_ui_component_set_style_type_focus(psy_ui_button_base(self),
		psy_ui_STYLE_BUTTON_FOCUS);
	psy_ui_button_set_input_handler(self, psy_ui_app_input_handler(
		psy_ui_app()));
}

void psy_ui_button_init_exchange(psy_ui_Button* self, psy_ui_Component* parent,
	psy_Property* property)
{
	assert(self);

	psy_ui_button_init(self, parent);

	if (property && psy_property_hint(property) == PSY_PROPERTY_HINT_CHECK) {
		psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(),
			"img.check-off"));
		psy_ui_button_set_svg_selected(self, psy_ui_app_svg(psy_ui_app(),
			"img.check-on"));
		psy_ui_button_set_text_alignment(self, (psy_ui_Alignment)
			(psy_ui_ALIGNMENT_CENTER_VERTICAL | psy_ui_ALIGNMENT_LEFT));
	}
	psy_ui_button_exchange(self, property);	
}

void psy_ui_button_init_text(psy_ui_Button* self, psy_ui_Component* parent,
	const char* text)
{
	assert(self);

	psy_ui_button_init(self, parent);
	psy_ui_button_set_text(self, text);
}

void psy_ui_button_init_resource(psy_ui_Button* self, psy_ui_Component* parent,
	const char* key)
{
	assert(self);

	psy_ui_button_init(self, parent);	
	psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(), key));
}

void psy_ui_button_init_text_resource(psy_ui_Button* self, psy_ui_Component* parent,
	const char* text, const char* key)
{
	assert(self);

	psy_ui_button_init(self, parent);
	psy_ui_button_set_text(self, text);
	psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(), key));
}

void psy_ui_button_init_connect(psy_ui_Button* self, psy_ui_Component* parent,
	void* context, void* fp)
{
	assert(self);

	psy_ui_button_init(self, parent);
	psy_signal_connect(&self->signal_clicked, context, fp);
}

void psy_ui_button_init_text_connect(psy_ui_Button* self, psy_ui_Component*
	parent, const char* text, void* context, void* fp)
{
	assert(self);

	psy_ui_button_init_connect(self, parent, context, fp);
	psy_ui_button_set_text(self, text);
}

void psy_ui_button_init_resource_connect(psy_ui_Button* self, psy_ui_Component*
	parent, const char* key, void* context, void* fp)
{
	assert(self);

	psy_ui_button_init_connect(self, parent, context, fp);
	psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(), key));
}

void psy_ui_button_init_check(psy_ui_Button* self, psy_ui_Component* parent,
	const char* text)
{
	assert(self);

	psy_ui_button_init(self, parent);
	psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(), "img.check-off"));
	psy_ui_button_set_svg_selected(self, psy_ui_app_svg(psy_ui_app(), "img.check-on"));
	psy_ui_button_set_text(self, text);
	psy_ui_button_set_text_alignment(self, (psy_ui_Alignment)
		(psy_ui_ALIGNMENT_CENTER_VERTICAL | psy_ui_ALIGNMENT_LEFT));
}

void psy_ui_button_init_check_connect(psy_ui_Button* self, psy_ui_Component* parent,
	const char* text, void* context, void* fp)
{
	assert(self);

	psy_ui_button_init_connect(self, parent, context, fp);
	psy_ui_button_set_svg(self, psy_ui_app_svg(psy_ui_app(), "img.check-off"));
	psy_ui_button_set_svg_selected(self, psy_ui_app_svg(psy_ui_app(), "img.check-on"));
	psy_ui_button_set_text(self, text);
	psy_ui_button_set_text_alignment(self, (psy_ui_Alignment)
		(psy_ui_ALIGNMENT_CENTER_VERTICAL | psy_ui_ALIGNMENT_LEFT));
}

psy_ui_Button* psy_ui_button_alloc(void)
{
	return (psy_ui_Button*)malloc(sizeof(psy_ui_Button));
}

psy_ui_Button* psy_ui_button_alloc_init(psy_ui_Component* parent)
{
	psy_ui_Button* rv;

	rv = psy_ui_button_alloc();
	if (rv) {
		psy_ui_button_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_button_on_destroyed(psy_ui_Button* self)
{
	assert(self);

	if (self->property) {
		psy_property_disconnect(self->property, self);
	}		
	psy_signal_dispose(&self->signal_clicked);
	inputhandler_disconnect(
		psy_ui_app_input_handler(psy_ui_app()),
		self, (fp_inputhandler_input)psy_ui_button_on_input);
}

void psy_ui_button_connect(psy_ui_Button* self, void* context, void* fp)
{
	assert(self);
	assert(fp);

	psy_signal_connect(&self->signal_clicked, context, fp);
}

void psy_ui_button_set_char_number(psy_ui_Button* self, double number)
{
	self->charnumber = psy_max(0.0, number);
}

void psy_ui_button_set_line_spacing(psy_ui_Button* self, double spacing)
{
	assert(self);

	self->linespacing = spacing;
}

void psy_ui_button_on_preferred_size(psy_ui_Button* self, psy_ui_Size* limit,
	psy_ui_Size* rv)
{
	assert(self);
	
	if (self->charnumber == 0) {		
		super_vtable.onpreferredsize(&self->component, limit, rv);
	} else {
		rv->width = psy_ui_value_make_ew(self->charnumber);
		rv->height = psy_ui_value_make_eh(self->linespacing);
	}
}

void psy_ui_button_on_mouse_down(psy_ui_Button* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	super_vtable.on_mouse_down(psy_ui_button_base(self), ev);
	if (self->stoppropagation) {
		psy_ui_mouseevent_stop_propagation(ev);
	}
	if (!psy_ui_component_input_prevented(&self->component)) {
		if ((self->allowrightclick || (psy_ui_mouseevent_button(ev) == 1)) &&
			((self->click_mode == psy_ui_CLICK_MODE_PRESS) ||
				(self->click_mode == psy_ui_CLICK_MODE_REPEAT))) {
			psy_ui_button_emit(self, ev);
			if (self->repeat.repeat_rate != 0) {
				self->repeat.repeat_event = *ev;
				self->repeat.first_repeat = TRUE;
				psy_ui_component_start_timer(&self->component, 0,
					self->repeat.first_repeat_rate);
			}
		}
		else {
			psy_ui_component_capture(psy_ui_button_base(self));
		}
	}
}

void psy_ui_button_on_mouse_up(psy_ui_Button* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	super_vtable.on_mouse_up(psy_ui_button_base(self), ev);
	if (!psy_ui_component_input_prevented(&self->component)) {
		psy_ui_component_release_capture(psy_ui_button_base(self));
		if (psy_ui_component_input_prevented(&self->component)) {
			psy_ui_mouseevent_stop_propagation(ev);
			return;
		}
		self->buttonstate = psy_ui_mouseevent_button(ev);
		if (self->allowrightclick || psy_ui_mouseevent_button(ev) == 1) {
			psy_ui_RealRectangle client_position;

			client_position = psy_ui_realrectangle_make(psy_ui_realpoint_zero(),
				psy_ui_component_scroll_size_px(psy_ui_button_base(self)));
			if (self->stoppropagation) {
				psy_ui_mouseevent_stop_propagation(ev);
			}
			if (self->click_mode == psy_ui_CLICK_MODE_RELEASE &&
				psy_ui_realrectangle_intersect(&client_position,
					psy_ui_mouseevent_offset(ev))) {
				psy_ui_button_emit(self, ev);
			}
		}
		if (self->repeat.repeat_rate != 0) {
			psy_ui_component_stop_timer(&self->component, 0);
		}
	}
}

void psy_ui_button_emit(psy_ui_Button* self, psy_ui_MouseEvent* ev)
{
	assert(self);

	self->shiftstate = psy_ui_mouseevent_shift_key(ev);
	self->ctrlstate = psy_ui_mouseevent_ctrl_key(ev);
	psy_signal_emit(&self->signal_clicked, self, 0);
	if (self->property) {
		if (psy_property_is_choice_item(self->property)) {
			intptr_t index;

			if (!psy_property_parent(self->property)) {
				return;
			}
			index = psy_property_index(self->property);
			psy_property_set_item_int(psy_property_parent(self->property),
				index);
		}
		else if (psy_property_is_bool(self->property)) {
			psy_property_set_item_bool(self->property,
				!psy_property_item_bool(self->property));
		}
		else {
			psy_signal_emit(&self->property->changed, self->property, 0);
		}
		psy_ui_mouseevent_stop_propagation(ev);
	}	
}

void psy_ui_button_set_text(psy_ui_Button* self, const char* text)
{
	assert(self);

	psy_ui_label_set_text(&self->label, text);
	self->prevent_property_text = TRUE;
	if (!psy_ui_image_empty(&self->img) || (!psy_ui_image_empty(&self->img_selected))) {
		if (psy_strlen(psy_ui_button_text(self))) {
			psy_ui_component_set_margin(psy_ui_image_base(&self->img),
				psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
			psy_ui_component_set_margin(psy_ui_image_base(&self->img_selected),
				psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
		}
	}
}

const char* psy_ui_button_text(const psy_ui_Button* self)
{
	assert(self);

	return psy_ui_label_text(&self->label);	
}

void psy_ui_button_set_svg(psy_ui_Button* self, const psy_ui_SVG* svg)
{
	assert(self);
	
	psy_ui_image_set_svg(&self->img, svg);
	psy_ui_image_set_fixed_icon_size(&self->img, 
		psy_ui_size_make_em(2.6, 1.0));	
	psy_ui_component_set_preferred_size(psy_ui_image_base(&self->img),
		psy_ui_size_make_em(2.6, 1.0));
	if (psy_strlen(psy_ui_button_text(self))) {
		psy_ui_component_set_margin(psy_ui_image_base(&self->img),
			psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	}
}

void psy_ui_button_set_svg_selected(psy_ui_Button* self, const psy_ui_SVG* svg)
{
	assert(self);

	psy_ui_image_set_svg(&self->img_selected, svg);
	psy_ui_component_set_preferred_size(psy_ui_image_base(&self->img_selected),
		psy_ui_size_make_em(3.0, 1.0));
	if (psy_strlen(psy_ui_button_text(self))) {
		psy_ui_component_set_margin(psy_ui_image_base(&self->img_selected),
			psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	}
}

void psy_ui_button_highlight(psy_ui_Button* self)
{
	assert(self);

	if (!psy_ui_button_highlighted(self)) {
		if (!psy_ui_image_empty(&self->img_selected)) {
			psy_ui_component_hide(psy_ui_image_base(&self->img));
			psy_ui_component_show(psy_ui_image_base(&self->img_selected));
			psy_ui_component_align(psy_ui_button_base(self));
		}
		psy_ui_component_add_style_state(psy_ui_button_base(self),
			psy_ui_STYLESTATE_SELECT);
	}
}

void psy_ui_button_disable_highlight(psy_ui_Button* self)
{
	assert(self);

	if (psy_ui_button_highlighted(self)) {
		if (!psy_ui_image_empty(&self->img_selected)) {
			psy_ui_component_hide(psy_ui_image_base(&self->img_selected));
			psy_ui_component_show(psy_ui_image_base(&self->img));
			psy_ui_component_align(psy_ui_button_base(self));
		}
		psy_ui_component_remove_style_state(psy_ui_button_base(self),
			psy_ui_STYLESTATE_SELECT);
	}
}

bool psy_ui_button_highlighted(const psy_ui_Button* self)
{
	assert(self);

	return (psy_ui_componentstyle_state(&self->component.style) &
		psy_ui_STYLESTATE_SELECT) == psy_ui_STYLESTATE_SELECT;
}

void psy_ui_button_settextcolour(psy_ui_Button* self, psy_ui_Colour colour)
{
	assert(self);

	psy_ui_component_set_colour(&self->component, colour);
}

void psy_ui_button_set_text_alignment(psy_ui_Button* self,
	psy_ui_Alignment alignment)
{
	assert(self);

	psy_ui_label_set_text_alignment(&self->label, alignment);	
}

void psy_ui_button_prevent_translation(psy_ui_Button* self)
{
	assert(self);

	psy_ui_label_prevent_translation(&self->label);	
}

void psy_ui_button_prevent_text(psy_ui_Button* self)
{
	assert(self);

	psy_ui_component_hide(psy_ui_label_base(&self->label));
}

void psy_ui_button_set_click_mode(psy_ui_Button* self, psy_ui_ClickMode mode)
{
	assert(self);

	self->click_mode = mode;
}

void psy_ui_button_set_repeat(psy_ui_Button* self, uintptr_t rate, uintptr_t first_rate)
{
	assert(self);

	self->repeat = psy_ui_buttonrepeat_make(rate, first_rate);
	self->click_mode = psy_ui_CLICK_MODE_REPEAT;
}

void button_on_key_down(psy_ui_Button* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);

	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_RETURN &&
			!psy_ui_component_input_prevented(&self->component)) {
		psy_ui_MouseEvent mouse_event;

		psy_ui_mouseevent_init_all(&mouse_event,
			psy_ui_realpoint_make(0, 0), 1, 0, 0, 0);
		psy_ui_button_emit(self, &mouse_event);
		psy_ui_keyboardevent_stop_propagation(ev);
	}
}

void psy_ui_button_exchange(psy_ui_Button* self, psy_Property* property)
{
	assert(self);
	assert(property);

	assert(self);

	self->property = property;
	if (property) {
		psy_ui_button_on_property_changed(self, property);
		if (!self->prevent_property_text) {
			psy_ui_button_set_text(self, psy_property_text(property));
		}
		psy_property_connect(property, self,
			psy_ui_button_on_property_changed);
		psy_signal_connect(&self->property->before_destroyed, self,
			psy_ui_button_before_property_destroyed);
	}
}

void psy_ui_button_on_property_changed(psy_ui_Button* self,
	psy_Property* sender)
{
	assert(self);

	if (psy_property_is_choice_item(sender)) {
		bool checked;

		checked = (psy_property_at_choice(psy_property_parent(sender))
			== sender);
		if (checked) {
			psy_ui_button_highlight(self);
		} else {
			psy_ui_button_disable_highlight(self);
		}
		return;
	}
	if (!psy_property_is_bool(sender)) {
		return;
	}
	if (psy_property_item_bool(sender)) {
		psy_ui_button_highlight(self);
	} else {
		psy_ui_button_disable_highlight(self);
	}
}

void psy_ui_button_before_property_destroyed(psy_ui_Button* self,
	psy_Property* sender)
{
	assert(self);

	self->property = NULL;
}

void psy_ui_button_on_repeat_timer(psy_ui_Button* self, uintptr_t id)
{
	assert(self);

	if (self->buttonstate == 0) {
		return;
	}
	if (self->repeat.first_repeat && self->repeat.first_repeat_rate !=
		self->repeat.repeat_rate) {
		psy_ui_component_start_timer(&self->component, id,
			self->repeat.repeat_rate);
		self->repeat.first_repeat = FALSE;
	}
	psy_ui_button_emit(self, &self->repeat.repeat_event);
}

void psy_ui_button_set_input_handler(psy_ui_Button* self, InputHandler* input_handler)
{
	assert(self);

	if (!input_handler) {
		return;
	}
	inputhandler_connect(input_handler, INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_button_on_input);
}

bool psy_ui_button_on_input(psy_ui_Button* self, InputHandler* sender)
{
	assert(self);
	
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVSELECT: {
		psy_ui_MouseEvent ev;

		psy_ui_mouseevent_init_all(&ev, psy_ui_realpoint_make(0, 0), 1, 0, 0, 0);
		psy_ui_button_emit(self, &ev);
		return TRUE; }
	default:
		return FALSE;
	}
}
