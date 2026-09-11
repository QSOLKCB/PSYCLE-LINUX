/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEVIEWCONFIG_H)
#define MACHINEVIEWCONFIG_H

/* container */
#include <configuration.h>
/* ui */
#include <uifont.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MachineViewConfig
*/
typedef struct MachineViewConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal !*/		
	struct psy_Property* theme;
	struct psy_Property* machine_skin;
} MachineViewConfig;

void machineviewconfig_init(MachineViewConfig*, struct psy_Property* parent);
void machineviewconfig_dispose(MachineViewConfig*);

INLINE psy_Configuration* machineviewconfig_base(MachineViewConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEVIEWCONFIG_H */
