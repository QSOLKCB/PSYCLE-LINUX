/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackerheaderstyles.h"

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
static void trackerheaderstyles_make_default(TrackerHeaderStyles*);
static void trackerheaderstyles_make_default_em(TrackerHeaderStyles*);
static void trackerheaderstyles_set_source(psy_ui_RealRectangle* r,
	intptr_t vals[4]);
static void trackerheaderstyles_set_dest(psy_ui_RealPoint* pt,
	intptr_t vals[4], uintptr_t num);
static void trackerheaderstyles_set_style_coords(uintptr_t styleid,
	uintptr_t select_styleid, psy_ui_RealRectangle src,
	psy_ui_RealPoint dst);
	
/* implementation */
void trackerheaderstyles_init(TrackerHeaderStyles* self,
	psy_Configuration* dir_cfg)
{
	assert(self);
	assert(dir_cfg);
	
	self->dir_cfg = dir_cfg;
	trackerheaderstyles_make_default(self);
	trackerheaderstyles_set_classic(self);
}

void trackerheaderstyles_make_default(TrackerHeaderStyles* self)
{
	psy_ui_Styles* styles;	
	psy_ui_Style* style;	
			
	styles = &psy_ui_app_defaults()->styles;
	
	assert(self);	
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(102.0, 23.0));
	psy_ui_style_set_background_position_px(style, -2.0, -0.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER,
		"pv_track_classic_header", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00999999));
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(102.0, 23.0));
	psy_ui_style_set_background_position_px(style, -2.0, -0.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SELECT,
		"pv_track_classic_header_select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(9, 17));
	psy_ui_style_set_background_position_px(style, 0.0, -23.0);
	psy_ui_style_set_padding_px(style, 3.0, 0.0, 0.0, 15.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_DIGITX0,
		"pv_track_classic_header_digitx0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style, psy_ui_realsize_make(9, 17));
	psy_ui_style_set_background_position_px(style, 0.0, -23.0);
	psy_ui_style_set_padding_px(style, 3.0, 0.0, 0.0, 22.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_DIGIT0X,
		"pv_track_classic_header_digit0x", style);

	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(66.0, 3.0), psy_ui_size_make_px(17.0, 17.)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE,
		"pv_track_classic_header_mute", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL,
		"pv_track_classic_header_mute_label", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL_SELECT,
		"pv_track_classic_header_mute_label_select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);	
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17.0, 17.0));
	psy_ui_style_set_background_position_px(style, -79.0, -40.0);	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(66.0, 3.0), psy_ui_size_make_px(17.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_SELECT,
		"pv_track_classic_header_mute::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(47.0, 3.0), psy_ui_size_make_px(17.0, 17.)));		
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO,
		"pv_track_classic_header_solo", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL,
		"pv_track_classic_header_solo_label", style);
				
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL_SELECT,
		"pv_track_classic_header_solo_label_select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17.0, 17.0));
	psy_ui_style_set_background_position_px(style, -62, -40.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(47.0, 3.0), psy_ui_size_make_px(17.0, 17.)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_SELECT,
		"pv_track_classic_header_solo::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(85.0, 3.0), psy_ui_size_make_px(17.0, 17.)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD,
		"pv_track_classic_header_record", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL,
		"pv_track_classic_header_record_label", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL_SELECT,
		"pv_track_classic_header_record_label_select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);	
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17.0, 17.0));
	psy_ui_style_set_background_position_px(style, -96.0, -40.0);	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(85.0, 3.0), psy_ui_size_make_px(17.0, 17.)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_SELECT,
		"pv_track_classic_header_record::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(5.0, 3.0), psy_ui_size_make_px(8.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_PLAY,
		"pv_track_classic_header_play", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_repeat(style, psy_ui_backgroundrepeat_make_all(psy_ui_NOREPEAT, psy_ui_NOREPEAT));
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(8.0, 17.0));
	psy_ui_style_set_background_position_px(style, 0.0, -40.0);	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(5.0, 3.0), psy_ui_size_make_px(8.0, 17.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT,
		"pv_track_classic_header_play::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_TEXT,
		"pv_track_classic_header_text", style);	
	
	/* text */
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(102.0, 23.0));
	psy_ui_style_set_background_position_px(style, -2.0, -57.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER,
		"pv_track_text_header", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style, psy_ui_realsize_make(6, 12));
	psy_ui_style_set_background_position_px(style, -0.0, -80.0);
	psy_ui_style_set_padding_px(style, 8.0, 0.0, 0.0, 5.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_DIGITX0,
		"pv_track_text_header_digitx0", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(6, 12));
	psy_ui_style_set_background_position_px(style, 0.0, -80.0);
	psy_ui_style_set_padding_px(style, 8.0, 0.0, 0.0, 11.0);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_DIGIT0X,
		"pv_track_text_header_digit0x", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(66.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_MUTE,
		"pv_track_text_header_mute", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style, psy_ui_realsize_make(17.0, 4.0));
	psy_ui_style_set_background_position_px(style, -29.0 - 2, -92.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(66.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_MUTE_SELECT,
		"pv_track_text_header_mute::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(47.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SOLO,
		"pv_track_text_header_solo", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style,
		psy_ui_realsize_make(17.0 - 2, 4.0));
	psy_ui_style_set_background_position_px(style, -12.0, -92.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(47.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SOLO_SELECT,
		"pv_track_text_header_solo::select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(85.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_RECORD,
		"pv_track_text_header_record", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
	psy_ui_style_set_background_size_px(style, psy_ui_realsize_make(17.0, 4.0));
	psy_ui_style_set_background_position_px(style, -46.0, -92.0);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(85.0, 3.0), psy_ui_size_make_px(17.0, 4.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_RECORD_SELECT,
		"pv_track_text_header_record::select", style);

	style = psy_ui_style_clone(psy_ui_styles_at(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_PLAY));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_PLAY,
		"pv_track_text_header_play", style);

	style = psy_ui_style_clone(psy_ui_styles_at(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_PLAY_SELECT,
		"pv_track_text_header_play::select", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_px(18.0, 8.0), psy_ui_size_make_px(84.0, 13.0)));
#if defined(DIVERSALIS__OS__UNIX)
		psy_ui_style_set_font(style, "FreeSans", 10);
#else
		psy_ui_style_set_font(style, "Tahoma", 12);
#endif
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_TEXT,
		"pv_track_text_header_text", style);				
}

