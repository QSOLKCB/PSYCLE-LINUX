/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "fileselect.h"

/* std */
#include <assert.h>
#include <stdio.h>
/* platform */
#include "../../detail/portable.h"


void psy_fileselectfilteritem_init(psy_FileSelectFilterItem* self,
	const char* label, const char* wildcard)
{
	assert(self);
	
	self->label = psy_strdup(label);
	self->wildcard = psy_strdup(wildcard);	
}

void psy_fileselectfilteritem_dispose(psy_FileSelectFilterItem* self)
{
	assert(self);
	
	free(self->label);
	self->label = NULL;
	free(self->wildcard);
	self->wildcard = NULL;	
}

psy_FileSelectFilterItem* psy_fileselectfilteritem_alloc(void)
{
	return (psy_FileSelectFilterItem*)malloc(sizeof(psy_FileSelectFilterItem));
}

psy_FileSelectFilterItem* psy_fileselectfilteritem_alloc_init(
	const char* label, const char* wildcard)
{
	psy_FileSelectFilterItem* rv;
	
	rv = psy_fileselectfilteritem_alloc();
	if (rv) {
		psy_fileselectfilteritem_init(rv, label, wildcard);
	}
	return rv;
}

psy_FileSelectFilterItem* psy_fileselectfilteritem_clone(
	psy_FileSelectFilterItem* src)
{
	assert(src);

	return psy_fileselectfilteritem_alloc_init(src->label, src->wildcard);
}

void psy_fileselectfilteritem_copy(psy_FileSelectFilterItem* self,
	psy_FileSelectFilterItem* other)
{
	psy_strreset(&self->label, other->label);
	psy_strreset(&self->wildcard, other->wildcard);	
}


/* implementation */
void psy_fileselect_init(psy_FileSelect* self)
{
	assert(self);
	
	self->context_change = NULL;
	self->change = NULL;
	self->context_cancel = NULL;
	self->cancel = NULL;
	self->context_file_name_change = NULL;
	self->file_name_change = NULL;
	self->context_change_dir = NULL;
	self->context_next = NULL;
	self->next = NULL;
	self->change_dir = NULL;
	self->value = psy_strdup("");
	self->directory = psy_strdup(""); 
	self->title = psy_strdup("");	
	self->default_extension = psy_strdup("");;
	self->filter_items = NULL;	
	self->has_preview = FALSE;
}

void psy_fileselect_dispose(psy_FileSelect* self)
{	
	assert(self);
	
	free(self->value);
	self->value = NULL;
	free(self->directory);
	self->directory = NULL;
	free(self->title);
	self->title = NULL;
	free(self->default_extension);
	self->default_extension = NULL;	
	psy_list_deallocate(&self->filter_items, (psy_fp_disposefunc)
		psy_fileselectfilteritem_dispose);
		
}

void psy_fileselect_copy(psy_FileSelect* self, const psy_FileSelect* other)
{
	assert(self);	
	
	if (self != other) {
		if (other) {
			self->context_change = other->context_change;
			self->change = other->change;
			self->context_cancel = other->context_cancel;
			self->cancel = other->cancel;
			self->context_file_name_change = other->context_file_name_change;
			self->file_name_change = other->file_name_change;
			self->context_change_dir = other->context_change_dir;
			self->change_dir = other->change_dir;
			self->context_next = other->context_next;
			self->next = other->next;
			psy_strreset(&self->value, other->value);
			psy_strreset(&self->directory, other->directory);
			psy_strreset(&self->title, other->title);
			psy_strreset(&self->default_extension, other->default_extension);
			self->has_preview = other->has_preview;
			{
				psy_List* p;

				psy_list_deallocate(&self->filter_items, (psy_fp_disposefunc)
					psy_fileselectfilteritem_dispose);
				for (p = other->filter_items; p != NULL; p = p->next) {
					psy_FileSelectFilterItem* item;

					item = (psy_FileSelectFilterItem*)p->entry;
					psy_list_append(&self->filter_items,
						psy_fileselectfilteritem_clone(item));
				}
			}			
		} else {
			self->context_change = NULL;
			self->change = NULL;
			self->context_cancel = NULL;
			self->cancel = NULL;
			self->context_file_name_change = NULL;
			self->file_name_change = NULL;
			self->context_change_dir = NULL;
			self->change_dir = NULL;
			self->context_next = NULL;
			self->next = NULL;
			psy_strreset(&self->value, "");
			psy_strreset(&self->directory, "");
			psy_strreset(&self->title, "");
			psy_strreset(&self->default_extension, "");
			self->has_preview = FALSE;																														
			psy_list_deallocate(&self->filter_items, (psy_fp_disposefunc)
				psy_fileselectfilteritem_dispose);
		}
	}
}

