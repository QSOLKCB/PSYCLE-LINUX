/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "../../detail/os.h"
#include "../../detail/stdint.h"

#if defined DIVERSALIS__OS__MICROSOFT    
#include <windows.h>
#else
#define _MAX_PATH 4096
#endif

#include <lock.h>
#include "psyclescript.h"
#include "luaimport.h"


#include <dir.h>

#include <lauxlib.h>
#include <lualib.h>
#include "../../detail/portable.h"


void psy_gcptr_init(psy_GCPtr* self, void* ud, psy_fp_gcptr_dispose fp)
{
	assert(self);
	
	self->ud = ud;
	self->dispose = fp;
}

void psy_gcptr_dispose(psy_GCPtr* self)
{
	assert(self);

	if (self->ud && self->dispose) {
		self->dispose(self->ud);
		free(self->ud);
	}
}

psy_GCPtr* psy_gcptr_alloc(void)
{
	return (psy_GCPtr*)malloc(sizeof(psy_GCPtr));
}

psy_GCPtr* psy_gcptr_alloc_init(void* ud, psy_fp_gcptr_dispose fp)
{
	psy_GCPtr* rv;
	
	rv = psy_gcptr_alloc();
	if (rv) {
		psy_gcptr_init(rv, ud, fp);
	}
	return rv;
}

void psy_gcptr_deallocate(psy_GCPtr* self)
{
	assert(self);

	psy_gcptr_dispose(self);
	free(self);	
}

static void psyclescript_setsearchpath(psy_PsycleScript*, const char* modulepath);
static void psyclescript_seterrorstr(psy_PsycleScript*, const char* str);

int psyclescript_init(psy_PsycleScript* self, psy_Logger* logger)
{
	self->logger = logger;
	self->L = luaL_newstate();
	luaL_openlibs(self->L);
	self->modulepath = strdup("");
	self->errstr = strdup("");
	return 0;
}

int psyclescript_dispose(psy_PsycleScript* self)
{
  int err = 0;

  if (self->L) {
    lua_close(self->L);	
  }
  self->L = 0;
  free(self->modulepath);
  self->modulepath = NULL;
  free(self->errstr);
  self->errstr = NULL;
  return err;
}

void psyclescript_seterrorstr(psy_PsycleScript* self, const char* str)
{
	free(self->errstr);
	self->errstr = strdup(str);
}

int psyclescript_load(psy_PsycleScript* self, const char* path)
{
	int status;
	char temp[_MAX_PATH];

	psyclescript_setsearchpath(self, path);
	// This is needed to prevent loading problems
	free(self->modulepath);
	self->modulepath = strdup(path);
	psy_snprintf(temp, _MAX_PATH, "%s", path);
#if defined DIVERSALIS__OS__MICROSOFT  	
	psy_replacechar(temp, '\\', '/');
#endif	
	status = luaL_loadfile(self->L, temp);
	if (status) {
		const char* msg =lua_tostring(self->L, -1);
		if (self->logger) {
			psy_logger_error(self->logger, msg);
		}
	}
	return status;
}

int psyclescript_loadstring(psy_PsycleScript* self, const char* script)
{
	int status;

	status = luaL_loadstring(self->L, script);
	if (status) {
		const char* msg;
		
		msg = lua_tostring(self->L, -1);
		if (self->logger) {
			psy_logger_error(self->logger, msg);
		}
	}
	return status;
}

int psyclescript_preparestate(psy_PsycleScript* self, const luaL_Reg methods[],
	void* host)
{
  psy_PsycleScript** ud;

  lua_newtable(self->L);
  luaL_setfuncs(self->L, methods, 0);
  ud = (psy_PsycleScript **)lua_newuserdata(self->L, sizeof(psy_PsycleScript *));
  luaL_newmetatable(self->L, "psyhostmeta");
  lua_setmetatable(self->L, -2);
  *ud = host;
  lua_setfield(self->L, -2, "__self");
  lua_setglobal(self->L, "psycle");
  lua_getglobal(self->L, "psycle");  
  lua_newtable(self->L); // table for canvasdata
  lua_setfield(self->L, -2, "userdata");
  lua_newtable(self->L); // table for canvasdata
  lua_newtable(self->L); // metatable
  lua_pushstring(self->L, "kv");
  lua_setfield(self->L, -2, "__mode");
  lua_setmetatable(self->L, -2);
  lua_setfield(self->L, -2, "weakuserdata");
  lua_pop(self->L, 1);
  lua_getglobal(self->L, "psycle");
  lua_pushlightuserdata(self->L, self->logger);
  lua_setfield(self->L, -2, "logger");
  lua_pop(self->L, 1);

  return 0;
}

void* psyclescript_host(lua_State* L)
{
	void* host;

	lua_getglobal(L, "psycle");
	lua_getfield(L, -1, "__self");
	host = *(void**)luaL_checkudata(L, -1, "psyhostmeta");
	return host;
}

int psyclescript_run(psy_PsycleScript* self)
{ 
  int status;

  status = lua_pcall(self->L, 0, LUA_MULTRET, 0);
  if (status) {      
    const char* msg = lua_tostring(self->L, -1);
	if (self->logger) {
		psy_logger_error(self->logger, msg);
	}
  }
  return status;
}

int psyclescript_start(psy_PsycleScript* self)
{
  lua_getglobal(self->L, "psycle");
  if (lua_isnil(self->L, -1)) {
    // throw std::runtime_error("Psycle not available.");
	  return 1;
  }
  lua_getfield(self->L, -1, "start");
  if (!lua_isnil(self->L, -1)) {
    int status = lua_pcall(self->L, 0, 0, 0);
    if (status) {         
		const char* msg;
		
		msg = lua_tostring(self->L, -1);
		if (self->logger) {
			psy_logger_error(self->logger, msg);
		}
		psyclescript_seterrorstr(self, msg);
		return status;
    }
  }
  return 0;
}

