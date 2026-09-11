/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQEDITCONFIG_H)
#define SEQEDITCONFIG_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! 
** @struct SeqEditConfig
*/
typedef struct SeqEditConfig {	
	/*! @extends */
	psy_CustomConfiguration configuration;		
} SeqEditConfig;

void seqeditconfig_init(SeqEditConfig*, struct psy_Property* parent);
void seqeditconfig_dispose(SeqEditConfig*);

INLINE psy_Configuration* seqeditconfig_base(SeqEditConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* SEQEDITCONFIG_H */