void trackerheaderstyles_make_default_em(TrackerHeaderStyles* self)
{
	psy_ui_Styles* styles;
	psy_ui_Style* style;	
	psy_ui_Size btn_size;
	double top;
		
	assert(self);
	
	btn_size = psy_ui_size_make_em(3.0, 1.2);
	top = 0.20;
	
	styles = &psy_ui_app_defaults()->styles;
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(20.0, 1.5));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER,
		"pv_track_classic_header", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(20.0, 1.5));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0xFFFFFF));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SELECT,
		"pv_track_classic_header_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(3.0, top + 0.15),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_DIGITX0,
		"pv_track_classic_header_digitx0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(4.5, top + 0.15),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_DIGIT0X,
		"pv_track_classic_header_digit0x", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO,
		"pv_track_classic_header_solo", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL,
		"pv_track_classic_header_solo_label", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL_SELECT,
		"pv_track_classic_header_solo_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4DFFA6));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_SELECT,
		"pv_track_classic_header_solo::select", style);

	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE,
		"pv_track_classic_header_mute", style);
		
	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL,
		"pv_track_classic_header_mute_label", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL_SELECT,
		"pv_track_classic_header_mute_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4DD2FF));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_SELECT,
		"pv_track_classic_header_mute::select", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD,
		"pv_track_classic_header_record", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top + 0.1), btn_size));	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL,
		"pv_track_classic_header_record_label", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL_SELECT,
		"pv_track_classic_header_record_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4D4DFF));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_SELECT,
		"pv_track_classic_header_record::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, top),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x333333));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_PLAY,
		"pv_track_classic_header_play", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x009B7800));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, top),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT,
		"pv_track_classic_header_play::select", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_CLASSIC_HEADER_TEXT,
		"pv_track_classic_header_text", style);	

	/* text */
	
	btn_size = psy_ui_size_make_em(3.0, 0.3);
	top = 0.20;
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(20.0, 1.5));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER,
		"pv_track_text_header", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_size(style, psy_ui_size_make_em(20.0, 1.5));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0xFFFFFF));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SELECT,
		"pv_track_text_header_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, top + 0.35),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_DIGITX0,
		"pv_track_text_header_digitx0", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(2.5, top + 0.35),
		psy_ui_size_make(psy_ui_value_make_ew(1.0), btn_size.height)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_DIGIT0X,
		"pv_track_text_header_digit0x", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SOLO,
		"pv_track_text_header_solo", style);
		
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SOLO_LABEL,
		"pv_track_text_header_solo_label", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_TEXT_HEADER_SOLO_LABEL_SELECT,
		"pv_track_text_header_solo_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4DFFA6));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_SOLO_SELECT,
		"pv_track_text_header_solo::select", style);

	style = psy_ui_style_alloc_init();		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_MUTE,
		"pv_track_text_header_mute", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_MUTE_LABEL,
		"pv_track_text_header_mute_label", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_TEXT_HEADER_MUTE_LABEL_SELECT,
		"pv_track_text_header_mute_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4DD2FF));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(13.2, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_MUTE_SELECT,
		"pv_track_text_header_mute::select", style);
	
	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_RECORD,
		"pv_track_text_header_record", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top + 0.1), btn_size));	
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_RECORD_LABEL,
		"pv_track_text_header_record_label", style);
		
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_display(style, psy_ui_DISPLAY_NONE);
	psy_ui_style_set_colour(style, psy_ui_colour_make(0x333333));		
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(10.0, top + 0.1), btn_size));
	psy_ui_styles_set_style(styles,
		STYLE_PV_TRACK_TEXT_HEADER_RECORD_LABEL_SELECT,
		"pv_track_text_header_record_label_select", style);

	style = psy_ui_style_alloc_init();
	psy_ui_style_set_colours(style,
		psy_ui_colour_make(0x333333),
		psy_ui_colour_make(0x4D4DFF));	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(16.4, top), btn_size));
	psy_ui_border_init_solid(&style->border, psy_ui_colour_make(0x00696969));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_RECORD_SELECT,
		"pv_track_text_header_record::select", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, top), btn_size));
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x333333));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_PLAY,
		"pv_track_text_header_play", style);

	style = psy_ui_style_alloc_init();	
	psy_ui_style_set_background_colour(style, psy_ui_colour_make(0x009B7800));
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(1.0, top),btn_size ));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_PLAY_SELECT,
		"pv_track_text_header_play::select", style);
	
	style = psy_ui_style_alloc_init();
	psy_ui_style_set_position(style, psy_ui_rectangle_make(
		psy_ui_point_make_em(5.0, top + 0.35),
		psy_ui_size_make_em(14.0, 1.0)));
	psy_ui_styles_set_style(styles, STYLE_PV_TRACK_TEXT_HEADER_TEXT,
		"pv_track_text_header_text", style);	
}

