/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "styles.h"
/* host */
#include "resources/resource.h"
#include "bitmaps.h"
/* file */
#include <dir.h>
/* ui */
#include <uiappstyles.h>
#include <uimaterial.h>
/* portable */
#include "../../detail/portable.h"


/* prototypes */
static void init_light_theme(psy_ui_Styles*);

/* implementation */
void init_host_styles(psy_ui_Styles* self, psy_ui_ThemeMode theme)
{
	psy_ui_MaterialTheme material;
	psy_ui_Style* style;
	
	if (theme == psy_ui_LIGHTTHEME) {
		init_light_theme(self);
		return;
	}
	self->theme_mode = theme;
	psy_ui_materialtheme_init(&material, theme);	
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_12p);	
	psy_ui_styles_set_style(self, STYLE_STATUSBAR, "statusbar", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_5p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON, "seqview_button",
		style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_9p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON_HOVER,
		"seqview_button::hover", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,  material.onsecondary,
		material.surface_overlay_9p);	
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON_SELECT,
		"seqview_button::select", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_7p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW, "seqlistview", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_FOCUS, "seqlistview::focus",
		style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_strong,
		material.surface_overlay_5p);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_7p);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_SELECT,
		"seqlistview::select", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_5p);	
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_TRACK, "seqlistview_track",
		style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colours(style, material.onprimary_strong,
		material.surface_overlay_7p);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_TRACK_SELECT,
		"seqlistview_track::select", style);
			
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colours(style, material.onsecondary,
		material.surface_overlay_9p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_TRACKS, "seqedt_tracks", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onsecondary, material.light),
		psy_ui_colour_weighted(material.secondary, material.pale));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM, "seqedt_item", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onsecondary, material.accent),
		psy_ui_colour_weighted(material.secondary, material.pale));
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM_HOVER, "seqedt_item::hover",
		style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onsecondary, material.accent),
		psy_ui_colour_weighted(material.secondary, material.pale));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM_SELECTED,
		"seqedt_item::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.light),
		psy_ui_colour_make(0x00475E38));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE, "seqedt_sample", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, material.surface_overlay_16p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LINE_CURSOR,
		"seqedt_line_cursor", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, 
		psy_ui_colour_weighted(material.secondary, 300));
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LINE_PLAY,
		"seqedt_line_play", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, 
		psy_ui_colour_weighted(material.secondary, material.pale));
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LINE_SEQPOS,
		"seqedt_line_seqpos", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_make(0x00475E38));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE_HOVER,
		"seqedt_sample::hover", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_make(0x00576E48));
	psy_ui_border_init_solid_radius(&style->border,
		psy_ui_colour_make(0x00677E58), 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE_SELECTED,
		"seqedt_sample::select",style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.light),
		material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER,
		"seqedt_marker", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER_HOVER,
		"seqedt_marker::hover", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_weighted(material.primary, material.pale));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER_SELECTED,
		"seqedt_marker::select", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_border_init_right(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_24p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_TRACK_NUMBER,
		"seqedt_track_number", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style,
		material.surface_overlay_16p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_DESCITEM_SELECTED,
		"seqedt_descitem::select", style);	

	style = psy_ui_style_alloc_init();
	// psy_ui_style_set_background_colour(style,
	//	material.surface_overlay_8p);	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_TOP, "seqedt_ruler_top",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.surface_overlay_24p);
	// psy_ui_style_set_background_colour(style,
	//	material.surface_overlay_6p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_BOTTOM,
		"seqedt_ruler_bottom", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_weighted(material.secondary,
		900));	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_CURSOR,
		"seqedt_ruler_cursor", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style,
		material.surface_overlay_11p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LEFT, "seqedt_left", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP, "seqedt_loop", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP_HOVER,
		"seqedt_loop::hover", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_strong);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP_ACTIVE,
		"loop::active", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.5, 0.5, 0.5, 0.5);	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_PROPERTIES,
		"seqedt_properties", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.secondary,
		psy_ui_colour_overlayed(&material.surface, &material.overlay, 0.15));
	psy_ui_styles_set_style(self, STYLE_SEQ_PROGRESS, "seq_progress", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.primary, material.medium),
		material.overlay_4p);
	psy_ui_styles_set_style(self, STYLE_ZOOMBOX, "zoombox", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_EDIT));
	psy_ui_styles_set_style(self, STYLE_ZOOMBOX_EDIT, "zoombox_edit", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00292929));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE,
		"stepsequencer_tile", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x009F7B00));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_ACTIVE,
		"stepsequencer_tile::active", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00CACACA));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SELECT,
		"stepsequencer_tile::select", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 0.25, 0.25, 0.25);	
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_STEP,
		"stepsequencer_step", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SHIFT,
		"stepsequencer_tile_shift", style);
		
		style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00999999));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SHIFT_SELECT,
		"stepsequencer_tile_shift::select", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 0.25, 0.25, 0.25);	
	// psy_ui_style_set_background_colour(style,
	//	material.surface_overlay_7p);
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_BAR,
		"stepsequencer_bar", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_16p);
	psy_ui_style_set_padding_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_styles_set_style(self, STYLE_VIEW_HEADER, "", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_padding_em(style, 0.25, 0.5, 0.25, 0.0);
	psy_ui_styles_set_style(self, STYLE_TRACKSCOPES, "trackscopes", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.surface_overlay_24p);
	psy_ui_styles_set_style(self, STYLE_TRACKSCOPE, "trackscope", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.2, 0.0, 0.2, 0.0);
	psy_ui_style_set_margin_em(style, 0.3, 0.0, 0.0, 0.0);	
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BAR, "dialogbar", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, material.surface_overlay_5p);	
	psy_ui_style_set_background_id(style, IDB_ABOUT);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(
		psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	style->background.align = psy_ui_ALIGNMENT_CENTER;
	psy_ui_styles_set_style(self, STYLE_ABOUT, "style_about", style);		
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.0, 0.0, 0.0, 1.0);
	psy_ui_style_set_background_overlay(style, 6);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 10.0);
	psy_ui_styles_set_style(self, STYLE_PLAYBAR, "playbar", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);
	psy_ui_style_set_background_overlay(style, 2);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_5p, 6);
	psy_ui_style_set_padding_em(style, 0.3, 0.3, 0.3, 0.3);
	psy_ui_styles_set_style(self, STYLE_DURATION_TIME, "duration_time", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		material.surface_overlay_16p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY, "key", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_12p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_ACTIVE, "key::active", style);
	
	style = psy_ui_style_clone(psy_ui_styles_at(self, STYLE_KEY));
	psy_ui_styles_set_style(self, STYLE_KEY_HOVER, "", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.secondary,
		psy_ui_colour_make_rgb(52, 53, 50));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_SELECT, "key::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(128, 172, 131));		
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_SHIFT_SELECT, "key_shift::select",
		style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(28, 138, 103));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_ALT_SELECT,
		"key_alt::select", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(176, 173, 130));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_CTRL_SELECT, "key_ctrl::select",
		style);		
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_9p);
	psy_ui_styles_set_style(self, STYLE_TABLEROW, "tablerow", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_TABLEROW_HOVER, "tablerow::hover",
		style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_16p);
	psy_ui_styles_set_style(self, STYLE_TABLEROW_SELECT, "tablerow::select",
		style);	
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border,
		psy_ui_colour_overlayed(&material.surface, &material.overlay, 0.20));
	psy_ui_border_setradius_px(&style->border, 6.0);	
	psy_ui_style_set_padding_em(style, 0.3, 0.3, 0.3, 0.0);	
	psy_ui_styles_set_style(self, STYLE_SEARCHFIELD, "searchfield", style);
	
	style = psy_ui_style_clone(style);	
	psy_ui_border_set_colour(&style->border, material.secondary);
	psy_ui_styles_set_style(self, STYLE_SEARCHFIELD_SELECT,
		"searchfield::select", style);	
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_16p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_style_set_background_colour(style, material.surface_overlay_9p);	
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_MAINSECTION,
		"propertyview_mainsection", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);
	psy_ui_style_set_background_colour(style, material.surface_overlay_7p);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_16p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_MAINSECTIONHEADER,
		"propertyview_mainsectionheader", style);	
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 1.0, 0.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SUBSECTION,
		"propertyview_subsection", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);	
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_16p);	
	psy_ui_style_set_padding_em(style, 1.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 1.0, 0.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SUBSECTIONHEADER,
		"propertyview_subsectionheader", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 1.0, 0.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SECTION,
		"propertyview_section", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);	
	psy_ui_style_set_padding_em(style, 1.0, 0.0, 0.5, 0.0);	
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SECTIONHEADER,
		"propertyview_sectionheader", style);
				
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		material.overlay_1p);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_7p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_RECENTVIEW_MAINSECTIONHEADER,
		"recentview_mainsectionheader", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_style_set_padding_em(style, 0.125, 0.5, 0.125, 1.0);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.4, 0.0);
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_HEADER, "header", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_right(&style->border, psy_ui_BORDER_SOLID,
		material.surface);
	psy_ui_styles_set_style(self, STYLE_NAVIGATION, "navbar", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_12p);
	psy_ui_style_set_size(style, psy_ui_size_make_em(2.0, 1.0));
	psy_ui_styles_set_style(self, STYLE_CLIPBOX, "clipbox", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_red());
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_12p);
	psy_ui_style_set_size(style, psy_ui_size_make_em(2.0, 1.0));
	psy_ui_styles_set_style(self, STYLE_CLIPBOX_SELECT,
		"clipbox::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_size(style, psy_ui_size_make_em(25.0, 0.8));
	psy_ui_style_set_margin_em(style, 0.0, 0.5, 0.2, 0.0);
	psy_ui_styles_set_style(self, STYLE_MAIN_VU, "main_vu", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_LABELPAIR,
		"labelpair", style);	
	
	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_LABELPAIR_FIRST,
		"labelpair_first", style);	
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_overlay(style, 2);
	psy_ui_styles_set_style(self, STYLE_LABELPAIR_SECOND,
		"labelpair_second", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON,
		"term_button", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_style_set_colour(style, psy_ui_colour_make_argb(0xFFF6B87F));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON_WARNING,
		"term_button_warning", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x007966CF));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON_ERROR,
		"term_button_error", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_BGMAIN);	
	psy_ui_style_animate_background(style, 20,
		psy_ui_realsize_make(96, 192), TRUE);
	psy_ui_styles_set_style(self, STYLE_GREET, "greet", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_BGTOP);	
	psy_ui_style_animate_background(style, 40,
		psy_ui_realsize_make(128, 128), TRUE);
	psy_ui_styles_set_style(self, STYLE_GREET_TOP, "greet_top", style);		

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON,
		"dialog::button", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.overlay_12p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON_HOVER,
		"dialog_button::hover", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsecondary,
		material.overlay_12p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON_SELECT,
		"dialog_button::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_border_init_right(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_12p);
	psy_ui_styles_set_style(self, STYLE_SCANTASK, "scantask", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xFF5555));
	psy_ui_styles_set_style(self, STYLE_FILEBOX_DIR_LINE,
		"filebox_dir_pane", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_FILEBOX_FILE_LINE, "", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.2, 0.0, 0.2, 0.0);
	psy_ui_style_set_margin_em(style, 0.3, 0.0, 0.0, 0.0);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x55FF45));
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_FILEVIEW_DIRBAR,
		"fileview_dirbar", style);	
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xFF5555));
	psy_ui_styles_set_style(self, STYLE_FILEVIEW_LINKS,
		"fileview_links", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style,
		material.surface_overlay_5p);
	psy_ui_styles_set_style(self, STYLE_SIDE_VIEW, "sideview", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 9);
	psy_ui_style_set_padding_em(style, 0.5, 0.5, 0.0, 0.5);
	psy_ui_styles_set_style(self, STYLE_CLIENT_VIEW, "clientview",
		style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_7p);
	psy_ui_border_setradius_px(&style->border, 6.0);	
	psy_ui_styles_set_style(self, STYLE_BOX, "box", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_9p);
	psy_ui_border_setradius_px(&style->border, 6.0);	
	psy_ui_styles_set_style(self, STYLE_BOX_MEDIUM, "box-medium", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_16p);
	psy_ui_border_setradius_px(&style->border, 6.0);	
	psy_ui_styles_set_style(self, STYLE_BOX_BRIGHT, "box-bright", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_padding_em(style, 0.125, 0.5, 0.125, 0.0);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.4, 0.0);
	psy_ui_border_init_left(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_9p);
	psy_ui_styles_set_style(self, STYLE_TITLEBAR, "titlebar", style);		
	
	style = psy_ui_style_alloc_init();		
	// psy_ui_border_init_top(&style->border, psy_ui_BORDER_SOLID,
	//	material.surface_overlay_12p);
	// psy_ui_style_set_margin_em(style, 1.6, 0.0, 1.6, 0.0);
	psy_ui_style_set_size(style, psy_ui_size_make_em(0.0, 0.1));
	psy_ui_styles_set_style(self, STYLE_SEPARATOR, "separator", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_5p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON, "navbar_button",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_9p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON_HOVER,
		"navbar_button::hover", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsecondary,
		material.surface_overlay_9p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON_SELECT,
		"navbar_button::select", style);

	init_envelope_styles(self);		
}

