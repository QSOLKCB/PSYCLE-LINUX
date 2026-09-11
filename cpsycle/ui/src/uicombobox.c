/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uicombobox.h"
/* local */
#include "uiapp.h"
#include "uiicons.h"
#include "trackercmds.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_combobox_init_buttons(psy_ui_ComboBox*,
	psy_ui_Component* parent, bool expand);
static void psy_ui_combobox_on_destroyed(psy_ui_ComboBox*);
static void psy_ui_combobox_on_property_changed(psy_ui_ComboBox*,
	psy_Property* sender);
static void psy_ui_combobox_before_property_destroyed(psy_ui_ComboBox*,
	psy_Property* sender);
static bool psy_ui_combobox_has_prev_entry(const psy_ui_ComboBox*);
static bool psy_ui_combobox_has_next_entry(const psy_ui_ComboBox*);
static void psy_ui_combobox_on_sel_change(psy_ui_ComboBox*,
	psy_ui_ListBox* sender);
static void psy_ui_combobox_on_less(psy_ui_ComboBox*, psy_ui_Button* sender);
static void psy_ui_combobox_on_more(psy_ui_ComboBox*, psy_ui_Button* sender);
static void psy_ui_combobox_on_expand(psy_ui_ComboBox*, psy_ui_Button* sender);
static void psy_ui_combobox_on_text_field(psy_ui_ComboBox*,
	psy_ui_Label* sender, psy_ui_MouseEvent*);
static void psy_ui_combobox_expand(psy_ui_ComboBox*);
static void psy_ui_combobox_on_mouse_wheel(psy_ui_ComboBox*,
	psy_ui_MouseEvent*);
static void psy_ui_combobox_on_textfield_changed(psy_ui_ComboBox*,
	psy_ui_Edit* sender);
static bool psy_ui_combobox_on_input(psy_ui_ComboBox*,
	InputHandler* sender);
static bool psy_ui_combobox_on_edit_input(psy_ui_ComboBox*,
	InputHandler* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_ui_ComboBox* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super_vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_combobox_on_destroyed;
		vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			psy_ui_combobox_on_mouse_wheel;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(psy_ui_combobox_base(self),
		&vtable);
}

/* implementation */
void psy_ui_combobox_init(psy_ui_ComboBox* self, psy_ui_Component* parent)
{	
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->property_ = NULL;
	self->simple_ = FALSE;
	self->prevent_wheel_select_ = FALSE;
	psy_signal_init(&self->signal_selchanged);	
	psy_ui_component_set_style_type(&self->component,
		psy_ui_STYLE_COMBOBOX);
	psy_ui_component_set_style_type_focus(&self->component,
		psy_ui_STYLE_FOCUS);	
	psy_ui_component_set_align_expand(psy_ui_combobox_base(self),
		psy_ui_HEXPAND);	
	/* dropdown */
	psy_ui_dropdownbox_init(&self->dropdown_, self->component.view);
	psy_ui_component_init(&self->pane_, &self->dropdown_.component,
		&self->dropdown_.component);
	psy_ui_component_set_padding(&self->pane_, psy_ui_margin_make_em(
		0.0, 0.2, 0.2, 0.2));
	psy_ui_component_set_align(&self->pane_, psy_ui_ALIGN_CLIENT);
	/* listbox */
	psy_ui_listbox_init(&self->listbox_, &self->pane_);
	self->listbox_.scroller.prevent_mouse_down_propagation = FALSE;
	psy_signal_connect(&self->listbox_.signal_selchanged, self,
		psy_ui_combobox_on_sel_change);
	psy_ui_component_set_align(psy_ui_listbox_base(&self->listbox_),
		psy_ui_ALIGN_CLIENT);	
	/* text */
	psy_ui_component_init(&self->editpane_, &self->component, NULL);	
	psy_ui_component_set_align(&self->editpane_, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_style_type_focus(&self->editpane_,
		psy_ui_STYLE_FOCUS);
	psy_ui_edit_init(&self->text_, &self->editpane_);
	psy_ui_component_set_style_type(psy_ui_edit_base(&self->text_),
		psy_ui_STYLE_COMBOBOX_TEXT);
	psy_ui_component_set_align(psy_ui_edit_base(&self->text_),
		psy_ui_ALIGN_CLIENT);	
	// psy_ui_edit_set_char_number(&self->text_, 10.0);
	psy_signal_connect(&psy_ui_edit_base(&self->text_)->signal_mouse_down,
		self, psy_ui_combobox_on_text_field);
	psy_ui_edit_prevent(&self->text_);	
	/* buttons */
	psy_ui_combobox_init_buttons(self, &self->component, TRUE /* expand button */);
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, NULL, (fp_inputhandler_input)psy_ui_combobox_on_input);
	inputhandler_connect(psy_ui_app_input_handler(psy_ui_app()),
		INPUTHANDLER_FOCUS,
		psy_EVENTDRIVER_CMD, "tracker", psy_INDEX_INVALID,
		self, &self->editpane_,
		(fp_inputhandler_input)psy_ui_combobox_on_edit_input);
}

