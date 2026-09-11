/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MESSAGEVIEW_H)
#define MESSAGEVIEW_H

/* host */
#include "closebar.h"
/* ui */
#include <uibutton.h>
#include <uinotebook.h>
#include <uitabbar.h>
#include <uiterminal.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct MessageView
** @brief Displays status messages
*/

typedef struct MessageView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	CloseBar close_;
	psy_ui_Notebook notebook_;
	psy_ui_TabBar tabbar_;
	psy_ui_Component status_;
	psy_ui_Component status_bar_;
	psy_ui_Button status_clear_;
	psy_ui_Terminal terminal_;		
} MessageView;

void messageview_init(MessageView*, psy_ui_Component* parent);

INLINE psy_ui_Component* messageview_base(MessageView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MESSAGEVIEW_H */
