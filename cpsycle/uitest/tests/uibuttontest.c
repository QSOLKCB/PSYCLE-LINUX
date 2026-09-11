// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2020 members of the psycle project http://psycle.sourceforge.net

// platform
#include "../../detail/prefix.h"
#include "../../detail/os.h"
// host
#include "uiframe.h"
#include "uibutton.h"
// ui
#include <uiapp.h>
// file
#include <dir.h>
#include <signal.h>
// std
#include <stdio.h>


int psycle_run(uintptr_t instance, int options)
{
	int err = 0;	
	psy_ui_App app;
	psy_ui_Frame frame;
	psy_ui_Button button;
		
	// Initialize the ui
	psy_ui_app_init(&app, psy_ui_DARKTHEME, instance);
	// Creates the frame
	psy_ui_frame_init(&frame, NULL);
	app.main = &frame;
	psy_ui_button_init(&button, &frame, NULL);
	psy_ui_button_set_text(&button, "Hello Psycle");		
	psy_ui_component_set_align(psy_ui_button_base(&button), psy_ui_ALIGN_CLIENT);
	psy_ui_component_show_state(&frame, options);	
	// Starts the app event loop	
	err = psy_ui_app_run(&app);
	printf("Loop finished\n");
	// The event loop has finished, dispose any global ui resources
	psy_ui_app_dispose(&app);	
	return err;
}
