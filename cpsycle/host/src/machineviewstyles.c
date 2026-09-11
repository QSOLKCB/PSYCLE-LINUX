/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineviewstyles.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
#include "skinio.h"
/* ui */
#include <uiapp.h>
/* file */
#include <dir.h>
/* platform */
#include "../../detail/os.h"
#include "../../detail/portable.h"


#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif


/* MachineViewStyles */

/* prototypes */
static void machineviewstyles_set_coords(psy_Property* p);
static void machineviewstyles_set_dest(psy_ui_RealPoint* pt,
	intptr_t vals[4], uintptr_t num);
static void machineviewstyles_set_source(psy_ui_RealRectangle* r,
	intptr_t vals[4]);
	
/* implementation */
void machineviewstyles_init(MachineViewStyles* self, psy_Configuration* dir_cfg)
{
	assert(self);
	assert(dir_cfg);
	
	self->dir_cfg = dir_cfg;	
	machineviewstyles_make_machine_default_em(self);
	machineviewstyles_make_view_default(self);
}

void machineviewstyles_make_view_default(MachineViewStyles* self)
{
	psy_ui_Styles* styles;
	psy_ui_Style* style;
	psy_ui_Style* gen_style;
	
	assert(self);		
		
	styles = &psy_ui_app_defaults()->styles;		

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00232323));
	psy_ui_styles_set_style(styles, STYLE_MV_WIRES, "mv_wires", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00232323));
	gen_style = psy_ui_style(STYLE_MV_GENERATOR);
	psy_ui_style_set_font_info(style, psy_ui_font_font_info(&gen_style->font));
	psy_ui_styles_set_style(styles, STYLE_MV_STACK, "mv_stack", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x005F5F5F));
	psy_ui_styles_set_style(styles, STYLE_MV_WIRE, "mv_wire", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x009F9F9F));
	psy_ui_styles_set_style(styles, STYLE_MV_WIRE_SELECT,
		"mv_wire::select", style);
	
	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x005F5F5F));
	psy_ui_style_set_background_size_px(psy_ui_style(STYLE_MV_WIRE_POLY),
		psy_ui_realsize_make(5, 5));
	psy_ui_styles_set_style(styles, STYLE_MV_WIRE_POLY, "mv_wire_poly", style);
}

