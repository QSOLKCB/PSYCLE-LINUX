/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MAINVIEWBAR_H)
#define MAINVIEWBAR_H

/* host */
#include "minmaximize.h"
#include "navigation.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uinotebook.h>
#include <uitabbar.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** MainViewBar
*/
typedef struct MainViewBar {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component row_0_;
	psy_ui_Component row_1_;
	psy_ui_Component view_buttons_;
	psy_ui_Component view_buttons_client_;
	psy_ui_Button extract_left_;
	psy_ui_Button extract_right_;
	psy_ui_Button extract_top_;
	psy_ui_Button extract_bottom_;
	psy_ui_Button view_float_;
	psy_ui_Button maximize_btn_;
	psy_ui_Component tab_bars_;
	Navigation navigation_;
	psy_ui_TabBar tab_bar_;
	psy_ui_TabBar script_tab_bar_;
	psy_ui_Button toggle_scripts_;
	MinMaximize min_maximize_;
	psy_ui_Notebook view_tab_bars_;
	psy_ui_Component empty_view_tab_bar_;		
} MainViewBar;

void mainviewbar_init(MainViewBar*, psy_ui_Component* parent,
	psy_ui_Component* pane, Workspace*);

void mainviewbar_add_minmaximze(MainViewBar*, psy_ui_Component*);
void mainviewbar_toggle_minmaximze(MainViewBar*);

INLINE psy_ui_Component* mainviewbar_base(MainViewBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MAINVIEWBAR_H */
