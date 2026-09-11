/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(psy_audio_AUDIODRIVERS_H)
#define psy_audio_AUDIODRIVERS_H

/* local */
#include "audiodriverplugin.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_audio_Sequencer;
struct psy_Configuration;
struct Logger;

/*!
** @struct psy_audio_AudioDrivers
*/
typedef struct psy_audio_AudioDrivers {
	/* extends */
	psy_audio_AudioDriverPlugin driver_plugin;
	void* systemhandle;	
	struct psy_Configuration* config_;	
	void* context;
	struct psy_audio_Sequencer* sequencer;
	AUDIODRIVERWORKFN fp;
	bool prevent_open;
} psy_audio_AudioDrivers;

void psy_audio_audiodrivers_init(psy_audio_AudioDrivers*, void* handle,
	struct psy_audio_Sequencer*, struct psy_Configuration*, void* context,
	AUDIODRIVERWORKFN, struct psy_Logger* logger);
void psy_audio_audiodrivers_dispose(psy_audio_AudioDrivers*);

void psy_audio_audiodrivers_render(psy_audio_AudioDrivers*);
void psy_audio_audiodrivers_restore(psy_audio_AudioDrivers*);
	
bool psy_audio_audiodrivers_enabled(const psy_audio_AudioDrivers*);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_AUDIODRIVERS_H */
