/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

/* ui */
#include <uiapp.h>
#include "uiframe.h"
#include "uitabbar.h"


int psycle_run(uintptr_t instance, int options)
{
	int err = 0;		
	psy_ui_App app;
	psy_ui_Frame mainframe;
	psy_ui_Component pane;
	psy_ui_TabBar tabbar;
		
	psy_ui_app_init(&app, psy_ui_DARKTHEME, instance);	
	psy_ui_frame_init(&mainframe, NULL);
	psy_ui_app_set_main(psy_ui_app(), &mainframe);	
	psy_ui_component_init(&pane, &mainframe, &mainframe);	
	psy_ui_component_set_align(&pane, psy_ui_ALIGN_CLIENT);
	psy_ui_tabbar_init(&tabbar, &pane);	
	psy_ui_tabbar_append_tabs(&tabbar, "tab1",  "tab2", "tab3", NULL);
	psy_ui_component_set_align(psy_ui_tabbar_base(&tabbar),
		psy_ui_ALIGN_TOP);
	psy_ui_tabbar_select(&tabbar, 0);	
	psy_ui_component_show_state(&mainframe, options);	
	psy_ui_component_align(&mainframe);	
	err = psy_ui_app_run(&app);	
	psy_ui_app_dispose(&app);	
	return err;
}
