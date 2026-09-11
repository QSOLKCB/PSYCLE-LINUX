/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_MACHINEFACTORY_H
#define psy_audio_MACHINEFACTORY_H

#include "machine.h"
#include "plugincatcher.h"
#include <hashtbl.h>
#include <logger.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_MachineFactory
** @brief It creates machines using the psy_audio_MachineType and the
**        plugincatchername defined by 'PluginFinder'
**
** @details
** Therefore it needs a 'PluginFinder' to translate the name to the
** module path. The plugincatchername ends with a shellindex ':index'
** to select the right plugin inside a plugin bundle (e.g. 'vstbundle:2'),
** Bundles can be Vst and Ladspa plugins, the others use as suffix ':0'.
** Internal machines use no path and the path can be NULL.
** Besides the factory can make a machine from a psy_audio_MachineType, module
** path and a shellindex. (psy_audio_machinefactory_make_machine_from_path)
** At creation it also uses the MachineCallback for the host and depending
** on the configuration it wraps the machine inside a machineproxy,
** that can handle exceptions. Compatibility options for blitz can be set.
**
** psy_audio_MachineFactory <>----- psy_audio_PluginFinder
**                <>
**                 |
**     psy_audio_MachineCallback
*/

typedef struct psy_audio_MachineFactory {
	psy_audio_MachineCallback* machinecallback;	
	psy_audio_PluginCatcher* catcher;
	bool loadnewgamefxblitz;
	bool createasproxy;
	char* errstr;
	psy_Logger* logger;
	bool use_jbridge;
} psy_audio_MachineFactory;

void psy_audio_machinefactory_init(psy_audio_MachineFactory*,
	psy_audio_MachineCallback*, psy_audio_PluginCatcher*, psy_Logger*);
void psy_audio_machinefactory_dispose(psy_audio_MachineFactory*);

void psy_audio_machinefactory_set_callback(psy_audio_MachineFactory*,
	psy_audio_MachineCallback*);
void psy_audio_machinefactory_set_plugin_catcher(psy_audio_MachineFactory*,
	psy_audio_PluginCatcher*);
psy_audio_Machine* psy_audio_machinefactory_make_machine(
	psy_audio_MachineFactory*, psy_audio_MachineType,
	const char* plugincatchername, uintptr_t instindex);
psy_audio_Machine* psy_audio_machinefactory_make_machine_from_path(
	psy_audio_MachineFactory*, psy_audio_MachineType,
	const char* path, uintptr_t shellidx, uintptr_t instindex);
psy_audio_Machine* psy_audio_machinefactory_make_info(
	psy_audio_MachineFactory*, const psy_audio_MachineInfo*);
void psy_audio_machinefactory_createasproxy(psy_audio_MachineFactory*);
void psy_audio_machinefactory_createwithoutproxy(psy_audio_MachineFactory*);
void psy_audio_machinefactory_loadnewgamefxandblitzifversionunknown(
	psy_audio_MachineFactory*);
void psy_audio_machinefactory_loadoldgamefxandblitzifversionunknown(
	psy_audio_MachineFactory*);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_MACHINEFACTORY_H */
