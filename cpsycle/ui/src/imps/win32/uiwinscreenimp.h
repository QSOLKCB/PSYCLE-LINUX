/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_WIN32
#ifndef psy_ui_win_SCREENIMP_H
#define psy_ui_win_SCREENIMP_H

#include "../../uiscreen.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_win_ScreenImp {
	psy_ui_ScreenImp imp;	
} psy_ui_win_ScreenImp;

void psy_ui_win_screenimp_init(psy_ui_win_ScreenImp* self);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_win_SCREENIMP_H */
#endif /* PSYCLE_TK_WIN32 */
