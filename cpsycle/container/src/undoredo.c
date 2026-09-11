/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "undoredo.h"
/* local */
#include "command.h"
/* std */
#include <assert.h>

/* prototypes */
static void psy_undoredo_clear_redo(psy_UndoRedo*);
static void psy_undoredo_clear_undo(psy_UndoRedo*);
static psy_Command* psy_undoredo_swap(psy_List** first, psy_List** second);

/* implementation */
void psy_undoredo_init(psy_UndoRedo* self)
{
	assert(self);

	self->undo_ = NULL;
	self->redo_ = NULL;
}

void psy_undoredo_dispose(psy_UndoRedo* self)
{
	assert(self);

	psy_undoredo_clear_undo(self);
	psy_undoredo_clear_redo(self);
}

void psy_undoredo_undo(psy_UndoRedo* self)
{	
	psy_Command* command;

	assert(self);

	command = psy_undoredo_swap(&self->undo_, &self->redo_);
	if (command) {
		psy_command_revert(command);
	}
}

void psy_undoredo_redo(psy_UndoRedo* self)
{	
	psy_Command* command;
	
	assert(self);

	command = psy_undoredo_swap(&self->redo_, &self->undo_);
	if (command) {
		psy_command_execute(command, 0);
	}	
}

psy_Command* psy_undoredo_swap(psy_List** first, psy_List** second)
{
	assert(first && second);

	if (*first) {		
		psy_List* last;

		last = psy_list_last(*first);
		if (last) {
			psy_Command* rv;

			rv = last->entry;
			psy_list_append(second, rv);
			psy_list_remove(first, last);
			return rv;
		}
	} 
	return NULL;
}

void psy_undoredo_execute(psy_UndoRedo* self, psy_Command* command)
{		
	assert(self);

	psy_list_append(&self->undo_, command);
	psy_command_execute(command, 0);
	psy_undoredo_clear_redo(self);
}

uintptr_t psy_undoredo_undo_size(const psy_UndoRedo* self)
{
	assert(self);

	return psy_list_size(self->undo_);
}

uintptr_t psy_undoredo_redo_size(const psy_UndoRedo* self)
{
	assert(self);

	return psy_list_size(self->redo_);
}

void psy_undoredo_clear_undo(psy_UndoRedo* self)
{
	assert(self);
	
	psy_list_deallocate(&self->undo_, (psy_fp_disposefunc)psy_command_dispose);
}

void psy_undoredo_clear_redo(psy_UndoRedo* self)
{
	assert(self);

	psy_list_deallocate(&self->redo_, (psy_fp_disposefunc)psy_command_dispose);
}