void psy_ui_combobox_init_simple(psy_ui_ComboBox* self,
	psy_ui_Component* parent)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->property_ = NULL;
	self->simple_ = TRUE;
	psy_signal_init(&self->signal_selchanged);
	psy_ui_component_set_style_type(&self->component, psy_ui_STYLE_COMBOBOX);
	psy_ui_component_set_align_expand(psy_ui_combobox_base(self),
		psy_ui_HEXPAND);	
	/* listbox */
	psy_ui_component_init(&self->pane_, &self->component, NULL);
	psy_ui_component_set_padding(&self->pane_, psy_ui_margin_make_em(
		0.0, 0.2, 0.2, 0.2));
	psy_ui_component_set_align(&self->pane_, psy_ui_ALIGN_CLIENT);
	psy_ui_listbox_init(&self->listbox_, &self->pane_);
	self->listbox_.scroller.prevent_mouse_down_propagation = FALSE;
	psy_signal_connect(&self->listbox_.signal_selchanged, self,
		psy_ui_combobox_on_sel_change);
	psy_ui_component_set_align(psy_ui_listbox_base(&self->listbox_),
		psy_ui_ALIGN_CLIENT);	
	/* text */
	psy_ui_component_init(&self->editpane_, &self->component, NULL);
	psy_ui_component_set_align(&self->editpane_, psy_ui_ALIGN_TOP);	
	psy_ui_component_set_style_type_focus(&self->editpane_,
		psy_ui_STYLE_FOCUS);
	psy_ui_edit_init(&self->text_, &self->editpane_);	
	psy_ui_component_set_style_type(psy_ui_edit_base(&self->text_),
		psy_ui_STYLE_COMBOBOX_TEXT);
	psy_ui_component_set_align(psy_ui_edit_base(&self->text_),
		psy_ui_ALIGN_LEFT);
	psy_ui_edit_set_char_number(&self->text_, 10.0);
	psy_signal_connect(&psy_ui_edit_base(&self->text_)->signal_mouse_down,
		self, psy_ui_combobox_on_text_field);
	psy_signal_connect(&self->text_.signal_change,
		self, psy_ui_combobox_on_textfield_changed);
	/* buttons */
	psy_ui_component_init(&self->buttons_, &self->editpane_, NULL);
	psy_ui_component_set_align_expand(&self->buttons_, psy_ui_HEXPAND);
	psy_ui_component_set_align(&self->buttons_, psy_ui_ALIGN_LEFT);
	/* less */
	psy_ui_button_init_connect(&self->less_, &self->buttons_,
		self, psy_ui_combobox_on_less);
	psy_ui_component_set_align(psy_ui_button_base(&self->less_),
		psy_ui_ALIGN_LEFT);
	psy_ui_button_set_svg(&self->less_, psy_ui_app_svg(psy_ui_app(), "img.icon-less"));
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->less_));
	/* more */
	psy_ui_button_init_connect(&self->more_, &self->buttons_,
		self, psy_ui_combobox_on_more);
	psy_ui_button_set_svg(&self->more_, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
	psy_ui_component_set_align(psy_ui_button_base(&self->more_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->more_));
}

