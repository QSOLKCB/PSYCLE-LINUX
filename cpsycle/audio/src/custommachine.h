/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_CUSTOMMACHINE_H
#define psy_audio_CUSTOMMACHINE_H

#include "machine.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_CustomMachine
** @brief Specialized base class of psy_audio_Machine
** 
** @details A specialized base class for "Machines", the audio producing
** elements, offering default implementations.
** Most plugins inherit from this class instead from Machine.
*/

typedef struct psy_audio_CustomMachine {
	/*! @extends */
	psy_audio_Machine machine;
	/*! @internal */
	double pan;
	int muted_;
	int bypassed_;
	int is_bus_;
	char* edit_name_;
	psy_audio_Buffer memorybuffer;
	uintptr_t memorybuffersize;
	uintptr_t slot_;
	double x_;
	double y_;
	uintptr_t selected_aux_col_;
	uintptr_t selected_param_;
	psy_audio_ParamTranslator param_translator_;
	psy_dsp_amp_range_t amp_range_;
} psy_audio_CustomMachine;

void psy_audio_custommachine_init(psy_audio_CustomMachine*,
	psy_audio_MachineCallback*);
void psy_audio_custommachine_dispose(psy_audio_CustomMachine*);

void psy_audio_custommachine_set_amp_range(psy_audio_CustomMachine*,
	psy_dsp_amp_range_t);

INLINE psy_audio_Machine* psy_audio_custommachine_base(psy_audio_CustomMachine* self)
{
	return &self->machine;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_CUSTOMMACHINE_H */
