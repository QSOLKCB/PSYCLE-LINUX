/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "stringbuffer.h"
/* platform */
#include "../../detail/portable.h"


void psy_stringbuffer_init(psy_StringBuffer* self)
{
	assert(self);

	self->buffer_ = NULL;
	psy_lock_init(&self->lock_);
}

void psy_stringbuffer_dispose(psy_StringBuffer* self)
{
	assert(self);

	psy_list_deallocate(&self->buffer_, NULL);
	psy_lock_dispose(&self->lock_);
}

void psy_stringbuffer_write(psy_StringBuffer* self, const char* text)
{
	assert(self);

	if (text) {
		psy_lock_enter(&self->lock_);
		psy_list_append(&self->buffer_, psy_strdup(text));
		psy_lock_leave(&self->lock_);
	}
}

void psy_stringbuffer_clear(psy_StringBuffer* self)
{
	assert(self);

	psy_lock_enter(&self->lock_);
	psy_list_deallocate(&self->buffer_, NULL);
	psy_lock_leave(&self->lock_);
}

psy_List* psy_stringbuffer_begin(psy_StringBuffer* self)
{
	assert(self);

	return self->buffer_;
}

bool psy_stringbuffer_empty(const psy_StringBuffer* self)
{
	bool rv;

	assert(self);
	
	psy_lock_enter(&((psy_StringBuffer*)self)->lock_);
	rv = (self->buffer_ == NULL);	
	psy_lock_leave(&((psy_StringBuffer*)self)->lock_);
	return rv;
}

void psy_stringbuffer_lock(psy_StringBuffer* self)
{
	assert(self);

	psy_lock_enter(&((psy_StringBuffer*)self)->lock_);
}

void psy_stringbuffer_unlock(psy_StringBuffer* self)
{
	assert(self);

	psy_lock_leave(&((psy_StringBuffer*)self)->lock_);
}
