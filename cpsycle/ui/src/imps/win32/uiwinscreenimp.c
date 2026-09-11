/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiwinscreenimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_WIN32

/* local */
#include "../../uiapp.h"
/* file */
#include <encoding.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_win_screen_imp_dev_dispose(psy_ui_win_ScreenImp*);
static double psy_ui_win_screen_imp_dev_width(const psy_ui_win_ScreenImp*);
static double psy_ui_win_screen_imp_dev_height(const psy_ui_win_ScreenImp*);
static double psy_ui_win_screen_imp_dev_avail_width(const psy_ui_win_ScreenImp*);
static double psy_ui_win_screen_imp_dev_avail_height(const psy_ui_win_ScreenImp*);
static uintptr_t psy_ui_win_screen_imp_dev_colour_depth(const psy_ui_win_ScreenImp*);
static uintptr_t psy_ui_win_screen_imp_dev_pixel_depth(const psy_ui_win_ScreenImp*);

/* vtable */
static psy_ui_ScreenImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_win_ScreenImp* self)
{
	assert(self);

	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_screen_imp_fp_dispose)
			psy_ui_win_screen_imp_dev_dispose;
		imp_vtable.dev_width =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_win_screen_imp_dev_width;
		imp_vtable.dev_height =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_win_screen_imp_dev_height;
		imp_vtable.dev_avail_width =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_win_screen_imp_dev_avail_width;
		imp_vtable.dev_avail_height =
			(psy_ui_screen_imp_fp_get_double)
			psy_ui_win_screen_imp_dev_avail_height;
		imp_vtable.dev_colour_depth =
			(psy_ui_screen_imp_fp_get_uint)
			psy_ui_win_screen_imp_dev_colour_depth;
		imp_vtable.dev_pixel_depth =
			(psy_ui_screen_imp_fp_get_uint)
			psy_ui_win_screen_imp_dev_pixel_depth;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

void psy_ui_win_screenimp_init(psy_ui_win_ScreenImp* self)
{
	assert(self);

	psy_ui_screenimp_init(&self->imp);
	imp_vtable_init(self);	
}

void psy_ui_win_screen_imp_dev_dispose(psy_ui_win_ScreenImp* self)
{	
	assert(self);
}

double psy_ui_win_screen_imp_dev_width(const psy_ui_win_ScreenImp* self)
{
	assert(self);

	return (double)GetSystemMetrics(SM_CXSCREEN);	
}

double psy_ui_win_screen_imp_dev_height(const psy_ui_win_ScreenImp* self)
{
	assert(self);

	return (double)GetSystemMetrics(SM_CYSCREEN);
}

double psy_ui_win_screen_imp_dev_avail_width(const psy_ui_win_ScreenImp* self)
{
	assert(self);

	/* excluding size of taskbar */
	return (double)GetSystemMetrics(SM_CXFULLSCREEN);
}

double psy_ui_win_screen_imp_dev_avail_height(const psy_ui_win_ScreenImp* self)
{
	assert(self);

	/* excluding size of taskbar */
	return (double)GetSystemMetrics(SM_CYFULLSCREEN);
}

uintptr_t psy_ui_win_screen_imp_dev_colour_depth(const psy_ui_win_ScreenImp* self)
{
	HDC screen;
	uintptr_t rv;

	assert(self);

	screen = GetDC(NULL);
	rv = (uintptr_t)GetDeviceCaps(screen, BITSPIXEL);
	ReleaseDC(NULL, screen);
	return rv;
}

uintptr_t psy_ui_win_screen_imp_dev_pixel_depth(const psy_ui_win_ScreenImp* self)
{
	assert(self);

	return psy_ui_win_screen_imp_dev_colour_depth(self);
}

#endif /* PSYCLE_USE_TK == PSYCLE_TK_WIN32 */
