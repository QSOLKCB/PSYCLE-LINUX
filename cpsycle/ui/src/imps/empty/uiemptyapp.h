/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_WINAPP_H
#define psy_ui_WINAPP_H

#include <list.h>
#include "../../uiapp.h"

#include "../../detail/stdint.h"
#include "../../detail/os.h"

#include <hashtbl.h>

#ifdef __cplusplus
extern "C" {
#endif


struct psy_ui_Component;

typedef struct psy_ui_EmptyApp {
	/* implements */
	psy_ui_AppImp imp;
	/*! @internal */
	int reserved;
	uintptr_t instance;		
	psy_Table selfmap;	
	psy_Table toplevelmap;
	psy_Table emptyidmap;
	uintptr_t emptyid;	
	psy_List* fonts;	
	/* references */
	psy_ui_App* app;
} psy_ui_EmptyApp;

void psy_ui_emptyapp_init(psy_ui_EmptyApp*, psy_ui_App* app, uintptr_t instance);


#ifdef __cplusplus
}
#endif

#endif /* psy_ui_WINAPP_H */
#endif /* PSYCLE_TK_EMPTY */
