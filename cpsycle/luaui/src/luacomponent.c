/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luacomponent.h"
#include "luabutton.h"
#include "lualabel.h"
#include <lauxlib.h>
#include <lualib.h>
#include "luaimport.h"
#include <psyclescript.h>
#include "luagraphics.h"

/* ui */
#include <uicomponent.h>
#include <uiapp.h>

static int psy_luaui_component_create(lua_State*);
static int psy_luaui_component_gc(lua_State*);
static psy_ui_Component* psy_luaui_testbase(lua_State* L);

static int psy_luaui_component_set_align(lua_State*);
static int psy_luaui_component_show(lua_State*);
static int psy_luaui_component_hide(lua_State*);

static void psy_luaui_component_on_draw(lua_State*, psy_ui_Component* sender,
	psy_ui_Graphics*);

const char* psy_luaui_component_meta = "psy.ui.component";

int psy_luaui_component_open(lua_State *L)
{
	static const luaL_Reg pm_lib[] = {	
		{"new", psy_luaui_component_create},
		{"setalign", psy_luaui_component_set_align},
		{"show", psy_luaui_component_show},
		{"hide", psy_luaui_component_hide},
		{ NULL, NULL }
	};
  static const luaL_Reg pm_meta[] = {    
	{ "__gc", psy_luaui_component_gc },    
    {NULL, NULL}
  };
  luaL_newmetatable(L, psy_luaui_component_meta);
  luaL_setfuncs(L, pm_meta, 0);
  lua_pop(L, 1);
  luaL_newlib(L, pm_lib);
  return 1;  
}

void psy_luaui_component_set_methods(lua_State* L)
{
	static const luaL_Reg methods[] = {	  
	  {"setalign", psy_luaui_component_set_align},
	  {NULL, NULL}
	};
	luaL_setfuncs(L, methods, 0);
}

int psy_luaui_component_create(lua_State* L)
{  	
	int n = lua_gettop(L);

	if (n == 1) {
		psy_ui_Component* component;
		psy_ui_Component* parent;

		lua_getglobal(L, "psycle");
		lua_getfield(L, -1, "ui");
		parent = (psy_ui_Component*)lua_touserdata(L, -1);
		lua_pop(L, 2);		
		component = psy_ui_component_allocinit(parent, NULL);		
		psyclescript_createuserdata(L, 1, psy_luaui_component_meta, component, NULL);
		psy_ui_component_set_background_colour(component, psy_ui_colour_blue());
		psy_ui_component_set_id(component, 20);				
		psy_ui_component_select_section(parent, 20, 0);		
	
		psy_signal_connect(&component->signal_draw, L, psy_luaui_component_on_draw);
	} else if (n == 2) {				
		psy_ui_Component* parent;
		psy_GCPtr* ptr;
		psy_ui_Component* component;
		
		ptr = psyclescript_testudata(L, 2, psy_luaui_component_meta);
		parent = (psy_ui_Component*)ptr->ud;
		component = psy_ui_component_allocinit(parent, NULL);		
		psyclescript_createuserdata(L, 1, psy_luaui_component_meta, component, NULL);
		psy_ui_component_set_background_colour(component, psy_ui_colour_red());		
		psy_ui_component_set_align(component, psy_ui_ALIGN_CLIENT);

		psy_signal_connect(&component->signal_draw, L, psy_luaui_component_on_draw);
	} else {
		luaL_error(L, "Wrong number of arguments");
	}
	return 1;
}

int psy_luaui_component_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_component_meta);
	return 0;
}

int psy_luaui_component_set_align(lua_State* L)
{
	psy_ui_Component* component;

	component = psy_luaui_testbase(L);
	psy_ui_component_set_align(component,
		(psy_ui_AlignType)luaL_checkinteger(L, 2));
	return psyclescript_chaining(L);
}

int psy_luaui_component_show(lua_State* L)
{
	psy_ui_Component* component;

	component = psy_luaui_testbase(L);
	psy_ui_component_show(component);
	return psyclescript_chaining(L);
}

int psy_luaui_component_hide(lua_State* L)
{
	psy_ui_Component* component;

	component = psy_luaui_testbase(L);
	psy_ui_component_hide(component);
	return psyclescript_chaining(L);
}

psy_ui_Component* psy_luaui_testbase(lua_State* L)
{
	psy_GCPtr* ptr;	

	ptr = psyclescript_testudata(L, 1, psy_luaui_component_meta);
	if (ptr) {
		return (psy_ui_Component*)ptr->ud;
	}
	ptr = psyclescript_testudata(L, 1, psy_luaui_label_meta);
	if (ptr) {
		return (psy_ui_Component*)ptr->ud;
	}
	ptr = psyclescript_testudata(L, 1, psy_luaui_button_meta);
	if (ptr) {
		return (psy_ui_Component*)ptr->ud;
	}
	return NULL;
}

void psy_luaui_component_on_draw(lua_State* L, psy_ui_Component* sender,
	psy_ui_Graphics* g)
{
	psy_LuaImport in;
	
	psy_luaimport_init(&in, L, sender, psyclescript_logger(L));
	if (psy_luaimport_open(&in, "draw")) {
		psyclescript_requirenew(L, "psycle.ui.canvas.graphics", g,
			psy_luaui_graphics_open,
			psy_luaui_graphics_meta,
			NULL);
		psy_luaimport_pcall(&in, 0);
	}	
	psy_luaimport_dispose(&in);
	/* LuaImport in(L, this, locker(L));
	if (in.open("draw")) {
		LuaHelper::requirenew<LuaGraphicsBind>(L, "psycle.ui.canvas.graphics", g, true);
		LuaHelper::requirenew<LuaRegionBind>(L, "psycle.ui.region", &draw_rgn, true);
		in.pcall(0);
		//    LuaHelper::collect_full_garbage(L);
	}
	*/
}
