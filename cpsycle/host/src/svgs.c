/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "svgs.h"
/* ui */
#include <uiapp.h>
#include <uisvg.h>
/* portable */
#include "../../detail/portable.h"

#define NO_FILL FALSE
#define FILL TRUE

/* prototypes */
static void psy_icons_add(psy_ui_Resources*, const char* key, psy_ui_SVG*);
static psy_ui_SVG* psy_alloc_svg_arrow_back(void);
static psy_ui_SVG* psy_alloc_svg_arrow_forward(void);
static psy_ui_SVG* psy_alloc_svg_arrow_up(void);
static psy_ui_SVG* psy_alloc_svg_arrow_down(void);
static psy_ui_SVG* psy_alloc_svg_add(void);
static psy_ui_SVG* psy_alloc_svg_minus(void);
static psy_ui_SVG* psy_alloc_svg_machines(void);
static psy_ui_SVG* psy_alloc_svg_stack(void);
static psy_ui_SVG* psy_alloc_svg_patterns(void);
static psy_ui_SVG* psy_alloc_svg_play(void);
static psy_ui_SVG* psy_alloc_svg_stop(void);
static psy_ui_SVG* psy_alloc_svg_new(void);
static psy_ui_SVG* psy_alloc_svg_folder(void);
static psy_ui_SVG* psy_alloc_svg_visual(void);
static psy_ui_SVG* psy_alloc_svg_keyboard(void);
static psy_ui_SVG* psy_alloc_svg_pulse(void);
static psy_ui_SVG* psy_alloc_svg_repeat(void);
static psy_ui_SVG* psy_alloc_svg_terminal(void);
static psy_ui_SVG* psy_alloc_svg_settings(void);
static psy_ui_SVG* psy_alloc_svg_open(void);
static psy_ui_SVG* psy_alloc_svg_save(void);
static psy_ui_SVG* psy_alloc_svg_power(void);
static psy_ui_SVG* psy_alloc_svg_new_machine(void);
static psy_ui_SVG* psy_alloc_svg_metronome(void);
static void psy_icons_make_modes(psy_ui_Resources* resources);
static void psy_icons_make_notes(psy_ui_Resources* resources);
static void psy_icons_add_note_head(psy_ui_SVG*, bool fill);
static void psy_icons_add_stem(psy_ui_SVG*);
static void psy_icons_add_dot(psy_ui_SVG*);


/* implementation */

void psy_icons_make(psy_ui_Resources* resources)
{
	assert(resources);
	
	psy_icons_add(resources, "img.arrow-back", psy_alloc_svg_arrow_back());
	psy_icons_add(resources, "img.arrow-forward", psy_alloc_svg_arrow_forward());	
	psy_icons_add(resources, "img.arrow-up", psy_alloc_svg_arrow_up());
	psy_icons_add(resources, "img.arrow-down", psy_alloc_svg_arrow_down());
	psy_icons_add(resources, "img.add", psy_alloc_svg_add());
	psy_icons_add(resources, "img.minus", psy_alloc_svg_minus());
	psy_icons_add(resources, "img.machines", psy_alloc_svg_machines());
	psy_icons_add(resources, "img.stack", psy_alloc_svg_stack());
	psy_icons_add(resources, "img.patterns", psy_alloc_svg_patterns());
	psy_icons_add(resources, "img.play", psy_alloc_svg_play());
	psy_icons_add(resources, "img.stop", psy_alloc_svg_stop());
	psy_icons_add(resources, "img.new", psy_alloc_svg_new());
	psy_icons_add(resources, "img.folder", psy_alloc_svg_folder());
	psy_icons_add(resources, "img.visual", psy_alloc_svg_visual());
	psy_icons_add(resources, "img.keyboard", psy_alloc_svg_keyboard());
	psy_icons_add(resources, "img.pulse", psy_alloc_svg_pulse());
	psy_icons_add(resources, "img.repeat", psy_alloc_svg_repeat());
	psy_icons_add(resources, "img.terminal", psy_alloc_svg_terminal());
	psy_icons_add(resources, "img.settings", psy_alloc_svg_settings());
	psy_icons_add(resources, "img.open", psy_alloc_svg_open());
	psy_icons_add(resources, "img.save", psy_alloc_svg_save());
	psy_icons_add(resources, "img.power", psy_alloc_svg_power());
	psy_icons_add(resources, "img.new-machine", psy_alloc_svg_new_machine());
	psy_icons_add(resources, "img.metronome", psy_alloc_svg_metronome());
	psy_icons_make_modes(resources);
	psy_icons_make_notes(resources);
}

