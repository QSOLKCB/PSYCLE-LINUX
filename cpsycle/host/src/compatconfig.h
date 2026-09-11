/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(COMPATIBILITYCONFIG_H)
#define COMPATIBILITYCONFIG_H

/* container */
#include <configuration.h>
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct CompatConfig
*/

typedef struct CompatConfig {
	/*! extends */
	psy_CustomConfiguration configuration;	
} CompatConfig;

void compatconfig_init(CompatConfig*, psy_Property* parent);
void compatconfig_dispose(CompatConfig*);

INLINE psy_Configuration* compatconfig_base(CompatConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* COMPATIBILITYCONFIG_H */
