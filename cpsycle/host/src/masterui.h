/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MASTERUI_H)
#define MASTERUI_H

/* host */
#include "machineui.h" /* vst view */
/* ui */
#include <uicomponent.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MasterUi */
typedef struct MasterUi {
	/*! @extends  */
	psy_ui_Component component;	
	/*! @internal */
	psy_ui_Label name_;
	bool prevent_machine_pos_;
	/* references */	
	psy_audio_Machine* machine_;
	ParamViews* param_views_;
	psy_Configuration* config_;
} MasterUi;

void masterui_init(MasterUi*, psy_ui_Component* parent, ParamViews*,
	psy_audio_Machines*, psy_Configuration* config);

MasterUi* masterui_alloc(void);
MasterUi* masterui_alloc_init(psy_ui_Component* parent, ParamViews*,
	psy_audio_Machines*, psy_Configuration* config);

INLINE void masterui_prevent_machine_pos(MasterUi* self)
{
	assert(self);
	
	self->prevent_machine_pos_ = TRUE;	
}

INLINE void masterui_enable_machine_pos(MasterUi* self)
{
	assert(self);
	
	self->prevent_machine_pos_ = FALSE;	
}

INLINE psy_ui_Component* masterui_base(MasterUi* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MASTERUI_H */
