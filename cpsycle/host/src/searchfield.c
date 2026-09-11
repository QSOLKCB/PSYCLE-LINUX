/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "searchfield.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void searchfield_on_destroyed(SearchField*);
static void searchfield_on_edit_focus(SearchField*, psy_ui_Component* sender);
static void searchfield_on_edit_focus_lost(SearchField*,
	psy_ui_Component* sender);
static void searchfield_on_edit_change(SearchField*, psy_ui_Component* sender);
static void searchfield_on_accept(SearchField*, psy_ui_Component* sender);
static void searchfield_on_reject(SearchField*, psy_ui_Component* sender);
static void searchfield_reset(SearchField*);
static void searchfield_on_language_changed(SearchField*);

/* vtable */
static psy_ui_ComponentVtable searchfield_vtable;
static bool searchfield_vtable_initialized = FALSE;

static void searchfield_vtable_init(SearchField* self)
{
	assert(self);

	if (!searchfield_vtable_initialized) {
		searchfield_vtable = *(self->component.vtable);
		searchfield_vtable.on_destroyed =
			(psy_ui_fp_component)
			searchfield_on_destroyed;
		searchfield_vtable.on_language_changed =
			(psy_ui_fp_component)
			searchfield_on_language_changed;
		searchfield_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &searchfield_vtable);
}

/* implementation */
void searchfield_init(SearchField* self, psy_ui_Component* parent,
	psy_ui_Component* restore_focus_, const psy_ui_SVG* svg)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	searchfield_vtable_init(self);
	psy_signal_init(&self->signal_changed);
	self->default_text_ = strdup("Search");
	self->restore_focus_ = restore_focus_;
	psy_ui_component_set_style_type(searchfield_base(self), STYLE_SEARCHFIELD);
	psy_ui_component_set_style_type_select(searchfield_base(self),
		STYLE_SEARCHFIELD_SELECT);	
	psy_ui_button_init(&self->image_, searchfield_base(self));
	psy_ui_button_set_svg(&self->image_, svg);
	psy_ui_component_set_align(psy_ui_button_base(&self->image_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_margin(psy_ui_button_base(&self->image_),
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 1.0));	
	psy_ui_edit_init(&self->edit_, &self->component);	
	psy_ui_component_set_align(psy_ui_edit_base(&self->edit_),
		psy_ui_ALIGN_CLIENT);
	searchfield_reset(self);
	psy_signal_connect(&self->edit_.pane.component.signal_focus,
		self, searchfield_on_edit_focus);
	psy_signal_connect(&self->edit_.pane.component.signal_focus_lost,
		self, searchfield_on_edit_focus_lost);
	psy_signal_connect(&self->edit_.signal_change,
		self, searchfield_on_edit_change);
	psy_ui_edit_enable_input_field(&self->edit_);
	psy_signal_connect(&self->edit_.signal_accept,
		self, searchfield_on_accept);
	psy_signal_connect(&self->edit_.signal_reject,
		self, searchfield_on_reject);		
}

void searchfield_on_destroyed(SearchField* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_changed);
	free(self->default_text_);
	self->default_text_ = NULL;
}

void searchfield_set_default_text(SearchField* self, const char* text)
{
	assert(self);
	
	psy_strreset(&self->default_text_, text);	
	if (self->has_default_text_) {		
		psy_ui_edit_set_text(&self->edit_, psy_ui_translate(
			self->default_text_));
	}
}

const char* searchfield_text(const SearchField* self)
{
	assert(self);
	
	if (self->has_default_text_) {
		return "";
	}
	return psy_ui_edit_text(&self->edit_);
}

void searchfield_set_restore_focus(SearchField* self,
	psy_ui_Component* restore_focus)
{
	assert(self);

	self->restore_focus_ = restore_focus;
}

void searchfield_on_edit_focus(SearchField* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_ui_component_add_style_state(searchfield_base(self),
		psy_ui_STYLESTATE_SELECT);
	if (self->has_default_text_) {
		psy_ui_edit_set_text(&self->edit_, "");
	}
}

void searchfield_on_edit_focus_lost(SearchField* self, psy_ui_Component* sender)
{
	assert(self);
	
	psy_ui_component_remove_style_state(searchfield_base(self),
		psy_ui_STYLESTATE_SELECT);
}

void searchfield_on_edit_change(SearchField* self, psy_ui_Component* sender)
{
	assert(self);
	
	self->has_default_text_ = FALSE;
	psy_signal_emit(&self->signal_changed, self, 0);	
}

void searchfield_on_accept(SearchField* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (psy_strlen(psy_ui_edit_text(&self->edit_)) == 0) {
		searchfield_reset(self);
	}	
}

void searchfield_on_reject(SearchField* self, psy_ui_Component* sender)
{
	assert(self);
		
	searchfield_reset(self);
	if (self->restore_focus_) {
		psy_ui_component_set_focus(self->restore_focus_);
	}
}

void searchfield_reset(SearchField* self)
{	
	assert(self);
			
	self->has_default_text_ = TRUE;
	psy_ui_edit_set_text(&self->edit_, psy_ui_translate(
		self->default_text_));
	psy_signal_emit(&self->signal_changed, self, 0);	
}

void searchfield_on_language_changed(SearchField* self)
{
	assert(self);
	
	if (self->has_default_text_) {		
		psy_ui_edit_set_text(&self->edit_, psy_ui_translate(
			self->default_text_));
	}
}
