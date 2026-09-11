/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(psy_audio_DRIVERREGISTER_H)
#define psy_audio_DRIVERREGISTER_H


/* container */
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** DriverRegister
**
*/

typedef struct psy_audio_DriverRegister {
	psy_Property* drivers_;	
	intptr_t default_guid;
} psy_audio_DriverRegister;

void psy_audio_driverregister_init(psy_audio_DriverRegister*,
	psy_Property* parent, const char* key, const char* text);
void psy_audio_driverregister_dispose(psy_audio_DriverRegister*);

psy_List* psy_audio_driverregister_begin(psy_audio_DriverRegister*);
void psy_audio_driverregister_register(psy_audio_DriverRegister*,
	intptr_t guid, const char* path, const char* label);
const char* psy_audio_driverregister_path(const psy_audio_DriverRegister*,
	intptr_t guid);
const char* psy_audio_driverregister_label(const psy_audio_DriverRegister*,
	intptr_t guid);
	
intptr_t psy_audio_driverregister_default_guid(const psy_audio_DriverRegister*);


INLINE void psy_audio_driverregister_set_default_guid(
	psy_audio_DriverRegister* self, intptr_t guid)
{
	assert(self);
	
	self->default_guid = guid;
}

void psy_audio_driverregister_update_default(psy_audio_DriverRegister*);

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_DRIVERREGISTER_H */