void trackerheaderstyles_set_classic(TrackerHeaderStyles* self)
{		
	static const int header_classic_styles[] = {
		STYLE_PV_TRACK_CLASSIC_HEADER,
		STYLE_PV_TRACK_CLASSIC_HEADER_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_DIGITX0,
		STYLE_PV_TRACK_CLASSIC_HEADER_DIGIT0X,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_LABEL_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_LABEL_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_LABEL_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_PLAY,
		STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_TEXT,
		0
	};

	assert(self);
	
	trackerheaderstyles_switch(self, header_classic_styles);
}

void trackerheaderstyles_set_text(TrackerHeaderStyles* self)
{
	static const int header_text_styles[] = {
		STYLE_PV_TRACK_TEXT_HEADER,
		STYLE_PV_TRACK_TEXT_HEADER_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_DIGITX0,
		STYLE_PV_TRACK_TEXT_HEADER_DIGIT0X,
		STYLE_PV_TRACK_TEXT_HEADER_MUTE,
		STYLE_PV_TRACK_TEXT_HEADER_MUTE_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_MUTE_LABEL,
		STYLE_PV_TRACK_TEXT_HEADER_MUTE_LABEL_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_SOLO,
		STYLE_PV_TRACK_TEXT_HEADER_SOLO_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_SOLO_LABEL,
		STYLE_PV_TRACK_TEXT_HEADER_SOLO_LABEL_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_RECORD,
		STYLE_PV_TRACK_TEXT_HEADER_RECORD_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_RECORD_LABEL,
		STYLE_PV_TRACK_TEXT_HEADER_RECORD_LABEL_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_PLAY,
		STYLE_PV_TRACK_TEXT_HEADER_PLAY_SELECT,
		STYLE_PV_TRACK_TEXT_HEADER_TEXT,
		0
	};
	
	assert(self);
		
	trackerheaderstyles_switch(self, header_text_styles);	
}