void psy_ui_combobox_init_buttons(psy_ui_ComboBox* self,
	psy_ui_Component* parent, bool expand)
{
	assert(self);

	/* buttons */
	psy_ui_component_init(&self->buttons_, parent, NULL);
	psy_ui_component_set_align_expand(&self->buttons_, psy_ui_HEXPAND);
	psy_ui_component_set_align(&self->buttons_, psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_default_align(&self->buttons_, psy_ui_ALIGN_LEFT,
		psy_ui_margin_zero());
	/* less */
	psy_ui_button_init_resource_connect(&self->less_, &self->buttons_, "img.icon-less",
		self, psy_ui_combobox_on_less);		
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->less_));
	/* more */
	psy_ui_button_init_resource_connect(&self->more_, &self->buttons_, "img.icon-more",
		self, psy_ui_combobox_on_more);	
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->more_));
	if (expand) {		
		psy_ui_button_init_resource_connect(&self->expand_, &self->buttons_,
			"img.icon-down", self, psy_ui_combobox_on_expand);		
		psy_ui_component_prevent_app_focus_out(psy_ui_button_base(&self->expand_));
	}
}
	
void psy_ui_combobox_on_destroyed(psy_ui_ComboBox* self)
{
	assert(self);	
	
	if (self->property_) {
		psy_property_disconnect(self->property_, self);
	}	
	psy_signal_dispose(&self->signal_selchanged);
	if (!self->simple_) {
		psy_ui_component_destroy(&self->dropdown_.component);
	}
	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),
		self, psy_ui_combobox_on_input);
	inputhandler_disconnect(psy_ui_app_input_handler(psy_ui_app()),
		self, psy_ui_combobox_on_edit_input);
}

psy_ui_ComboBox* psy_ui_combobox_alloc(void)
{
	return (psy_ui_ComboBox*)malloc(sizeof(psy_ui_ComboBox));
}

