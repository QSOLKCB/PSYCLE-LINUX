/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luaconfig.h"
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>


static int psy_lua_config_create(lua_State* L);
static int psy_lua_config_gc(lua_State* L);

const char* psy_luac_config_meta = "psyconfigmeta";

int psy_lua_config_open(lua_State* L)
{
	static const luaL_Reg pm_lib[] = {
		{ NULL, NULL }
	};
	static const luaL_Reg pm_meta[] = {
	  {"new", psy_lua_config_create},
	  { "__gc", psy_lua_config_gc },
	  {NULL, NULL}
	};
	luaL_newmetatable(L, psy_luac_config_meta);
	luaL_setfuncs(L, pm_meta, 0);
	lua_pop(L, 1);
	luaL_newlib(L, pm_lib);
	return 1;
}

int psy_lua_config_create(lua_State* L)
{
	/*psy_ui_Point** udata;
	int n = lua_gettop(L);

	if (n == 1) {
		udata = (psy_ui_Point**)lua_newuserdata(L, sizeof(psy_ui_Point*));
		luaL_setmetatable(L, psy_luac_config_meta);
		else if (n == 2) {
		  Point::Ptr other = LuaHelper::check_sptr<Point>(L, 2, LuaPointBind::meta);
		  LuaHelper::new_shared_userdata<>(L, meta, new Point(*other.get()));
		} else
		if (n == 3) {
		  double x = luaL_checknumber(L, 2);
		  double y = luaL_checknumber(L, 3);
		  LuaHelper::new_shared_userdata<>(L, meta, new Point(x, y));
	}
	else {
		luaL_error(L, "Wrong number of arguments");
	}*/
	return 1;
}

int psy_lua_config_gc(lua_State* L)
{
	/* psy_ui_Point* ptr;

	ptr = *(psy_ui_Point**)luaL_checkudata(L, 1, psy_luac_config_meta);
	free(ptr); */
	return 0;
}
