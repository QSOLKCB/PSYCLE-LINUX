/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PRESETSBAR_H)
#define PRESETSBAR_H

/* ui */
#include <uicombobox.h>
/* audio */
#include <machine.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** ProgramBar
**
** Select Bank and Program
*/

typedef struct ProgramBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_ComboBox combo_bank;	
	psy_ui_ComboBox combo_program;
	/* reference */
	psy_audio_Machine* machine;	
} ProgramBar;

void programbar_init(ProgramBar*, psy_ui_Component* parent);
void programbar_set_machine(ProgramBar*, psy_audio_Machine*);

INLINE psy_ui_Component* programbar_base(ProgramBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PRESETSBAR_H */
