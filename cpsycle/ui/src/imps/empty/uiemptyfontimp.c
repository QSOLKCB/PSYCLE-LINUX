/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptyfontimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

/* local */
#include "../../uiapp.h"
/* file */
#include <encoding.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_empty_font_imp_dispose(psy_ui_empty_FontImp*);
static void psy_ui_empty_font_imp_copy(psy_ui_empty_FontImp*, psy_ui_empty_FontImp* other);
static psy_ui_FontInfo dev_fontinfo(psy_ui_empty_FontImp*);
static const psy_ui_TextMetric* dev_textmetric(const psy_ui_empty_FontImp*);
static bool dev_equal(const psy_ui_empty_FontImp*, const psy_ui_empty_FontImp* other);
static psy_ui_Size dev_textsize(const psy_ui_empty_FontImp*,
	const char* text, uintptr_t count);

/* vtable */
static psy_ui_FontImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_empty_FontImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_font_imp_fp_dispose)
			psy_ui_empty_font_imp_dispose;
		imp_vtable.dev_copy =
			(psy_ui_font_imp_fp_copy)
			psy_ui_empty_font_imp_copy;
		imp_vtable.dev_fontinfo =
			(psy_ui_font_imp_fp_dev_fontinfo)
			dev_fontinfo;
		imp_vtable.dev_textmetric =
			(psy_ui_font_imp_fp_dev_textmetric)
			dev_textmetric;
		imp_vtable.dev_textsize =
			(psy_ui_font_imp_fp_dev_textsize)
			dev_textsize;
		imp_vtable.dev_equal =
			(psy_ui_font_imp_fp_dev_equal)
			dev_equal;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

void psy_ui_empty_fontimp_init(psy_ui_empty_FontImp* self, const psy_ui_FontInfo* fontinfo)
{
	psy_ui_font_imp_init(&self->imp);
	imp_vtable_init(self);
	self->tmcachevalid = FALSE;
	if (fontinfo) {		
		
	} else {
		self->hfont = 0;		
	}	
}

void psy_ui_empty_font_imp_dispose(psy_ui_empty_FontImp* self)
{	
	if (self->hfont) {		
		self->hfont = 0;
	}
}

void psy_ui_empty_font_imp_copy(psy_ui_empty_FontImp* self, psy_ui_empty_FontImp* other)
{				
	
}

psy_ui_FontInfo dev_fontinfo(psy_ui_empty_FontImp* self)
{
	psy_ui_FontInfo rv;
	
	psy_ui_fontinfo_init(&rv, "system", 8);
	return rv;
}

const psy_ui_TextMetric* dev_textmetric(const psy_ui_empty_FontImp* self)
{
	psy_ui_TextMetric rv;
	
	psy_ui_textmetric_init(&rv);
	if (!self->tmcachevalid) {
		
		((psy_ui_empty_FontImp*)self)->tmcache = rv;
		((psy_ui_empty_FontImp*)self)->tmcachevalid = TRUE;		
	}
	return &self->tmcache;
}

psy_ui_Size dev_textsize(const psy_ui_empty_FontImp* self,
	const char* text, uintptr_t count)
{	
	return psy_ui_size_make_em((double)count, 1.0);
}

bool dev_equal(const psy_ui_empty_FontImp* self, const psy_ui_empty_FontImp* other)
{
	return self->hfont == other->hfont;
}

#endif
