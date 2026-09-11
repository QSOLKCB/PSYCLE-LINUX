/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(HOSTMACHINECALLBACK_H)
#define HOSTMACHINECALLBACK_H

/* container */
#include <configuration.h>
#include <properties.h>
/* audio */
#include <machinefactory.h>
#include <signal.h>
/* luaui */
#include "luaui.h"
/* file */
#include <logger.h>
#include <fileselect.h>

/*
** HostMachineCallback
**
** connects the machines with the psycle host ui
**
** psy_audio_MachineCallback
**         ^
**         |
**     HostMachineCallback
**             <>---- DirConfig
**             <>---- psy_audio_MachineFactory
**             <>---- psy_Logger
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HostMachineCallback {
	/* implements */
	psy_audio_MachineCallback machinecallback;
	/* references */	
	psy_Configuration* dir_config;
	psy_Logger* logger;
	psy_Signal* signal_machine_edit_resize;
	psy_Signal* signal_bus_changed;
	char* message;
	char* inout_name;
	int res;
	psy_DiskOp* disk_op;
	void* ui_;
} HostMachineCallback;

void hostmachinecallback_init(HostMachineCallback*, 
	psy_Configuration* dir_config, psy_Signal* signal_machine_edit_resize,
	psy_Signal* signal_bus_changed, psy_DiskOp*);
void hostmachinecallback_dispose(HostMachineCallback*);
	

INLINE psy_audio_MachineCallback* hostmachinecallback_base(
	HostMachineCallback* self)
{
	return &self->machinecallback;
}

#ifdef __cplusplus
}
#endif

#endif /* HOSTMACHINECALLBACK_H */
