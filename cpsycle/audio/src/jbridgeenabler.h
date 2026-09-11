/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_JBRIDGEENABLER_H
#define psy_audio_JBRIDGEENABLER_H

#include "../../detail/psyconf.h"
#include "../../detail/psydef.h"
#include "../../detail/os.h"

#ifdef PSYCLE_USE_VST2

#if defined(DIVERSALIS__OS__MICROSOFT)

#include <windows.h>
#include <excpt.h>

#include "aeffectx.h"


#ifdef __cplusplus
extern "C" {
#endif

/* Typedef for BridgeMain proc */
typedef AEffect* (*PFNBRIDGEMAIN)(audioMasterCallback audiomaster, char* pszPluginPath);

bool psy_audio_jbridge_is_boot_strap_module_path(const char* path);
bool psy_audio_jbridge_is_boot_strap_module(HMODULE hModule);
void psy_audio_jbridge_getjbridgelibrary(char szProxyPath[], DWORD pathsize);
PFNBRIDGEMAIN psy_audio_jbridge_getbridgemainentry(const HMODULE hModuleProxy);

#ifdef __cplusplus
}
#endif

#endif

#endif /* PSYCLE_USE_VST2 */

#endif /* JBRIDGEENABLER_H */
