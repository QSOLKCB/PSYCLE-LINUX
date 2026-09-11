/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiresources.h"
/* platform */
#include "../../detail/portable.h"


/* implementation */
void psy_ui_resource_init(psy_ui_Resource* self)
{
	assert(self);

	self->type = psy_ui_RESOURCE_TYPE_SVG;
}

void psy_ui_resource_init_all(psy_ui_Resource* self, char* data, psy_ui_ResourceType type,
	psy_fp_disposefunc dispose)
{
	assert(self);

	psy_ui_resource_init(self);
	self->type = type;
	self->data = data;
	self->dispose = dispose;
}

void psy_ui_resource_dispose(psy_ui_Resource* self)
{
	assert(self);

	if (self->dispose && self->data) {
		self->dispose(self->data);
		free(self->data);
		self->data = NULL;
	}
}

psy_ui_Resource* psy_ui_resource_alloc(void)
{
	return (psy_ui_Resource*)malloc(sizeof(psy_ui_Resource));
}

psy_ui_Resource* psy_ui_resource_alloc_init(void)
{
	psy_ui_Resource* rv;

	rv = psy_ui_resource_alloc();
	if (rv) {
		psy_ui_resource_init(rv);		
	}
	return rv;
}

psy_ui_Resource* psy_ui_resource_alloc_init_all(void* data, psy_ui_ResourceType type,
	psy_fp_disposefunc dispose)
{
	psy_ui_Resource* rv;

	rv = psy_ui_resource_alloc();
	if (rv) {
		psy_ui_resource_init_all(rv, data, type, dispose);
	}
	return rv;
}


/* implementation */
void psy_ui_resources_init(psy_ui_Resources* self)
{
	assert(self);
	
	psy_table_init(&self->resources_);	
}

void psy_ui_resources_dispose(psy_ui_Resources* self)
{
	assert(self);
	
	psy_table_dispose_all(&self->resources_, 
		psy_ui_resource_dispose);
}

void psy_ui_resources_add(psy_ui_Resources* self, const char* key,
	psy_ui_Resource* resource)
{
	assert(self);

	psy_table_insert_strhash(&self->resources_, key, resource);		
}

const psy_ui_Resource* psy_ui_resources_at(const psy_ui_Resources* self, const char* key)	
{
	assert(self);

	return (const psy_ui_Resource*)psy_table_at_strhash_const(&self->resources_, key);
}
