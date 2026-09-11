/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_SVG_H
#define psy_ui_SVG_H

/* local */
#include "uigraphics.h"
#include "uipath.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_ui_SVGRect
*/

typedef struct psy_ui_SVGRect {
	psy_ui_RealRectangle r;
	bool filled;
	psy_ui_RealSize corner;
	bool use_corner;
} psy_ui_SVGRect;


void psy_ui_svgrect_init(psy_ui_SVGRect*);
void psy_ui_svgrect_init_all(psy_ui_SVGRect*,
	psy_ui_RealRectangle, psy_ui_RealSize corner,
	bool filled);
void psy_ui_svgrect_dispose(psy_ui_SVGRect*);

psy_ui_SVGRect* psy_ui_svgrect_alloc(void);
psy_ui_SVGRect* psy_ui_svgrect_alloc_init(void);

void psy_ui_svgrect_copy(psy_ui_SVGRect*, const psy_ui_SVGRect* other);
psy_ui_SVGRect* psy_ui_svgrect_clone(const psy_ui_SVGRect* src);

/*!
** @struct psy_ui_SVG
*/

typedef struct psy_ui_SVG {
	psy_List* paths;
	psy_List* rects;
	psy_ui_RealPoint cp;
	psy_ui_RealSize size;
	psy_ui_RealRectangle viewbox;	
} psy_ui_SVG;

void psy_ui_svg_init(psy_ui_SVG*);
void psy_ui_svg_init_size(psy_ui_SVG*, psy_ui_RealSize);
void psy_ui_svg_dispose(psy_ui_SVG*);

psy_ui_SVG* psy_ui_svg_alloc(void);
psy_ui_SVG* psy_ui_svg_alloc_init(void);
psy_ui_SVG* psy_ui_svg_alloc_init_size(psy_ui_RealSize size);

void psy_ui_svg_copy(psy_ui_SVG*, const psy_ui_SVG* other);
psy_ui_SVG* psy_ui_svg_clone(const psy_ui_SVG* src);
void psy_ui_svg_add(psy_ui_SVG*, const psy_ui_Path*);
void psy_ui_svg_add_rect(psy_ui_SVG*, const psy_ui_SVGRect*);

void psy_ui_svg_set_viewport(psy_ui_SVG*, psy_ui_RealSize);
psy_ui_RealSize psy_ui_svg_viewport(const psy_ui_SVG*);
bool psy_ui_svg_empty(const psy_ui_SVG*);

typedef struct psy_ui_SVGDraw {
	psy_ui_RealPoint cp;
	const psy_ui_SVG* svg;
	psy_ui_RealPoint zoom;
} psy_ui_SVGDraw;


void psy_ui_svgdraw_init(psy_ui_SVGDraw*, const psy_ui_SVG*,
	psy_ui_RealSize viewport);

void psy_ui_svgdraw_draw(psy_ui_SVGDraw*, psy_ui_Graphics*, psy_ui_RealPoint);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_PATH_H */
