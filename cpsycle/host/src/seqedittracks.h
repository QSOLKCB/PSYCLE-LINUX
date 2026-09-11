/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQEDITTRACKS_H)
#define SEQEDITTRACKS_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "seqeditline.h"
#include "seqedittrack.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SeqEditorTracks {
	psy_ui_Component component;
	SeqEditState* state;
	Workspace* workspace;	
} SeqEditorTracks;

void seqeditortracks_init(SeqEditorTracks*, psy_ui_Component* parent,
	SeqEditState*, Workspace*);

void seqeditortracks_build(SeqEditorTracks*);
void seqeditortracks_checkcursorline(SeqEditorTracks*);

INLINE psy_ui_Component* seqeditortracks_base(SeqEditorTracks* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_SEQEDITOR */

#endif /* SEQEDITTRACKS_H */
