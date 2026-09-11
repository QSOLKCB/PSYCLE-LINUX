/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luabutton.h"
#include "luacomponent.h"
#include "luaimport.h"
#include <lauxlib.h>
#include <lualib.h>
#include <psyclescript.h>

/* ui */
#include <uibutton.h>
#include <uiapp.h>

#include <assert.h>

static int psy_luaui_button_create(lua_State*);
static int psy_luaui_button_gc(lua_State*);
static int psy_luaui_button_set_text(lua_State*);
static int psy_luaui_button_text(lua_State*);

static void psy_luaui_button_on_clicked(lua_State*, psy_ui_Button* sender);

const char* psy_luaui_button_meta = "psy.ui.component.button";

int psy_luaui_button_open(lua_State *L)
{
	 static const luaL_Reg methods[] = {
		{"new", psy_luaui_button_create},
		{"settext", psy_luaui_button_set_text},
		{"text", psy_luaui_button_text},
		{ NULL, NULL }		
	};
	return psyclescript_open(L, psy_luaui_button_meta, methods,
		psy_luaui_button_gc, NULL);
}

int psy_luaui_button_create(lua_State* L)
{  	
	int n = lua_gettop(L);

	if (n == 1) {
		/*psy_ui_button* component;
		psy_ui_button* parent;

		lua_getglobal(L, "psycle");
		lua_getfield(L, -1, "ui");
		parent = (psy_ui_button*)lua_touserdata(L, -1);
		lua_pop(L, 2);		
		component = psy_ui_button_alloc_init(parent, NULL);
		psyclescript_createuserdata(L, -1, psy_luaui_button_meta, component);
		psy_ui_button_set_background_colour(component, psy_ui_colour_blue());
		psy_ui_button_set_id(component, 20);				
		psy_ui_button_select_section(parent, 20, 0);*/
	} else if (n == 2) {				
		psy_ui_Component* parent;
		psy_ui_Button* button;
		psy_GCPtr* ptr;
				
		ptr = psyclescript_testudata(L, 2, psy_luaui_component_meta);
		parent = (psy_ui_Component*)ptr->ud;		
		button = psy_ui_button_alloc_init(parent);
		psy_ui_button_set_text(button, "button");
		psy_ui_component_set_align(psy_ui_button_base(button), psy_ui_ALIGN_LEFT);
		psyclescript_createuserdata(L, 1, psy_luaui_button_meta, button, NULL);
		psy_ui_button_connect(button, L, psy_luaui_button_on_clicked);
	} else {
		luaL_error(L, "Wrong number of arguments");
	}
	return 1;
}

int psy_luaui_button_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_button_meta);
	return 0;
}

int psy_luaui_button_set_text(lua_State* L)
{
	psy_GCPtr* ptr;
	psy_ui_Button* button;

	ptr = psyclescript_check(L, 1, psy_luaui_button_meta);
	button = (psy_ui_Button*)(ptr->ud);
	psy_ui_button_set_text(button, luaL_checkstring(L, 2));
	return psyclescript_chaining(L);
}

int psy_luaui_button_text(lua_State* L)
{
	psy_GCPtr* ptr;
	psy_ui_Button* button;

	ptr = psyclescript_check(L, 1, psy_luaui_button_meta);
	button = (psy_ui_Button*)(ptr->ud);
	lua_pushstring(L, psy_ui_button_text(button));	
	return 1;
}

void psy_luaui_button_on_clicked(lua_State* L, psy_ui_Button* sender)
{
	psy_LuaImport in;

	psy_luaimport_init(&in, L, sender, psyclescript_logger(L));
	if (psy_luaimport_open(&in, "onclicked")) {
		psy_luaimport_pcall(&in, 0);		
	}
	psy_luaimport_dispose(&in);	
}
