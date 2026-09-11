/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "jbridgeenabler.h"

#ifdef PSYCLE_USE_VST2

#if defined(DIVERSALIS__OS__MICROSOFT)

/* Name of the proxy DLL to load */
#define JBRIDGE_PROXY_REGKEY        TEXT("Software\\JBridge")

#ifdef _M_X64
#define JBRIDGE_PROXY_REGVAL        TEXT("Proxy64")  /* use this for x64 builds */
#else
#define JBRIDGE_PROXY_REGVAL        TEXT("Proxy32")  /* use this for x86 builds */
#endif


/* Check if it’s a plugin_name.xx.dll */
bool psy_audio_jbridge_is_boot_strap_module_path(const char* path)
{
	bool ret = FALSE;

	HMODULE hModule = LoadLibrary(path);
	if (!hModule)
	{
		//some error…
		return ret;
	}
	ret = psy_audio_jbridge_is_boot_strap_module(hModule);

	FreeLibrary(hModule);

	return ret;
}

bool psy_audio_jbridge_is_boot_strap_module(HMODULE hModule)
{
	/* Exported dummy function to identify this as a bootstrap dll. */
	return GetProcAddress(hModule, "JBridgeBootstrap") != 0;
}

/* Get path to JBridge proxy */
void psy_audio_jbridge_getjbridgelibrary(char szProxyPath[], DWORD pathsize) {
	HKEY hKey;
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, JBRIDGE_PROXY_REGKEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		RegQueryValueEx(hKey, JBRIDGE_PROXY_REGVAL, NULL, NULL, (LPBYTE)szProxyPath, &pathsize);
		RegCloseKey(hKey);
	}
}

/* Get bridge's entry point */
PFNBRIDGEMAIN psy_audio_jbridge_getbridgemainentry(const HMODULE hModuleProxy)
{
	return (PFNBRIDGEMAIN)GetProcAddress((HMODULE)(hModuleProxy), "BridgeMain");
}

#endif /* DIVERSALIS__OS__MICROSOFT */

#endif /* PSYCLE_USE_VST2 */
