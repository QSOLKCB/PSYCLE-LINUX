/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(EVENTDRIVERCONFIG_H)
#define EVENTDRIVERCONFIG_H

/* local */
#include "driverregister.h"
/* container */
#include <configuration.h>
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
** EventDriverConfig
**
** Configures and selects the eventdrivers
*/

typedef struct EventDriverConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal */
	psy_Property* eventinputs;
	psy_Property* edit_configure;
	psy_Property* configurations;		
	psy_Property* drivers;
	psy_Property* activedrivers;	
	/* references */
	psy_audio_DriverRegister driver_register;
	psy_Property* config;	
	psy_Property* cmds;
} EventDriverConfig;

void eventdriverconfig_init(EventDriverConfig*, psy_Property* parent,
	psy_Property* cmds);
void eventdriverconfig_dispose(EventDriverConfig*);

intptr_t eventdriverconfig_current(EventDriverConfig*);

INLINE psy_Configuration* eventdriverconfig_base(EventDriverConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* EVENTDRIVERCONFIG_H */
