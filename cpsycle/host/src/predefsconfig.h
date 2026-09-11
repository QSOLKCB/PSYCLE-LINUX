/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PREDEFSCONFIG_H)
#define PREDEFSCONFIG_H

/* audio */
#include <instrument.h>

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct PredefsConfig
** @brief Stores predefinitions of instruments in the configuration like in FT2
*/
typedef struct PredefsConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;		
} PredefsConfig;

void predefsconfig_init(PredefsConfig*, struct psy_Property* parent);
void predefsconfig_dispose(PredefsConfig*);

void predefsconfig_predef(PredefsConfig*, intptr_t index, psy_dsp_Envelope* rv);
void predefsconfig_store_predef(PredefsConfig*, intptr_t index, psy_dsp_Envelope* rv);

INLINE psy_Configuration* predefsconfig_base(PredefsConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* PREDEFSCONFIG_H */
