/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptybitmapimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

/* local */
#include "../../uiapp.h"
#include "uiemptyapp.h"
#include "../../uibmpreader.h"

/* prototypes */
static void dev_dispose(psy_ui_empty_BitmapImp*);
static int dev_load(psy_ui_empty_BitmapImp*, struct psy_ui_Bitmap*,
	const char* path);
static int dev_loadresource(psy_ui_empty_BitmapImp*, uintptr_t resourceid);
static psy_ui_RealSize dev_size(const psy_ui_empty_BitmapImp*);
static bool dev_empty(const psy_ui_empty_BitmapImp*);
static void dev_settransparency(psy_ui_empty_BitmapImp*, psy_ui_Colour);
static void dev_preparemask(psy_ui_empty_BitmapImp*, psy_ui_Colour clrtrans);
static void dev_copy(psy_ui_empty_BitmapImp*,
	const psy_ui_empty_BitmapImp* other);
static uintptr_t dev_native(psy_ui_empty_BitmapImp*);
static uintptr_t dev_native_mask(psy_ui_empty_BitmapImp*);
	
/* vtable */
static psy_ui_BitmapImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_empty_BitmapImp* self)
{
	assert(self);

	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_bitmap_imp_fp_dispose)
			dev_dispose;
		imp_vtable.dev_load =
			(psy_ui_bitmap_imp_fp_load)
			dev_load;
		imp_vtable.dev_loadresource =
			(psy_ui_bitmap_imp_fp_loadresource)
			dev_loadresource;
		imp_vtable.dev_size =
			(psy_ui_bitmap_imp_fp_size)
			dev_size;
		imp_vtable.dev_empty =
			(psy_ui_bitmap_imp_fp_empty)
			dev_empty;
		imp_vtable.dev_settransparency =
			(psy_ui_bitmap_imp_fp_settransparency)
			dev_settransparency;
		imp_vtable.dev_copy =
			(psy_ui_bitmap_imp_fp_copy)
			dev_copy;
		imp_vtable.dev_native =
			(psy_ui_bitmap_imp_fp_native)
			dev_native;
		imp_vtable.dev_native_mask =
			(psy_ui_bitmap_imp_fp_native)
			dev_native_mask;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

/* implementation */
void psy_ui_empty_bitmapimp_init(psy_ui_empty_BitmapImp* self, psy_ui_RealSize size)
{
	assert(self);

	psy_ui_bitmap_imp_init(&self->imp);
	imp_vtable_init(self);	
	self->mask = 0;
	if (size.width == 0 && size.height == 0) {
		self->bitmap = 0;
	} else {
		
	}	
}

void dev_dispose(psy_ui_empty_BitmapImp* self)
{
	assert(self);

	if (self->bitmap) {		
		self->bitmap = 0;
	}
	if (self->mask) {		
		self->mask = 0;
	}
}

int dev_load(psy_ui_empty_BitmapImp* self, struct psy_ui_Bitmap* bitmap,
	const char* path)
{
	int rv;

	assert(path);

	psy_ui_BmpReader bmpreader;

	psy_ui_bmpreader_init(&bmpreader, bitmap);
	rv = psy_ui_bmpreader_load(&bmpreader, path);
	psy_ui_bmpreader_dispose(&bmpreader);
	return rv;
}

int dev_loadresource(psy_ui_empty_BitmapImp* self, uintptr_t resourceid)
{
	psy_ui_EmptyApp* emptyapp;	

	assert(self);
	
	dev_dispose(self);
	self->mask = 0;
	emptyapp = (psy_ui_EmptyApp*)psy_ui_app()->imp;
	self->bitmap = 0;
	return (self->bitmap == 0);
}

psy_ui_RealSize dev_size(const psy_ui_empty_BitmapImp* self)
{
	assert(self);
	
	return psy_ui_realsize_zero();	
}

bool dev_empty(const psy_ui_empty_BitmapImp* self)
{
	assert(self);

	return self->bitmap == 0;
}

void dev_settransparency(psy_ui_empty_BitmapImp* self, psy_ui_Colour colour)
{
	dev_preparemask(self, colour);
}

void dev_preparemask(psy_ui_empty_BitmapImp* self, psy_ui_Colour clrtrans)
{
	
}	

void dev_copy(psy_ui_empty_BitmapImp* self, const psy_ui_empty_BitmapImp* other)
{
	dev_dispose(self);
}	

uintptr_t dev_native(psy_ui_empty_BitmapImp* self)
{
	return (uintptr_t)self->bitmap;
}

uintptr_t dev_native_mask(psy_ui_empty_BitmapImp* self)
{
	return (uintptr_t)self->mask;
}

#endif /* PSYCLE_TK_EMPTY */
