/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luagraphics.h"
#include <lauxlib.h>
#include <lualib.h>
#include "luapoint.h"
#include "luarectangle.h"
#include <psyclescript.h>

#include <uigraphics.h>
#include <stdlib.h>

#include "../../detail/portable.h"

static int psy_luaui_graphics_create(lua_State*);
static int psy_luaui_graphics_gc(lua_State*);
static int psy_luaui_graphics_draw_line(lua_State*);
static int psy_luaui_graphics_draw_string(lua_State*);
static int psy_luaui_graphics_fill_rect(lua_State*);

const char* psy_luaui_graphics_meta = "psygraphicsmeta";

int psy_luaui_graphics_open(lua_State *L)
{  
  static const luaL_Reg methods[] = {
		{"new", psy_luaui_graphics_create},
		{"drawline", psy_luaui_graphics_draw_line},
		{"drawstring", psy_luaui_graphics_draw_string},
		{"fillrect", psy_luaui_graphics_fill_rect},
		{ NULL, NULL }
  };
  return psyclescript_open(L, psy_luaui_graphics_meta, methods,
      psy_luaui_graphics_gc, NULL);
}

int psy_luaui_graphics_create(lua_State* L)
{  
  psy_ui_Point** udata;
  int n = lua_gettop(L);

  if (n == 1) {
	udata = (psy_ui_Point**)lua_newuserdata(L, sizeof(psy_ui_Graphics*));
	luaL_setmetatable(L, psy_luaui_graphics_meta);
  /*else if (n == 2) {
    Point::Ptr other = LuaHelper::check_sptr<Point>(L, 2, LuaPointBind::meta); 
    LuaHelper::new_shared_userdata<>(L, meta, new Point(*other.get()));
  } else
  if (n == 3) {
    double x = luaL_checknumber(L, 2);
    double y = luaL_checknumber(L, 3);
    LuaHelper::new_shared_userdata<>(L, meta, new Point(x, y));*/
  } else {
    luaL_error(L, "Wrong number of arguments");
  }
  return 1;
}

int psy_luaui_graphics_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_graphics_meta);
	return 0;	
}

int psy_luaui_graphics_draw_line(lua_State* L)
{
	psy_GCPtr* p;
	psy_ui_Graphics* g;
	psy_ui_RealPoint* p1;
	psy_ui_RealPoint* p2;

	p = psyclescript_check(L, 1, psy_luaui_graphics_meta);
	g = (psy_ui_Graphics*)(p->ud);
	p = psyclescript_check(L, 2, psy_luaui_point_meta);
	p1 = (psy_ui_RealPoint*)(p->ud);
	assert(p1);
	p = psyclescript_check(L, 3, psy_luaui_point_meta);	
	p2 = (psy_ui_RealPoint*)(p->ud);
	assert(p2);
	psy_ui_drawline(g, *p1, *p2);	
	return psyclescript_chaining(L);
}

int psy_luaui_graphics_draw_string(lua_State* L)
{
	psy_GCPtr* p;
	psy_ui_Graphics* g;
	psy_ui_RealPoint* pt;
	const char* str;

	p = psyclescript_check(L, 1, psy_luaui_graphics_meta);
	g = (psy_ui_Graphics*)(p->ud);	
	str = luaL_checkstring(L, 2);
	p = psyclescript_check(L, 3, psy_luaui_point_meta);
	pt = (psy_ui_RealPoint*)(p->ud);
	assert(pt);
	psy_ui_graphics_textout(g, *pt, str, psy_strlen(str));	
	return psyclescript_chaining(L);
}

int psy_luaui_graphics_fill_rect(lua_State* L)
{
    /* using namespace ui;
	int n = lua_gettop(L);
	if (n == 2) {
      Graphics::Ptr g = LuaHelper::check_sptr<Graphics>(L, 1, meta);
      Rect::Ptr pos = LuaHelper::test_sptr<Rect>(L, 2, LuaUiRectBind::meta);
	  if (pos) {
        g->FillRect(*pos.get());
	  } else {
	    Dimension::Ptr dimension = LuaHelper::check_sptr<Dimension>(L, 2, LuaDimensionBind::meta);
		g->FillRect(Rect(Point(), *dimension.get()));
	  }
	} else {	
	  return luaL_error(L, "Wrong number of arguments.");
	}
    return LuaHelper::chaining(L); */
        int n;
	psy_GCPtr* p;
	psy_ui_Graphics* g;
	psy_ui_RealRectangle* pos;	

	n = lua_gettop(L);
	p = psyclescript_check(L, 1, psy_luaui_graphics_meta);
	g = (psy_ui_Graphics*)(p->ud);	
	p = psyclescript_check(L, 2, psy_luaui_rectangle_meta);
	pos = (psy_ui_RealRectangle*)(p->ud);	
	assert(pos);
	psy_ui_graphics_draw_solid_rectangle(g, *pos,
	  psy_ui_colour_white());
	return psyclescript_chaining(L);
  }