psy_ui_ComboBox* psy_ui_combobox_alloc_init(psy_ui_Component* parent)
{
	psy_ui_ComboBox* rv;

	rv = psy_ui_combobox_alloc();
	if (rv) {
		psy_ui_combobox_init(rv, parent);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void psy_ui_combobox_exchange(psy_ui_ComboBox* self,
	psy_Property* property)
{
	self->property_ = property;
	if (self->property_) {
		psy_List* p;

		p = psy_property_begin(self->property_);
		for (; p != NULL; p = p->next) {
			psy_Property* property;

			property = (psy_Property*)p->entry;
			psy_ui_combobox_add_text(self,
				(psy_property_translation_prevented(property))
				? psy_property_text(property)
				: psy_ui_translate(psy_property_text(property)));
		}
		psy_ui_combobox_on_property_changed(self, self->property_);
		psy_property_connect(self->property_, self,
			psy_ui_combobox_on_property_changed);
		psy_signal_connect(&self->property_->before_destroyed, self,
			psy_ui_combobox_before_property_destroyed);
	}
}

void psy_ui_combobox_on_property_changed(psy_ui_ComboBox* self,
	psy_Property* sender)
{
	if (psy_property_is_int(sender) || psy_property_is_choice(sender)) {
		psy_ui_combobox_select(self, psy_property_item_int(
			self->property_));
	}
}

void psy_ui_combobox_before_property_destroyed(psy_ui_ComboBox* self,
	psy_Property* sender)
{
	assert(self);

	self->property_ = NULL;
}

uintptr_t psy_ui_combobox_add_text(psy_ui_ComboBox* self, const char* text)
{
	assert(self);

	return psy_ui_listbox_add_text(&self->listbox_, text);
}

void psy_ui_combobox_set_text(psy_ui_ComboBox* self, const char* text,
	uintptr_t index)
{
	assert(self);

	psy_ui_listbox_set_text(&self->listbox_, text, index);
	if (index == psy_ui_combobox_cur_sel(self)) {
		char text[512];

		psy_ui_combobox_text(self, text);
		psy_ui_edit_set_text(&self->text_, text);
	}
}

void psy_ui_combobox_text(psy_ui_ComboBox* self, char* rv)
{
	const char* edit_text;
	
	assert(self);

	edit_text = psy_ui_edit_text(&self->text_);
	psy_snprintf(rv, 256, "%s", edit_text);
}

void psy_ui_combobox_text_at(psy_ui_ComboBox* self, char* text, uintptr_t index)
{
	assert(self);

	psy_ui_listbox_text(&self->listbox_, text, index);
}

uintptr_t psy_ui_combobox_count(const psy_ui_ComboBox* self)
{
	assert(self);

	return psy_ui_listbox_count(&self->listbox_);
}

void psy_ui_combobox_clear(psy_ui_ComboBox* self)
{
	assert(self);

	psy_ui_listbox_clear(&self->listbox_);	
	psy_ui_edit_set_text(&self->text_, "");
}

void psy_ui_combobox_select(psy_ui_ComboBox* self, uintptr_t index)
{
	char text[512];

	assert(self);

	psy_ui_listbox_set_cur_sel(&self->listbox_, index);
	psy_ui_listbox_text(&self->listbox_, text, index);	
	psy_ui_edit_set_text(&self->text_, text);
}

uintptr_t psy_ui_combobox_cur_sel(const psy_ui_ComboBox* self)
{
	assert(self);

	return psy_ui_listbox_cur_sel(&self->listbox_);
}

void psy_ui_combobox_set_char_number(psy_ui_ComboBox* self, double number)
{
	assert(self);

	psy_ui_edit_set_char_number(&self->text_, number);
	if (number == 0.0) {
		psy_ui_component_set_align(psy_ui_edit_base(&self->text_),
			psy_ui_ALIGN_CLIENT);
		psy_ui_component_set_align(&self->buttons_, psy_ui_ALIGN_RIGHT);
	} else {
		psy_ui_component_set_align(&self->buttons_, psy_ui_ALIGN_LEFT);
		psy_ui_component_set_align(&self->editpane_, psy_ui_ALIGN_LEFT);
	}	
}

void psy_ui_combobox_set_item_id(psy_ui_ComboBox* self, uintptr_t index,
	uintptr_t id)
{
	psy_ui_ListItem* item;

	assert(self);

	item = psy_ui_listbox_at(&self->listbox_, index);
	if (item) {
		item->id = id;
	}	
}

uintptr_t psy_ui_combobox_item_id(psy_ui_ComboBox* self, uintptr_t index)
{
	const psy_ui_ListItem* item;

	assert(self);

	item = psy_ui_listbox_at_const(&self->listbox_, index);
	if (item) {
		return item->id;
	}
	return psy_INDEX_INVALID;
}

bool psy_ui_combobox_has_prev_entry(const psy_ui_ComboBox* self)
{
	assert(self);

	return psy_ui_combobox_cur_sel(self) > 0;
}

bool psy_ui_combobox_has_next_entry(const psy_ui_ComboBox* self)
{
	assert(self);
	
	if (psy_ui_combobox_cur_sel(self) == psy_INDEX_INVALID) {
		return FALSE;
	}
	return (psy_ui_combobox_cur_sel(self) + 1 < psy_ui_combobox_count(self));
}

void psy_ui_combobox_on_sel_change(psy_ui_ComboBox* self,
	psy_ui_ListBox* sender)
{
	char text[512];
	uintptr_t index;

	assert(self);

	if (!self->simple_) {
		psy_ui_dropdownbox_hide(&self->dropdown_);
	}
	index = psy_ui_listbox_cur_sel(sender);
	psy_ui_listbox_set_cur_sel(&self->listbox_, index);	
	psy_ui_listbox_text(&self->listbox_, text, index);
	psy_ui_edit_set_text(&self->text_, text);
	if (self->property_ && index != psy_property_item_int(self->property_)) {
		psy_property_set_item_int(self->property_, index);
	}
	psy_signal_emit(&self->signal_selchanged, self, 0);
}

void psy_ui_combobox_on_less(psy_ui_ComboBox* self, psy_ui_Button* sender)
{
	uintptr_t index;

	assert(self);

	index = psy_ui_combobox_cur_sel(self);
	if (index == psy_INDEX_INVALID) {
		return;
	}
	if (index > 0) {
		psy_ui_combobox_select(self, index - 1);
		if (self->property_) {
			psy_property_set_item_int(self->property_, index - 1);
		}
		psy_signal_emit(&self->signal_selchanged, self, 0);
	}
}

void psy_ui_combobox_on_more(psy_ui_ComboBox* self, psy_ui_Button* sender)
{
	uintptr_t count;
	uintptr_t index;

	assert(self);

	index = psy_ui_combobox_cur_sel(self);
	if (index == psy_INDEX_INVALID) {
		return;
	}
	count = psy_ui_combobox_count(self);
	if (index < count - 1) {
		psy_ui_combobox_select(self, index + 1);
		if (self->property_) {
			psy_property_set_item_int(self->property_, index + 1);
		}
		psy_signal_emit(&self->signal_selchanged, self, 0);
	}
}

void psy_ui_combobox_on_text_field(psy_ui_ComboBox* self, psy_ui_Label* sender,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_combobox_expand(self);
	psy_ui_mouseevent_stop_propagation(ev);
}

void psy_ui_combobox_on_expand(psy_ui_ComboBox* self, psy_ui_Button* sender)
{
	assert(self);
	
	psy_ui_combobox_expand(self);	
}

void psy_ui_combobox_expand(psy_ui_ComboBox* self)
{
	assert(self);

	if (self->simple_) {
		return;
	}
	if (!psy_ui_component_visible(&self->dropdown_.component)) {
		psy_ui_dropdownbox_show(&self->dropdown_, &self->component);
		psy_ui_component_capture(&self->dropdown_.component);
		self->dropdown_.component.capture_relative = TRUE;
	}
}

void psy_ui_combobox_on_mouse_wheel(psy_ui_ComboBox* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (self->prevent_wheel_select_) {		
		return;
	}
	if (psy_ui_mouseevent_delta(ev) != 0) {
		uintptr_t index;
		intptr_t delta;

		index = psy_ui_combobox_cur_sel(self);
		if (index == psy_INDEX_INVALID) {
			return;
		}
		delta = psy_sgn(psy_ui_mouseevent_delta(ev));
		if ((intptr_t)index + delta > 0) {
			index += delta;
		} else {
			index = 0;
		}
		if (index >= 0 && index < psy_ui_combobox_count(self)) {
			psy_ui_combobox_select(self, index);
			if (self->property_ && index != psy_property_item_int(
					self->property_)) {
				psy_property_set_item_int(self->property_, index);
			}
			psy_signal_emit(&self->signal_selchanged, self, 0);
		}
	}
	psy_ui_mouseevent_prevent_default(ev);
}

void psy_ui_combobox_on_textfield_changed(psy_ui_ComboBox* self,
	psy_ui_Edit* sender)
{
	psy_ui_listbox_set_cur_sel(&self->listbox_, psy_INDEX_INVALID);
}

bool psy_ui_combobox_on_input(psy_ui_ComboBox* self, InputHandler* sender)
{
	psy_ui_Component* restore;

	assert(self);

	restore = psy_ui_app_focus(psy_ui_app());
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVUP: {
		psy_ui_combobox_on_less(self, NULL);
		psy_ui_component_set_focus(restore);
		return TRUE; }
	case CMD_NAVDOWN: {
		psy_ui_combobox_on_more(self, NULL);
		psy_ui_component_set_focus(restore);
		return TRUE; }
	case CMD_NAVSELECT: {
		psy_ui_component_set_tab_index(&self->editpane_, 0);
		psy_ui_component_set_focus(&self->editpane_);
		/* psy_ui_combobox_expand(self);
		if (psy_ui_component_draw_visible(&self->listbox_.component)) {
			psy_ui_component_set_focus(&self->listbox_.pane.component);
		}*/
		return TRUE; }
	default:
		return FALSE;
	}
}

bool psy_ui_combobox_on_edit_input(psy_ui_ComboBox* self, InputHandler* sender)
{
	psy_ui_Component* restore;

	assert(self);

	restore = psy_ui_app_focus(psy_ui_app());
	switch (inputhandler_cmd(sender).id) {
	case CMD_NAVUP: {
		psy_ui_combobox_on_less(self, NULL);
		psy_ui_component_set_focus(restore);
		return TRUE; }
	case CMD_NAVDOWN: {
		psy_ui_combobox_on_more(self, NULL);
		psy_ui_component_set_focus(restore);
		return TRUE; }
	case CMD_NAVSELECT: {
		psy_ui_component_set_tab_index(&self->editpane_, psy_INDEX_INVALID);
		psy_ui_component_set_focus(&self->component);
		/* psy_ui_combobox_expand(self);
		if (psy_ui_component_draw_visible(&self->listbox_.component)) {
			psy_ui_component_set_focus(&self->listbox_.pane.component);
		}*/
		return TRUE; }
	default:
		return FALSE;
	}
}
