/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "paramviewstyles.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
#include "skinio.h"
/* file */
#include <dir.h>
/* ui */
#include <uiapp.h>
/* platform */
#include "../../detail/os.h"
#include "../../detail/portable.h"


#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif


void paramviewstyles_init(ParamViewStyles* self, psy_Configuration* dir_cfg)
{
	assert(self);
	assert(dir_cfg);
	
	self->dir_cfg = dir_cfg;
	paramviewstyles_make_default(self);
}

void paramviewstyles_make_default(ParamViewStyles* self)
{
	psy_ui_Styles* styles;
	psy_ui_Style* style;
	
	assert(self);
	
	styles = &psy_ui_app_defaults()->styles;	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00B4B4B4),
		psy_ui_colour_make(0x00292929));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_TITLE,
		"macparam_title", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00CDCDCD),
		psy_ui_colour_make(0x00555555));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_TOP,
		"macparam_top", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00E7BD18),
		psy_ui_colour_make(0x00444444));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_BOTTOM,
		"macparam_bottom", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00CDCDCD),
		psy_ui_colour_make(0x00555555));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_TOP_ACTIVE,
		"macparam_top::active", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00E7BD18),
		psy_ui_colour_make(0x00292929));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_BOTTOM_ACTIVE,
		"macparam_bottom::active", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_PARAMKNOB);	
	psy_ui_style_animate_background(style, 0,
		psy_ui_realsize_make(28.0, 28.0), TRUE);
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_KNOB,
		"macparam_knob", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(30.0, 182.0));
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00E7BD18),
		psy_ui_colour_make(0x00292929));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_SLIDER,
		"macparam_slider", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(22.0, 10.0));
	psy_ui_style_set_background_position_px(style, 0.0, -182.0);
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_SLIDERKNOB,
		"macparam_sliderknob", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(28.0, 28.0));
	psy_ui_style_set_background_position_px(style, -30.0, -118.0);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00CDCDCD),
		psy_ui_colour_make(0x00444444));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_SWITCHON,
		"macparam_switchon", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(28.0, 28.0));
	psy_ui_style_set_background_position_px(style, -30.0, -90.0);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00CDCDCD),
		psy_ui_colour_make(0x00444444));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_SWITCHOFF,
		"macparam_switchoff", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(13.0, 13.0));
	psy_ui_style_set_background_position_px(style, -30.0, -159.0);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00E7BD18),
		psy_ui_colour_make(0x00444444));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_CHECKON,
		"macparam_checkon", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(13.0, 13.0));
	psy_ui_style_set_background_position_px(style, -30.0, -146.0);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x00CDCDCD),
		psy_ui_colour_make(0x00444444));
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_CHECKOFF,
		"macparam_checkoff", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(16.0, 90.0));
	psy_ui_style_set_background_position_px(style, -46.0, 0.0);
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_VUON,
		"macparam_vuon", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_MIXERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(16.0, 90.0));
	psy_ui_style_set_background_position_px(style, -30.0, 0.0);
	psy_ui_styles_set_style(styles, STYLE_MACPARAM_VUOFF,
		"macparam_vuoff", style);
}

void paramviewstyles_load(ParamViewStyles* self, const char* path)
{	
	assert(self);
			
	if (psy_strlen(path) != 0) {
		psy_ui_Style* style;
				
		style = psy_ui_style(STYLE_MACPARAM_KNOB);		
		psy_ui_style_set_background_id(style, psy_INDEX_INVALID);
		if (psy_ui_style_setbackgroundpath(style, path) != PSY_OK) {			
			psy_ui_style_set_background_id(style, IDB_PARAMKNOB);
		}
		psy_ui_style_animate_background(style, 0, psy_ui_realsize_make(
			28.0, 28.0), TRUE);
	} else {
		paramviewstyles_make_default(self);
	}
}

/* prototypes */
static void paramstyleconfigurator_on_machine_param_skin(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_titlecolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_titlefontcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_topcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_fonttopcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_bottomcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_fontbottomcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_htopcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_hfonttopcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_hbottomcolor(
	ParamStyleConfigurator*, psy_Property* sender);
static void paramstyleconfigurator_on_machine_param_hfontbottomcolor(
	ParamStyleConfigurator*, psy_Property* sender);

/* implementation */
void paramstyleconfigurator_init(ParamStyleConfigurator* self,
	ParamViewStyles* styles, psy_Configuration* param_cfg)
{
	assert(self);
	assert(styles);
	assert(param_cfg);
	
	self->styles = styles;
	self->cfg = param_cfg;
}

void paramstyleconfigurator_connect(ParamStyleConfigurator* self)
{
	assert(self);
		
	psy_configuration_connect(self->cfg, "theme.machinedial_bmp",
		self, paramstyleconfigurator_on_machine_param_skin);	
	psy_configuration_connect(self->cfg, "theme.machineGUITitleColor",
		self, paramstyleconfigurator_on_machine_param_titlecolor);
	psy_configuration_connect(self->cfg, "theme.machineGUITitleFontColor",
		self, paramstyleconfigurator_on_machine_param_titlefontcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUITopColor",
		self, paramstyleconfigurator_on_machine_param_topcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIFontTopColor",
		self, paramstyleconfigurator_on_machine_param_fonttopcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIBottomColor",
		self, paramstyleconfigurator_on_machine_param_bottomcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIFontBottomColor",
		self, paramstyleconfigurator_on_machine_param_fontbottomcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIHTopColor",
		self, paramstyleconfigurator_on_machine_param_htopcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIHFontTopColor",
		self, paramstyleconfigurator_on_machine_param_hfonttopcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIHBottomColor",
		self, paramstyleconfigurator_on_machine_param_hbottomcolor);
	psy_configuration_connect(self->cfg, "theme.machineGUIHFontBottomColor",
		self, paramstyleconfigurator_on_machine_param_hfontbottomcolor);
}

void paramstyleconfigurator_configure(ParamStyleConfigurator* self)
{	
	assert(self);
		
	psy_configuration_configure(self->cfg, "theme.machinedial_bmp");
	psy_configuration_configure(self->cfg, "theme.machineGUITitleColor");		
	psy_configuration_configure(self->cfg, "theme.machineGUITitleFontColor");
	psy_configuration_configure(self->cfg, "theme.machineGUITopColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIFontTopColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIBottomColor");	
	psy_configuration_configure(self->cfg, "theme.machineGUIFontBottomColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIHTopColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIHFontTopColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIHBottomColor");
	psy_configuration_configure(self->cfg, "theme.machineGUIHFontBottomColor");	
}

void paramstyleconfigurator_on_machine_param_skin(
	ParamStyleConfigurator* self, psy_Property* sender)
{	
	assert(self);
	
	paramviewstyles_load(self->styles, psy_property_item_str(sender));	
}

void paramstyleconfigurator_on_machine_param_titlecolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TITLE);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_titlefontcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TITLE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_topcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TOP);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_fonttopcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TOP);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_bottomcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_BOTTOM);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_fontbottomcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_BOTTOM);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_htopcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TOP_ACTIVE);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_hfonttopcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_TOP_ACTIVE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_hbottomcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_BOTTOM_ACTIVE);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void paramstyleconfigurator_on_machine_param_hfontbottomcolor(
	ParamStyleConfigurator* self, psy_Property* sender)
{
	psy_ui_Style* style;
	
	assert(self);
	
	style = psy_ui_style(STYLE_MACPARAM_BOTTOM_ACTIVE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}
