/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luafontinfo.h"
#include <lauxlib.h>
#include <lualib.h>

#include <uifont.h>
#include <psyclescript.h>

#include <stdlib.h>

static int psy_luaui_fontinfo_create(lua_State* L);
static int psy_luaui_fontinfo_gc(lua_State* L);

const char* psy_luaui_fontinfo_meta = "fontinfometa";

int psy_luaui_fontinfo_open(lua_State *L)
{
	static const luaL_Reg pm_lib[] = {	
		{ NULL, NULL }
	};
	static const luaL_Reg pm_meta[] = {
    {"new", psy_luaui_fontinfo_create},
	{ "__gc", psy_luaui_fontinfo_gc },
    {NULL, NULL}
  };
  luaL_newmetatable(L, psy_luaui_fontinfo_meta);
  luaL_setfuncs(L, pm_meta, 0);
  lua_pop(L, 1);
  luaL_newlib(L, pm_lib);
  return 1;  
}

int psy_luaui_fontinfo_create(lua_State* L)
{    
	int n = lua_gettop(L);

	if (n == 1) {
		psyclescript_createuserdata(L, 1, psy_luaui_fontinfo_meta, psy_ui_fontinfo_alloc_init(),
			NULL);
	} else if (n == 2) {
		psy_ui_FontInfo* font_info;

		font_info = psy_ui_fontinfo_alloc();
		const char* family_name = luaL_checkstring(L, 2);

		psy_ui_fontinfo_init(font_info, family_name, 18.0);
		psyclescript_createuserdata(L, 1, psy_luaui_fontinfo_meta, font_info, NULL);
	} else if (n == 3) {
		psy_ui_FontInfo* font_info;

		font_info = psy_ui_fontinfo_alloc();
		const char* family_name = luaL_checkstring(L, 2);
		lua_Integer size = luaL_checkinteger(L, 3);

		psy_ui_fontinfo_init(font_info, family_name, (double)size);
		psyclescript_createuserdata(L, 1, psy_luaui_fontinfo_meta, font_info, NULL);
	} else if (n == 4) {
		psy_ui_FontInfo* font_info;

		font_info = psy_ui_fontinfo_alloc();
		const char* family_name = luaL_checkstring(L, 2);
		lua_Integer size = luaL_checkinteger(L, 3);
		lua_Integer style = luaL_checkinteger(L, 4); /*! @todo */

		psy_ui_fontinfo_init(font_info, family_name, (double)size);
		psyclescript_createuserdata(L, 1, psy_luaui_fontinfo_meta, font_info, NULL);
	} else {
		luaL_error(L, "Wrong Number of Arguments.");
	}	
	return 1;
}

int psy_luaui_fontinfo_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_fontinfo_meta);	
	return 0;
}
