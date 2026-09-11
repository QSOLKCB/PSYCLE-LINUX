/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_X11
#ifndef psy_ui_x11_CLIPBOARDIMP_H
#define psy_ui_x11_CLIPBOARDIMP_H

#include "../../uiclipboard.h"

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_ui_X11App;

/*!
** @struct psy_ui_x11_ClipboardImp
** @brief X11 Imp for psy_ui_Clipboard to handle text copy paste
*/
typedef struct psy_ui_x11_ClipboardImp {
	psy_ui_ClipboardImp imp;
	char* text;
} psy_ui_x11_ClipboardImp;

void psy_ui_x11_clipboardimp_init(psy_ui_x11_ClipboardImp* self);

/* called by the x11 app event loop */
void psy_ui_x11_clipboardimp_handle_selection_request(
	psy_ui_x11_ClipboardImp*, struct psy_ui_X11App*, XEvent*);
void psy_ui_x11_clipboardimp_handle_selection_notify(
	psy_ui_x11_ClipboardImp*, XEvent*);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_x11_CLIPBOARDIMP_H */
#endif /* PSYCLE_TK_X11 */
