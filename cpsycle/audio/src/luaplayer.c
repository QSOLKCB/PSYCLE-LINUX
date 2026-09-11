/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2007-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "luaplayer.h"

#include "array.h"
#include "luaarray.h"
#include "player.h"

#include <envelope.h>
#include <stdlib.h>

#include <lauxlib.h>
#include <lualib.h>

#include "psyclescript.h"

#include "../../detail/stdint.h"

static const char* luaplayer_meta = "psyplayermeta";
static psy_audio_Player* player = NULL;

static int luaplayer_create(lua_State* L);
static int luaplayer_gc(lua_State* L);
static int luaplayer_samplerate(lua_State* L);
static int luaplayer_playing(lua_State* L);

int psy_audio_luabind_player_open(lua_State *L)
{
	static const luaL_Reg methods[] = {
		{"new", luaplayer_create},
		{"samplerate", luaplayer_samplerate},
		 {"playing", luaplayer_playing},
		{NULL, NULL}
	};
	psyclescript_open(L, luaplayer_meta, methods, luaplayer_gc, 0);
	return 1;	
}

int luaplayer_create(lua_State* L)
{
	psyclescript_createuserdata(L, 1, luaplayer_meta, player, NULL);
	return 1;
}

int luaplayer_gc(lua_State* L)
{
	psyclescript_deallocate_userdata(L, luaplayer_meta);	
	return 0;
}

void psy_audio_luabind_set_player(psy_audio_Player* theplayer)
{
	player = theplayer;
}

int luaplayer_samplerate(lua_State* L)
{
	psy_GCPtr* ptr;
	psy_audio_Player* self;	
	
	ptr = psyclescript_check(L, 1, luaplayer_meta);
	self = (psy_audio_Player*)(ptr->ud);	
	lua_pushinteger(L, (lua_Integer)psy_audio_player_sample_rate(self));
	return 1;
}

int luaplayer_playing(lua_State* L)
{
	psy_GCPtr* ptr;
	psy_audio_Player* self;	
	
	ptr = psyclescript_check(L, 1, luaplayer_meta);
	self = (psy_audio_Player*)(ptr->ud);
	lua_pushboolean(L, (lua_Integer)psy_audio_player_playing(self));	
	return 1;
}
