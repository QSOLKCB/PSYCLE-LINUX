/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(GEAR_H)
#define GEAR_H

/* host */
#include "instrumentsbox.h"
#include "machinesbox.h"
#include "samplesbox.h"
#include "titlebar.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uinotebook.h>
#include <uitabbar.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ParamViews;

/*!
** @struct GearButtons
** @brief Command buttons of Gear
*/
typedef struct GearButtons {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Button createreplace;
	psy_ui_Button del;
	psy_ui_Button parameters;
	psy_ui_Button properties;
	psy_ui_Button exchange;
	psy_ui_Button clone;
	psy_ui_Button showmaster;
	psy_ui_NavHandler nav_handler;
} GearButtons;

void gearbuttons_init(GearButtons*, psy_ui_Component* parent,
	struct ParamViews*);

INLINE psy_ui_Component* gearbuttons_base(GearButtons* self)
{
	return &self->component;
}

/*!
** @struct Gear
** @brief Organise your machines
** @details
** The Gear Rack is a convenient way to organise your machines. Use the
** buttons "Generators", "Effects" and "Instruments" to switch between the
** different tabs
*/
typedef struct Gear {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component client;
	psy_ui_Component bottom;
	psy_ui_TabBar tabbar;	
	TitleBar title_bar_;
	psy_ui_Component view_buttons;
	psy_ui_Component view_buttons_client;	
	psy_ui_Button view_float;
	psy_ui_Label label;
	psy_ui_Notebook notebook;	
	MachinesBox machinesboxgen;
	MachinesBox machinesboxfx;
	InstrumentsBox instrumentsbox;
	SamplesBox samplesbox;	
	GearButtons buttons;
	psy_ui_Sizer sizer_;
	psy_ui_Component* splitter_;
	/* references */	
	struct ParamViews* param_views;
	Workspace* workspace;	
} Gear;

void gear_init(Gear*, psy_ui_Component* parent, struct ParamViews*,
	Workspace*);

INLINE void gear_set_splitter(Gear* self, psy_ui_Component* splitter)
{
	assert(self);

	self->splitter_ = splitter;
}


INLINE psy_ui_Component* gear_base(Gear* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* GEAR_H */