void psy_icons_add(psy_ui_Resources* resources, const char* key, psy_ui_SVG* svg)
{
	assert(resources);
	assert(svg);

	psy_ui_resources_add(resources, key, psy_ui_resource_alloc_init_all(svg,
		psy_ui_RESOURCE_TYPE_SVG, (psy_fp_disposefunc)psy_ui_svg_dispose));
}

psy_ui_SVG* psy_alloc_svg_arrow_back(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;
	
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));	
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(244, 400));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(100, 256));	
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(244, 112));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(120, 256));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(412, 256));	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_arrow_forward(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(268, 112));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(412, 256));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(268, 400));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(392, 256));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(100, 256));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_arrow_up(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;	

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(112, 244));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112 + 144, 244 - 144));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112 + 144 + 144, 244 - 144 + 144));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(256, 120));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(256, 120 + 292));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_arrow_down(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(112, 268));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112 + 144, 268 + 144));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112 + 144 + 144, 268 + 144 - 144));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(256, 392));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(256, 100));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_add(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(256, 112));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(256, 112 + 288));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(400, 232));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112, 232));	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_minus(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);	
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(400, 232));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112, 232));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
	return svg;
}



psy_ui_SVG* psy_alloc_svg_machines(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;	

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 100.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(400.0, 100.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(250.0, 300.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_stack(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 100.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(400.0, 100.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 300.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(400.0, 300.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}


psy_ui_SVG* psy_alloc_svg_patterns(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 300.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(300.0, 250.0),
			psy_ui_realsize_make(100.0, 100.0)),
		psy_ui_realsize_make(0.0, 0.0), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(200, 300));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(200, 100));	
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(400, 250));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(400, 50));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(200, 100));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);

	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_play(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(112, 111));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112, 111 + 290));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112 + 200, 111 + 145));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(112, 111));
	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_stop(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(96.0, 96.0),
			psy_ui_realsize_make(320.0, 320.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);	

	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_new(void)
{
	psy_ui_SVG* svg;	
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));	
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path,
		"M 300 50 L 100 50 L 100 450 L 400 450 L 400 150 L 300 50 L 300 50 L 300 150 L 400 150");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_folder(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 200.0),
			psy_ui_realsize_make(350.0, 200.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(80.0, 120.0),
			psy_ui_realsize_make(200.0, 80.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_visual(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(100.0, 100.0),
			psy_ui_realsize_make(300.0, 200.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(150.0, 350.0),
			psy_ui_realsize_make(200.0, 50.0)),
		psy_ui_realsize_make(0.0, 0.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_keyboard(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	int i, j;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	for (i = 0; i < 3; ++i) {
		for (j = 0; j < 2; ++j) {
			psy_ui_svgrect_init_all(&r,
				psy_ui_realrectangle_make(
					psy_ui_realpoint_make(100.0 + (i * 160), 112.0 + (j * 200)),
					psy_ui_realsize_make(100.0, 100.0)),
				psy_ui_realsize_make(0.0, 0.0), FALSE);
			psy_ui_svg_add_rect(svg, &r);
		}
	}
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_pulse(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(48, 320));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64, 320));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64 + 64, 320 - 256));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64 + 64 + 64, 320 - 256 + 384));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64 + 64 + 64 + 64, 320 - 256 + 384 - 224));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64 + 64 + 64 + 64 + 32, 320 - 256 + 384 - 224 + 96));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(48 + 64 + 64 + 64 + 64 + 32 + 64, 320 - 256 + 384 - 224 + 96));	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_repeat(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));	
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(352, 168));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(144, 168));	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(160, 344));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(160 + 208, 344));	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_terminal(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));	
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(32.0, 48.0),
			psy_ui_realsize_make(448.0, 416.0)),
		psy_ui_realsize_make(6.0, 6.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_path_init(&path);	
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(96, 112));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(96 + 80 , 112 + 64));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(96 + 80 - 80, 112 + 64 + 64));
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(192, 240));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(192 + 64, 240));
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_settings(void)
{
	psy_ui_SVG* svg;	
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(32.0, 32.0));	
	psy_ui_path_init(&path);	
	psy_ui_path_parse(&path, "M 13.1875 3 L 13.03125 3.8125 L 12.4375 6.78125 C 11.484375 7.15625 10.625 7.683594 9.84375 8.3125 L 6.9375 7.3125 L 6.15625 7.0625 L 5.75 7.78125 L 3.75 11.21875 L 3.34375 11.9375 L 3.9375 12.46875 L 6.1875 14.4375 C 6.105469 14.949219 6 15.460938 6 16 C 6 16.539063 6.105469 17.050781 6.1875 17.5625 L 3.9375 19.53125 L 3.34375 20.0625 L 3.75 20.78125 L 5.75 24.21875 L 6.15625 24.9375 L 6.9375 24.6875 L 9.84375 23.6875 C 10.625 24.316406 11.484375 24.84375 12.4375 25.21875 L 13.03125 28.1875 L 13.1875 29 L 18.8125 29 L 18.96875 28.1875 L 19.5625 25.21875 C 20.515625 24.84375 21.375 24.316406 22.15625 23.6875 L 25.0625 24.6875 L 25.84375 24.9375 L 26.25 24.21875 L 28.25 20.78125 L 28.65625 20.0625 L 28.0625 19.53125 L 25.8125 17.5625 C 25.894531 17.050781 26 16.539063 26 16 C 26 15.460938 25.894531 14.949219 25.8125 14.4375 L 28.0625 12.46875 L 28.65625 11.9375 L 28.25 11.21875 L 26.25 7.78125 L 25.84375 7.0625 L 25.0625 7.3125 L 22.15625 8.3125 C 21.375 7.683594 20.515625 7.15625 19.5625 6.78125 L 18.96875 3.8125 L 18.8125 3 Z M 14.8125 5 L 17.1875 5 L 17.6875 7.59375 L 17.8125 8.1875 L 18.375 8.375 C 19.511719 8.730469 20.542969 9.332031 21.40625 10.125 L 21.84375 10.53125 L 22.40625 10.34375 L 24.9375 9.46875 L 26.125 11.5 L 24.125 13.28125 L 23.65625 13.65625 L 23.8125 14.25 C 23.941406 14.820313 24 15.402344 24 16 C 24 16.597656 23.941406 17.179688 23.8125 17.75 L 23.6875 18.34375 L 24.125 18.71875 L 26.125 20.5 L 24.9375 22.53125 L 22.40625 21.65625 L 21.84375 21.46875 L 21.40625 21.875 C 20.542969 22.667969 19.511719 23.269531 18.375 23.625 L 17.8125 23.8125 L 17.6875 24.40625 L 17.1875 27 L 14.8125 27 L 14.3125 24.40625 L 14.1875 23.8125 L 13.625 23.625 C 12.488281 23.269531 11.457031 22.667969 10.59375 21.875 L 10.15625 21.46875 L 9.59375 21.65625 L 7.0625 22.53125 L 5.875 20.5 L 7.875 18.71875 L 8.34375 18.34375 L 8.1875 17.75 C 8.058594 17.179688 8 16.597656 8 16 C 8 15.402344 8.058594 14.820313 8.1875 14.25 L 8.34375 13.65625 L 7.875 13.28125 L 5.875 11.5 L 7.0625 9.46875 L 9.59375 10.34375 L 10.15625 10.53125 L 10.59375 10.125 C 11.457031 9.332031 12.488281 8.730469 13.625 8.375 L 14.1875 8.1875 L 14.3125 7.59375 Z "
		"M 16 11 C 13.25 11 11 13.25 11 16 C 11 18.75 13.25 21 16 21 C 18.75 21 21 18.75 21 16 C 21 13.25 18.75 11 16 11 Z "
		"M 16 13 C 17.667969 13 19 14.332031 19 16 C 19 17.667969 17.667969 19 16 19 C 14.332031 19 13 17.667969 13 16 C 13 14.332031 14.332031 13 16 13"
	);
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;
}

