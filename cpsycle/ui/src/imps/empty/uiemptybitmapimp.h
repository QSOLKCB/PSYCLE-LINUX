/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_empty_BITMAPIMP_H
#define psy_ui_empty_BITMAPIMP_H

#include "../../uibitmap.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_empty_BitmapImp {
	psy_ui_BitmapImp imp;
	uintptr_t bitmap;
	uintptr_t mask;
} psy_ui_empty_BitmapImp;

void psy_ui_empty_bitmapimp_init(psy_ui_empty_BitmapImp*, psy_ui_RealSize);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_empty_BITMAPIMP_H */
#endif /* PSYCLE_TK_EMPTY */
