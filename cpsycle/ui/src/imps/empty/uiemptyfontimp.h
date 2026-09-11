/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_empty_FONTIMP_H
#define psy_ui_empty_FONTIMP_H

#include "../../uifont.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_empty_FontImp {
	psy_ui_FontImp imp;
	uintptr_t hfont;
	psy_ui_TextMetric tmcache;
	bool tmcachevalid;
} psy_ui_empty_FontImp;

void psy_ui_empty_fontimp_init(psy_ui_empty_FontImp* self,
	const psy_ui_FontInfo*);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_empty_FONTIMP_H */
#endif /* PSYCLE_TK_EMPTY */
