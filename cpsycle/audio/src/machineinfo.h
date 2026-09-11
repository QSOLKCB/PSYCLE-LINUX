/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_MACHINEINFO_H
#define psy_audio_MACHINEINFO_H

#include "../../detail/psydef.h"

/* container */
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_audio_MachineInfo {
	/* API version.Use MI_VERSION */
	intptr_t apiversion;
	/* plug version. Your machine version. Shown in Hexadecimal. */
	intptr_t plugversion;
	/* psy_audio_Machine flags */
	intptr_t flags;
	/* Defines the type of machine */
	intptr_t mode;
	/* Name of the machine */
	char* name;
	/* Name of the machine in machine Display */
	char* shortname;
	/* Name of author */
	char* author;
	/* Text to show as custom command (see Command method) */
	char* command;	
	/* host type */
	intptr_t type;
	/* module path */
	char* modulepath;
	/* shellidx */
	uintptr_t shellidx;	
	char* helptext;
	char* desc;
	char* category;
	uintptr_t image_id;
	char* image_desc;
} psy_audio_MachineInfo;

void machineinfo_init(psy_audio_MachineInfo*);
void psy_audio_machineinfo_init_property(psy_audio_MachineInfo*,
	const psy_Property*);
void machineinfo_dispose(psy_audio_MachineInfo*);

psy_audio_MachineInfo* machineinfo_alloc(void);
psy_audio_MachineInfo* machineinfo_allocinit(void);
psy_audio_MachineInfo* machineinfo_clone(const psy_audio_MachineInfo*);

void machineinfo_set(psy_audio_MachineInfo*,
		const char* author,
		const char* command,
		intptr_t flags,
		intptr_t mode,
		const char* name,
		const char* shortname,
		intptr_t apiversion,
		intptr_t plugversion,
		intptr_t type,
		const char* modulepath,
		uintptr_t shellidx,
		const char* helptext,
		const char* desc,
		const char* category,
		uintptr_t image_id,
		const char* image_desc);
void machineinfo_set_path(psy_audio_MachineInfo*, const char* path);

void machineinfo_init_copy(psy_audio_MachineInfo*, psy_audio_MachineInfo* other);
void machineinfo_copy(psy_audio_MachineInfo*, const psy_audio_MachineInfo* other);
void machineinfo_clear(psy_audio_MachineInfo*);

bool machineinfo_internal(const psy_audio_MachineInfo*);
void machineinfo_catchername(const psy_audio_MachineInfo*, char* rv);
const char* machineinfo_module_path(const psy_audio_MachineInfo*);

void psy_audio_plugin_name(const char* filename, char* rv, uintptr_t shellidx);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_MACHINEINFO_H */
