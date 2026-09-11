/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_CLIPBOARD_H
#define psy_ui_CLIPBOARD_H


#ifdef __cplusplus
extern "C" {
#endif


/* psy_ui_ClipboardImp	*/

struct psy_ui_ClipboardImp;

typedef void (*psy_ui_clipboard_imp_fp_dispose)(struct psy_ui_ClipboardImp*);
typedef const char* (*psy_ui_clipboard_imp_fp_read_text)(
	struct psy_ui_ClipboardImp*);
typedef void (*psy_ui_clipboard_imp_fp_write_text)(struct psy_ui_ClipboardImp*,
	const char*);

typedef struct psy_ui_ClipboardImpVTable {
	psy_ui_clipboard_imp_fp_dispose dev_dispose;
	psy_ui_clipboard_imp_fp_read_text dev_read_text;
	psy_ui_clipboard_imp_fp_write_text dev_write_text;
} psy_ui_ClipboardImpVTable;

typedef struct psy_ui_ClipboardImp {
	psy_ui_ClipboardImpVTable* vtable;
} psy_ui_ClipboardImp;

void psy_ui_clipboard_imp_init(psy_ui_ClipboardImp*);


/* psy_ui_Clipboard */

typedef struct psy_ui_Clipboard {	
	psy_ui_ClipboardImp* imp;
} psy_ui_Clipboard;

void psy_ui_clipboard_init(psy_ui_Clipboard*);
void psy_ui_clipboard_dispose(psy_ui_Clipboard*);

void psy_ui_clipboard_set_text(psy_ui_Clipboard*, const char* text);
const char* psy_ui_clipboard_text(const psy_ui_Clipboard*);





#ifdef __cplusplus
}
#endif

#endif /* psy_ui_CLIPBOARD */
