/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(DIRCONFIG_H)
#define DIRCONFIG_H

/* container.h */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_DirConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;	
} psy_DirConfig;

void psy_dirconfig_init(psy_DirConfig*, struct psy_Property* parent);
void psy_dirconfig_dispose(psy_DirConfig*);

INLINE psy_Configuration* psy_dirconfig_base(psy_DirConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* DIRCONFIG_H */
