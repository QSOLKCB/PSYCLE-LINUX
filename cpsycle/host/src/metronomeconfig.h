/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(METRONOMECONFIG_H)
#define METRONOMECONFIG_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MetronomeConfig
*/
typedef struct MetronomeConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;	
} MetronomeConfig;

void metronomeconfig_init(MetronomeConfig*, struct psy_Property* parent);
void metronomeconfig_dispose(MetronomeConfig*);

INLINE psy_Configuration* metronomeconfig_base(MetronomeConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* METRONOMECONFIG_H */
