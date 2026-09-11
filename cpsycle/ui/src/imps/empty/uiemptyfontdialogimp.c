/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptyfontdialogimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "../../uiapp.h"
#include <stdlib.h>
#include "../../uiapp.h"
#include "uiemptycomponentimp.h"
/* platform */
#include "../../detail/portable.h"

/* prototypes */
static void dev_dispose(psy_ui_empty_FontDialogImp*);
static int dev_execute(psy_ui_empty_FontDialogImp*);
static const char* dev_path(psy_ui_empty_FontDialogImp*);
static void dev_setfontinfo(psy_ui_empty_FontDialogImp*,
	psy_ui_FontInfo);
psy_ui_FontInfo dev_fontinfo(psy_ui_empty_FontDialogImp*);

/* vtable */
static psy_ui_FontDialogImpVTable imp_vtable;
static int imp_vtable_initialized = 0;

static void imp_vtable_init(psy_ui_empty_FontDialogImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose = (psy_ui_fp_fontdialogimp_dev_dispose) dev_dispose;
		imp_vtable.dev_execute = (psy_ui_fp_fontdialogimp_dev_execute) dev_execute;		
		imp_vtable.dev_fontinfo = (psy_ui_fp_fontdialogimp_dev_fontinfo) dev_fontinfo;
		imp_vtable.dev_setfontinfo = (psy_ui_fp_fontdialogimp_dev_setfontinfo)dev_setfontinfo;
		imp_vtable_initialized = 1;
	}
}
/* implementation */
void psy_ui_empty_fontdialogimp_init(psy_ui_empty_FontDialogImp* self)
{
	psy_ui_fontdialogimp_init(&self->imp);
	imp_vtable_init(self);	
	self->imp.vtable = &imp_vtable;
	psy_ui_fontinfo_init(&self->fontinfo, "arial", 8);
}

void dev_dispose(psy_ui_empty_FontDialogImp* self)
{
}

int dev_execute(psy_ui_empty_FontDialogImp* self)
{
	int rv;
	
	return rv;
}

psy_ui_FontInfo dev_fontinfo(psy_ui_empty_FontDialogImp* self)
{
	return self->fontinfo;	
}

void dev_setfontinfo(psy_ui_empty_FontDialogImp* self,
	psy_ui_FontInfo fontinfo)
{
	self->fontinfo = fontinfo;
}


#endif /* PSYCLE_TK_EMPTY */