/* LightTheme */
void init_light_theme(psy_ui_Styles* self)
{
	psy_ui_MaterialTheme material;	
	psy_ui_Style* style;
	
	self->theme_mode = psy_ui_LIGHTTHEME;	
	psy_ui_materialtheme_init(&material, self->theme_mode);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsecondary, material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_SEQVIEW_BUTTON_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium, material.surface_overlay_5p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_7p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid_radius(&style->border,
		psy_ui_colour_weighted(material.secondary, 900), 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_FOCUS, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_strong, material.surface_overlay_5p);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_7p);
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_SELECT, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_TRACK, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQLISTVIEW_TRACK_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_styles_set_style(self, STYLE_SIDE_VIEW, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_white(), psy_ui_colour_blue());
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_white(), psy_ui_colour_blue());
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_white(), psy_ui_colour_blue());
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_ITEM_SELECTED, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.light),
		psy_ui_colour_make(0x00475E38));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_make(0x00475E38));
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_make(0x00576E48));
	psy_ui_border_init_solid_radius(&style->border,
		psy_ui_colour_make(0x00677E58), 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_SAMPLE_SELECTED, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.light),
		material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.onprimary, material.accent),
		psy_ui_colour_weighted(material.primary, material.pale));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 6.0);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_MARKER_SELECTED, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_border_init_right(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_24p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_TRACK_NUMBER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style,
		material.surface_overlay_16p);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_DESCITEM_SELECTED, "", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_white(), psy_ui_colour_blue());
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_TOP, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_white(), psy_ui_colour_blue());
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_BOTTOM, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_weighted(material.secondary, 900));
	psy_ui_styles_set_style(self, STYLE_SEQEDT_RULER_CURSOR, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LEFT, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP_HOVER, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_SEQEDT_LOOP_ACTIVE, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, psy_ui_colour_black(),
		psy_ui_colour_overlayed(&material.surface, &material.overlay, 0.24));
	psy_ui_styles_set_style(self, STYLE_SEQ_PROGRESS, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_weighted(material.primary, material.medium),
		material.overlay_4p);
	psy_ui_styles_set_style(self, STYLE_ZOOMBOX, "", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_EDIT));
	psy_ui_styles_set_style(self, STYLE_ZOOMBOX_EDIT, "", style);	
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, 
		psy_ui_colour_make(0x00DADADA));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE,
		"stepsequencer_tile", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x009F7B00));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_ACTIVE,
		"stepsequencer_tile::active", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00AAAAAA));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SELECT,
		"stepsequencer_tile::select", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid_radius(&style->border,
		psy_ui_colour_make(0x00BABABA), 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 0.25, 0.25, 0.25);	
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_STEP,
		"stepsequencer_step", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SHIFT,
		"stepsequencer_tile_shift", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x00999999));
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_TILE_SHIFT_SELECT,
		"stepsequencer_tile_shift::select", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 0.25, 0.25, 0.25);	
	// psy_ui_style_set_background_colour(style,
	//	material.surface_overlay_7p);
	psy_ui_styles_set_style(self, STYLE_STEPSEQUENCER_BAR,
		"stepsequencer_bar", style);
	

	style = psy_ui_style_alloc_init();	
	//psy_ui_style_set_padding_em(style, 0.0, 0.0, 0.0, 0.0);
	psy_ui_styles_set_style(self, STYLE_STATUSBAR, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.25, 0.5, 0.25, 0.0);
	psy_ui_styles_set_style(self, STYLE_TRACKSCOPES, "trackscopes", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xAAACAE));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0xE9E9E9));
	psy_ui_styles_set_style(self, STYLE_TRACKSCOPE, "trackscope", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsurface,
		material.surface);
	psy_ui_styles_set_style(self, STYLE_SEQEDT_TRACKS, "seqedt_tracks", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 9);
	psy_ui_style_set_padding_em(style, 0.5, 0.5, 0.5, 0.5);
	psy_ui_styles_set_style(self, STYLE_CLIENT_VIEW, "", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, material.surface_overlay_5p);
	psy_ui_style_set_background_id(style, IDB_ABOUT);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	style->background.align = psy_ui_ALIGNMENT_CENTER;
	psy_ui_styles_set_style(self, STYLE_ABOUT, "", style);

	style = psy_ui_style_alloc_init();
	// psy_ui_style_set_padding_em(style, 0.6, 0.0, 0.6, 1.0);
	psy_ui_style_set_background_colour(style, psy_ui_colour_white());	
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_11p, 10.0);
	psy_ui_styles_set_style(self, STYLE_PLAYBAR, "", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);
	psy_ui_style_set_background_overlay(style, 2);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_5p, 6);
	psy_ui_style_set_padding_em(style, 0.3, 0.3, 0.3, 0.3);
	psy_ui_styles_set_style(self, STYLE_DURATION_TIME, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		material.surface_overlay_16p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_12p);
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_ACTIVE, "", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, STYLE_KEY));
	psy_ui_styles_set_style(self, STYLE_KEY_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.secondary,
		psy_ui_colour_make_rgb(52, 53, 50));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(128, 172, 131));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_SHIFT_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(28, 138, 103));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_ALT_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		psy_ui_colour_make_rgb(176, 173, 130));
	psy_ui_border_init_solid_radius(&style->border,
		material.surface_overlay_16p, 4.0);
	psy_ui_styles_set_style(self, STYLE_KEY_CTRL_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_9p);
	psy_ui_styles_set_style(self, STYLE_TABLEROW, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_TABLEROW_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_medium,
		material.surface_overlay_16p);
	psy_ui_styles_set_style(self, STYLE_TABLEROW_SELECT, "", style);	
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border,
		psy_ui_colour_overlayed(&material.surface, &material.overlay, 0.20));
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_style_set_padding_em(style, 0.3, 0.3, 0.3, 0.0);
	psy_ui_style_set_margin_em(style, 0.0, 8.0, 0.0, 0.0);
	psy_ui_styles_set_style(self, STYLE_SEARCHFIELD, "", style);

	style = psy_ui_style_clone(style);
	psy_ui_border_set_colour(&style->border, material.secondary);
	psy_ui_styles_set_style(self, STYLE_SEARCHFIELD_SELECT, "",
		style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_7p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_MAINSECTION, "",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);
	psy_ui_style_set_background_overlay(style, 1);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_7p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_MAINSECTIONHEADER, "",
		style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_7p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SUBSECTION, "",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_weak);
	psy_ui_style_set_background_overlay(style, 1);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_7p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_PROPERTYVIEW_SUBSECTIONHEADER, "",
		style);		

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		material.overlay_1p);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_7p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_RECENTVIEW_MAINSECTIONHEADER, "",
		style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary_weak,
		material.overlay_1p);
	psy_ui_border_init_bottom(&style->border, psy_ui_BORDER_SOLID,
		material.surface_overlay_7p);
	psy_ui_style_set_margin_em(style, 0.0, 0.0, 0.5, 0.0);
	psy_ui_style_set_padding_em(style, 0.5, 0.0, 0.5, 1.0);
	psy_ui_styles_set_style(self, STYLE_HEADER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_border_init_right(&style->border, psy_ui_BORDER_SOLID,
		material.surface);
	psy_ui_styles_set_style(self, STYLE_NAVIGATION, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_12p);
	psy_ui_styles_set_style(self, STYLE_CLIPBOX, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, psy_ui_colour_red());
	psy_ui_border_init_solid(&style->border,
		material.surface_overlay_12p);
	psy_ui_styles_set_style(self, STYLE_CLIPBOX_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_styles_set_style(self, STYLE_LABELPAIR, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_styles_set_style(self, STYLE_LABELPAIR_FIRST, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 2);
	psy_ui_styles_set_style(self, STYLE_LABELPAIR_SECOND, "", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON, "", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_style_set_colour(style, psy_ui_colour_make_argb(0xFFF6B87F));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON_WARNING, "", style);

	style = psy_ui_style_clone(psy_ui_styles_at(self, psy_ui_STYLE_BUTTON));
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x007966CF));
	psy_ui_styles_set_style(self, STYLE_TERM_BUTTON_ERROR, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_BGMAIN);	
	psy_ui_style_animate_background(style, 20,
		psy_ui_realsize_make(96, 192), TRUE);
	psy_ui_styles_set_style(self, STYLE_GREET, "", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_BGTOP);	
	psy_ui_style_animate_background(style, 40,
		psy_ui_realsize_make(128, 128), TRUE);
	psy_ui_styles_set_style(self, STYLE_GREET_TOP, "", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsecondary, material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_DIALOG_BUTTON_SELECT, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xFF5555));
	psy_ui_styles_set_style(self, STYLE_FILEBOX_DIR_LINE,
		"filebox_dir_pane", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(self, STYLE_FILEBOX_FILE_LINE, "", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_padding_em(style, 0.2, 0.0, 0.2, 0.0);
	psy_ui_style_set_margin_em(style, 0.3, 0.0, 0.0, 0.0);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x10BA00));
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_styles_set_style(self, STYLE_FILEVIEW_DIRBAR,
		"fileview_dirbar", style);	
		
	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_padding_em(style, 0.5, 2.0, 0.5, 2.0);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0xFF5555));
	psy_ui_styles_set_style(self, STYLE_FILEVIEW_LINKS,
		"fileview_links", style);	

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_colour(style, material.surface_overlay_5p);
	// psy_ui_style_set_colours(style, material.onprimary, material.surface);
	psy_ui_styles_set_style(self, STYLE_SIDE_VIEW, "sideview", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onprimary, material.surface);
	psy_ui_style_set_padding_em(style, 0.5, 0.5, 0.0, 0.5);
	psy_ui_styles_set_style(self, STYLE_CLIENT_VIEW, "clientview",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_7p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_styles_set_style(self, STYLE_BOX, "box", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_9p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_styles_set_style(self, STYLE_BOX_MEDIUM, "box-medium", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid(&style->border, material.surface_overlay_16p);
	psy_ui_border_setradius_px(&style->border, 6.0);
	psy_ui_styles_set_style(self, STYLE_BOX_BRIGHT, "box-bright", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, material.onprimary_medium);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_7p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_overlay(style, 4);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_11p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON_HOVER, "", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style, material.onsecondary, material.overlay_4p);
	psy_ui_border_init_solid_radius(&style->border, material.surface_overlay_16p, 6.0);
	psy_ui_style_set_padding_em(style, 0.25, 1.0, 0.25, 1.0);
	psy_ui_styles_set_style(self, STYLE_NAVBAR_BUTTON_SELECT, "", style);

	init_envelope_styles(self);		
}

void init_envelope_styles(psy_ui_Styles* self)
{
	psy_ui_Style* style;

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_size(style, psy_ui_size_make_em(20.0, 15.0));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE, "envelope", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00B1C8B0));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE_POINT, "envelope_point",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00B1C8B0));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE_CURVE, "envelope_curve",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00333333));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE_GRID, "envelope_grid",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00516850));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE_SUSTAIN, "envelope_sustain",
		style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x00434343));
	psy_ui_styles_set_style(self, STYLE_ENVELOPE_RULER, "envelope_ruler",
		style);
}
