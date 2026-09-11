/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMVIEW_H)
#define PARAMVIEW_H

/* ui */
#include <uicomponent.h>
/* audio */
#include <machine.h>
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct ParamView
** @brief Displays the native machine parameter uis
*/
typedef struct ParamView {
	/*! @extends */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_edit;
	/*! @internal */
	psy_ui_Component pane;
	psy_audio_Machine* machine;   
	psy_Configuration* cfg;
	int sizechanged;
	uintptr_t paramstrobe;
	psy_ui_FontInfo fontinfo;   
	psy_ui_Component** frameview;	
	bool use_column_hint;
} ParamView;

void paramview_init(ParamView*, psy_ui_Component* parent, psy_audio_Machine*,
	psy_Configuration* param_cfg, psy_ui_Component** frameview,
	bool use_column_hint);

ParamView* paramview_alloc(void);
ParamView* paramview_allocinit(psy_ui_Component* parent, psy_audio_Machine*,
	psy_Configuration* param_cfg, psy_ui_Component** frameview,
	bool use_column_hint);
	
void paramview_set_zoom(ParamView*, double zoomrate);

#ifdef __cplusplus
}
#endif

#endif /* PARAMVIEW_H */
