/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uix11screenimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_X11

/* local */
#include "../../uiapp.h"
/* file */
#include <encoding.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_x11_screen_imp_dev_dispose(psy_ui_x11_ScreenImp*);
static double psy_ui_x11_screen_imp_dev_width(const psy_ui_x11_ScreenImp*);
static double psy_ui_x11_screen_imp_dev_height(const psy_ui_x11_ScreenImp*);
static double psy_ui_x11_screen_imp_dev_avail_width(const psy_ui_x11_ScreenImp*);
static double psy_ui_x11_screen_imp_dev_avail_height(const psy_ui_x11_ScreenImp*);
static uintptr_t psy_ui_x11_screen_imp_dev_colour_depth(const psy_ui_x11_ScreenImp*);
static uintptr_t psy_ui_x11_screen_imp_dev_pixel_depth(const psy_ui_x11_ScreenImp*);

/* vtable */
static psy_ui_ScreenImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_x11_ScreenImp* self)
{
	assert(self);

	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_screen_imp_fp_dispose)
			psy_ui_x11_screen_imp_dev_dispose;
		imp_vtable.dev_width =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_x11_screen_imp_dev_width;
		imp_vtable.dev_height =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_x11_screen_imp_dev_height;
		imp_vtable.dev_avail_width =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_x11_screen_imp_dev_avail_width;
		imp_vtable.dev_avail_height =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_x11_screen_imp_dev_avail_height;
		imp_vtable.dev_colour_depth =
			(psy_ui_screen_imp_fp_get_uint)
			psy_ui_x11_screen_imp_dev_colour_depth;
		imp_vtable.dev_pixel_depth =
			(psy_ui_screen_imp_fp_get_uint)
			psy_ui_x11_screen_imp_dev_pixel_depth;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

void psy_ui_x11_screenimp_init(psy_ui_x11_ScreenImp* self)
{
	assert(self);

	psy_ui_screenimp_init(&self->imp);
	imp_vtable_init(self);	
}

void psy_ui_x11_screen_imp_dev_dispose(psy_ui_x11_ScreenImp* self)
{	
	assert(self);
}

double psy_ui_x11_screen_imp_dev_width(const psy_ui_x11_ScreenImp* self)
{
	assert(self);

	return 1920.0;
}

double psy_ui_x11_screen_imp_dev_height(const psy_ui_x11_ScreenImp* self)
{
	assert(self);

	return 1080.0;
}

double psy_ui_x11_screen_imp_dev_avail_width(const psy_ui_x11_ScreenImp* self)
{
	assert(self);

	return 1920.0;
}

double psy_ui_x11_screen_imp_dev_avail_height(const psy_ui_x11_ScreenImp* self)
{
	assert(self);

	return 1080.0;
}

uintptr_t psy_ui_x11_screen_imp_dev_colour_depth(const psy_ui_x11_ScreenImp* self)
{
	return 24;
}

uintptr_t psy_ui_x11_screen_imp_dev_pixel_depth(const psy_ui_x11_ScreenImp* self)
{
	assert(self);

	return psy_ui_x11_screen_imp_dev_colour_depth(self);
}

#endif /* PSYCLE_USE_TK == PSYCLE_TK_X11 */
