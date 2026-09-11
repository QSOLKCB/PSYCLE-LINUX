/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "luamachine.h"
/* lua */
#include "lauxlib.h"
#include "lualib.h"
/* local */
#include "luaimport.h"
#include "luaplayer.h"
#include "array.h"
#include "luaarray.h"
#include "luaenvelope.h"
#include "luafilter.h"
#include "luadspmath.h"
#include "luawavedata.h"
#include "luawaveosc.h"
#include "luaresampler.h"
#include "luamidinotes.h"
#include "songio.h"
#include "custommachine.h"
#include "plugin_interface.h"
#include "exclusivelock.h"
#include "lock.h"
#include "machines.h"
#include "machinefactory.h"
#include <uicomponent.h>
#include <luapoint.h>
#include <list.h>

#include <stdlib.h>
#include <string.h>
#include "../../detail/portable.h"

void psy_audio_luamachine_init(psy_audio_LuaMachine* self)
{
	psy_audio_CustomMachine* custommachine;

	custommachine = (psy_audio_CustomMachine*)malloc(sizeof(psy_audio_CustomMachine));
	if (custommachine) {
		psy_audio_custommachine_init(custommachine, NULL);
		self->machine = &custommachine->machine;
	}
	else {
		self->machine = 0;
	}
	self->bc = 0;
	self->numparameters_ = 0;
	self->numcols_ = 0;
	self->numprograms_ = 0;
	self->shared_ = FALSE;
	self->bcshared_ = FALSE;
}

void psy_audio_luamachine_init_shared(psy_audio_LuaMachine* self,
	psy_audio_Machine* machine)
{
	self->machine = machine;
	self->bc = 0;
	self->numparameters_ = 0;
	self->numcols_ = 0;
	self->numprograms_ = 0;
	self->shared_ = TRUE;
	self->bcshared_ = FALSE;
}

void psy_audio_luamachine_init_machine(psy_audio_LuaMachine* self,
	psy_audio_Machine* machine)
{
	self->machine = machine;
	self->bc = 0;
	self->numparameters_ = 0;
	self->numcols_ = 0;
	self->numprograms_ = 0;
	self->shared_ = FALSE;
	self->bcshared_ = FALSE;
}

void psy_audio_luamachine_dispose(psy_audio_LuaMachine* self)
{
	if (self->machine && !luamachine_shared(self)) {
		machine_base_dispose(self->machine);
		free(self->machine);
		self->machine = 0;
	}
	if (self->bcshared_ == FALSE) {
		if (self->bc && self->bc->output_) {
			psy_audio_buffer_dispose(self->bc->output_);
		}
		free(self->bc);
	}
}
