/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "fileedit.h"
/* host */
#include "styles.h"
#include "workspace.h"
/* ui */
#include <uifolderdialog.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void fileedit_on_destroyed(FileEdit*);
static void fileedit_on_property_changed(FileEdit*,
	psy_Property* sender);
static void fileedit_before_property_destroyed(FileEdit*, psy_Property* sender);
static void fileedit_on_dialog(FileEdit*, psy_ui_Component* sender);
static void fileedit_on_select_changed(FileEdit*, psy_FileSelect* sender);
static void fileedit_on_edit_keydown(FileEdit*, psy_ui_Component* sender,
	psy_ui_KeyboardEvent*);
static void fileedit_on_edit_reject(FileEdit*, psy_ui_Edit* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static psy_ui_ComponentVtable super_vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(FileEdit* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		super_vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			fileedit_on_destroyed;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(fileedit_base(self), &vtable);
}

/* implementation */
void fileedit_init(FileEdit* self, psy_ui_Component* parent, psy_DiskOp* disk_op_)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->property_ = NULL;
	self->workspace_ = NULL;
	self->disk_op_ = disk_op_;
	psy_ui_edit_init(&self->edit_, &self->component);
	psy_ui_component_set_align(psy_ui_edit_base(&self->edit_),
		psy_ui_ALIGN_CLIENT);
	psy_ui_edit_enable_input_field(&self->edit_);
	psy_signal_connect(&self->edit_.signal_reject, self,
		fileedit_on_edit_reject);
	psy_signal_connect(&self->edit_.component.signal_key_down, self,
		fileedit_on_edit_keydown);	
	psy_ui_button_init(&self->dialog_, &self->component);
	psy_ui_component_set_align(psy_ui_button_base(&self->dialog_),
		psy_ui_ALIGN_RIGHT);
	psy_ui_button_prevent_translation(&self->dialog_);
	psy_ui_button_set_text(&self->dialog_, "...");
	psy_signal_connect(&self->dialog_.signal_clicked, self,
		fileedit_on_dialog);
}

void fileedit_on_destroyed(FileEdit* self)
{	
	assert(self);

	if (self->property_) {
		psy_property_disconnect(self->property_, self);
	}	
}

FileEdit* fileedit_alloc(void)
{
	return (FileEdit*)malloc(sizeof(FileEdit));
}

FileEdit* fileedit_allocinit(psy_ui_Component* parent, psy_DiskOp* disk_op_)
{
	FileEdit* rv;

	rv = fileedit_alloc();
	if (rv) {
		fileedit_init(rv, parent, disk_op_);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void fileedit_exchange(FileEdit* self, psy_Property* property)
{
	assert(self);
	assert(property);

	self->property_ = property;
	if (property) {
		fileedit_on_property_changed(self, property);
		psy_ui_edit_exchange(&self->edit_, self->property_);
		psy_property_connect(property, self,
			fileedit_on_property_changed);
		psy_signal_connect(&self->property_->before_destroyed, self,
			fileedit_before_property_destroyed);
	}
}

void fileedit_set_workspace(FileEdit* self, Workspace* workspace)
{
	self->workspace_ = workspace;
}

void fileedit_on_property_changed(FileEdit* self, psy_Property* sender)
{
}

void fileedit_before_property_destroyed(FileEdit* self, psy_Property* sender)
{
	assert(self);

	self->property_ = NULL;
}

void fileedit_on_dialog(FileEdit* self,psy_ui_Component* sender)
{
	if (self->workspace_ && self->property_ && self->disk_op_) {
		if (psy_property_hint(self->property_) == PSY_PROPERTY_HINT_EDITDIR) {
			psy_FileSelect dir;				

			assert(self);
					
			psy_fileselect_init(&dir);
			psy_fileselect_connect_change(&dir, self,
				(psy_fp_fileselect_change)fileedit_on_select_changed);
			psy_fileselect_set_title(&dir, psy_ui_translate(
				psy_property_text(self->property_)));
			psy_fileselect_set_directory(&dir, psy_property_item_str(
				self->property_));
			psy_diskop_execute(self->disk_op_, NULL, NULL, &dir);
			psy_fileselect_dispose(&dir);				
		} else {
			psy_FileSelect load;				

			assert(self);
					
			psy_fileselect_init(&load);
			psy_fileselect_connect_change(&load, self,
				(psy_fp_fileselect_change)fileedit_on_select_changed);
			psy_fileselect_set_directory(&load, psy_property_item_str(
					self->property_));
			psy_fileselect_set_title(&load, psy_ui_translate(
				psy_property_text(self->property_)));
			// psy_fileselect_set_default_extension(&load,
			// psy_audio_songfile_standardloadfilter());
			// psy_fileselect_add_filter(&load, "psy", "*.psy");
			// psy_audio_songfile_loadfilter()
			psy_diskop_execute(self->disk_op_, &load, NULL, NULL);
			psy_fileselect_dispose(&load);				
		}	
	}
}

void fileedit_on_select_changed(FileEdit* self, psy_FileSelect* sender)
{
	assert(self);
	
	psy_property_set_item_str(self->property_, psy_fileselect_value(sender));
	workspace_restore_view(self->workspace_);
}

void fileedit_on_edit_keydown(FileEdit* self, psy_ui_Component* sender,
	psy_ui_KeyboardEvent* ev)
{
	if (psy_property_is_hex(self->property_)) {
		if ((psy_ui_keyboardevent_keycode(ev) >= psy_ui_KEY_DIGIT0 && psy_ui_keyboardevent_keycode(ev) <= psy_ui_KEY_DIGIT9) ||
			(psy_ui_keyboardevent_keycode(ev) >= psy_ui_KEY_A && psy_ui_keyboardevent_keycode(ev) <= psy_ui_KEY_F) ||
			(psy_ui_keyboardevent_keycode(ev) < psy_ui_KEY_HELP)) {
			return;
		}
		psy_ui_keyboardevent_prevent_default(ev);
	}
	psy_ui_keyboardevent_stop_propagation(ev);
}

void fileedit_on_edit_reject(FileEdit* self, psy_ui_Edit* sender)
{
	psy_ui_component_set_focus(&self->component);
}

