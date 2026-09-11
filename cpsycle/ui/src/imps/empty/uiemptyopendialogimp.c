/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptyopendialogimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "../../uiapp.h"
#include <stdlib.h>
#include "../../uiapp.h"
#include "uiemptycomponentimp.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void dev_dispose(psy_ui_empty_OpenDialogImp*);
static int dev_execute(psy_ui_empty_OpenDialogImp*);
static const psy_Path* dev_path(const psy_ui_empty_OpenDialogImp*);

/* vtable */
static psy_ui_OpenDialogImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_empty_OpenDialogImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_fp_opendialogimp_dev_dispose)
			dev_dispose;
		imp_vtable.dev_execute =
			(psy_ui_fp_opendialogimp_dev_execute)
			dev_execute;
		imp_vtable.dev_path =
			(psy_ui_fp_opendialogimp_dev_path)
			dev_path;
		imp_vtable_initialized = TRUE;
	}
}
/* implementation */
void psy_ui_empty_opendialogimp_init(psy_ui_empty_OpenDialogImp* self,
	psy_ui_OpenDialog* dlg, psy_ui_Component* parent)
{
	psy_ui_opendialogimp_init(&self->imp);
	imp_vtable_init(self);	
	self->imp.vtable = &imp_vtable;
	self->dlg = dlg;
	self->parent = parent;
	self->title = strdup("");
	self->filter = strdup("");
	self->defaultextension = strdup("");
	self->initialdir = strdup("");	
	psy_path_init(&self->path, NULL);
}

void psy_ui_empty_opendialogimp_init_all(psy_ui_empty_OpenDialogImp* self,
	psy_ui_OpenDialog* dlg,
	psy_ui_Component* parent,
	const char* title,
	const char* filter,
	const char* defaultextension,
	const char* initialdir)
{
	psy_ui_opendialogimp_init(&self->imp);
	imp_vtable_init(self);
	self->imp.vtable = &imp_vtable;
	self->dlg = dlg;
	self->parent = parent;
	self->title = strdup(title);
	self->filter = strdup(filter);
	self->defaultextension = strdup(defaultextension);
	self->initialdir = strdup(initialdir);
	psy_path_init(&self->path, NULL);
}

void dev_dispose(psy_ui_empty_OpenDialogImp* self)
{
	free(self->title);
	self->title = NULL;
	free(self->filter);
	self->filter = NULL;
	free(self->defaultextension);
	self->defaultextension = NULL;
	free(self->initialdir);
	self->initialdir = NULL;
	psy_path_dispose(&self->path);
}

int dev_execute(psy_ui_empty_OpenDialogImp* self)
{
	return FALSE;
}

const psy_Path* dev_path(const psy_ui_empty_OpenDialogImp* self)
{
	return &self->path;
}

#endif /* PSYCLE_TK_EMPTY */
