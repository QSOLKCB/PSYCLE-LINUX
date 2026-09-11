/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(UINAVHANDLER_H)
#define UINAVHANDLER_H

/* local */
#include "uidef.h"
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_ui_Component;
struct InputHandler;

typedef struct psy_ui_NavHandler {	
	psy_Signal signal_selected;
	/* internal */	
	bool send_cmd;	
} psy_ui_NavHandler;

void psy_ui_navhandler_init(psy_ui_NavHandler*);
void psy_ui_navhandler_dispose(psy_ui_NavHandler*);

void psy_ui_navhandler_connect(psy_ui_NavHandler*, void* context, void* fp);
void psy_ui_navhandler_execute(psy_ui_NavHandler*, uintptr_t id);
void psy_ui_navhandler_start(psy_ui_NavHandler*);


#ifdef __cplusplus
}
#endif

#endif /* UINAVHANDLER_H */
