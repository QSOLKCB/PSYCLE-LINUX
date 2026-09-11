/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(GENERALCONFIG_H)
#define GENERALCONFIG_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! 
** @struct GeneralConfig
*/
typedef struct GeneralConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;	
} GeneralConfig;

void generalconfig_init(GeneralConfig*, struct psy_Property* parent);
void generalconfig_dispose(GeneralConfig*);

INLINE psy_Configuration* generalconfig_base(GeneralConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

INLINE const psy_Configuration* generalconfig_base_const(const GeneralConfig* self)
{
	assert(self);

	return psy_customconfiguration_base_const(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* GENERALCONFIG_H */
