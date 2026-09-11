/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewstyles.h"

#ifdef PSYCLE_USE_TRACKERVIEW


/* host */
#include "resources/resource.h"
#include "skinio.h"
#include "styles.h"
/* file */
#include <dir.h>
/* ui */
#include <uiapp.h>
/* portable */
#include "../../detail/os.h"
#include "../../detail/portable.h"


#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif


/* prototypes */
static void patternviewstyles_make_default(PatternViewStyles*);

/* implementation */
void patternviewstyles_init(PatternViewStyles* self,
	psy_Configuration* dir_cfg)
{
	assert(self);
	assert(dir_cfg);
	
	self->dir_cfg = dir_cfg;
	patternviewstyles_make_default(self);
}

void patternviewstyles_make_default(PatternViewStyles* self)
{
	psy_ui_Styles* styles;	
			
	assert(self);
	
	styles = &psy_ui_app_defaults()->styles;	
	psy_ui_styles_set_style(styles, STYLE_PATTERNVIEW,
		"style_patternview", psy_ui_style_alloc_init());	
	psy_ui_styles_set_style(styles, STYLE_PV_ROW,
		"pv_row", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_ROW_SELECT,
		"pv_row::select", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_ROWBEAT,
		"pv_rowbeat", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_ROWBEAT_SELECT,
		"pv_rowbeat::select", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_ROW4BEAT,
		"pv_row4beat", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_ROW4BEAT_SELECT,
		"pv_row4beat::select", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_CURSOR,
		"pv_cursor", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_CURSOR_SELECT,
		"pv_cursor::select", psy_ui_style_alloc_init());
	psy_ui_styles_set_style(styles, STYLE_PV_PLAYBAR,
		"pv_playbar", psy_ui_style_alloc_init());
}


/* PatternStyleConfigurator */

