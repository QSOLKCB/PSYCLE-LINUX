/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


/* host */
#include "bitmaps.h"
#include "mainframe.h"
/* ui */
#include <uiapp.h>


int psycle_run(uintptr_t instance, int options, int argc, char** argv)
{	
	PsycleCmdLine cmdline;
	psy_ui_App app;
	MainFrame* frame;
	int err;
	psy_Path path;
			
	psy_path_init(&path, argv[0]);
	psy_chdir(psy_path_prefix(&path));
	psy_path_dispose(&path);
	psyclecmdline_init(&cmdline);
	psyclecmdline_parse(&cmdline, argc, argv);
	psy_ui_app_init(&app, psy_ui_DARKTHEME, instance);
	register_bitmaps(&app, PSYCLE_RES_DIR);	
	frame = mainframe_alloc_init(&cmdline);	
	if (!frame) {	
		fprintf(stderr, "psycle: Can't alloc MainFrame.\n");
		return 1;
	}	
	if (mainframe_show_maximized_at_start(frame)) {
		psy_ui_component_show_maximized(mainframe_base(frame));
	} else {
		psy_ui_component_show_state(mainframe_base(frame), options);
	}
	err = psy_ui_app_run(&app);		
	psy_ui_app_dispose(&app);
	psyclecmdline_dispose(&cmdline);
	return err;
}
