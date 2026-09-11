/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_EVENTDRIVERPLUGIN_H
#define psy_audio_EVENTDRIVERPLUGIN_H

/* local */
#include "library.h"
/* driver */
#include "../../driver/eventdriver.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_audio_EventDriverPlugin {
	/*! @extends  */
	psy_EventDriver driver;
	/*! @internal */
	psy_Library module;
	psy_EventDriver* client;
} psy_audio_EventDriverPlugin;

void psy_audio_eventdriverplugin_init(psy_audio_EventDriverPlugin*);
void psy_audio_eventdriverplugin_dispose(psy_audio_EventDriverPlugin*);

psy_audio_EventDriverPlugin* psy_audio_eventdriverplugin_alloc(void);
psy_audio_EventDriverPlugin* psy_audio_eventdriverplugin_alloc_init(void);

void psy_audio_eventdriverplugin_load(psy_audio_EventDriverPlugin*,
	const char* path);
void psy_audio_eventdriverplugin_clear(psy_audio_EventDriverPlugin*);
void psy_audio_eventdriverplugin_connect(psy_audio_EventDriverPlugin*,
	void* context, EVENTDRIVERWORKFN fp);
	
INLINE psy_EventDriver* psy_audio_eventdriverplugin_base(
	psy_audio_EventDriverPlugin* self)
{
	return &self->driver;
}

INLINE const psy_EventDriver* psy_audio_eventdriverplugin_base_const(
	const psy_audio_EventDriverPlugin* self)
{
	return &self->driver;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_EVENTDRIVERPLUGIN_H */