psy_ui_SVG* psy_alloc_svg_open(void)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(32.0, 48.0),
			psy_ui_realsize_make(448.0, 416.0)),
		psy_ui_realsize_make(6.0, 6.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r,
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(102.0, 128.0),
			psy_ui_realsize_make(380.0, 380.0)),
		psy_ui_realsize_make(6.0, 6.0), FALSE);
	psy_ui_svg_add_rect(svg, &r);		
	svg->size = psy_ui_realsize_make(20.0, 20.0);
	return svg;
}

psy_ui_SVG* psy_alloc_svg_save(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));	
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path, "M 350 50 L 50 50 L 50 450 L 450 450 L 450 150 L 350 50 M 100 100 L 300 100 L 300 170 L 100 170 L 100 100 ");
	psy_ui_path_circle(&path, psy_ui_realpoint_make(250.0, 300.0), 50.0);
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;	
}

psy_ui_SVG* psy_alloc_svg_power(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(300.0, 300.0));
	psy_ui_path_init(&path);
	psy_ui_path_parse(&path, "M 244.802 61.643 C 234.168 50.875 224.661 42.297 210.837 35.523 C 202.151 31.261 191.669 34.934 187.47 43.717 C 183.243 52.501 186.889 63.072 195.566 67.334 C 205.86 72.375 212.776 77.95 220.72 85.992 C 259.717 125.427 259.717 189.586 220.72 229.011 C 201.83 248.125 176.694 258.624 149.984 258.624 C 123.266 258.624 98.138 248.116 79.247 229.011 C 40.251 189.586 40.251 125.427 79.247 85.992 C 87.218 77.941 95.099 72.384 104.885 67.352 C 113.15 63.081 116.608 52.519 112.597 43.726 C 108.584 34.952 99.952 31.341 91.482 35.487 C 78.051 42.082 65.844 50.875 55.184 61.643 C 2.901 114.498 2.901 200.488 55.184 253.352 C 81.33 279.775 115.662 293 149.994 293 C 184.334 293 218.666 279.784 244.803 253.352 C 297.104 200.506 297.104 114.507 244.802 61.643 Z  M 149.984 174 C 159.849 174 167.855 165.993 167.855 156.129 L 167.882 24.871 C 167.882 15.007 159.876 7 150.011 7 C 140.145 7 132.139 15.007 132.139 24.871 L 132.139 78.486 L 132.112 156.128 C 132.112 166.002 140.118 174 149.984 174");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;
}

