/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uisvg.h"
/* platform */
#include "../../detail/trace.h"
#include "../../detail/portable.h"

#define PI 3.14159265358979323846

void psy_ui_svgrect_init(psy_ui_SVGRect* self)
{
	assert(self);

	psy_ui_realrectangle_init(&self->r);
	self->filled = FALSE;
	psy_ui_realsize_init(&self->corner);
	self->use_corner = FALSE;
}

void psy_ui_svgrect_init_all(psy_ui_SVGRect* self,
	psy_ui_RealRectangle r, psy_ui_RealSize corner,
	bool filled)
{
	assert(self);

	self->r = r;
	self->filled = filled;
	self->corner = corner;
	self->use_corner = TRUE;
}

void psy_ui_svgrect_dispose(psy_ui_SVGRect* self)
{
	assert(self);
	
}

psy_ui_SVGRect* psy_ui_svgrect_alloc(void)
{
	return (psy_ui_SVGRect*)malloc(sizeof(psy_ui_SVGRect));
}

psy_ui_SVGRect* psy_ui_svgrect_alloc_init(void)
{
	psy_ui_SVGRect* rv;

	rv = psy_ui_svgrect_alloc();
	if (rv) {
		psy_ui_svgrect_init(rv);
	}
	return rv;
}

void psy_ui_svgrect_copy(psy_ui_SVGRect* self, const psy_ui_SVGRect* other)
{
	assert(self);
	assert(other);

	self->r = other->r;
	self->filled = other->filled;
	self->corner = other->corner;
	self->use_corner = other->use_corner;
}

psy_ui_SVGRect* psy_ui_svgrect_clone(const psy_ui_SVGRect* src)
{
	psy_ui_SVGRect* new_rect;

	new_rect = psy_ui_svgrect_alloc_init();
	if (new_rect) {
		psy_ui_svgrect_copy(new_rect, src);
	}
	else {
		assert(new_rect);
	}
	return new_rect;
}

/* implementation */
void psy_ui_svg_init(psy_ui_SVG* self)
{
	assert(self);
	
	self->paths = NULL;
	self->rects = NULL;
	psy_ui_realsize_init(&self->size);
	psy_ui_realrectangle_init(&self->viewbox);	
}

void psy_ui_svg_init_size(psy_ui_SVG* self, psy_ui_RealSize size)
{
	assert(self);

	psy_ui_svg_init(self);
	self->size = size;
	self->viewbox = psy_ui_realrectangle_make(psy_ui_realpoint_zero(), size);
}

void psy_ui_svg_dispose(psy_ui_SVG* self)
{
	assert(self);
	
	psy_list_deallocate(&self->paths, NULL);
	psy_list_deallocate(&self->rects, NULL);
}

psy_ui_SVG* psy_ui_svg_alloc(void)
{
	return (psy_ui_SVG*)malloc(sizeof(psy_ui_SVG));
}

psy_ui_SVG* psy_ui_svg_alloc_init(void)
{
	psy_ui_SVG* rv;

	rv = psy_ui_svg_alloc();
	if (rv) {
		psy_ui_svg_init(rv);
	}
	return rv;
}

psy_ui_SVG* psy_ui_svg_alloc_init_size(psy_ui_RealSize size)
{
	psy_ui_SVG* rv;

	rv = psy_ui_svg_alloc();
	if (rv) {
		psy_ui_svg_init_size(rv, size);		
	}
	return rv;
}


psy_ui_SVG* psy_ui_svg_alloc_size(psy_ui_RealSize size)
{
	psy_ui_SVG* rv;

	rv = psy_ui_svg_alloc();
	if (rv) {
		psy_ui_svg_init_size(rv, size);
	}
	return rv;
}

psy_ui_SVG* psy_ui_svg_clone(const psy_ui_SVG* src)
{
	psy_ui_SVG* new_svg;

	new_svg = psy_ui_svg_alloc_init();
	if (new_svg) {
		psy_ui_svg_copy(new_svg, src);
	}
	else {
		assert(new_svg);
	}
	return new_svg;
}


