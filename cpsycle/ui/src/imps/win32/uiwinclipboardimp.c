/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiwinclipboardimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_WIN32

/* local */
#include "../../uiapp.h"
/* file */
#include <encoding.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_win_clipboardimp_dispose(psy_ui_win_ClipboardImp*);
static const char* psy_ui_win_clipboardimp_read_text(psy_ui_win_ClipboardImp*);
static void psy_ui_win_clipboardimp_write_text(psy_ui_win_ClipboardImp*,
	const char* text);

/* vtable */
static psy_ui_ClipboardImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_win_ClipboardImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_clipboard_imp_fp_dispose)
			psy_ui_win_clipboardimp_dispose;
		imp_vtable.dev_read_text = 
			(psy_ui_clipboard_imp_fp_read_text)
			psy_ui_win_clipboardimp_read_text;
		imp_vtable.dev_write_text =
			(psy_ui_clipboard_imp_fp_write_text)
			psy_ui_win_clipboardimp_write_text;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

/* implementation */
void psy_ui_win_clipboardimp_init(psy_ui_win_ClipboardImp* self)
{
	assert(self);

	psy_ui_clipboard_imp_init(&self->imp);
	imp_vtable_init(self);
	self->text = NULL;
}

void psy_ui_win_clipboardimp_dispose(psy_ui_win_ClipboardImp* self)
{
	assert(self);

	free(self->text);
	self->text = NULL;
}

const char* psy_ui_win_clipboardimp_read_text(psy_ui_win_ClipboardImp* self)
{
	HANDLE data;
	char* text;

	assert(self);

	if (!OpenClipboard(NULL)) {
		psy_strreset(&self->text, "");
		return self->text;
	}			
	data = GetClipboardData(CF_TEXT);
	if (!data) {
		psy_strreset(&self->text, "");
		return self->text;
	}			 
	text = (char*)(GlobalLock(data));
	psy_strreset(&self->text, text);
	GlobalUnlock(data);	
	CloseClipboard();
	return self->text;
}

void psy_ui_win_clipboardimp_write_text(psy_ui_win_ClipboardImp* self,
	const char* text)
{
	HGLOBAL hMem;
	uintptr_t len;

	assert(self);

	psy_strreset(&self->text, text);		
	len = psy_strlen(text) + 1;
	hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	if (!hMem) {
		return;
	}
	memcpy(GlobalLock(hMem), text ? text : "", len);
	GlobalUnlock(hMem);
	if (!OpenClipboard(NULL)) {
		return;
	}
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
}

#endif /* PSYCLE_TK_WIN32 */
