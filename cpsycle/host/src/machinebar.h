/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MACHINEBAR_H)
#define MACHINEBAR_H

/* host */
#include "workspace.h"
/* ui */
#include "uibutton.h"
#include "uicombobox.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {	
	MACHINEBAR_AUX,
	MACHINEBAR_PARAM
} MachineBarInstParamMode;


/*
** @struct MachineBar
** @brief Selects machine settings
**
** @detail
** Opens Gear Rack, CPU, MIDI, Lua Plugin Editor View
** Sampler : Select Instrument – Selects the instrument to be used or
** other Generator/Effect: Select Tweak Parameter
*/

typedef struct MachineBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_ComboBox machinebox;	
	psy_ui_Button gear;
	psy_ui_Button dock;
	psy_ui_Button editor;	
	psy_ui_ComboBox select_inst_param_;
	psy_ui_ComboBox inst_param_box_;
	psy_ui_Button load;
	psy_ui_Button edit;	
	psy_audio_Machines* machines;
	psy_audio_Instruments* instruments;	
	psy_Table comboboxslots;
	psy_Table slotscombobox;
	uintptr_t curr_instr_group;
	/* references */
	Workspace* workspace;
} MachineBar;

void machinebar_init(MachineBar*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* machinebar_base(MachineBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MACHINEBAR_H */