/* prototypes */
static void patternstyleconfigurator_on_pvc_background(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_background2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_font(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_font2(
	PatternStyleConfigurator*, psy_Property* sender);	
static void patternstyleconfigurator_on_pvc_row(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_row2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_rowbeat(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_rowbeat2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_row4beat(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_row4beat2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_playbar(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_playbar2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_midline(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_midline2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_fontcur(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_fontcur2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_cursor(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_cursor2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_fontsel(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_fontsel2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_selection(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_on_pvc_selection2(
	PatternStyleConfigurator*, psy_Property* sender);
static void patternstyleconfigurator_set_style_colour(
	PatternStyleConfigurator*, uintptr_t id, psy_Property*);
static void patternstyleconfigurator_set_style_bg_colour(
	PatternStyleConfigurator*, uintptr_t id, psy_Property*);	

/* implementation */
void patternstyleconfigurator_init(PatternStyleConfigurator* self,
	PatternViewStyles* styles, psy_Configuration* pat_cfg)
{
	assert(self);
	assert(styles);
	assert(pat_cfg);
	
	self->styles = styles;
	self->cfg = pat_cfg;		
}

void patternstyleconfigurator_connect(PatternStyleConfigurator* self)
{
	assert(self);
				
	psy_configuration_connect(self->cfg, "theme.pvc_background",
		self, patternstyleconfigurator_on_pvc_background);
	psy_configuration_connect(self->cfg, "theme.pvc_background2",
		self, patternstyleconfigurator_on_pvc_background2);
	psy_configuration_connect(self->cfg, "theme.pvc_font",
		self, patternstyleconfigurator_on_pvc_font);	
	psy_configuration_connect(self->cfg, "theme.pvc_font2",
		self, patternstyleconfigurator_on_pvc_font2);	
	psy_configuration_connect(self->cfg, "theme.pvc_row",
		self, patternstyleconfigurator_on_pvc_row);
	psy_configuration_connect(self->cfg, "theme.pvc_row2",
		self, patternstyleconfigurator_on_pvc_row2);
	psy_configuration_connect(self->cfg, "theme.pvc_rowbeat",
		self, patternstyleconfigurator_on_pvc_rowbeat);
	psy_configuration_connect(self->cfg, "theme.pvc_rowbeat2",
		self, patternstyleconfigurator_on_pvc_rowbeat2);
	psy_configuration_connect(self->cfg, "theme.pvc_row4beat",
		self, patternstyleconfigurator_on_pvc_row4beat);
	psy_configuration_connect(self->cfg, "theme.pvc_row4beat2",
		self, patternstyleconfigurator_on_pvc_row4beat2);
	psy_configuration_connect(self->cfg, "theme.pvc_playbar",
		self, patternstyleconfigurator_on_pvc_playbar);
	psy_configuration_connect(self->cfg, "theme.pvc_playbar2",
		self, patternstyleconfigurator_on_pvc_playbar);
	psy_configuration_connect(self->cfg, "theme.pvc_fontcur",
		self, patternstyleconfigurator_on_pvc_fontcur);
	psy_configuration_connect(self->cfg, "theme.pvc_fontcur2",
		self, patternstyleconfigurator_on_pvc_fontcur2);
	psy_configuration_connect(self->cfg, "theme.pvc_cursor",
		self, patternstyleconfigurator_on_pvc_cursor);	
	psy_configuration_connect(self->cfg, "theme.pvc_cursor2",
		self, patternstyleconfigurator_on_pvc_cursor2);
	psy_configuration_connect(self->cfg, "theme.pvc_midline",
		self, patternstyleconfigurator_on_pvc_midline);
	psy_configuration_connect(self->cfg, "theme.pvc_midline2",
		self, patternstyleconfigurator_on_pvc_midline2);	
	psy_configuration_connect(self->cfg, "theme.pvc_fontsel",
		self, patternstyleconfigurator_on_pvc_fontsel);
	psy_configuration_connect(self->cfg, "theme.pvc_fontsel2",
		self, patternstyleconfigurator_on_pvc_fontsel2);
	psy_configuration_connect(self->cfg, "theme.pvc_selection",
		self, patternstyleconfigurator_on_pvc_selection);	
	psy_configuration_connect(self->cfg, "theme.pvc_selection2",
		self, patternstyleconfigurator_on_pvc_selection2);
}

void patternstyleconfigurator_configure(PatternStyleConfigurator* self)
{	
	assert(self);
		
	psy_configuration_configure(self->cfg, "theme.pvc_background");
	psy_configuration_configure(self->cfg, "theme.pvc_background2");
	psy_configuration_configure(self->cfg, "theme.pvc_font");
	psy_configuration_configure(self->cfg, "theme.pvc_font2");
	psy_configuration_configure(self->cfg, "theme.pvc_row");
	psy_configuration_configure(self->cfg, "theme.pvc_row2");
	psy_configuration_configure(self->cfg, "theme.pvc_rowbeat");
	psy_configuration_configure(self->cfg, "theme.pvc_rowbeat2");
	psy_configuration_configure(self->cfg, "theme.pvc_row4beat");
	psy_configuration_configure(self->cfg, "theme.pvc_row4beat2");
	psy_configuration_configure(self->cfg, "theme.pvc_playbar");
	psy_configuration_configure(self->cfg, "theme.pvc_playbar2");
	psy_configuration_configure(self->cfg, "theme.pvc_fontcur");
	psy_configuration_configure(self->cfg, "theme.pvc_fontcur2");
	psy_configuration_configure(self->cfg, "theme.pvc_cursor");
	psy_configuration_configure(self->cfg, "theme.pvc_cursor2");
	psy_configuration_configure(self->cfg, "theme.pvc_midline");
	psy_configuration_configure(self->cfg, "theme.pvc_midline2");
	psy_configuration_configure(self->cfg, "theme.pvc_fontsel");
	psy_configuration_configure(self->cfg, "theme.pvc_fontsel2");
	psy_configuration_configure(self->cfg, "theme.pvc_selection");
	psy_configuration_configure(self->cfg, "theme.pvc_selection2");
	psy_configuration_configure(self->cfg, "theme.pattern_header_skin");
}

void patternstyleconfigurator_on_pvc_background(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PATTERNVIEW,
		sender);	
}

void patternstyleconfigurator_on_pvc_background2(PatternStyleConfigurator*
	self, psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PATTERNVIEW, sender);
}	
	
void patternstyleconfigurator_on_pvc_font(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_colour(self, STYLE_PATTERNVIEW, sender);
}

void patternstyleconfigurator_on_pvc_font2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_colour(self, STYLE_PATTERNVIEW, sender);
}

void patternstyleconfigurator_on_pvc_row(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROW, sender);
}

void patternstyleconfigurator_on_pvc_row2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROW, sender);
}

void patternstyleconfigurator_on_pvc_rowbeat(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROWBEAT, sender);
}

void patternstyleconfigurator_on_pvc_rowbeat2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROWBEAT, sender);
}

void patternstyleconfigurator_on_pvc_row4beat(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROW4BEAT, sender);
}

void patternstyleconfigurator_on_pvc_row4beat2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_ROW4BEAT, sender);
}

