/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptyfolderdialogimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "../../uiapp.h"
#include <stdlib.h>
#include "uiemptycomponentimp.h"
#include "../../detail/portable.h"


/* prototypes */
static void dev_dispose(psy_ui_empty_FolderDialogImp*);
static int dev_execute(psy_ui_empty_FolderDialogImp*);
static const char* dev_path(psy_ui_empty_FolderDialogImp*);

/* vtable */
static psy_ui_FolderDialogImpVTable imp_vtable;
static int imp_vtable_initialized = 0;

static void imp_vtable_init(psy_ui_empty_FolderDialogImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose = (psy_ui_fp_folderdialogimp_dev_dispose) dev_dispose;
		imp_vtable.dev_execute = (psy_ui_fp_folderdialogimp_dev_execute) dev_execute;
		imp_vtable.dev_path = (psy_ui_fp_folderdialogimp_dev_path) dev_path;
		imp_vtable_initialized = 1;
	}
}
/* implementation */
void psy_ui_empty_folderdialogimp_init(psy_ui_empty_FolderDialogImp* self)
{
	psy_ui_folderdialogimp_init(&self->imp);
	imp_vtable_init(self);	
	self->imp.vtable = &imp_vtable;	
}

void psy_ui_empty_folderdialogimp_init_all(psy_ui_empty_FolderDialogImp* self,
	psy_ui_Component* parent,
	const char* title,
	const char* initialdir)
{
	psy_ui_folderdialogimp_init(&self->imp);
	imp_vtable_init(self);
	self->imp.vtable = &imp_vtable;
	self->parent = parent;
	self->title = strdup(title ? title : "");
	self->initialdir = strdup(initialdir ? initialdir : "");
	self->path = strdup("");
}

void dev_dispose(psy_ui_empty_FolderDialogImp* self)
{
	free(self->title);
	free(self->initialdir);
	free(self->path);
	self->title = NULL;
	self->initialdir = NULL;
	self->path = NULL;
}

int dev_execute(psy_ui_empty_FolderDialogImp* self)
{
	return 0;
}

const char* dev_path(psy_ui_empty_FolderDialogImp* self)
{
	return self->path;
}

#endif /* PSYCLE_TK_EMPTY */
