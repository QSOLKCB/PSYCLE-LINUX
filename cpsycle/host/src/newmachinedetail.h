/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(NEWMACHINEDETAIL_H)
#define NEWMACHINEDETAIL_H

/* host */
#include "labelpair.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uicheckbox.h>
#include <uitext.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* NewMachineDetail */

typedef struct NewMachineDetail {
	/*! @extends  */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_categorychanged;
	/* intern */
	psy_ui_Component details;
	psy_ui_Label detaildesc;
	LabelPair plugname;
	LabelPair desc;		
	LabelPair dllname;
	LabelPair version;
	LabelPair apiversion;
	psy_ui_Component category;
	psy_ui_Label categorydesc;
	psy_ui_Edit categoryedit;
	psy_ui_Component bottom;
    psy_ui_Label compatlabel;
    psy_ui_CheckBox compatblitzgamefx;	
	psy_audio_MachineInfo plugin;
    Workspace* workspace;
} NewMachineDetail;

void newmachinedetail_init(NewMachineDetail*, psy_ui_Component* parent,
	Workspace*);

void newmachinedetail_set_plugin(NewMachineDetail*,
	const psy_audio_MachineInfo*);
void newmachinedetail_setdescription(NewMachineDetail*, const char* text);
void newmachinedetail_setplugname(NewMachineDetail*, const char* text);
void newmachinedetail_setdllname(NewMachineDetail*, const char* text);
void newmachinedetail_setcategoryname(NewMachineDetail*, const char* text);
const char* newmachinedetail_category(const NewMachineDetail* self);	
void newmachinedetail_setplugversion(NewMachineDetail* self, intptr_t version);
void newmachinedetail_setapiversion(NewMachineDetail* self,
	intptr_t apiversion);
const psy_audio_MachineInfo* newmachinedetail_plugin(const NewMachineDetail*);

#ifdef __cplusplus
}
#endif

#endif /* NEWMACHINEDETAIL_H */