psy_ui_SVG* psy_alloc_svg_new_machine(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_circle(&path, psy_ui_realpoint_make(256.0, 256.0), 220.0);	
	psy_ui_path_parse(&path, "M 256 250 L 100 60 M 256 250 L 100 420 M 256 250 L 450 250");	
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;
}

psy_ui_SVG* psy_alloc_svg_metronome(void)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);
	psy_ui_path_circle(&path, psy_ui_realpoint_make(250.0,350.0), 30.0);
	psy_ui_path_parse(&path, "M100 400 L250 100 L410 400 L 100 400 M250 320 L 250 150");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));
	return svg;
}

void psy_icons_make_modes(psy_ui_Resources* resources)
{
	psy_ui_SVG* svg;
	psy_ui_Path path;

	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(512.0, 512.0));
	psy_ui_path_init(&path);	
	psy_ui_path_parse(&path, "M 256 100 L 256 500 M 200 100 L 350 100 M 200 500 L 350 500");
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_svg_set_viewport(svg, psy_ui_realsize_make(20.0, 20.0));	
	psy_icons_add(resources, "img.select", svg);
}

void psy_icons_make_notes(psy_ui_Resources* resources)
{
	psy_ui_SVG* svg;
	psy_ui_SVGRect r;
	psy_ui_Path path;

	/* psy_ui_ICON_ENDLESS */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0, 25), psy_ui_realsize_make(7, 10)),
		psy_ui_realsize_zero(), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(8, 25), psy_ui_realsize_make(7, 10)),
		psy_ui_realsize_zero(), TRUE);
	psy_ui_svg_add_rect(svg, &r);
	psy_icons_add(resources, "img.endless", svg);

	/* psy_ui_ICON_SEMIBREVE */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_icons_add_note_head(svg, NO_FILL);
	psy_icons_add(resources, "img.semi-breve", svg);
	/* psy_ui_ICON_MINIM */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_icons_add_note_head(svg, NO_FILL);
	psy_icons_add_stem(svg);
	psy_icons_add(resources, "img.minim", svg);
	/* psy_ui_ICON_MINIM_DOT */
	svg = psy_ui_svg_clone(svg);
	psy_icons_add_dot(svg);
	psy_icons_add(resources, "img.minim-dot", svg);
	/* psy_ui_ICON_CROTCHET */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_icons_add_note_head(svg, FILL);
	psy_icons_add_stem(svg);
	psy_icons_add(resources, "img.crotchet", svg);
	/* psy_ui_ICON_CROTCHET_DOT */
	svg = psy_ui_svg_clone(svg);
	psy_icons_add_dot(svg);
	psy_icons_add(resources, "img.crotchet-dot", svg);
	/* psy_ui_ICON_QUAVER */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_icons_add_note_head(svg, FILL);
	psy_icons_add_stem(svg);
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(10, 0));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(15, 10));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(15, 15));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(14, 15));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(14, 10));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 1));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 0));
	psy_ui_path_close(&path);
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_icons_add(resources, "img.quaver", svg);
	/* psy_ui_ICON_QUAVER_DOT */
	svg = psy_ui_svg_clone(svg);
	psy_icons_add_dot(svg);
	psy_icons_add(resources, "img.quaver-dot", svg);
	/* psy_ui_ICON_SEMIQUAVER */
	svg = psy_ui_svg_alloc_init_size(psy_ui_realsize_make(16.0, 36.0));
	psy_icons_add_note_head(svg, FILL);
	psy_icons_add_stem(svg);
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(10, 0));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(15, 10));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(15, 30));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(14, 30));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(14, 10));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 2));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 0));
	psy_ui_path_close(&path);
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(10, 10));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(15, 20));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(13, 20));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 7));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(10, 5));
	psy_ui_path_close(&path);
	psy_ui_svg_add(svg, &path);
	psy_ui_path_dispose(&path);
	psy_icons_add(resources, "img.semi-quaver", svg);
}

void psy_icons_add_note_head(psy_ui_SVG* svg, bool fill)
{
	psy_ui_SVGRect r;

	assert(svg);

	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(0, 25), psy_ui_realsize_make(10, 10)),
		psy_ui_realsize_zero(), fill);
	psy_ui_svg_add_rect(svg, &r);
}

void psy_icons_add_dot(psy_ui_SVG* svg)
{
	psy_ui_SVGRect r;

	assert(svg);

	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(13, 32), psy_ui_realsize_make(2, 2)),
		psy_ui_realsize_zero(), FILL);
	psy_ui_svg_add_rect(svg, &r);
}

void psy_icons_add_stem(psy_ui_SVG* svg)
{
	psy_ui_SVGRect r;

	assert(svg);

	psy_ui_svgrect_init_all(&r, psy_ui_realrectangle_make(
		psy_ui_realpoint_make(9, 0), psy_ui_realsize_make(1, 30)),
		psy_ui_realsize_zero(), FILL);
	psy_ui_svg_add_rect(svg, &r);
}

