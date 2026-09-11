/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiscreen.h"
/* local */
#include "uiapp.h"
#include "uiimpfactory.h"
/* platform */
#include "../../detail/portable.h"


/* psy_ui_Screen */

/* implementation */
void psy_ui_screen_init(psy_ui_Screen* self)
{	
	assert(self);

	self->imp = psy_ui_impfactory_allocinit_screenimp(
		psy_ui_app_impfactory(psy_ui_app()));
	assert(self->imp);	
}

void psy_ui_screen_dispose(psy_ui_Screen* self)
{	
	assert(self);

	if (self->imp) {
		self->imp->vtable->dev_dispose(self->imp);
		free(self->imp);
		self->imp = NULL;
	}	
}

double psy_ui_screen_width(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_width(self->imp);
	}
	return 1920.0;
}

double psy_ui_screen_height(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_height(self->imp);
	}
	return 1080.0;
}

double psy_ui_screen_avail_width(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_avail_width(self->imp);
	}
	return 1920.0;
}

double psy_ui_screen_avail_height(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_avail_height(self->imp);
	}
	return 1080.0;
}

uintptr_t psy_ui_screen_colour_depth(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_colour_depth(self->imp);
	}
	return 24;
}

uintptr_t psy_ui_screen_pixel_depth(const psy_ui_Screen* self)
{
	assert(self);

	if (self->imp) {
		return self->imp->vtable->dev_pixel_depth(self->imp);
	}
	return 24;
}

/* psy_ui_ScreenImp */

/* prototypes */
static void dev_dispose(psy_ui_ScreenImp* self) { }
static double dev_width(const psy_ui_ScreenImp* self) { return 1920.0; }
static double dev_height(const psy_ui_ScreenImp* self) { return 1080.0; }
static double dev_avail_width(const psy_ui_ScreenImp* self) { return 1920.0; }
static double dev_avail_height(const psy_ui_ScreenImp* self) { return 1080.0; }
static uintptr_t dev_colour_depth(const psy_ui_ScreenImp* self) { return 24; }
static uintptr_t dev_pixel_depth(const psy_ui_ScreenImp* self) { return 24; }

/* vtable */
static psy_ui_ScreenImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

/* implementation */
static void imp_vtable_init(psy_ui_ScreenImp* self)
{
	assert(self);

	if (!imp_vtable_initialized) {
		imp_vtable.dev_dispose = dev_dispose;
		imp_vtable.dev_width = dev_width;
		imp_vtable.dev_height = dev_height;
		imp_vtable.dev_avail_width = dev_avail_width;
		imp_vtable.dev_avail_height = dev_avail_height;
		imp_vtable.dev_colour_depth = dev_colour_depth;
		imp_vtable.dev_pixel_depth = dev_pixel_depth;
		imp_vtable_initialized = TRUE;
	}
	self->vtable = &imp_vtable;
}

/* implementation */
void psy_ui_screenimp_init(psy_ui_ScreenImp* self)
{
	assert(self);

	imp_vtable_init(self);		
}
