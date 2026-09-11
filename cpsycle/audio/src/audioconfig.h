/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(AUDIOCONFIG_H)
#define AUDIOCONFIG_H

/* container */
#include <configuration.h>


#ifdef __cplusplus
extern "C" {
#endif

struct psy_Property;

/*!
** @struct psy_audio_AudioConfig
** @brief PCM Audiodriver configurations
*/

typedef struct psy_audio_AudioConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal */
	struct psy_Property* inputoutput;
	struct psy_Property* driver_choice;
	struct psy_Property* edit_configure;
	struct psy_Property* configurations;	
	bool audio_enabled;	
} psy_audio_AudioConfig;

void psy_audio_audioconfig_init(psy_audio_AudioConfig*, struct psy_Property* parent);
void psy_audio_audioconfig_dispose(psy_audio_AudioConfig*);

INLINE psy_Configuration* psy_audio_audioconfig_base(psy_audio_AudioConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* AUDIOCONFIG_H */
