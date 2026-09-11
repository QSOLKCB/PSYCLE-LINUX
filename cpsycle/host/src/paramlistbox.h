/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMLISTBOX_H)
#define PARAMLISTBOX_H

/* host */
#include "knobui.h"
/* ui */
#include <uilistbox.h>
/* audio */
#include <machine.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ParameterListBox {
	/*! @extends  */
	psy_ui_Component component;
	/* intern */
	psy_ui_ListBox listbox;
	KnobUi knob;
	/* references */
	psy_audio_Machine* machine;	
} ParameterListBox;

void parameterlistbox_init(ParameterListBox*, psy_ui_Component* parent, 
	psy_audio_Machine*);
void parameterlistbox_setmachine(ParameterListBox*, psy_audio_Machine*);
uintptr_t parameterlistbox_selected(ParameterListBox*);

INLINE psy_ui_Component* paramlistbox_base(ParameterListBox* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PARAMLISTBOX_H */
