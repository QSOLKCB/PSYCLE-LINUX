/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luapoint.h"
#include <lauxlib.h>
#include <lualib.h>

#include <uigeometry.h>
#include <stdlib.h>
#include <psyclescript.h>

static int psy_luaui_point_create(lua_State*);
static int psy_luaui_point_gc(lua_State*);

const char* psy_luaui_point_meta = "psypointmeta";

int psy_luaui_point_open(lua_State *L)
{  
    static const luaL_Reg methods[] = {    
	{"new", psy_luaui_point_create},
	{NULL, NULL}
  };
  return psyclescript_open(L, psy_luaui_point_meta, methods,
      psy_luaui_point_gc, NULL);
}

int psy_luaui_point_create(lua_State* L)
{  
  int n = lua_gettop(L);

  if (n == 1) {
      psyclescript_createuserdata(L, 1, psy_luaui_point_meta, 
	psy_ui_realpoint_alloc_init(),
	(psy_fp_gcptr_dispose)psy_ui_realpoint_dispose);	  
  } else if (n == 2) {
    psy_GCPtr* ptr;
    psy_ui_RealPoint* new_pt;
    psy_ui_RealPoint* other;
    
    ptr = psyclescript_check(L, 2, psy_luaui_point_meta);
    other = (psy_ui_RealPoint*)(ptr->ud);
    new_pt = psy_ui_realpoint_alloc_init();
    new_pt->x = other->x;
    new_pt->y = other->y;
    psyclescript_createuserdata(L, 1, psy_luaui_point_meta, new_pt,
	(psy_fp_gcptr_dispose)psy_ui_realpoint_dispose);    
  } else if (n == 3) {
    double x = luaL_checknumber(L, 2);
    double y = luaL_checknumber(L, 3);    
    psy_ui_RealPoint* new_pt;    
            
    new_pt = psy_ui_realpoint_alloc_init();
    new_pt->x = x;
    new_pt->y = y;
    psyclescript_createuserdata(L, 1, psy_luaui_point_meta, new_pt,
	(psy_fp_gcptr_dispose)psy_ui_realpoint_dispose);
  } else {
    luaL_error(L, "Wrong number of arguments");
  }
  return 1;
}

int psy_luaui_point_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_point_meta);
	return 0;
}
