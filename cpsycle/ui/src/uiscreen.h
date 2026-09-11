/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_SCREEN_H
#define psy_ui_SCREEN_H

#include "../../detail/psydef.h"
/* local */
#include "uigeometry.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Forward Handler to ScreenImp */
struct psy_ui_ScreenImp;

/*
** @struct psy_ui_Screen
** @brief Information about the user screen.
** 
** @detail
**
** Example:
**   Getting the current height in px of the monitor screen
** Includes:
**   #include <uiscreen.h>
** Code:
**   psy_ui_Screen screen;
**   double height;
**
**   psy_ui_screen_init(&screen);
**   height = psy_ui_screen_height(&screen);
**   psy_ui_screen_dispose(&screen);
*/

typedef struct psy_ui_Screen {
	/* @internal */
	struct psy_ui_ScreenImp* imp;    
} psy_ui_Screen;

void psy_ui_screen_init(psy_ui_Screen*);
void psy_ui_screen_dispose(psy_ui_Screen*);

double psy_ui_screen_width(const psy_ui_Screen*);
double psy_ui_screen_height(const psy_ui_Screen*);
double psy_ui_screen_avail_width(const psy_ui_Screen*);
double psy_ui_screen_avail_height(const psy_ui_Screen*);
uintptr_t psy_ui_screen_colour_depth(const psy_ui_Screen*);
uintptr_t psy_ui_screen_pixel_depth(const psy_ui_Screen*);


/* psy_ui_ScreenImpVTable */
typedef void (*psy_ui_screen_imp_fp_dispose)(struct psy_ui_ScreenImp*);
typedef double (*psy_ui_screen_imp_fp_get_double)(const struct psy_ui_ScreenImp*);
typedef uintptr_t (*psy_ui_screen_imp_fp_get_uint)(const struct psy_ui_ScreenImp*);


typedef struct psy_ui_ScreenImpVTable {
	psy_ui_screen_imp_fp_dispose dev_dispose;
	psy_ui_screen_imp_fp_get_double dev_width;
	psy_ui_screen_imp_fp_get_double dev_height;
	psy_ui_screen_imp_fp_get_double dev_avail_width;
	psy_ui_screen_imp_fp_get_double dev_avail_height;
	psy_ui_screen_imp_fp_get_uint dev_colour_depth;
	psy_ui_screen_imp_fp_get_uint dev_pixel_depth;
} psy_ui_ScreenImpVTable;

/* psy_ui_ScreenImp */
typedef struct psy_ui_ScreenImp {
	psy_ui_ScreenImpVTable* vtable;
} psy_ui_ScreenImp;

void psy_ui_screenimp_init(psy_ui_ScreenImp*);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_SCREEN_H */
