/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_LUAPLUGIN_H
#define psy_audio_LUAPLUGIN_H

#include "custommachine.h"
#include "luamachine.h"
/* script */
#include <psyclescript.h>
/* container */
#include <logger.h>

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_LuaPlugin;
struct psy_ui_Component;

typedef struct psy_audio_LuaPluginMachineParam {
	psy_audio_CustomMachineParam custommachineparam;
	struct psy_audio_LuaPlugin* plugin;
	uintptr_t index;
} psy_audio_LuaPluginMachineParam;

void psy_audio_luapluginmachineparam_init(psy_audio_LuaPluginMachineParam*, struct psy_audio_LuaPlugin* plugin,
	uintptr_t index);
void psy_audio_luapluginmachineparam_dispose(psy_audio_LuaPluginMachineParam*);
psy_audio_LuaPluginMachineParam* psy_audio_luapluginmachineparam_alloc(void);

struct psy_Lock;
struct psy_ui_Component;

typedef struct psy_audio_LuaPlugin {
	psy_audio_CustomMachine custommachine;
	psy_PsycleScript script;	
	psy_audio_LuaMachine* client;
	psy_audio_MachineInfo* plugininfo;
	struct psy_Lock* lock;
	psy_audio_LuaPluginMachineParam parameter;
	bool usenoteon;
	struct psy_ui_Component* hostview;	
} psy_audio_LuaPlugin;

int psy_audio_luaplugin_init(psy_audio_LuaPlugin*, psy_audio_MachineCallback*,
	const char* path, psy_Logger* logger, struct psy_ui_Component* ui);
int psy_audio_luaplugin_test(const char* path, psy_audio_MachineInfo*,
	psy_Logger* logger);

INLINE psy_audio_Machine* psy_audio_luaplugin_base(psy_audio_LuaPlugin* self)
{
	return &(self->custommachine.machine);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_LUAPLUGIN_H */
