/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptygraphicsimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "uiemptyfontimp.h"
#include "uiemptybitmapimp.h"
#include "../../uiapp.h"
#include "../../uicomponent.h"
/* file */
#include <encoding.h>
/* portable */
#include "../../detail/portable.h"


/* prototypes */
static void psy_ui_empty_g_imp_dispose(psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_imp_textout(psy_ui_empty_GraphicsImp*,
	double x, double y, const char*, uintptr_t len);
static void psy_ui_empty_g_imp_textoutrectangle(psy_ui_empty_GraphicsImp*,
	double x, double y, uintptr_t options,
	psy_ui_RealRectangle r, const char* text, uintptr_t len);
static void psy_ui_empty_g_imp_drawrectangle(psy_ui_empty_GraphicsImp*,
	const psy_ui_RealRectangle);
static void psy_ui_empty_g_imp_drawroundrectangle(psy_ui_empty_GraphicsImp*,
	const psy_ui_RealRectangle, psy_ui_RealSize cornersize);
static psy_ui_Size psy_ui_empty_g_imp_textsize(psy_ui_empty_GraphicsImp*,
	const char*, uintptr_t count);
static void psy_ui_empty_g_imp_drawsolidrectangle(psy_ui_empty_GraphicsImp*,
	const psy_ui_RealRectangle r, psy_ui_Colour);
static void psy_ui_empty_g_imp_drawsolidroundrectangle(psy_ui_empty_GraphicsImp*,
	const psy_ui_RealRectangle r, psy_ui_RealSize cornersize, psy_ui_Colour);
static void psy_ui_empty_g_imp_drawsolidpolygon(psy_ui_empty_GraphicsImp*,
	psy_ui_RealPoint*, uintptr_t numpoints, uint32_t inner, uint32_t outter);
static void psy_ui_empty_g_imp_drawline(psy_ui_empty_GraphicsImp*,
	double x1, double y1, double x2, double y2);
static void psy_ui_empty_g_imp_drawfullbitmap(psy_ui_empty_GraphicsImp*,
	psy_ui_Bitmap*, double x, double y);
static void psy_ui_empty_g_imp_drawbitmap(psy_ui_empty_GraphicsImp*,
	psy_ui_Bitmap*, double x, double y, double width,
	double height, double xsrc, double ysrc);
static void psy_ui_empty_g_imp_drawstretchedbitmap(psy_ui_empty_GraphicsImp*,
	psy_ui_Bitmap*, double x, double y, double width,
	double height, double xsrc, double ysrc, double wsrc, double hsrc);
static void psy_ui_empty_g_imp_setbackgroundcolour(psy_ui_empty_GraphicsImp*,
	psy_ui_Colour);
static void psy_ui_empty_g_imp_setbackgroundmode(psy_ui_empty_GraphicsImp*,
	uintptr_t mode);
static void psy_ui_empty_g_imp_settextcolour(psy_ui_empty_GraphicsImp*,
	psy_ui_Colour);
static void psy_ui_empty_g_imp_settextalign(psy_ui_empty_GraphicsImp*,
	uintptr_t align);
static void psy_ui_empty_g_imp_setcolour(psy_ui_empty_GraphicsImp*,
	psy_ui_Colour colour);
static psy_ui_Colour psy_ui_empty_g_imp_textcolour(
	const psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_imp_setfont(psy_ui_empty_GraphicsImp*,
	psy_ui_Font* font);
static const psy_ui_Font* psy_ui_empty_g_imp_font(
	const psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_imp_moveto(psy_ui_empty_GraphicsImp*,
	psy_ui_RealPoint);
static void psy_ui_empty_g_imp_devcurveto(psy_ui_empty_GraphicsImp*,
	psy_ui_RealPoint control_p1, psy_ui_RealPoint control_p2,
	psy_ui_RealPoint p);
static void psy_ui_empty_g_imp_devdrawarc(psy_ui_empty_GraphicsImp*,
	psy_ui_RealRectangle, double angle_start, double angle_end);
static void psy_ui_empty_g_devsetlinewidth(psy_ui_empty_GraphicsImp*,
	uintptr_t width);
static unsigned int psy_ui_empty_g_devlinewidth(psy_ui_empty_GraphicsImp*);
static psy_ui_TextMetric psy_ui_empty_g_textmetric(
	const psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_devsetorigin(psy_ui_empty_GraphicsImp*,
	double x, double y);
static psy_ui_RealPoint psy_ui_empty_g_devorigin(
	const psy_ui_empty_GraphicsImp*);
static uintptr_t psy_ui_empty_g_dev_gc(psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_dev_setcliprect(psy_ui_empty_GraphicsImp*,
	psy_ui_RealRectangle);
static psy_ui_RealRectangle psy_ui_empty_g_dev_cliprect(
	const psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_imp_dispose(psy_ui_empty_GraphicsImp*);
static void psy_ui_empty_g_imp_set_encoding(psy_ui_empty_GraphicsImp*,
	psy_Encoding encoding);

/* vtable */
static psy_ui_GraphicsImpVTable empty_imp_vtable;
static bool empty_imp_vtable_initialized = FALSE;

static void empty_imp_vtable_init(psy_ui_empty_GraphicsImp* self)
{
	if (!empty_imp_vtable_initialized) {
		empty_imp_vtable = *self->imp.vtable;
		empty_imp_vtable.dev_dispose =
			(psy_ui_fp_graphicsimp_dev_dispose)
			psy_ui_empty_g_imp_dispose;
		empty_imp_vtable.dev_textout =
			(psy_ui_fp_graphicsimp_dev_textout)
			psy_ui_empty_g_imp_textout;
		empty_imp_vtable.dev_textoutrectangle =
			(psy_ui_fp_graphicsimp_dev_textoutrectangle)
			psy_ui_empty_g_imp_textoutrectangle;
		empty_imp_vtable.dev_drawrectangle =
			(psy_ui_fp_graphicsimp_dev_drawrectangle)
			psy_ui_empty_g_imp_drawrectangle;
		empty_imp_vtable.dev_drawroundrectangle =
			(psy_ui_fp_graphicsimp_dev_drawroundrectangle)
			psy_ui_empty_g_imp_drawroundrectangle;
		empty_imp_vtable.dev_textsize =
			(psy_ui_fp_graphicsimp_dev_textsize)
			psy_ui_empty_g_imp_textsize;
		empty_imp_vtable.dev_drawsolidrectangle =
			(psy_ui_fp_graphicsimp_dev_drawsolidrectangle)
			psy_ui_empty_g_imp_drawsolidrectangle;
		empty_imp_vtable.dev_drawsolidroundrectangle =
			(psy_ui_fp_graphicsimp_dev_drawsolidroundrectangle)
			psy_ui_empty_g_imp_drawsolidroundrectangle;
		empty_imp_vtable.dev_drawsolidpolygon =
			(psy_ui_fp_graphicsimp_dev_drawsolidpolygon)
			psy_ui_empty_g_imp_drawsolidpolygon;
		empty_imp_vtable.dev_drawline =
			(psy_ui_fp_graphicsimp_dev_drawline)
			psy_ui_empty_g_imp_drawline;
		empty_imp_vtable.dev_drawfullbitmap =
			(psy_ui_fp_graphicsimp_dev_drawfullbitmap)
			psy_ui_empty_g_imp_drawfullbitmap;
		empty_imp_vtable.dev_drawbitmap =
			(psy_ui_fp_graphicsimp_dev_drawbitmap)
			psy_ui_empty_g_imp_drawbitmap;
		empty_imp_vtable.dev_drawstretchedbitmap =
			(psy_ui_fp_graphicsimp_dev_drawstretchedbitmap)
			psy_ui_empty_g_imp_drawstretchedbitmap;
		empty_imp_vtable.dev_setbackgroundcolour =
			(psy_ui_fp_graphicsimp_dev_setbackgroundcolour)
			psy_ui_empty_g_imp_setbackgroundcolour;
		empty_imp_vtable.dev_setbackgroundmode =
			(psy_ui_fp_graphicsimp_dev_setbackgroundmode)
			psy_ui_empty_g_imp_setbackgroundmode;
		empty_imp_vtable.dev_settextcolour =
			(psy_ui_fp_graphicsimp_dev_settextcolour)
			psy_ui_empty_g_imp_settextcolour;
		empty_imp_vtable.dev_textcolour =
			(psy_ui_fp_graphicsimp_dev_colour)
			psy_ui_empty_g_imp_textcolour;
		empty_imp_vtable.dev_settextalign =
			(psy_ui_fp_graphicsimp_dev_settextalign)
			psy_ui_empty_g_imp_settextalign;
		empty_imp_vtable.dev_setcolour =
			(psy_ui_fp_graphicsimp_dev_setcolour)
			psy_ui_empty_g_imp_setcolour;
		empty_imp_vtable.dev_setfont =
			(psy_ui_fp_graphicsimp_dev_setfont)
			psy_ui_empty_g_imp_setfont;
		empty_imp_vtable.dev_font =
			(psy_ui_fp_graphicsimp_dev_font)
			psy_ui_empty_g_imp_font;
		empty_imp_vtable.dev_moveto =
			(psy_ui_fp_graphicsimp_dev_moveto)
			psy_ui_empty_g_imp_moveto;
		empty_imp_vtable.dev_curveto =
			(psy_ui_fp_graphicsimp_dev_curveto)
			psy_ui_empty_g_imp_devcurveto;
		empty_imp_vtable.dev_drawarc =
			(psy_ui_fp_graphicsimp_dev_drawarc)
			psy_ui_empty_g_imp_devdrawarc;
		empty_imp_vtable.dev_setlinewidth =
			(psy_ui_fp_graphicsimp_dev_setlinewidth)
			psy_ui_empty_g_devsetlinewidth;
		empty_imp_vtable.dev_linewidth =
			(psy_ui_fp_graphicsimp_dev_linewidth)
			psy_ui_empty_g_devlinewidth;
		empty_imp_vtable.dev_textmetric =
			(psy_ui_fp_graphicsimp_dev_textmetric)
			psy_ui_empty_g_textmetric;
		empty_imp_vtable.dev_setorigin =
			(psy_ui_fp_graphicsimp_dev_setorigin)
			psy_ui_empty_g_devsetorigin;
		empty_imp_vtable.dev_origin =
			(psy_ui_fp_graphicsimp_dev_origin)
			psy_ui_empty_g_devorigin;
		empty_imp_vtable.dev_gc =
			(psy_ui_fp_graphicsimp_dev_gc)
			psy_ui_empty_g_dev_gc;
		empty_imp_vtable.dev_setcliprect =
			(psy_ui_fp_graphicsimp_dev_setcliprect)
			psy_ui_empty_g_dev_setcliprect;
		empty_imp_vtable.dev_cliprect =
			(psy_ui_fp_graphicsimp_dev_cliprect)
			psy_ui_empty_g_dev_cliprect;
		empty_imp_vtable.dev_set_encoding =
			(psy_ui_fp_graphicsimp_dev_set_encoding)
			psy_ui_empty_g_imp_set_encoding;
		empty_imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &empty_imp_vtable;
}

/* implementation */
void psy_ui_empty_graphicsimp_init(psy_ui_empty_GraphicsImp* self,
	uintptr_t hdc)
{
	psy_ui_graphics_imp_init(&self->imp);
	empty_imp_vtable_init(self);
	self->hdc = hdc;
	self->shareddc = TRUE;
	self->colour = psy_ui_style_const(psy_ui_STYLE_ROOT)->colour;	
	psy_ui_realpoint_init(&self->org);	
	self->font = NULL;
	self->encoding = PSY_ENCODING_UTF8;	
}

void psy_ui_empty_graphicsimp_init_bitmap(psy_ui_empty_GraphicsImp* self,
	psy_ui_Bitmap* bitmap)
{
	uintptr_t hdc;
	psy_ui_empty_BitmapImp* imp;

	imp = NULL;
	psy_ui_graphics_imp_init(&self->imp);
	empty_imp_vtable_init(self);
	self->imp.vtable = &empty_imp_vtable;
	hdc = 0;
	self->shareddc = FALSE;		
	self->colour = psy_ui_style_const(psy_ui_STYLE_ROOT)->colour;	
	psy_ui_realpoint_init(&self->org);	
	self->font = NULL;
	self->encoding = PSY_ENCODING_UTF8;
}

void psy_ui_empty_g_imp_dispose(psy_ui_empty_GraphicsImp* self)
{
	
}

void psy_ui_empty_g_imp_textout(psy_ui_empty_GraphicsImp* self, double x, double y,
	const char* str, uintptr_t len)
{
	
}

void psy_ui_empty_g_imp_textoutrectangle(psy_ui_empty_GraphicsImp* self,
	double x, double y, uintptr_t options,
	psy_ui_RealRectangle r, const char* str, uintptr_t len)
{
	
}

psy_ui_Size psy_ui_empty_g_imp_textsize(psy_ui_empty_GraphicsImp* self,
	const char* text, uintptr_t count)
{
	psy_ui_Size	rv;

	if (text) {		
		rv.width = psy_ui_value_make_px(count * 8.0);
		rv.height = psy_ui_value_make_px(12.0);
	} else {
		rv.width = psy_ui_value_make_px(0);
		rv.height = psy_ui_value_make_px(0);
	}
	return rv;
}

void psy_ui_empty_g_imp_drawrectangle(psy_ui_empty_GraphicsImp* self,
	const psy_ui_RealRectangle r)
{	
}

void psy_ui_empty_g_imp_drawroundrectangle(psy_ui_empty_GraphicsImp* self,
	const psy_ui_RealRectangle r, psy_ui_RealSize cornersize)
{
	
}

void psy_ui_empty_g_imp_drawsolidrectangle(psy_ui_empty_GraphicsImp* self,
	const psy_ui_RealRectangle r, psy_ui_Colour colour)
{
	if (colour.mode.gc) {
		colour = self->colour;
	}
	if (!colour.mode.transparent) {		
	}
}

void psy_ui_empty_g_imp_drawsolidroundrectangle(psy_ui_empty_GraphicsImp* self,
	const psy_ui_RealRectangle r, psy_ui_RealSize cornersize,
	psy_ui_Colour colour)
{
	if (colour.mode.gc) {
		colour = self->colour;
	}
	if (!colour.mode.transparent) {
		
	}
}

void psy_ui_empty_g_imp_drawsolidpolygon(psy_ui_empty_GraphicsImp* self,
	psy_ui_RealPoint* pts, uintptr_t numpoints, uint32_t inner,
	uint32_t outter)
{
	if (numpoints > 0) {
	}
}

void psy_ui_empty_g_imp_drawfullbitmap(psy_ui_empty_GraphicsImp* self,
	psy_ui_Bitmap* bitmap, double x, double y)
{
	
}

void psy_ui_empty_g_imp_drawbitmap(psy_ui_empty_GraphicsImp* self,
	psy_ui_Bitmap* bitmap, double x, double y, double width,
	double height, double xsrc, double ysrc)
{
		
}

void psy_ui_empty_g_imp_drawstretchedbitmap(psy_ui_empty_GraphicsImp* self,
	psy_ui_Bitmap* bitmap, double x, double y, double width,
	double height, double xsrc, double ysrc, double wsrc, double hsrc)
{
	
}

void psy_ui_empty_g_imp_setcolour(psy_ui_empty_GraphicsImp* self, psy_ui_Colour colour)
{	
	self->colour = colour;	
}

void psy_ui_empty_g_imp_setbackgroundmode(psy_ui_empty_GraphicsImp* self,
	uintptr_t mode)
{
	if (mode == psy_ui_TRANSPARENT) {	
	} else if (mode == psy_ui_OPAQUE) {		
	}
}

void psy_ui_empty_g_imp_setbackgroundcolour(psy_ui_empty_GraphicsImp* self,
	psy_ui_Colour colour)
{
	
}

void psy_ui_empty_g_imp_settextcolour(psy_ui_empty_GraphicsImp* self,
	psy_ui_Colour colour)
{	
	
}

psy_ui_Colour psy_ui_empty_g_imp_textcolour(const psy_ui_empty_GraphicsImp*
	self) 
{	
	return psy_ui_colour_white();
}

void psy_ui_empty_g_imp_settextalign(psy_ui_empty_GraphicsImp* self, uintptr_t align)
{
}

void psy_ui_empty_g_imp_setfont(psy_ui_empty_GraphicsImp* self, psy_ui_Font* font)
{
	self->font = font;	
}

const psy_ui_Font* psy_ui_empty_g_imp_font(
	const psy_ui_empty_GraphicsImp* self)
{
	return self->font;
}

void psy_ui_empty_g_imp_drawline(psy_ui_empty_GraphicsImp* self,
	double x1, double y1, double x2, double y2)
{
	
}

void psy_ui_empty_g_imp_moveto(psy_ui_empty_GraphicsImp* self,
	psy_ui_RealPoint pt)
{

}

void psy_ui_empty_g_imp_devcurveto(psy_ui_empty_GraphicsImp* self,
	psy_ui_RealPoint control_p1, psy_ui_RealPoint control_p2,
	psy_ui_RealPoint p)
{
	
}

void psy_ui_empty_g_imp_devdrawarc(psy_ui_empty_GraphicsImp* self,
	psy_ui_RealRectangle r, double angle_start, double angle_end)
{
	
}

void psy_ui_empty_g_devsetlinewidth(psy_ui_empty_GraphicsImp* self,
	uintptr_t width)
{
	
}

unsigned int psy_ui_empty_g_devlinewidth(psy_ui_empty_GraphicsImp* self)
{	
	return 1;
}

psy_ui_TextMetric psy_ui_empty_g_textmetric(
	const psy_ui_empty_GraphicsImp* self)
{
	psy_ui_TextMetric rv;

	psy_ui_textmetric_init(&rv);

	return rv;
}

void psy_ui_empty_g_devsetorigin(psy_ui_empty_GraphicsImp* self,
	double x, double y)
{
	self->org.x = x;
	self->org.y = y;
}

psy_ui_RealPoint psy_ui_empty_g_devorigin(const psy_ui_empty_GraphicsImp* self)
{
	return self->org;
}

uintptr_t psy_ui_empty_g_dev_gc(psy_ui_empty_GraphicsImp* self)
{
	return self->hdc;
}

void psy_ui_empty_g_dev_setcliprect(psy_ui_empty_GraphicsImp* self, psy_ui_RealRectangle clip)
{
	self->clip = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(
			clip.left - (int)(self->org.x),
			clip.top - (int)(self->org.y)),
		psy_ui_realsize_make(
			clip.right - clip.left,
			clip.bottom - clip.top));
	if ((((int)clip.right - (int)clip.left) == 0) ||
		(((int)clip.bottom - (int)clip.top) == 0)) {
		/* clear clip */
	} else {
		/* set clip */
	}
}

psy_ui_RealRectangle psy_ui_empty_g_dev_cliprect(
	const psy_ui_empty_GraphicsImp* self)
{
	psy_ui_RealRectangle rv;

	rv = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(
			self->clip.left + (int)(self->org.x),
			self->clip.top + (int)(self->org.y)),
		psy_ui_realsize_make(
			self->clip.right - self->clip.left,
			self->clip.bottom - self->clip.top));
	return rv;	
}

void psy_ui_empty_g_imp_set_encoding(psy_ui_empty_GraphicsImp* self,
	psy_Encoding encoding)
{
	self->encoding = encoding;
}

#endif /* PSYCLE_USE_TK == PSYCLE_TK_EMPTY */