void psy_fileselect_connect_change(psy_FileSelect* self,
	void* context, psy_fp_fileselect_change fp)
{
	assert(self);
	
	self->context_change = context;
	self->change = fp;
}

void psy_fileselect_connect_cancel(psy_FileSelect* self,
	void* context, psy_fp_fileselect_change fp)
{
	assert(self);
	
	self->context_cancel = context;
	self->cancel = fp;
}

void psy_fileselect_connect_file_name_change(psy_FileSelect* self,
	void* context, psy_fp_fileselect_change fp)
{
	assert(self);

	self->context_file_name_change = context;
	self->file_name_change = fp;
}

void psy_fileselect_connect_change_dir(psy_FileSelect* self,
	void* context, psy_fp_fileselect_change fp)
{
	assert(self);

	self->context_change_dir = context;
	self->change_dir = fp;
}

void psy_fileselect_connect_next(psy_FileSelect* self,
	void* context, psy_fp_fileselect_change fp)
{
	assert(self);

	self->context_next = context;
	self->next = fp;
}

void psy_fileselect_notify(psy_FileSelect* self)
{
	assert(self);
		
	if (self->change) {
		self->change(self->context_change, self);
	}
}

void psy_fileselect_notify_file_name_change(psy_FileSelect* self)
{
	assert(self);

	if (self->file_name_change) {
		self->file_name_change(self->context_file_name_change, self);
	}
}

void psy_fileselect_notify_change_dir(psy_FileSelect* self)
{
	assert(self);

	if (self->change_dir) {
		self->change_dir(self->context_change_dir, self);
	}
}

void psy_fileselect_cancel(psy_FileSelect* self)
{
	assert(self);
		
	if (self->cancel) {
		self->cancel(self->context_cancel, self);
	}
}

void psy_fileselect_next(psy_FileSelect* self)
{
	assert(self);

	if (self->next) {
		self->next(self->context_next, self);
	}
}

const char* psy_fileselect_value(const psy_FileSelect* self)
{
	assert(self);
	
	return self->value;
}

void psy_fileselect_set_value(psy_FileSelect* self, const char* str)
{
	assert(self);
	
	psy_strreset(&self->value, str);
}

const char* psy_fileselect_directory(const psy_FileSelect* self)
{
	assert(self);
	
	return self->directory;
}

void psy_fileselect_set_directory(psy_FileSelect* self, const char* str)
{
	assert(self);
	
	psy_strreset(&self->directory, str);
}

const char* psy_fileselect_title(const psy_FileSelect* self)
{
	assert(self);
	
	return self->title;
}

void psy_fileselect_set_title(psy_FileSelect* self, const char* str)
{
	assert(self);
	
	psy_strreset(&self->title, str);
}

const char* psy_fileselect_default_extension(const psy_FileSelect* self)
{
	assert(self);
	
	return self->default_extension;
}

void psy_fileselect_set_default_extension(psy_FileSelect* self, const char* str)
{
	assert(self);
	
	psy_strreset(&self->default_extension, str);
}

void psy_fileselect_add_filter(psy_FileSelect* self, const char* label,
	const char* wildcard)
{
	psy_FileSelectFilterItem* filter_item;
	
	assert(self);
	
	filter_item = psy_fileselectfilteritem_alloc_init(label, wildcard);
	if (filter_item) {
		psy_list_append(&self->filter_items, (void*)filter_item);
	}	
}

void psy_fileselect_filter_str(psy_FileSelect* self, char* rv,
	uintptr_t maxchars)	
{	
	const psy_List* p;
	char* text;
	uintptr_t i;
	uintptr_t numchars;
	
	assert(self);
		
	rv[0] = '\0';
	text = rv;
	numchars = maxchars;
	for (p = self->filter_items, i = 0; p != NULL; p = p->next, ++i) {
		const psy_FileSelectFilterItem* filter_item;
		uintptr_t num;
		
		filter_item = (const psy_FileSelectFilterItem*)p->entry;
		psy_snprintf(text, numchars, "%s|%s%s", filter_item->label,
			filter_item->wildcard, (p->next) ? "|" : "");
		num = psy_strlen(text);
		if (num < numchars) {
			numchars -= num;
			text += num;
		} else {
			break;
		}				
	}	
}

/* prototypes */
static void execute(psy_DiskOp* self, psy_FileSelect* load,
	psy_FileSelect* save, psy_FileSelect* dir)
{
}

/* vtable */
static psy_DiskOpVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_DiskOp* self)
{
	if (!vtable_initialized) {
		vtable.execute = execute;		
		vtable.execute_wait = execute;
		vtable_initialized = TRUE;
	}
	self->vtable = &vtable;
}

/* implementation */
void psy_diskop_init(psy_DiskOp* self)
{
	assert(self);
	
	vtable_init(self);
}
