/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(psy_UNDOREDO_H)
#define psy_UNDOREDO_H

/* local */
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_Command;

/*! 
** @struct psy_UndoRedo
** @brief Command Handler for undo redo commands */

typedef struct psy_UndoRedo {
	psy_List* undo_;
	psy_List* redo_;
} psy_UndoRedo;

void psy_undoredo_init(psy_UndoRedo*);
void psy_undoredo_dispose(psy_UndoRedo*);

void psy_undoredo_undo(psy_UndoRedo*);
void psy_undoredo_redo(psy_UndoRedo*);
void psy_undoredo_execute(psy_UndoRedo*, struct psy_Command*);
uintptr_t psy_undoredo_undo_size(const psy_UndoRedo*);
uintptr_t psy_undoredo_redo_size(const psy_UndoRedo*);


#ifdef __cplusplus
}
#endif

#endif /* psy_UNDOREDO_H */
