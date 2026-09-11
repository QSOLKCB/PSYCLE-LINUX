/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_DUMMY_H
#define psy_audio_DUMMY_H

/* local */
#include "custommachine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_DummyMachine
** @brief Replaces at song load missing machines or groups machine connections
**
** @details
** When Psycle loads a song, if it cannot find the appropriate VST or
** native plugin, it inserts this plugin, and adds an "X" to the beginning of
** the machine name.
** The internal Psycle machine simply allows sound from any other machine
** connected to it to pass through. Its panning can be changed in the
** Machine View, and it can be muted or bypassed. This allows it to be used for
** grouping machine connections, however this can be done better with a
** Gainer plugin or a machine used for mastering e.g. a compressor.
*/

typedef struct psy_audio_DummyMachine {
	psy_audio_CustomMachine custommachine;
	intptr_t mode;
} psy_audio_DummyMachine;

void psy_audio_dummymachine_init(psy_audio_DummyMachine*, psy_audio_MachineCallback*);
const psy_audio_MachineInfo* psy_audio_dummymachine_info(void);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_DUMMY_H */