void machineviewstyles_make_machine_default_em(MachineViewStyles* self)
{
	psy_ui_Styles* styles;
	psy_ui_Style* style;		
	
	assert(self);
		
	styles = &psy_ui_app_defaults()->styles;	
				
	/* master */
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");	
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00B1C8B0));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00333333));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_style_set_size(style, psy_ui_size_make_em(23.0, 2.333));	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(23.0, 2.333));
	psy_ui_styles_set_style(styles, STYLE_MV_MASTER, "mv_master", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_unset_display(style);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xBABABA));
	psy_ui_styles_set_style(styles, STYLE_MV_MASTER_NAME,
		"mv_master_name", style);
		
	/* generator */
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00B1C8B0));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x002f3E25));	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00475E38));
	psy_ui_style_set_size(style, psy_ui_size_make_em(23.0, 3.5));	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(23.0, 3.5));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR,
		"mv_generator", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_position(style,
		psy_ui_rectangle_make(
			psy_ui_point_make_em(1.0, 0.2),
			psy_ui_size_make_em(17.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_NAME,
		"mv_generator_name", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00475E38));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(20.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE,
		"mv_generator_mute", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_unset_display(style);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE_LABEL,
		"mv_generator_mute_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00000000));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DD2FF));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x004DD2FF));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(20.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE_SELECT,
		"mv_generator_mute::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00475E38));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO,
		"mv_generator_solo", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_unset_display(style);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO_LABEL,
		"mv_generator_solo_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00000000));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DFFA6));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x004DFFA6));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO_SELECT,
		"mv_generator_solo::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00475E38));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, 1.333), psy_ui_size_make_em(21.0, 0.5)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VU,
		"mv_generator_vu", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DFFA6));
	psy_ui_style_set_background_size(style,
		psy_ui_size_make_em(20.0, 0.5));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VU0,
		"mv_generator_vu0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_size(style,
		psy_ui_size_make(psy_ui_value_make_px(1.0), 
			psy_ui_value_make_eh(0.5)));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DD2FF));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VUPEAK,
		"mv_generator_vupeak", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, 2.2), psy_ui_size_make_em(14.0, 1.0)));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00475E38));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_PAN,
		"mv_generator_pan", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(1.0, 1.0));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(0.0, 0.0), psy_ui_size_make_em(1.0, 1.0)));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00475E38));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_PAN_SLIDER,	
		"mv_generator_pan_slider", style);

	/* effect */
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00D1C5B6));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x003E2f25));	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x005D4738));
	psy_ui_style_set_size(style, psy_ui_size_make_em(23.0, 3.5));	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(23.0, 3.5));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT,
		"mv_effect", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, 0.2), psy_ui_size_make_em(17.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_NAME,
		"mv_effect_name", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x005D4738));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(20.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE,
		"mv_effect_mute", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_unset_display(style);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE_LABEL,
		"mv_effect_mute_label", style);

	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00000000));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DD2FF));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x004DD2FF));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(20.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE_SELECT,
		"mv_effect_mute::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_colour(style,
		psy_ui_colour_transparent());
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x005D4738));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS,
		"mv_effect_bypass", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_unset_display(style);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS_LABEL,
		"mv_effect_bypass_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00000000));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00FFA64D));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00FFA64D));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.0, 2.2), psy_ui_size_make_em(3.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS_SELECT,
		"mv_effect_bypass::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x005D4738));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, 1.333), psy_ui_size_make_em(21.0, 0.5)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VU,
		"mv_effect_vu", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DFFA6));
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(20.0, 0.5));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VU0,
		"mv_effect_vu0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_size(style,
		psy_ui_size_make(psy_ui_value_make_px(1.0), 
			psy_ui_value_make_eh(0.5)));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x004DD2FF));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VUPEAK,
		"mv_effect_vupeak", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, 2.2), psy_ui_size_make_em(14.0, 1.0)));		
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x005D4738));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_PAN,
		"mv_effect_pan", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_size(style, 
		psy_ui_size_make_em(1.0, 1.0));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(0.0, 0.0), psy_ui_size_make_em(1.0, 1.0)));	
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x005D4738));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_PAN_SLIDER,
		"mv_effect_pan_slider", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_size(style, psy_ui_size_make_em(18.0, 4.0));
	psy_ui_styles_set_style(styles, STYLE_MV_LEVEL,
		"mv_level", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_size(style, psy_ui_size_make_em(2.0, 9.0));
	psy_ui_styles_set_style(styles, STYLE_MV_ARROW,
		"mv_arrow", style);
				
	psy_ui_styles_set_style(styles, STYLE_MV_CHECK,
		"mv_check", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_KNOB,
		"mv_knob", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_LABEL,
		"mv_label", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_HEADER,
		"mv_header", psy_ui_style_alloc_init());	
}

