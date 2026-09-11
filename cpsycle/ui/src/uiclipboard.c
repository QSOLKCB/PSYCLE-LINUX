/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiclipboard.h"
/* local */
#include "uiapp.h"
#include "uiimpfactory.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_ClipboardImp */

/* psy_ui_ClipboardImp */

static void psy_ui_clipboard_imp_dispose(psy_ui_ClipboardImp* self)
{

}

static const char* psy_ui_clipboard_imp_read_text(psy_ui_ClipboardImp* self)
{ 
	return "";
}

static void psy_ui_clipboard_imp_write_text(psy_ui_ClipboardImp* self,
	const char* text)
{	
}

/* vtable */
static psy_ui_ClipboardImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(void)
{
	if (!imp_vtable_initialized) {
		imp_vtable.dev_dispose = psy_ui_clipboard_imp_dispose;
		imp_vtable.dev_read_text = psy_ui_clipboard_imp_read_text;
		imp_vtable.dev_write_text = psy_ui_clipboard_imp_write_text;		
		imp_vtable_initialized = TRUE;
	}
}

/* implementation */
void psy_ui_clipboard_imp_init(psy_ui_ClipboardImp* self)
{
	imp_vtable_init();
	self->vtable = &imp_vtable;
}


/* psy_ui_Clipboard */

void psy_ui_clipboard_init(psy_ui_Clipboard* self)
{
	assert(self);
	
	self->imp = psy_ui_impfactory_allocinit_clipboardimp(
		psy_ui_app_impfactory(psy_ui_app()));
}

void psy_ui_clipboard_dispose(psy_ui_Clipboard* self)
{
	assert(self);

	if (self->imp) {
		self->imp->vtable->dev_dispose(self->imp);
		free(self->imp);
		self->imp = NULL;
	}
}

void psy_ui_clipboard_set_text(psy_ui_Clipboard* self, const char* text)
{
	assert(self);

	if (self->imp) {
		self->imp->vtable->dev_write_text(self->imp, text);
	}
}

const char* psy_ui_clipboard_text(const psy_ui_Clipboard* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_read_text(self->imp);
	}
	return "";
}