void trackerheaderstyles_switch(TrackerHeaderStyles* self,
	const int* src)
{	
	psy_ui_Styles* styles;
	uintptr_t i;
	static const int header_styles[] = {
		STYLE_PV_TRACK_HEADER,
		STYLE_PV_TRACK_HEADER_SELECT,
		STYLE_PV_TRACK_HEADER_DIGITX0,
		STYLE_PV_TRACK_HEADER_DIGIT0X,
		STYLE_PV_TRACK_HEADER_MUTE,
		STYLE_PV_TRACK_HEADER_MUTE_SELECT,
		STYLE_PV_TRACK_HEADER_MUTE_LABEL,
		STYLE_PV_TRACK_HEADER_MUTE_LABEL_SELECT,
		STYLE_PV_TRACK_HEADER_SOLO,
		STYLE_PV_TRACK_HEADER_SOLO_SELECT,
		STYLE_PV_TRACK_HEADER_SOLO_LABEL,
		STYLE_PV_TRACK_HEADER_SOLO_LABEL_SELECT,
		STYLE_PV_TRACK_HEADER_RECORD,
		STYLE_PV_TRACK_HEADER_RECORD_SELECT,
		STYLE_PV_TRACK_HEADER_RECORD_LABEL,
		STYLE_PV_TRACK_HEADER_RECORD_LABEL_SELECT,
		STYLE_PV_TRACK_HEADER_PLAY,
		STYLE_PV_TRACK_HEADER_PLAY_SELECT,
		STYLE_PV_TRACK_HEADER_TEXT,
		0
	};
	
	assert(self);
	
	styles = &psy_ui_app_defaults()->styles;
	for (i = 0; header_styles[i] != 0; ++i) {
		psy_ui_Style* style;
		
		if (src[i] == 0) {
			break;
		}
		style = psy_ui_style_clone(psy_ui_style(src[i]));
		psy_ui_styles_set_style(styles, header_styles[i], style->name, style);
	}	
}