void machineviewstyles_make_machine_default(MachineViewStyles* self)
{
	psy_ui_Styles* styles;
	psy_ui_Style* style;	
	
	assert(self);
		
	styles = &psy_ui_app_defaults()->styles;	
		
	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_size(style, psy_ui_size_make_px(138.0, 35.0));	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(
		psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_position_px(style, 0.0, -52.0);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(138.0, 35.0));
	psy_ui_styles_set_style(styles, STYLE_MV_MASTER,
		"mv_master", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_MV_MASTER_NAME,
		"mv_master_name", style);	

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_style_set_size(style, psy_ui_size_make_px(138.0, 52.0));
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(
		psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_position_px(style, 0.0, -87.0);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(138.0, 52.0));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR,
		"mv_generator", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style,
		psy_ui_rectangle_make(
			psy_ui_point_make_px(20.0, 3.0),
			psy_ui_size_make_px(117.0, 15.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_NAME,
		"mv_generator_name", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(117.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE,
		"mv_generator_mute", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");	
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_position_px(style, -23.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(117.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE_SELECT,
		"mv_generator_mute::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_MUTE_LABEL,
		"", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(98.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO,
		"mv_generator_solo", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17, 17));
	psy_ui_style_set_background_position_px(style, -6.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(98.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO_SELECT,
		"mv_generator_solo::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_SOLO_LABEL,
		"mv_generator_solo_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(4.0, 20.0), psy_ui_size_make_px(129.0, 7.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VU,
		"mv_generator_vu", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(129.0, 7));
	psy_ui_style_set_background_position_px(style, 0.0, -156.0);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VU0,
		"mv_generator_vu0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(1, 7));
	psy_ui_style_set_background_position_px(style, -108.0, -156.0);
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_VUPEAK,
		"mv_generator_vupeak", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(6.0, 33.0),
		psy_ui_size_make_px(82.0 + 6.0, 13.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_PAN,
		"mv_generator_pan", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style, 
		psy_ui_realsize_make(6.0, 13.0));
	psy_ui_style_set_background_position_px(style, -0.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(0.0, 0.0), psy_ui_size_make_px(6.0, 13.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_GENERATOR_PAN_SLIDER,
		"mv_generator_pan_slider", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_font_string(style, PSYCLE_DEFAULT_MACHINE_FONT);
	psy_ui_style_set_size(style, psy_ui_size_make_px(138.0, 52.0));
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_position_px(style, 0.0, 0.0);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(138.0, 52.0));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT,
		"mv_effect", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(20.0, 3.0), psy_ui_size_make_px(117.0, 15.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_NAME,
		"mv_effect_name", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(117.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE,
		"mv_effect_mute", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_position_px(style, -23.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(117.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE_SELECT,
		"mv_effect_mute::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_MUTE_LABEL,
		"mv_effect_mute_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_colour(style,
		psy_ui_colour_transparent());
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(98.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS,
		"mv_effect_bypass", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17, 17));
	psy_ui_style_set_background_position_px(style, -6.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(98.0, 31.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS_SELECT,
		"style_mv_effect_bypass::select", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_BYPASS_LABEL,
		"mv_effect_bypass_label", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(4.0, 20.0), psy_ui_size_make_px(129.0, 7.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VU,
		"mv_effect_vu", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");	
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(129.0, 7.0));
	psy_ui_style_set_background_position_px(style, 0.0, -163.0);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VU0,
		"mv_effect_vu0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");	
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(1.0, 7.0));
	psy_ui_style_set_background_position_px(style, -96.0, -144.0);
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_VUPEAK,
		"mv_effect_vupeak", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(6.0, 33.0),
		psy_ui_size_make_px(82.0 + 3.0, 13.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_PAN,
		"mv_effect_pan", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_setbackgroundpath(style, "");
	psy_ui_style_set_background_id(style, IDB_MACHINESKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(6.0, 13.0));
	psy_ui_style_set_background_position_px(style, -0.0, -139.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(0.0, 0.0), psy_ui_size_make_px(6.0, 13.0)));
	psy_ui_styles_set_style(styles, STYLE_MV_EFFECT_PAN_SLIDER,
		"mv_effect_pan_slider", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_size(style, psy_ui_size_make_px(138.0, 52.0));
	psy_ui_styles_set_style(styles, STYLE_MV_LEVEL,
		"mv_level", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_size(style, psy_ui_size_make_px(16.0, 90.0));	
	psy_ui_styles_set_style(styles, STYLE_MV_ARROW,
		"mv_arrow", style);
							
	psy_ui_styles_set_style(styles, STYLE_MV_CHECK,
		"mv_check", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_KNOB,
		"mv_knob", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_LABEL,
		"mv_label", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_MV_HEADER,
		"mv_header", psy_ui_style_alloc_init());	
}

void machineviewstyles_load(MachineViewStyles* self, const char* skin_name)
{
	const char* machine_skin_name;	
	static int styles[] = {
		STYLE_MV_MASTER,		
		STYLE_MV_GENERATOR,		
		STYLE_MV_GENERATOR_MUTE_SELECT,		
		STYLE_MV_GENERATOR_SOLO_SELECT,		
		STYLE_MV_GENERATOR_VU0,
		STYLE_MV_GENERATOR_VUPEAK,		
		STYLE_MV_GENERATOR_PAN_SLIDER,
		STYLE_MV_EFFECT,		
		STYLE_MV_EFFECT_MUTE_SELECT,		
		STYLE_MV_EFFECT_BYPASS_SELECT,		
		STYLE_MV_EFFECT_VU0,
		STYLE_MV_EFFECT_VUPEAK,		
		STYLE_MV_EFFECT_PAN_SLIDER,
		0 };	
		
	machine_skin_name = skin_name;
	if (psy_strlen(machine_skin_name) > 0) {		
		if (strcmp(machine_skin_name, "default-text") == 0) {
			machineviewstyles_make_machine_default_em(self);			
		} else {
			const char* skindir;		

			skindir = psy_configuration_value_str(self->dir_cfg,
				"skins", PSYCLE_SKINS_DEFAULT_DIR);						
			if (skindir) {
				psy_Path filename;
				char path[_MAX_PATH];			
				
				/* search bmp */
				psy_path_init_all(&filename, "", machine_skin_name, "bmp");
				psy_dir_find_file(skindir, psy_path_full(&filename), path);
				if (path[0] != '\0') {				
					int i;						

					for (i = 0; styles[i] != 0; ++i) {
						psy_ui_Style* style;
						
						style = psy_ui_style(styles[i]);
						psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
						if (psy_ui_style_setbackgroundpath(style, path) !=
								PSY_OK) {
							psy_ui_style_set_background_id(style,
								IDB_MACHINESKIN);
						}
						psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
					}				
				}				
				psy_ui_style_set_display(psy_ui_style(STYLE_MV_MASTER_NAME),
					psy_ui_DISPLAY_NONE);
				psy_ui_style_set_display(psy_ui_style(
					STYLE_MV_EFFECT_MUTE_LABEL), psy_ui_DISPLAY_NONE);
				psy_ui_style_set_display(psy_ui_style(
					STYLE_MV_EFFECT_BYPASS_LABEL), psy_ui_DISPLAY_NONE);
				psy_ui_style_set_display(psy_ui_style(
					STYLE_MV_GENERATOR_MUTE_LABEL), psy_ui_DISPLAY_NONE);
				psy_ui_style_set_display(psy_ui_style(
					STYLE_MV_GENERATOR_SOLO_LABEL), psy_ui_DISPLAY_NONE);
				psy_path_set_ext(&filename, "psm");
				psy_dir_find_file(skindir, psy_path_full(&filename), path);
				psy_path_dispose(&filename);
				if (psy_strlen(path) > 0) {
					psy_Property coords;
					psy_Property* transparency;

					psy_property_init(&coords);
					if (skin_load_machine(&coords, path) == PSY_OK) {
						machineviewstyles_set_coords(&coords);
						
						transparency = psy_property_at(&coords,
							"transparency", PSY_PROPERTY_TYPE_NONE);
						if (transparency && transparency->item.marked) {
							psy_ui_Colour cltransparency;
							int i;

							cltransparency = psy_ui_colour_make(
								strtol(psy_property_item_str(transparency),	
									0, 16));
							for (i = 0; styles[i] != 0; ++i) {
								psy_ui_Style* style;
								
								style = psy_ui_style(styles[i]);
								psy_ui_bitmap_set_transparency(
									&style->background.bitmap,
									cltransparency);
							}
						}
					}
					psy_property_dispose(&coords);
				}
			}			
		}
	} else {
		machineviewstyles_make_machine_default(self);		
	}
}

void machineviewstyles_set_coords(psy_Property* p)
{
	const char* s;
	intptr_t vals[4];
	psy_ui_Style* style;
	psy_ui_RealRectangle srcmac;
	psy_ui_RealRectangle src;
	psy_ui_RealPoint dst;

	src = srcmac = psy_ui_realrectangle_zero();
	dst = psy_ui_realpoint_zero();
	/* master */	
	if ((s = psy_property_at_str(p, "master_source", 0))) {
		style = psy_ui_style(STYLE_MV_MASTER);
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
		psy_ui_style_set_size(style, psy_ui_size_make_px(src.right - src.left,
			src.bottom - src.top));
		psy_ui_style_set_background_size_px(style, 
			psy_ui_realrectangle_size(&src));
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
	}
	/* generator */
	if ((s = psy_property_at_str(p, "generator_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&srcmac, vals);
		style = psy_ui_style(STYLE_MV_GENERATOR);
		psy_ui_style_set_size(style, psy_ui_size_make_px(
			srcmac.right - srcmac.left,
			srcmac.bottom - srcmac.top));
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&srcmac));
		psy_ui_style_set_background_position_px(style,
			-srcmac.left, -srcmac.top);
	}
	if ((s = psy_property_at_str(p, "generator_mute_source", 0))) {
		skin_psh_values(s, 4, vals);		
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "generator_mute_dest", 0))) {
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		style = psy_ui_style(STYLE_MV_GENERATOR_MUTE);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_GENERATOR_MUTE_SELECT);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "generator_solo_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "generator_solo_dest", 0))) {
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		style = psy_ui_style(STYLE_MV_GENERATOR_SOLO);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_GENERATOR_SOLO_SELECT);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "generator_name_dest", 0))) {
		style = psy_ui_style(STYLE_MV_GENERATOR_NAME);
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(srcmac.right - dst.x, 15.0)));
	}
	if ((s = psy_property_at_str(p, "generator_pan_source", 0))) {
		skin_psh_values(s, 4, vals);
		style = psy_ui_style(STYLE_MV_GENERATOR_PAN);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "generator_pan_dest", 0))) {
		skin_psh_values(s, 3, vals);		
		machineviewstyles_set_dest(&dst, vals, 3);
		style = psy_ui_style(STYLE_MV_GENERATOR_PAN);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px((double)vals[2] + src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_GENERATOR_PAN_SLIDER);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&src));			
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(0.0, 0.0), psy_ui_size_make_px(
				src.right - src.left,
				src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "generator_vu0_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "generator_vu_dest", 0))) {
		skin_psh_values(s, 3, vals);
		machineviewstyles_set_dest(&dst, vals, 3);
		style = psy_ui_style(STYLE_MV_GENERATOR_VU);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px((double)vals[2], src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_GENERATOR_VU0);				
		psy_ui_style_set_background_size_px(style, 
			psy_ui_realsize_make((double)vals[2], src.bottom - src.top));
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);		
	}
	if ((s = psy_property_at_str(p, "generator_vu_peak_source", 0))) {
		style = psy_ui_style(STYLE_MV_GENERATOR_VUPEAK);
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&src));			
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
	}		
	/* effect */	
	if ((s = psy_property_at_str(p, "effect_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&srcmac, vals);
		style = psy_ui_style(STYLE_MV_EFFECT);
		psy_ui_style_set_size(style, psy_ui_size_make_px(
			srcmac.right - srcmac.left,
			srcmac.bottom - srcmac.top));
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&srcmac));			
		psy_ui_style_set_background_position_px(style,
			-srcmac.left, -srcmac.top);
	}
	if ((s = psy_property_at_str(p, "effect_mute_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "effect_mute_dest", 0))) {
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		style = psy_ui_style(STYLE_MV_EFFECT_MUTE);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_EFFECT_MUTE_SELECT);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "effect_bypass_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "effect_bypass_dest", 0))) {
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		style = psy_ui_style(STYLE_MV_EFFECT_BYPASS);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_EFFECT_BYPASS_SELECT);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(src.right - src.left,
				src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "effect_name_dest", 0))) {		
		skin_psh_values(s, 2, vals);
		machineviewstyles_set_dest(&dst, vals, 2);
		style = psy_ui_style(STYLE_MV_EFFECT_NAME);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px(srcmac.right - dst.x, 15.0)));
	}
	if ((s = psy_property_at_str(p, "effect_pan_source", 0))) {
		skin_psh_values(s, 4, vals);
		style = psy_ui_style(STYLE_MV_EFFECT_PAN);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "effect_pan_dest", 0))) {
		skin_psh_values(s, 3, vals);
		machineviewstyles_set_dest(&dst, vals, 3);
		style = psy_ui_style(STYLE_MV_EFFECT_PAN);
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px((double)vals[2] + src.right - src.left,
				src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_EFFECT_PAN_SLIDER);
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&src));			
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(0.0, 0.0),
			psy_ui_size_make_px(src.right - src.left, src.bottom - src.top)));
	}
	if ((s = psy_property_at_str(p, "effect_vu0_source", 0))) {
		skin_psh_values(s, 4, vals);
		machineviewstyles_set_source(&src, vals);
	}
	if ((s = psy_property_at_str(p, "effect_vu_dest", 0))) {
		skin_psh_values(s, 3, vals);
		machineviewstyles_set_dest(&dst, vals, 3);
		style = psy_ui_style(STYLE_MV_EFFECT_VU);		
		psy_ui_style_set_position(style, psy_ui_rectangle_make(
			psy_ui_point_make_px(dst.x, dst.y),
			psy_ui_size_make_px((double)vals[2], src.bottom - src.top)));
		style = psy_ui_style(STYLE_MV_EFFECT_VU0);
		psy_ui_style_set_background_size_px(style, psy_ui_realsize_make(
			(double)vals[2], src.bottom - src.top));
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
	}
	if ((s = psy_property_at_str(p, "effect_vu_peak_source", 0))) {		
		skin_psh_values(s, 4, vals);
		style = psy_ui_style(STYLE_MV_EFFECT_VUPEAK);
		machineviewstyles_set_source(&src, vals);
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&src));			
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
	}			
}

