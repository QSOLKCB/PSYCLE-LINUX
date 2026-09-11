/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TITLEBAR_H)
#define TITLEBAR_H

/* host */
#include "closebar.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct TitleBar
*/
typedef struct TitleBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	CloseBar close_bar_;
	psy_ui_Component client_;
	psy_ui_Label title_;
	char* dragid_;
} TitleBar;

void titlebar_init(TitleBar*, psy_ui_Component* parent, const char* title);

void titlebar_enable_drag(TitleBar*, const char* dragid);

INLINE psy_ui_Component* titlebar_base(TitleBar* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* TITLEBAR_H */