int psyclescript_open(lua_State* L,
		const char* meta,
		const luaL_Reg methods[],
		lua_CFunction gc,
		lua_CFunction tostring)
{
	luaL_newmetatable(L, meta);
    if (gc) {
        lua_pushcclosure(L, gc, 0);
        lua_setfield(L,-2, "__gc");
    }
    if (tostring) {
        lua_pushcclosure(L, tostring, 0);
        lua_setfield(L,-2, "__tostring");
    }
    luaL_newlib(L, methods);
    return 1;
}

void psyclescript_require(psy_PsycleScript* self, const char* name,
	lua_CFunction openf)
{
	luaL_requiref(self->L, name, openf, 1);
    lua_pop(self->L, 1);
}

void psyclescript_requirenew(lua_State* L, const char* name, void* ud,
		lua_CFunction openf, const char* meta,
		psy_fp_gcptr_dispose dispose) {
	luaL_requiref(L, name, openf, 1);
	psyclescript_createuserdata(L, lua_gettop(L), meta, ud, dispose);
	lua_remove(L, -2);
}

void psyclescript_setsearchpath(psy_PsycleScript* self, const char* modulepath)
{    
	psy_Path path;  
	char luapath[_MAX_PATH];	

	psy_path_init(&path, modulepath);	
	psy_snprintf(luapath, _MAX_PATH, "%s/?.lua;%s/%s/?.lua;%s/psycle/?.lua",
		psy_path_prefix(&path),
		psy_path_prefix(&path), psy_path_name(&path),
		psy_path_prefix(&path));
#if defined DIVERSALIS__OS__MICROSOFT   		
	psy_replacechar(luapath, '/', '\\');
#endif
	lua_getglobal(self->L, "package");
	lua_pushstring(self->L, luapath);
	lua_setfield(self->L, -2, "path");
	psy_path_dispose(&path);
}

void psyclescript_register_weakuserdata(lua_State* L, void* ud)
{
	lua_getglobal(L, "psycle");
	lua_getfield(L, -1, "weakuserdata");
	lua_pushlightuserdata(L, ud);
	lua_pushvalue(L, -4);
	lua_settable(L, -3);
	lua_pop(L, 2);
}

psy_GCPtr* psyclescript_check(lua_State* L, int index, const char* meta)
{
	psy_GCPtr* ud;

	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "__self");      
	ud = luaL_checkudata(L, -1, meta);
	luaL_argcheck(L, (ud) != 0, 1, meta);
	lua_pop(L, 1);
	// luaL_argcheck(L, (*SPTR) != 0, 1, (meta+" expected").c_str());     
	return ud;
}

int psyclescript_createuserdata(lua_State* L, int self, const char* meta, void* T,
	psy_fp_gcptr_dispose dispose)
{
	int n;
	psy_GCPtr* ptr;

	lua_pushvalue(L, self);
	n = lua_gettop(L);
	luaL_checktype(L, -1, LUA_TTABLE);  // self
	lua_newtable(L);  // new
	lua_pushvalue(L, self);
	lua_setmetatable(L, -2);
	lua_pushvalue(L, self);
	lua_setfield(L, self, "__index");
	ptr = (psy_GCPtr*)lua_newuserdata(L, sizeof(psy_GCPtr));
	psy_gcptr_init(ptr, T, dispose);
	luaL_getmetatable(L, meta);
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "__self");
	lua_remove(L, n);
	psyclescript_register_weakuserdata(L, T);
	return 1;
}

int psyclescript_deallocate_userdata(lua_State* L, const char* meta)
{      
      psy_GCPtr* ud = (psy_GCPtr*)luaL_checkudata(L, 1, meta);
      psy_gcptr_dispose(ud);   
      return 0;
    }

/* mimics the bahaviour of luaL_testudata for our own userdata structure */
void* psyclescript_testudata(lua_State* L, int index, const char* meta)
{
	if (lua_istable(L, index)) {
		psy_GCPtr* ud;
		lua_getfield(L, index, "__self");
		
		ud = (psy_GCPtr*)luaL_testudata(L, -1, meta);
		if (ud == NULL) {
			lua_pop(L, 1);
			return NULL;
		}
		luaL_argcheck(L, (ud) != 0, 1, "wrong meta");
		lua_pop(L, 1);
		return ud;
	}	
	return NULL;	
}

/* needs to be registered with register_weakuserdata */
void find_weakuserdata(lua_State* L, void* ud)
{
	lua_getglobal(L, "psycle");
	lua_getfield(L, -1, "weakuserdata");
	if (!lua_isnil(L, -1)) {
		lua_pushlightuserdata(L, ud);
		lua_gettable(L, -2);
	if (lua_istable(L, -1)) {
		lua_remove(L, -2);
	}        
	lua_remove(L, -2);
	} else {
		assert(0);
	}
}

/* build enum table */
int psyclescript_buildenum(lua_State* L, const char* const e[], int len, int startidx)
{
	int i;

	for (i = 0; i < len; ++i) {
		lua_pushnumber(L, i + startidx);
		lua_setfield(L, -2, e[i]);
	}
	return 0;
}

psy_Logger* psyclescript_logger(lua_State* L)
{
	psy_Logger* rv;

	lua_getglobal(L, "psycle");
	lua_getfield(L, -1, "logger");
	rv = (psy_Logger*)lua_touserdata(L, -1);
	lua_pop(L, 2);
	return rv;
}
