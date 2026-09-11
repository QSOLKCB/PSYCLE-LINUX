/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiicons.h"
#include "uiapp.h"
#include "uisvg.h"

/* platform */
#include "../../detail/trace.h"
#include "../../detail/portable.h"

#define NO_FILL FALSE
#define FILL TRUE

/* prototypes */
static void psy_ui_icons_add(const char* key, psy_ui_SVG*);
static psy_ui_SVG* psy_alloc_svg_switch_on(void);
static psy_ui_SVG* psy_alloc_svg_switch_off(void);
static psy_ui_SVG* psy_alloc_svg_check_on(void);
static psy_ui_SVG* psy_alloc_svg_check_off(void);
static psy_ui_SVG* psy_alloc_svg_search(void);
static psy_ui_SVG* psy_alloc_svg_undo(void);
static psy_ui_SVG* psy_alloc_svg_redo(void);
static psy_ui_SVG* psy_alloc_svg_expand(void);
static psy_ui_SVG* psy_alloc_svg_float(void);


/* implementation */
void psy_ui_icons_add(const char* key, psy_ui_SVG* svg)
{
	assert(svg);

	psy_ui_resources_add(&psy_ui_app()->resources, key, psy_ui_resource_alloc_init_all(svg,
		psy_ui_RESOURCE_TYPE_SVG, (psy_fp_disposefunc)psy_ui_svg_dispose));
}

void psy_ui_icons_make(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;
	psy_ui_SVGRect r;
	double dx;
	double dy;
	double sz;

	dy = 6;
	dx = 6;
	sz = 20;
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(sz, sz));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(0.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(8.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(4.0 + dx, 0.0 + dy));
	psy_ui_path_close(&path);
	psy_ui_path_set_fill(&path, psy_ui_colour_white());
	psy_ui_path_set_stroke(&path, psy_ui_colour_white());
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_icons_add("img.icon-up", svg);

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(sz, sz));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(0.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(8.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(4.0 + dx, 8.0 + dy));
	psy_ui_path_close(&path);
	psy_ui_path_set_fill(&path, psy_ui_colour_white());
	psy_ui_path_set_stroke(&path, psy_ui_colour_white());
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_icons_add("img.icon-down", svg);

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(sz, sz));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(0.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(8.0 + dx, 0.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(8.0 + dx, 8.0 + dy));
	psy_ui_path_close(&path);
	psy_ui_path_set_fill(&path, psy_ui_colour_white());
	psy_ui_path_set_stroke(&path, psy_ui_colour_white());
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_icons_add("img.icon-less", svg);

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(sz, sz));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(0.0 + dx, 0.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(8.0 + dx, 4.0 + dy));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(0.0 + dx, 8.0 + dy));
	psy_ui_path_close(&path);
	psy_ui_path_set_fill(&path, psy_ui_colour_white());
	psy_ui_path_set_stroke(&path, psy_ui_colour_white());
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_icons_add("img.icon-more", svg);

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(20.0, 20.0));	
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(psy_ui_realpoint_zero(),
			psy_ui_realsize_make(5.0, 5.0)),
		psy_ui_realsize_make(0.0, 0.0), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_icons_add("img.icon-close", svg);
	
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(24.0, 24.0));
	psy_ui_path_init(&path);		
	psy_ui_path_parse(&path, "M21 15 L15 21 M21 8 L8 21");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);	
	psy_ui_icons_add("img.icon-grip", svg);

	psy_ui_icons_add("img.switch-on", psy_alloc_svg_switch_on());
	psy_ui_icons_add("img.switch-off", psy_alloc_svg_switch_off());
	psy_ui_icons_add("img.check-on", psy_alloc_svg_check_on());
	psy_ui_icons_add("img.check-off", psy_alloc_svg_check_off());
	psy_ui_icons_add("img.search", psy_alloc_svg_search());
	psy_ui_icons_add("img.undo", psy_alloc_svg_undo());
	psy_ui_icons_add("img.redo", psy_alloc_svg_redo());
	psy_ui_icons_add("img.expand", psy_alloc_svg_expand());
	psy_ui_icons_add("img.float", psy_alloc_svg_float());
}

psy_ui_SVG* psy_alloc_svg_switch_on(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	psy_ui_RealSize knob_size;
	psy_ui_RealSize corner;
	psy_ui_RealPoint ident;
	double switch_width;
	psy_ui_RealRectangle viewbox;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(20.0, 10.0));
	knob_size = psy_ui_realsize_make(20.0, 10.0);
	viewbox = psy_ui_realrectangle_make(psy_ui_realpoint_zero(), knob_size);
	switch_width = 8.0;
	corner = psy_ui_realsize_make(6.0, 6.0);
	ident = psy_ui_realpoint_make(3.0, 2.0);
	svg->size = knob_size;
	svg->viewbox = viewbox;
	psy_ui_svgrect_init_all(&r, viewbox, corner, FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(knob_size.width - switch_width - ident.x, ident.y),
		psy_ui_realsize_make(switch_width, knob_size.height - ident.y * 2)),
		corner, TRUE);
	psy_ui_svg_add_rect(svg, &r);
	return svg;
}
psy_ui_SVG* psy_alloc_svg_switch_off(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	psy_ui_RealSize knob_size;
	psy_ui_RealSize corner;
	psy_ui_RealPoint ident;
	double switch_width;
	psy_ui_RealRectangle viewbox;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(20.0, 10.0));
	knob_size = psy_ui_realsize_make(20.0, 10.0);
	viewbox = psy_ui_realrectangle_make(psy_ui_realpoint_zero(), knob_size);
	switch_width = 8.0;
	corner = psy_ui_realsize_make(6.0, 6.0);
	ident = psy_ui_realpoint_make(3.0, 2.0);
	svg->size = knob_size;
	svg->viewbox = viewbox;
	psy_ui_svgrect_init_all(&r, viewbox, corner, FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(ident,
			psy_ui_realsize_make(switch_width, knob_size.height - ident.y * 2)),
		corner, TRUE);
	psy_ui_svg_add_rect(svg, &r);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_check_off(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(200.0, 100.0),
			psy_ui_realsize_make(300.0, 400.0)),
		psy_ui_realsize_make(4.0, 4.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_check_on(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(200.0, 100.0),
			psy_ui_realsize_make(300.0, 400.0)),
		psy_ui_realsize_make(4.0, 4.0), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_search(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(32.0, 32.0));
	psy_ui_path_init(&path);
	psy_ui_path_circle(&path, psy_ui_realpoint_make(16.0, 16.0), 6);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(20.0, 20.0));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(25.0, 25.0));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;
}

psy_ui_SVG* psy_alloc_svg_undo(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path, "M240 424L240 328M240 328C356.4 328 399.39 361.76 448 424"
		"C448 304.77 408.43 184 240 184L240 88L64 256 L240 424");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_redo(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path, "M448 256L272 88L272 184M272 184C103.57 184 64 304.77 64 424"
		"C112.61 361.76 155.6 328 272 328L272 424L448 256");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_expand(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 200.0),
			psy_ui_realsize_make(250.0, 250.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(250.0, 100.0),
			psy_ui_realsize_make(200.0, 200.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_float(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path, "M100 200L100 100L200 100"
							 "M300 100L400 100L400 200"
							 "M400 300L400 400L300 400"
							 "M200 400L100 400L100 300");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}
