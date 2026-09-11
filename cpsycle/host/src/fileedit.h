/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FILEEDIT_H)
#define FILEEDIT_H

/* ui */
#include <uibutton.h>
#include <uitext.h>
/* file */
#include <fileselect.h>


struct Workspace;

/*! @struct FileEdit */

typedef struct FileEdit {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Edit edit_;
	psy_ui_Button dialog_;
	/* references */
	psy_Property* property_;
	struct Workspace* workspace_;
	psy_DiskOp* disk_op_;
} FileEdit;

void fileedit_init(FileEdit*, psy_ui_Component* parent, psy_DiskOp*);

FileEdit* fileedit_alloc(void);
FileEdit* fileedit_allocinit(psy_ui_Component* parent, psy_DiskOp*);

void fileedit_exchange(FileEdit*, psy_Property*);
void fileedit_set_workspace(FileEdit*, struct Workspace*);

INLINE psy_ui_Component* fileedit_base(FileEdit* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* FILEEDIT_H */