void psy_ui_svg_copy(psy_ui_SVG* self, const psy_ui_SVG* other)
{
	const psy_List* p;

	assert(self);	

	psy_list_deallocate(&self->paths, NULL);
	psy_list_deallocate(&self->rects, NULL);
	self->paths = NULL;
	self->rects = NULL;		
	if (!other) {
		return;
	}
	self->size = other->size;
	self->viewbox = other->viewbox;
	for (p = other->rects; p != NULL; p = p->next) {
		const psy_ui_SVGRect* r;		

		r = (const psy_ui_SVGRect*)p->entry;
		assert(r);
		psy_ui_svg_add_rect(self, psy_ui_svgrect_clone(r));
	}
	for (p = other->paths; p != NULL; p = p->next) {		
		const psy_ui_Path* path;

		path = (const psy_ui_Path*)p->entry;
		assert(path);
		psy_ui_svg_add(self, psy_ui_path_clone(path));
	}
}

void psy_ui_svg_add(psy_ui_SVG* self, const psy_ui_Path* path)
{
	psy_ui_Path* clone;

	clone = psy_ui_path_clone(path);
	psy_list_append(&self->paths, clone);
}

void psy_ui_svg_add_rect(psy_ui_SVG* self, const psy_ui_SVGRect* rect)
{
	psy_ui_SVGRect* clone;

	clone = psy_ui_svgrect_clone(rect);
	psy_list_append(&self->rects, clone);
}

void psy_ui_svg_set_viewport(psy_ui_SVG* self, psy_ui_RealSize size)
{
	assert(self);

	self->size = size;	
}

psy_ui_RealSize psy_ui_svg_viewport(const psy_ui_SVG* self)
{
	assert(self);

	return self->size;
}

bool psy_ui_svg_empty(const psy_ui_SVG* self)
{
	assert(self);

	return (!self->paths && !self->rects);
}

/* psy_ui_SVGDraw */

static void psy_ui_svgdraw_update_scale(psy_ui_SVGDraw*,
	psy_ui_RealSize viewport);
static void nsvg__pathArcTo(psy_ui_SVGDraw*, psy_ui_Graphics*,
	double* cpx, double* cpy, double* args, int rel);

void psy_ui_svgdraw_init(psy_ui_SVGDraw* self, const psy_ui_SVG* svg,
	psy_ui_RealSize viewport)
{
	assert(self);

	self->svg = svg;	
	psy_ui_realpoint_init(&self->cp);
	psy_ui_svgdraw_update_scale(self, viewport);
}

void psy_ui_svgdraw_update_scale(psy_ui_SVGDraw* self,
	psy_ui_RealSize viewport)
{
	if (!self->svg) {		
		return;
	}
	self->zoom.x = viewport.width / self->svg->viewbox.right;
	self->zoom.y = viewport.height / self->svg->viewbox.bottom;
}


