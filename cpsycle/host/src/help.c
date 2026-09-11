/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "help.h"
/* file */
#include <filereader.h>
#include <dir.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/os.h"


/* prototypes */
static void help_on_destroyed(Help*);
static void help_register_files(Help*);
static void help_clear_file_names(Help*);
static void help_build_tabs(Help*);
static void help_on_tabbar_changed(Help*, psy_ui_Component* sender,
	uintptr_t tabindex);
static void help_load_page(Help*, uintptr_t index);
static void help_load(Help*, const char* path);
static void help_on_align(Help*, psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(Help* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			help_on_destroyed;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

 /* implementation  */
void help_init(Help* self, psy_ui_Component* parent,
	const char* doc_path)
{
	assert(self);
	
	psy_ui_component_init(help_base(self), parent, NULL);
	vtable_init(self);	
	self->dos_to_utf8_ = TRUE;
	psy_ui_tabbar_init(&self->tabbar_, help_base(self));
	self->last_align_ = psy_ui_ALIGN_NONE;
	self->doc_path_ = psy_strdup(doc_path);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar_),
		psy_ui_ALIGN_RIGHT);	
	psy_ui_component_set_margin(psy_ui_tabbar_base(&self->tabbar_),
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 1.5));	
	psy_ui_label_init(&self->text_, help_base(self));	
	psy_ui_component_set_overflow(psy_ui_label_base(&self->text_),
		psy_ui_OVERFLOW_SCROLL);
	psy_ui_component_set_margin(psy_ui_label_base(&self->text_),
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 3.0));
	psy_ui_label_enable_wrap(&self->text_);
	psy_ui_component_set_align(psy_ui_label_base(&self->text_),
		psy_ui_ALIGN_CLIENT);	
	psy_ui_label_prevent_translation(&self->text_);	
	psy_ui_label_set_text_alignment(&self->text_, psy_ui_ALIGNMENT_LEFT);
	psy_ui_component_set_scroll_step_height(psy_ui_label_base(
		&self->text_), psy_ui_value_make_eh(1.0));		
	psy_ui_component_set_wheel_scroll(psy_ui_label_base(&self->text_), 4);
	psy_ui_component_set_align(psy_ui_label_base(&self->text_),
		psy_ui_ALIGN_FIXED);		
	psy_ui_scroller_init(&self->scroller_, help_base(self), NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller_, psy_ui_label_base(&self->text_));
	psy_ui_component_set_align(psy_ui_scroller_base(&self->scroller_),
		psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->tabbar_.signal_change,
		self, help_on_tabbar_changed);
	psy_table_init(&self->file_names_);	
	psy_signal_connect(&help_base(self)->signal_align, self, help_on_align);
	help_register_files(self);
	help_load_page(self, 0);
}

void help_on_destroyed(Help* self)
{
	free(self->doc_path_);
	self->doc_path_ = NULL;
	psy_table_dispose_all(&self->file_names_, (psy_fp_disposefunc)NULL);
}

void help_register_files(Help* self)
{			
#ifdef PSYCLE_HELPFILES
	char* text;
	char seps[] = " ";
	char* token;
		
	assert(self);
		
	text = psy_strdup(PSYCLE_HELPFILES);
	token = strtok(text, seps);
	while (token != 0) {
		psy_table_insert(&self->file_names_,
			psy_table_size(&self->file_names_),
			psy_strdup(token));
		token = strtok(0, seps);
	}
	free(text);
	help_build_tabs(self);
#endif
}

void help_build_tabs(Help* self)
{
	psy_TableIterator it;

	assert(self);
	
	psy_ui_tabbar_clear(&self->tabbar_);
	for (it = psy_table_begin(&self->file_names_);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_Path path;

		psy_path_init(&path, (char*)psy_tableiterator_value(&it));		
		psy_ui_tabbar_append(&self->tabbar_, psy_path_name(&path),
			psy_INDEX_INVALID);
		psy_path_dispose(&path);
	}
}

void help_clear_file_names(Help* self)
{
	assert(self);
	
	psy_table_dispose_all(&self->file_names_, (psy_fp_disposefunc)NULL);	
	psy_table_init(&self->file_names_);
}

void help_on_tabbar_changed(Help* self, psy_ui_Component* sender,
	uintptr_t tabindex)
{
	assert(self);
	
	help_load_page(self, tabindex);
	psy_ui_component_align(&self->scroller_.pane);
}

void help_load_page(Help* self, uintptr_t index)
{	
	assert(self);
	
	if ((psy_strlen(self->doc_path_) != 0) && 
			(psy_table_at(&self->file_names_, index) != NULL)) {
		psy_Path path;
		char norm[4096];
		
		psy_path_init(&path, NULL);		
		psy_dir_normalize(self->doc_path_, norm);
		psy_path_set_prefix(&path, norm);		
		psy_path_set_name(&path,(const char*)psy_table_at(
			&self->file_names_, index));
		help_load(self, psy_path_full(&path));
		psy_path_dispose(&path);
	}
	psy_ui_tabbar_mark(&self->tabbar_, index);
}

void help_load(Help* self, const char* path)
{
	psy_FileReader file_reader;
	char* buffer;	

	psy_filereader_init(&file_reader);
	buffer = NULL;
	if (psy_filereader_load(&file_reader, path, &buffer) == PSY_OK) {
		psy_ui_label_set_text(&self->text_, buffer);
	}
	psy_filereader_dispose(&file_reader);
	free(buffer);
	buffer = NULL;	
}

void help_on_align(Help* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (self->last_align_ != psy_ui_component_parent(sender)->align) {
		if (psy_ui_component_parent(sender)->align == psy_ui_ALIGN_RIGHT) {
			psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar_),
				psy_ui_ALIGN_TOP);
			psy_ui_tabbar_set_tab_align(&self->tabbar_, psy_ui_ALIGN_RIGHT);
		} else {
			psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar_),
				psy_ui_ALIGN_RIGHT);
			psy_ui_tabbar_set_tab_align(&self->tabbar_, psy_ui_ALIGN_TOP);
		}
		psy_ui_component_align(psy_ui_tabbar_base(&self->tabbar_));
		self->last_align_ = psy_ui_component_parent(sender)->align;
	}
}
