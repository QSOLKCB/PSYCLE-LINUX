/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "luarectangle.h"
#include <lauxlib.h>
#include <lualib.h>

#include <uigeometry.h>
#include <stdlib.h>
#include <psyclescript.h>
#include "luadimension.h"
#include "luapoint.h"

static int psy_luaui_rectangle_create(lua_State*);
static int psy_luaui_rectangle_gc(lua_State*);

const char* psy_luaui_rectangle_meta = "psyuirectmeta";

int psy_luaui_rectangle_open(lua_State *L)
{  
    static const luaL_Reg methods[] = {    
	{"new", psy_luaui_rectangle_create},
	{NULL, NULL}
  };
  return psyclescript_open(L, psy_luaui_rectangle_meta, methods,
      psy_luaui_rectangle_gc, NULL);
}

int psy_luaui_rectangle_create(lua_State* L)
{  
  int n = lua_gettop(L);

  if (n == 1) {
      psyclescript_createuserdata(L, 1, psy_luaui_rectangle_meta, 
	psy_ui_realrectangle_alloc_init(),
	(psy_fp_gcptr_dispose)psy_ui_realrectangle_dispose);	  
  } else if (n == 2) {
    psy_GCPtr* ptr;
    psy_ui_RealRectangle* new_rect;
    psy_ui_RealRectangle* other;
    
    ptr = psyclescript_check(L, 2, psy_luaui_rectangle_meta);
    other = (psy_ui_RealRectangle*)(ptr->ud);
    new_rect = psy_ui_realrectangle_alloc_init();
    psy_ui_realrectangle_copy(new_rect, other);    
    psyclescript_createuserdata(L, 1, psy_luaui_rectangle_meta,
	new_rect, (psy_fp_gcptr_dispose)psy_ui_realrectangle_dispose);
  } else if (n == 3) {
    psy_GCPtr* ptr;
    psy_ui_RealRectangle* new_rect;
    psy_ui_RealPoint* pt;

        
    new_rect = psy_ui_realrectangle_alloc_init();
    ptr = psyclescript_check(L, 2, psy_luaui_point_meta);
    pt = (psy_ui_RealPoint*)(ptr->ud);
    assert(pt);
    psy_ui_realrectangle_set_topleft(new_rect, *pt);
    ptr = psyclescript_testudata(L, 3, psy_luaui_point_meta);
    if (ptr) {
      psy_ui_RealPoint* bottom_right;
      
      bottom_right = (psy_ui_RealPoint*)(ptr->ud);
      assert(bottom_right);
      new_rect->right = bottom_right->x;
      new_rect->bottom = bottom_right->y;
    } else {
      psy_ui_RealSize* size;
	  
      ptr = psyclescript_check(L, 3, psy_luaui_dimension_meta);
      size =(psy_ui_RealSize*)(ptr->ud);
      assert(size);    
      psy_ui_realrectangle_resize(new_rect, size->width, size->height);
    }
    psyclescript_createuserdata(L, 1, psy_luaui_rectangle_meta,
	new_rect, (psy_fp_gcptr_dispose)psy_ui_realrectangle_dispose);
  } else {
    luaL_error(L, "Wrong number of arguments");
  }
  return 1;
}

/*  if (n == 3) {    
    Point::Ptr top_left = LuaHelper::check_sptr<Point>(L, 2, LuaPointBind::meta);
	Point::Ptr bottom_right = LuaHelper::test_sptr<Point>(L, 3, LuaPointBind::meta);
	if (bottom_right) {
	  LuaHelper::new_shared_userdata<>(L, meta, new Rect(*top_left.get(), *bottom_right.get()));
	} else {
      Dimension::Ptr dimension =
        LuaHelper::check_sptr<Dimension>(L, 3, LuaDimensionBind::meta);
      LuaHelper::new_shared_userdata<>(L, meta, new Rect(*top_left.get(), *dimension.get()));
	}   
  } */

int psy_luaui_rectangle_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, psy_luaui_rectangle_meta);
	return 0;
}