void trackerheaderstyles_load(TrackerHeaderStyles* self,
	const char* header_skin_name)
{	
	static int styles[] = {
		STYLE_PV_TRACK_CLASSIC_HEADER,
		STYLE_PV_TRACK_CLASSIC_HEADER_DIGITX0,
		STYLE_PV_TRACK_CLASSIC_HEADER_DIGIT0X,
		STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_SELECT,
		STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT,
		0
	};
		
	assert(self);
	
	if (psy_strlen(header_skin_name) == 0) {
		trackerheaderstyles_make_default(self);
		trackerheaderstyles_set_classic(self);		
	} else {
		if (strcmp(header_skin_name, "default-em") == 0) {
			trackerheaderstyles_make_default_em(self);
			trackerheaderstyles_set_classic(self);			
		} else {
			psy_Path filename;
			char path[_MAX_PATH];
			const char* skindir;
					
			skindir = psy_configuration_value_str(self->dir_cfg,
				"skins", PSYCLE_SKINS_DEFAULT_DIR);		
			psy_path_init_all(&filename, "", header_skin_name, "bmp");		
			psy_dir_find_file(skindir, psy_path_full(&filename), path);
			/*
			** printf("search pattern header skin bmp\n");
			** printf("%s\n", skindir);
			** printf("%s\n", psy_path_full(&filename));
			** printf("%s\n", path);
			*/
			if (path[0] != '\0') {			
				uintptr_t i;

				for (i = 0; styles[i] != 0; ++i) {
					psy_ui_Style* style;

					style = psy_ui_style(styles[i]);
					if (psy_ui_style_setbackgroundpath(style, path) != PSY_OK) {
						psy_ui_style_set_background_id(style, IDB_HEADERSKIN);
					}
				}
			}
			psy_path_set_ext(&filename, "psh");
			psy_dir_find_file(skindir, psy_path_full(&filename), path);
			psy_path_dispose(&filename);
			if (psy_strlen(path) > 0) {
				psy_Property* coords;

				coords = psy_property_allocinit_key(NULL);
				if (skin_load_pattern_header(coords, path) == PSY_OK) {
					const char* s;
					intptr_t vals[4];
					psy_ui_RealRectangle src;
					psy_ui_RealPoint dst;
					psy_ui_Style* style;
					psy_Property* transparency;

					src = psy_ui_realrectangle_zero();
					dst = psy_ui_realpoint_zero();
					if ((s = psy_property_at_str(coords, "background_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);
						style = psy_ui_style(STYLE_PV_TRACK_CLASSIC_HEADER);
						psy_ui_style_set_background_size_px(style,
							psy_ui_realsize_make(
								src.right - src.left,
								src.bottom - src.top));
						psy_ui_style_set_background_position_px(style, -src.left, -src.top);
					}
					if ((s = psy_property_at_str(coords, "mute_on_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);
					}
					if ((s = psy_property_at_str(coords, "mute_on_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);
						trackerheaderstyles_set_style_coords(
							STYLE_PV_TRACK_CLASSIC_HEADER_MUTE,
							STYLE_PV_TRACK_CLASSIC_HEADER_MUTE_SELECT,
							src, dst);
					}
					if ((s = psy_property_at_str(coords, "solo_on_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);
					}
					if ((s = psy_property_at_str(coords, "solo_on_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);
						trackerheaderstyles_set_style_coords(
							STYLE_PV_TRACK_CLASSIC_HEADER_SOLO,
							STYLE_PV_TRACK_CLASSIC_HEADER_SOLO_SELECT,
							src, dst);
					}
					if ((s = psy_property_at_str(coords, "record_on_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);
					}
					if ((s = psy_property_at_str(coords, "record_on_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);
						trackerheaderstyles_set_style_coords(
							STYLE_PV_TRACK_CLASSIC_HEADER_RECORD,
							STYLE_PV_TRACK_CLASSIC_HEADER_RECORD_SELECT,
							src, dst);
					}
					if ((s = psy_property_at_str(coords, "number_0_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);
					}
					if ((s = psy_property_at_str(coords, "digit_x0_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);
						style = psy_ui_style(STYLE_PV_TRACK_CLASSIC_HEADER_DIGITX0);
						psy_ui_style_set_background_size_px(style,
							psy_ui_realrectangle_size(&src));
						psy_ui_style_set_background_position_px(style, -src.left, -src.top);
						psy_ui_style_set_padding_px(style, dst.y, 0.0, 0.0, dst.x);
					}
					if ((s = psy_property_at_str(coords, "digit_0x_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);
						style = psy_ui_style(STYLE_PV_TRACK_CLASSIC_HEADER_DIGIT0X);
						psy_ui_style_set_background_size_px(style,
							psy_ui_realrectangle_size(&src));
						psy_ui_style_set_background_position_px(style, -src.left, -src.top);
						psy_ui_style_set_padding_px(style, dst.y, 0.0, 0.0, dst.x);
					}
					if ((s = psy_property_at_str(coords, "playing_on_source", 0))) {
						skin_psh_values(s, 4, vals);
						trackerheaderstyles_set_source(&src, vals);					
					}
					if ((s = psy_property_at_str(coords, "playing_on_dest", 0))) {
						skin_psh_values(s, 2, vals);
						trackerheaderstyles_set_dest(&dst, vals, 2);					
						trackerheaderstyles_set_style_coords(
							STYLE_PV_TRACK_CLASSIC_HEADER_PLAY,
							STYLE_PV_TRACK_CLASSIC_HEADER_PLAY_SELECT,
							src, dst);									
					}
					if ((transparency = psy_property_at(coords, "transparency",
						PSY_PROPERTY_TYPE_NONE))) {
						if (transparency->item.marked) {
							psy_ui_Colour cltransparency;
							int i;

							cltransparency = psy_ui_colour_make(
								strtol(psy_property_item_str(transparency),
									0, 16));
							for (i = 0; styles[i] != 0; ++i) {
								style = psy_ui_style(styles[i]);
								psy_ui_bitmap_set_transparency(
									&style->background.bitmap, cltransparency);
							}
						}
					}												
				}
				psy_property_deallocate(coords);
			}
		}
	}	
}

void trackerheaderstyles_set_source(psy_ui_RealRectangle* r, intptr_t vals[4])
{
	r->left = (double)vals[0];
	r->top = (double)vals[1];
	r->right = (double)vals[0] + (double)vals[2];
	r->bottom = (double)vals[1] + (double)vals[3];
}

void trackerheaderstyles_set_dest(psy_ui_RealPoint* pt, intptr_t vals[4],
	uintptr_t num)
{
	pt->x = (double)vals[0];
	pt->y = (double)vals[1];
}

void trackerheaderstyles_set_style_coords(uintptr_t styleid,
	uintptr_t select_styleid, psy_ui_RealRectangle src, psy_ui_RealPoint dst)
{
	psy_ui_Style* style;
	psy_ui_Point pt;
	psy_ui_Size size;

	size = psy_ui_size_make_real(psy_ui_realrectangle_size(&src));
	pt = psy_ui_point_make_real(dst);
	style = psy_ui_style(styleid);
	if (style) {
		psy_ui_style_set_position(style, psy_ui_rectangle_make(pt, size));
	}
	style = psy_ui_style(select_styleid);
	if (style) {
		psy_ui_style_set_background_position_px(style, -src.left, -src.top);
		psy_ui_style_set_background_size_px(style,
			psy_ui_realrectangle_size(&src));
		psy_ui_style_set_position(style, psy_ui_rectangle_make(pt, size));
	}
}

#endif /* PSYCLE_USE_TRACKERVIEW */