void patternstyleconfigurator_on_pvc_playbar(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_PLAYBAR, sender);
}

void patternstyleconfigurator_on_pvc_playbar2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_PLAYBAR, sender);
}

void patternstyleconfigurator_on_pvc_midline(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	/* todo: no midline style (playbar used) */
}

void patternstyleconfigurator_on_pvc_midline2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	/* todo: no midline style (playbar used) */
}

void patternstyleconfigurator_on_pvc_fontcur(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_colour(self, STYLE_PV_CURSOR, sender);
}

void patternstyleconfigurator_on_pvc_fontcur2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_colour(self, STYLE_PV_CURSOR, sender);
}

void patternstyleconfigurator_on_pvc_cursor(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_CURSOR, sender);
}

void patternstyleconfigurator_on_pvc_cursor2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_bg_colour(self, STYLE_PV_CURSOR, sender);
}

void patternstyleconfigurator_on_pvc_fontsel(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_set_style_colour(self, STYLE_PV_ROW_SELECT, sender);		
}

void patternstyleconfigurator_on_pvc_fontsel2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	assert(self);
	
	patternstyleconfigurator_on_pvc_fontsel(self, sender);
}

void patternstyleconfigurator_on_pvc_selection(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	psy_ui_Style* style;
	psy_Property* row;
	psy_Property* rowbeat;
	psy_Property* row4beat;
	psy_ui_Colour colour1;
	psy_ui_Colour colour2;	
		
	assert(self);
	
	if (self->cfg) {
		row = psy_configuration_at(self->cfg, "theme.pvc_row");
		rowbeat = psy_configuration_at(self->cfg, "theme.pvc_rowbeat");
		row4beat = psy_configuration_at(self->cfg, "theme.pvc_row4beat");
	} else {
		row = rowbeat = row4beat = NULL;
	}
	if (row) {
		colour1 = psy_ui_colour_make((uint32_t)psy_property_item_int(row));
	} else {
		colour1 = psy_ui_colour_make(0x003E3E3E);
	}
	if (rowbeat) {
		colour2 = psy_ui_colour_make((uint32_t)psy_property_item_int(rowbeat));
	} else {
		colour2 = psy_ui_colour_make(0x00363636);
	}
	style = psy_ui_style(STYLE_PV_ROWBEAT_SELECT);
	psy_ui_style_set_background_colour(style, psy_ui_diffadd_colours(		
		colour1, colour2, psy_ui_colour_make((uint32_t)psy_property_item_int(sender))));
	if (row4beat) {
		colour2 = psy_ui_colour_make((uint32_t)psy_property_item_int(row4beat));
	}
	style = psy_ui_style(STYLE_PV_ROW4BEAT_SELECT);
	psy_ui_style_set_background_colour(style, psy_ui_diffadd_colours(		
		colour1, colour2, psy_ui_colour_make((uint32_t)psy_property_item_int(sender))));
	style = psy_ui_style(STYLE_PV_ROW_SELECT);
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(
		(uint32_t)psy_property_item_int(sender)));
}

void patternstyleconfigurator_on_pvc_selection2(PatternStyleConfigurator* self,
	psy_Property* sender)
{
	patternstyleconfigurator_on_pvc_selection(self, sender);
}

void patternstyleconfigurator_set_style_colour(PatternStyleConfigurator* self,
	uintptr_t id, psy_Property* p)
{
	psy_ui_Style* style;
	
	assert(self);

	style = psy_ui_style(id);
	if (style) {
		psy_ui_style_set_colour(style, psy_ui_colour_make(
			(uint32_t)psy_property_item_int(p)));
	}	
	/* psy_ui_component_invalidate(&self->component); */
}

void patternstyleconfigurator_set_style_bg_colour(PatternStyleConfigurator*
	self, uintptr_t id, psy_Property* p)
{
	psy_ui_Style* style;
	
	assert(self);

	style = psy_ui_style(id);
	if (style) {
		psy_ui_style_set_background_colour(style, psy_ui_colour_make(
			(uint32_t)psy_property_item_int(p)));
	}
	/* psy_ui_component_invalidate(&self->component); */
}


#endif /* PSYCLE_USE_TRACKERVIEW */
