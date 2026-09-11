/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_STRINGBUFFER_H
#define psy_ui_STRINGBUFFERL_H

/* local */
#include "list.h"
/* thread */
#include <lock.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef psy_List psy_StringBufferNode;

typedef struct psy_StringBuffer {
	psy_List* buffer_;
	psy_Lock lock_;
} psy_StringBuffer;
	
void psy_stringbuffer_init(psy_StringBuffer*);
void psy_stringbuffer_dispose(psy_StringBuffer*);

void psy_stringbuffer_clear(psy_StringBuffer*);
psy_List* psy_stringbuffer_begin(psy_StringBuffer*);
bool psy_stringbuffer_empty(const psy_StringBuffer*);
void psy_stringbuffer_write(psy_StringBuffer*, const char* text);
void psy_stringbuffer_lock(psy_StringBuffer*);
void psy_stringbuffer_unlock(psy_StringBuffer*);

INLINE const char* psy_stringbuffernode_str(const psy_StringBufferNode* node)
{
	assert(node);

	return node->entry;
}


#ifdef __cplusplus
}
#endif

#endif /* psy_ui_STRINGBUFFERL_H */
