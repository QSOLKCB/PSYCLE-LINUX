/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "driverregister.h"

/* platform */
#include "../../driver/audiodriver.h"
#include "../../detail/portable.h"


/* implementation */
void psy_audio_driverregister_init(psy_audio_DriverRegister* self,
	psy_Property* parent, const char* key, const char* text)
{
	assert(self);
	assert(parent);
	
	self->default_guid = 0;	
	self->drivers_ = psy_property_set_text(psy_property_append_choice(
		parent, key, 0), text);	
}

void psy_audio_driverregister_dispose(psy_audio_DriverRegister* self)
{
	assert(self);
		
}

void psy_audio_driverregister_register(psy_audio_DriverRegister* self,
	intptr_t guid, const char* path, const char* label)
{
	assert(self);	
	
	if (psy_strlen(path) > 0) {			
		char key[64];
		
		psy_snprintf(key, 64, "%d", (int)guid);
		psy_property_prevent_save(psy_property_set_text(psy_property_append_str(
			self->drivers_, key, path), label));		
	}
}

const char* psy_audio_driverregister_path(const psy_audio_DriverRegister* self,
	intptr_t guid)
{	
	char key[64];
	
	assert(self);	
	
	psy_snprintf(key, 64, "%d", (int)guid);
	return psy_property_at_str(self->drivers_, key, "");
}

const char* psy_audio_driverregister_label(const psy_audio_DriverRegister* self,
	intptr_t guid)
{		
	char key[64];
	psy_Property* p;
	
	assert(self);	
	
	psy_snprintf(key, 64, "%d", (int)guid);
	p = psy_property_at(self->drivers_, key, PSY_PROPERTY_TYPE_NONE);
	if (p) {
		return psy_property_text(p);
	}
	return "-";
}

intptr_t psy_audio_driverregister_default_guid(const psy_audio_DriverRegister*
	self)
{
	assert(self);
	
	return self->default_guid;
}

void psy_audio_driverregister_update_default(psy_audio_DriverRegister* self)
{		
	uintptr_t default_index;
	uintptr_t c;
	psy_List* it;

	assert(self);
			
	default_index = 0;
	for (it = psy_property_begin(self->drivers_),
			c = 0; it != NULL; it = it->next, ++c) {
		psy_Property* property;			

		property = (psy_Property*)it->entry;
		if (atoi(psy_property_key(property)) == psy_audio_driverregister_default_guid(self)) {
			default_index = c;
			break;
		}		
	}	
	psy_property_set_item_int(self->drivers_, default_index);	
}

psy_List* psy_audio_driverregister_begin(psy_audio_DriverRegister* self)
{
	assert(self);
	
	return psy_property_begin(self->drivers_);	
}
