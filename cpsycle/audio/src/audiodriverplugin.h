/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_AUDIODRIVERPLUGIN_H
#define psy_audio_AUDIODRIVERPLUGIN_H

/* local */
#include "library.h"
/* driver */
#include "../../driver/audiodriver.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_Logger;

typedef struct psy_audio_AudioDriverPlugin {
	/*! @extends  */
	psy_AudioDriver driver;
	/*! @internal */
	psy_Library module;
	psy_AudioDriver* client;	
} psy_audio_AudioDriverPlugin;

void psy_audio_audiodriverplugin_init(psy_audio_AudioDriverPlugin*,
	struct psy_Logger* logger);
void psy_audio_audiodriverplugin_dispose(psy_audio_AudioDriverPlugin*);

void psy_audio_audiodriverplugin_load(psy_audio_AudioDriverPlugin*,
	const char* path);
void psy_audio_audiodriverplugin_clear(psy_audio_AudioDriverPlugin*);
void psy_audio_audiodriverplugin_connect(psy_audio_AudioDriverPlugin*,
	void* platform_handle, void* context, AUDIODRIVERWORKFN fp);
	
INLINE psy_AudioDriver* psy_audio_audiodriverplugin_base(
	psy_audio_AudioDriverPlugin* self)
{
	return &self->driver;
}

INLINE const psy_AudioDriver* psy_audio_audiodriverplugin_base_const(
	const psy_audio_AudioDriverPlugin* self)
{
	return &self->driver;
}

INLINE bool psy_audio_audiodriverplugin_empty(const
	psy_audio_AudioDriverPlugin* self)
{
	assert(self);
	
	return (self->client != NULL);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_AUDIODRIVERPLUGIN_H */