void machineviewstyles_set_source(psy_ui_RealRectangle* r,
	intptr_t vals[4])
{	
	r->left = (double)vals[0];
	r->top = (double)vals[1];
	r->right = (double)vals[0] + (double)vals[2];
	r->bottom = (double)vals[1] + (double)vals[3];	
}

void machineviewstyles_set_dest(psy_ui_RealPoint* pt,
	intptr_t vals[4], uintptr_t num)
{
	pt->x = (double)vals[0];
	pt->y = (double)vals[1];	
}


/* MachineStyleConfigurator */

/* prototypes */
static psy_ui_FontInfo machinestyleconfigurator_font_info(
	MachineStyleConfigurator*, const char* key, double zoom);
static void machinestyleconfigurator_on_background_image(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_background_colour(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_wire_colour(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_generator_font_colour(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_effect_font_colour(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_poly_colour(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_triangle_size(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_generator_font(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_effect_font(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_machine_skin(
	MachineStyleConfigurator*, psy_Property* sender);
static void machinestyleconfigurator_on_app_zoom(
	MachineStyleConfigurator*, psy_ui_AppZoom* sender);
static void machinestyleconfigurator_on_zoom(
	MachineStyleConfigurator*, psy_Property* sender);
static double machinestyleconfigurator_zoom(
	const MachineStyleConfigurator*);

/* implementation */
void machinestyleconfigurator_init(MachineStyleConfigurator* self,
	MachineViewStyles* styles, psy_Configuration* mac_cfg)
{
	assert(self);
	assert(styles);
	assert(mac_cfg);
	
	self->styles = styles;
	self->cfg = mac_cfg;		
}

void machinestyleconfigurator_connect(MachineStyleConfigurator* self)
{
	assert(self);
		
	psy_signal_connect(&psy_ui_app_zoom(psy_ui_app())->signal_zoom, self,
		machinestyleconfigurator_on_app_zoom);
	psy_configuration_connect(self->cfg, "zoom",
		self, machinestyleconfigurator_on_zoom);
	psy_configuration_connect(self->cfg, "theme.machine_skin",
		self, machinestyleconfigurator_on_machine_skin);
	psy_configuration_connect(self->cfg, "theme.machine_background",
		self, machinestyleconfigurator_on_background_image);
	psy_configuration_connect(self->cfg, "theme.mv_colour",
		self, machinestyleconfigurator_on_background_colour);
	psy_configuration_connect(self->cfg, "theme.mv_wirecolour",
		self, machinestyleconfigurator_on_wire_colour);
	psy_configuration_connect(self->cfg, "theme.mv_generator_fontcolour",
		self, machinestyleconfigurator_on_generator_font_colour);
	psy_configuration_connect(self->cfg, "theme.mv_effect_fontcolour",
		self, machinestyleconfigurator_on_effect_font_colour);
	psy_configuration_connect(self->cfg, "theme.mv_triangle_size",
		self, machinestyleconfigurator_on_triangle_size);
	psy_configuration_connect(self->cfg, "theme.mv_polycolour",
		self, machinestyleconfigurator_on_poly_colour);
	psy_configuration_connect(self->cfg, "theme.generator_font",
		self, machinestyleconfigurator_on_generator_font);
	psy_configuration_connect(self->cfg, "theme.effect_font",
		self, machinestyleconfigurator_on_effect_font);
}

void machinestyleconfigurator_configure(MachineStyleConfigurator* self)
{	
	assert(self);
		
	psy_configuration_configure(self->cfg, "theme.machine_background");	
	psy_configuration_configure(self->cfg, "theme.mv_colour");		
	psy_configuration_configure(self->cfg, "theme.mv_wirecolour");		
	psy_configuration_configure(self->cfg, "theme.mv_generator_fontcolour");		
	psy_configuration_configure(self->cfg, "theme.mv_effect_fontcolour");		
	psy_configuration_configure(self->cfg, "theme.mv_triangle_size");		
	psy_configuration_configure(self->cfg, "theme.mv_polycolour");	
	psy_configuration_configure(self->cfg, "theme.generator_font");		
	psy_configuration_configure(self->cfg, "theme.effect_font");
	psy_configuration_configure(self->cfg, "theme.machine_skin");	
	psy_configuration_configure(self->cfg, "zoom");	
}

void machinestyleconfigurator_on_background_image(
	MachineStyleConfigurator* self, psy_Property* sender)
{
	char path[_MAX_PATH];
	psy_ui_Style* style;
	
	assert(self);
							
	psy_dir_find_file(psy_configuration_value_str(self->styles->dir_cfg, "skins",
		PSYCLE_SKINS_DEFAULT_DIR), psy_property_item_str(sender), path);
	style = psy_ui_style(STYLE_MV_WIRES);
	if (psy_strlen(path) == 0) {
		psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
		psy_ui_style_setbackgroundpath(style, "");
		psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	} else {
		psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
		psy_ui_style_setbackgroundpath(style, path);
		psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	}
}

void machinestyleconfigurator_on_background_colour(
	MachineStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
					
	style = psy_ui_style(STYLE_MV_WIRES);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	style = psy_ui_style(STYLE_MV_STACK);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_wire_colour(MachineStyleConfigurator* self,
	psy_Property* sender)
{
	psy_ui_Style* style;
	psy_ui_Colour wire_colour;
	
	assert(self);
				
	style = psy_ui_style(STYLE_MV_WIRES);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	style = psy_ui_style(STYLE_MV_WIRE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
	wire_colour = psy_ui_colour_make((uint32_t)psy_property_item_int(sender));
	psy_ui_colour_add_rgb(&wire_colour, 30.0, 30.0, 30.0);
	style = psy_ui_style(STYLE_MV_WIRE_SELECT);
	psy_ui_style_set_colour(style, wire_colour);	
	style = psy_ui_style(STYLE_MV_STACK);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_generator_font_colour(
	MachineStyleConfigurator* self, psy_Property* sender)
{	
	assert(self);
	
	psy_ui_style_set_colour(psy_ui_style(STYLE_MV_GENERATOR),
		psy_ui_colour_make((uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_effect_font_colour(
	MachineStyleConfigurator* self, psy_Property* sender)
{	
	assert(self);
	
	psy_ui_style_set_colour(psy_ui_style(STYLE_MV_EFFECT),
		psy_ui_colour_make((uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_poly_colour(MachineStyleConfigurator* self,
	psy_Property* sender)
{	
	assert(self);
	
	psy_ui_style_set_colour(psy_ui_style(STYLE_MV_WIRE_POLY),
		psy_ui_colour_make((uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_triangle_size(MachineStyleConfigurator* self,
	psy_Property* sender)
{	
	assert(self);
	
	psy_ui_style_set_background_size_px(psy_ui_style(STYLE_MV_WIRE_POLY),
		psy_ui_realsize_make(
			(uint32_t)psy_property_item_int(sender),
			(uint32_t)psy_property_item_int(sender)));
}

void machinestyleconfigurator_on_generator_font(MachineStyleConfigurator* self,
	psy_Property* sender)
{
	psy_ui_FontInfo font_info;
	psy_ui_Style* style;	
	
	assert(self);
			
	font_info = machinestyleconfigurator_font_info(self, "theme.generator_font",
		machinestyleconfigurator_zoom(self));
	style = psy_ui_style(STYLE_MV_GENERATOR);
	psy_ui_style_set_font_info(style, font_info);
	style = psy_ui_style(STYLE_MV_MASTER);
	psy_ui_style_set_font_info(style, font_info);
}

void machinestyleconfigurator_on_effect_font(MachineStyleConfigurator* self,
	psy_Property* sender)
{		
	assert(self);
			
	psy_ui_style_set_font_info(psy_ui_style(STYLE_MV_EFFECT),
		machinestyleconfigurator_font_info(self, "theme.effect_font",
		machinestyleconfigurator_zoom(self)));
}

void machinestyleconfigurator_on_machine_skin(MachineStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	machineviewstyles_load(self->styles, psy_property_item_str(sender));
}

void  machinestyleconfigurator_on_app_zoom(MachineStyleConfigurator* self,
	psy_ui_AppZoom* sender)
{
	assert(self);
	
	machinestyleconfigurator_on_zoom(self, NULL);
}

void machinestyleconfigurator_on_zoom(MachineStyleConfigurator* self,
	psy_Property* sender)
{
	psy_ui_Style* style;	
	double zoom;	
	
	assert(self);
		
	zoom = machinestyleconfigurator_zoom(self);
	style = psy_ui_style(STYLE_MV_MASTER);	
	psy_ui_style_set_font_info(style, machinestyleconfigurator_font_info(self,
		"theme.generator_font", zoom));
	style = psy_ui_style(STYLE_MV_GENERATOR);
	psy_ui_style_set_font_info(style, machinestyleconfigurator_font_info(self,
		"theme.generator_font", zoom));		
	style = psy_ui_style(STYLE_MV_EFFECT);	
	psy_ui_style_set_font_info(style, machinestyleconfigurator_font_info(self,
		"theme.effect_font", zoom));
	style = psy_ui_style(STYLE_MV_STACK);
	psy_ui_style_set_font_info(style, machinestyleconfigurator_font_info(self,
		"theme.generator_font", zoom));
}

double machinestyleconfigurator_zoom(const MachineStyleConfigurator* self)
{
	psy_Property* p;
	double rv;
	
	rv = psy_ui_app_zoom_rate(psy_ui_app());
	p = psy_configuration_at(self->cfg, "zoom");
	if (p) {
		rv *= psy_property_item_double(p);
	}
	return rv;	
}

psy_ui_FontInfo machinestyleconfigurator_font_info(
	MachineStyleConfigurator* self, const char* key, double zoom)
{
	psy_ui_FontInfo rv;
	psy_Property* p;
	
	assert(self);
	
	p = psy_configuration_at(self->cfg, key);
	if (p) {
		psy_ui_fontinfo_init_string(&rv, psy_property_item_str(p));	
		rv.lfHeight = (int32_t)((double)rv.lfHeight * zoom);
	} else {
		psy_ui_fontinfo_init_string(&rv, "tahoma; 16");
	}
	return rv;
}
