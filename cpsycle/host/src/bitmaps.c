/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "bitmaps.h"
/* host */
#include "svgs.h"
#include "./resources/resource.h"
/* ui */
#include <uiapp.h>
/* portable */
#include "../../detail/portable.h"


/* implementation */
void register_bitmaps(psy_ui_App* app, const char* app_bmp_path)
{
	psy_icons_make(&app->resources);
	if (psy_strlen(app_bmp_path) == 0) {
		return;
	}
	psy_ui_app_set_bmp_path(app, app_bmp_path);		
	psy_ui_app_add_app_bmp(app, IDB_MACHINESKIN, "machine_skin.bmp");
	psy_ui_app_add_app_bmp(app, IDB_PARAMKNOB, "TbMainKnob.bmp");
	psy_ui_app_add_app_bmp(app, IDB_ABOUT, "splash_screen.bmp");
	psy_ui_app_add_app_bmp(app, IDB_HEADERSKIN, "pattern_header_skin.bmp");
	psy_ui_app_add_app_bmp(app, IDB_MIXERSKIN, "mixer_skin.bmp");
	psy_ui_app_add_app_bmp(app, IDI_PSYCLEICON, "");
	psy_ui_app_add_app_bmp(app, IDI_MACPARAM, "");	
	psy_ui_app_add_app_bmp(app, IDB_BGMAIN_1, "");
	psy_ui_app_add_app_bmp(app, IDB_BGMAIN, "bgmain.bmp");
	psy_ui_app_add_app_bmp(app, IDB_BGTOP, "bgtop.bmp");	
	psy_ui_app_add_app_bmp(app, IDB_SAMPULSE, "sampulse.bmp");
	psy_ui_app_add_app_bmp(app, IDI_PSYCLEICON, "icon1.bmp");
	psy_ui_app_add_app_bmp(app, IDI_MACPARAM, "macparam.bmp");	
	/* psy_ui_app_add_app_bmp(app, IDB_GUI, "gui.bmp"); */
}
