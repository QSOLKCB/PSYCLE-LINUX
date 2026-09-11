/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FILESELECT_H)
#define FILESELECT_H

#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif


struct psy_FileSelect;

typedef void (*psy_fp_fileselect_change)(void* context,
	struct psy_FileSelect* sender);

typedef struct psy_FileSelectFilterItem {
	char* label;
	char* wildcard;
} psy_FileSelectFilterItem;

void psy_fileselectfilteritem_init(psy_FileSelectFilterItem*,
	const char* label, const char* wildcard);
void psy_fileselectfilteritem_dispose(psy_FileSelectFilterItem*);


psy_FileSelectFilterItem* psy_fileselectfilteritem_alloc(void);
psy_FileSelectFilterItem* psy_fileselectfilteritem_alloc_init(
	const char* label, const char* wildcard);
psy_FileSelectFilterItem* psy_fileselectfilteritem_clone(
	psy_FileSelectFilterItem* src);

void psy_fileselectfilteritem_copy(psy_FileSelectFilterItem* self,
	psy_FileSelectFilterItem* other);	

typedef struct psy_FileSelect {
	psy_fp_fileselect_change change;
	void* context_change;
	psy_fp_fileselect_change cancel;
	void* context_cancel;
	psy_fp_fileselect_change file_name_change;
	void* context_change_dir;
	psy_fp_fileselect_change change_dir;
	void* context_next;
	psy_fp_fileselect_change next;

	void* context_file_name_change;
	char* value;
	char* directory;
	char* title;	
	char* default_extension;
	bool has_preview;
	psy_List* filter_items;	
} psy_FileSelect;

void psy_fileselect_init(psy_FileSelect*);
void psy_fileselect_dispose(psy_FileSelect*);

void psy_fileselect_copy(psy_FileSelect*, const psy_FileSelect* other);
void psy_fileselect_connect_change(psy_FileSelect*, void* context,
	psy_fp_fileselect_change);
void psy_fileselect_connect_cancel(psy_FileSelect*, void* context,
	psy_fp_fileselect_change);
void psy_fileselect_connect_file_name_change(psy_FileSelect*, void* context,
	psy_fp_fileselect_change);
void psy_fileselect_connect_change_dir(psy_FileSelect*,
	void* context, psy_fp_fileselect_change);
void psy_fileselect_connect_next(psy_FileSelect*, void* context,
	psy_fp_fileselect_change);

void psy_fileselect_notify(psy_FileSelect*);
void psy_fileselect_notify_file_name_change(psy_FileSelect*);
void psy_fileselect_notify_change_dir(psy_FileSelect*);
void psy_fileselect_cancel(psy_FileSelect*);
void psy_fileselect_next(psy_FileSelect*);

const char* psy_fileselect_value(const psy_FileSelect*);
void psy_fileselect_set_value(psy_FileSelect*, const char* str);

const char* psy_fileselect_directory(const psy_FileSelect*);
void psy_fileselect_set_directory(psy_FileSelect*, const char* str);

const char* psy_fileselect_title(const psy_FileSelect*);
void psy_fileselect_set_title(psy_FileSelect*, const char* str);

const char* psy_fileselect_default_extension(const psy_FileSelect*);
void psy_fileselect_set_default_extension(psy_FileSelect*, const char* str);

void psy_fileselect_add_filter(psy_FileSelect*, const char* label,
	const char* wildcard);
	
void psy_fileselect_filter_str(psy_FileSelect*, char* rv, uintptr_t maxchars);

INLINE void psy_fileselect_enable_preview(psy_FileSelect* self)
{
	assert(self);

	self->has_preview = TRUE;
}

INLINE bool psy_fileselect_connected(const psy_FileSelect* self)
{
	assert(self);

	return (self->context_change != NULL);
}

INLINE bool psy_fileselect_cancel_connected(const psy_FileSelect* self)
{
	assert(self);

	return (self->context_cancel != NULL);
}

INLINE bool psy_fileselect_equal_connections(psy_FileSelect* self,
	const psy_FileSelect* other)
{
	if (!other) {
		return FALSE;
	}
	return (self->change == other->change &&
		self->change_dir == other->change_dir &&
		self->context_cancel == other->context_cancel &&
		self->context_change == other->context_change &&
		self->context_file_name_change == other->context_file_name_change &&
		self->file_name_change == other->file_name_change);
}


/* psy_DiskOp */

/*
** Defines the interface for a FileChooser
*/

struct psy_DiskOp;

typedef void (*psy_fp_diskop_execute)(struct psy_DiskOp*,
	psy_FileSelect* load, psy_FileSelect* save, psy_FileSelect* dir);

typedef struct psy_DiskOpVtable {	
	psy_fp_diskop_execute execute;	
	psy_fp_diskop_execute execute_wait;
} psy_DiskOpVtable;

typedef struct psy_DiskOp {
	psy_DiskOpVtable* vtable;
} psy_DiskOp;

void psy_diskop_init(psy_DiskOp* self);


INLINE void psy_diskop_execute(psy_DiskOp* self,
	psy_FileSelect* load, psy_FileSelect* save, psy_FileSelect* dir)
{
	assert(self);

	self->vtable->execute(self, load, save, dir);
}

INLINE void psy_diskop_execute_wait(psy_DiskOp* self,
	psy_FileSelect* load, psy_FileSelect* save, psy_FileSelect* dir)
{
	assert(self);

	self->vtable->execute_wait(self, load, save, dir);
}

#ifdef __cplusplus
}
#endif

#endif /* FILESELECT_H */
