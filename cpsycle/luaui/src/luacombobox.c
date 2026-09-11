/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luacombobox.h"
#include "luacomponent.h"
#include <lauxlib.h>
#include <lualib.h>
#include <psyclescript.h>

/* ui */
#include <uicombobox.h>
#include <uiapp.h>

static int psy_luaui_combobox_create(lua_State*);
static int psy_luaui_combobox_gc(lua_State*);

const char* psy_luaui_combobox_meta = "psycombobox";

int psy_luaui_combobox_open(lua_State *L)
{
	static const luaL_Reg methods[] = {
		{"new", psy_luaui_combobox_create},
		{ NULL, NULL }		
	};
	return psyclescript_open(L, psy_luaui_combobox_meta, methods,
		psy_luaui_combobox_gc, NULL);
}

int psy_luaui_combobox_create(lua_State* L)
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
		psy_ui_ComboBox* combobox;
		psy_GCPtr* ptr;
				
		ptr = psyclescript_testudata(L, 2, psy_luaui_component_meta);
		parent = (psy_ui_Component*)ptr->ud;		
		combobox = psy_ui_combobox_alloc_init(parent);
		psy_ui_combobox_add_text(combobox, "combobox");
		psy_ui_component_set_align(psy_ui_combobox_base(combobox),
			psy_ui_ALIGN_LEFT);
		psyclescript_createuserdata(L, 1, psy_luaui_combobox_meta, combobox,
			NULL);
	} else {
		luaL_error(L, "Wrong number of arguments");
	}
	return 1;
}

int psy_luaui_combobox_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_combobox_meta);
	return 0;
}