void psy_ui_svgdraw_draw(psy_ui_SVGDraw* self, psy_ui_Graphics* g, psy_ui_RealPoint pt)
{
	psy_List* p;	

	assert(self);

	if (!self->svg) {
		return;
	}
	for (p = self->svg->rects; p != NULL; p = p->next) {		
		const psy_ui_SVGRect* r;
		psy_ui_RealRectangle scaled;

		r = (const psy_ui_SVGRect*)p->entry;
		assert(r);

		scaled = r->r;
		scaled.left *= self->zoom.x;
		scaled.top *= self->zoom.y;
		scaled.right *= self->zoom.x;
		scaled.bottom *= self->zoom.y;
		psy_ui_realrectangle_move(&scaled, pt);
		if (r->filled) {
			if (r->use_corner) {
				psy_ui_graphics_draw_solid_round_rectangle(g, scaled,
					r->corner, psy_ui_graphics_text_colour(g));
			} else {
				psy_ui_graphics_draw_solid_rectangle(g, scaled,
					psy_ui_graphics_text_colour(g));
			}
		} else {
			if (r->use_corner) {
				psy_ui_graphics_draw_round_rectangle(g, scaled, r->corner);
			} else {
				psy_ui_graphics_draw_rectangle(g, scaled);
			}
		}
	}

	for (p = self->svg->paths; p != NULL; p = p->next) {
		psy_List* q;
		const psy_ui_Path* path;

		path = (const psy_ui_Path*)p->entry;
		if (psy_ui_path_closed(path)) {
			psy_ui_RealPoint* pts;
			uintptr_t numpts;
			uintptr_t i;

			numpts = psy_list_size(path->elements);
			pts = (psy_ui_RealPoint*)malloc(sizeof(psy_ui_RealPoint)* numpts);
			if (pts) {
				i = 0;
				pts[i] = self->cp;
				pts[i].x *= self->zoom.x;
				pts[i].y *= self->zoom.y;
				pts[i].x += pt.x;
				pts[i].y += pt.y;
				for (q = path->elements; q != NULL; q = q->next) {
					const psy_ui_PathCmd* cmd;

					cmd = (const psy_ui_PathCmd*)q->entry;
					switch (cmd->type) {
					case psy_ui_PATHCMD_TYPE_MOVE_TO:
						self->cp = cmd->p1;
						pts[i] = self->cp;
						pts[i].x *= self->zoom.x;
						pts[i].y *= self->zoom.y;
						pts[i].x += pt.x;
						pts[i].y += pt.y;
						break;
					case psy_ui_PATHCMD_TYPE_LINE_TO:
						++i;
						pts[i] = cmd->p1;
						pts[i].x *= self->zoom.x;
						pts[i].y *= self->zoom.y;
						pts[i].x += pt.x;
						pts[i].y += pt.y;
						break;
					default:
						break;
					}
				}
				psy_ui_graphics_draw_solid_polygon(g, pts, i + 1, path->fill,
					path->stroke);
				free(pts);
			}
			pts = NULL;
		} else {
			for (q = path->elements; q != NULL; q = q->next) {
				const psy_ui_PathCmd* cmd;
				psy_ui_RealPoint p;
				psy_ui_RealPoint c1;
				psy_ui_RealPoint c2;				

				cmd = (const psy_ui_PathCmd*)q->entry;
				switch (cmd->type) {
				case psy_ui_PATHCMD_TYPE_MOVE_TO:
					self->cp = cmd->p1;
					self->cp.x *= self->zoom.x;
					self->cp.y *= self->zoom.y;
					self->cp.x += pt.x;
					self->cp.y += pt.y;
					break;
				case psy_ui_PATHCMD_TYPE_LINE_TO:
					p = cmd->p1;
					p.x *= self->zoom.x;
					p.y *= self->zoom.y;
					p.x += pt.x;
					p.y += pt.y;
					psy_ui_drawline(g, self->cp, p);
					self->cp = p;					
					break;
				case psy_ui_PATHCMD_TYPE_CURVE_TO:
					p = cmd->p3;
					p.x *= self->zoom.x;
					p.y *= self->zoom.y;
					p.x += pt.x;
					p.y += pt.y;
					c1 = cmd->p1;
					c1.x *= self->zoom.x;
					c1.y *= self->zoom.y;
					c1.x += pt.x;
					c1.y += pt.y;
					c2 = cmd->p2;
					c2.x *= self->zoom.x;
					c2.y *= self->zoom.y;
					c2.x += pt.x;
					c2.y += pt.y;
					psy_ui_graphics_move_to(g, self->cp);
					psy_ui_graphics_curve_to(g, c1, c2, p);
					self->cp = p;
					break;				
				case psy_ui_PATHCMD_TYPE_CIRCLE: {
					double r;

					p = cmd->p1;
					p.x *= self->zoom.x;
					p.y *= self->zoom.y;
					p.x += pt.x;
					p.y += pt.y;
					r = cmd->p2.x * self->zoom.x;					
					psy_ui_graphics_draw_arc(g,
						psy_ui_realrectangle_make(
							psy_ui_realpoint_make(p.x - r, p.y - r),
							psy_ui_realsize_make(r * 2, r * 2)),
							0, 2 * PI);							
					self->cp = p;
					break; }

				default:
					break;
				}
			}
		}
	}
}

