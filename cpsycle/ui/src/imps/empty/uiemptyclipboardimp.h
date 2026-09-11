/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_empty_CLIPBOARDIMP_H
#define psy_ui_empty_CLIPBOARDIMP_H

#include "../../uiclipboard.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_empty_ClipboardImp {
	psy_ui_ClipboardImp imp;
	char* text;
} psy_ui_empty_ClipboardImp;

void psy_ui_empty_clipboardimp_init(psy_ui_empty_ClipboardImp* self);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_empty_CLIPBOARDIMP_H */
#endif /* PSYCLE_TK_EMPTY */
