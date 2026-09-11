/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_empty_COMPONENTIMP_H
#define psy_ui_empty_COMPONENTIMP_H

#include "../../uicomponent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_empty_ComponentImp {
	psy_ui_ComponentImp imp;
	struct psy_ui_Component* component;
	uintptr_t hwnd;
	uintptr_t emptyid;	
	psy_ui_Point topleftcache;
	bool topleftcachevalid;
	psy_ui_Size sizecache;
	bool sizecachevalid;	
	bool visible;	
	psy_List* viewcomponents;
	bool fullscreen;
	int restore_style;
	int restore_exstyle;
	char* title;
	psy_ui_RealRectangle position;
	psy_ui_Component* parent;
} psy_ui_empty_ComponentImp;

void psy_ui_empty_componentimp_init(psy_ui_empty_ComponentImp* self,
	struct psy_ui_Component* component,
	psy_ui_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand);

psy_ui_empty_ComponentImp* psy_ui_empty_componentimp_alloc(void);
psy_ui_empty_ComponentImp* psy_ui_empty_componentimp_allocinit(
	struct psy_ui_Component* component,
	psy_ui_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_empty_ComponentImp_H */
#endif /* PSYCLE_TK_EMPTY */
