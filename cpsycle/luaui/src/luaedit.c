/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luacomponent.h"
#include <lauxlib.h>
#include <lualib.h>
#include <psyclescript.h>

/* ui */
#include <uitext.h>
#include <uiapp.h>

static int psy_luaui_edit_create(lua_State*);
static int psy_luaui_edit_gc(lua_State*);
static int psy_luaui_edit_set_text(lua_State*);
static int psy_luaui_edit_text(lua_State*);

const char* psy_luaui_edit_meta = "psy.ui.component.edit";

int psy_luaui_edit_open(lua_State *L)
{
	static const luaL_Reg methods[] = {
		{"new", psy_luaui_edit_create},
		{"settext", psy_luaui_edit_set_text},
		{"text", psy_luaui_edit_text},
		{ NULL, NULL }		
	};
	return psyclescript_open(L, psy_luaui_edit_meta, methods,
		psy_luaui_edit_gc, NULL);
}

int psy_luaui_edit_create(lua_State* L)
{  	
	int n = lua_gettop(L);

	if (n == 1) {
		/*psy_ui_label* component;
		psy_ui_label* parent;

		lua_getglobal(L, "psycle");
		lua_getfield(L, -1, "ui");
		parent = (psy_ui_label*)lua_touserdata(L, -1);
		lua_pop(L, 2);		
		component = psy_ui_label_allocinit(parent, NULL);
		psyclescript_createuserdata(L, -1, psy_luaui_label_meta, component);
		psy_ui_label_set_background_colour(component, psy_ui_colour_blue());
		psy_ui_label_set_id(component, 20);				
		psy_ui_label_select_section(parent, 20, 0);*/
	} else if (n == 2) {				
		psy_ui_Component* parent;
		psy_ui_Edit* edit;
		psy_GCPtr* ptr;
				
		ptr = psyclescript_testudata(L, 2, psy_luaui_component_meta);
		parent = (psy_ui_Component*)ptr->ud;		
		edit = psy_ui_edit_allocinit(parent);
		psy_ui_edit_set_text(edit, "Edit");
		psy_ui_component_set_align(psy_ui_edit_base(edit),
			psy_ui_ALIGN_LEFT);
		psyclescript_createuserdata(L, 1, psy_luaui_edit_meta, edit,
			NULL);
	} else {
		luaL_error(L, "Wrong number of arguments");
	}
	return 1;
}

int psy_luaui_edit_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_edit_meta);
	return 0;
}

int psy_luaui_edit_set_text(lua_State* L)
{
	psy_GCPtr* ptr;
	psy_ui_Edit* edit;

	ptr = psyclescript_check(L, 1, psy_luaui_edit_meta);
	edit = (psy_ui_Edit*)(ptr->ud);
	psy_ui_edit_set_text(edit, luaL_checkstring(L, 2));
	return psyclescript_chaining(L);
}

int psy_luaui_edit_text(lua_State* L)
{	
	psy_GCPtr* ptr;
	psy_ui_Edit* edit;

	ptr = psyclescript_check(L, 1, psy_luaui_edit_meta);
	edit = (psy_ui_Edit*)(ptr->ud);	
	lua_pushstring(L, psy_ui_edit_text(edit));
	return 1;
}
