/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_LUAMACHINE_H
#define psy_audio_LUAMACHINE_H

#include "custommachine.h"
/* script */
#include <psyclescript.h>
#include <luaui.h>
/* container */
#include <logger.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_audio_LuaMachine {
	psy_audio_Machine* machine;	
	psy_audio_BufferContext* bc;
	uintptr_t numparameters_;
	int numcols_;
	int numprograms_;
	bool shared_;
	bool bcshared_;	
} psy_audio_LuaMachine;

void psy_audio_luamachine_init(psy_audio_LuaMachine*);
void psy_audio_luamachine_init_shared(psy_audio_LuaMachine*,
	psy_audio_Machine*);
void psy_audio_luamachine_init_machine(psy_audio_LuaMachine*,
	psy_audio_Machine*);
void psy_audio_luamachine_dispose(psy_audio_LuaMachine*);

INLINE bool luamachine_shared(psy_audio_LuaMachine* self)
{
	return self->shared_;
}

INLINE void luamachine_setnumparameters(psy_audio_LuaMachine* self, uintptr_t num)
{
	self->numparameters_ = num;
}

INLINE uintptr_t luamachine_numparameters(psy_audio_LuaMachine* self)
{
	return self->numparameters_;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_LUAMACHINE_H */
