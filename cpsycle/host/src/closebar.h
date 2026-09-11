/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CLOSEBAR_H)
#define CLOSEBAR_H


/* driver */
#include "../../driver/eventdriver.h"
/* ui */
#include <uibutton.h>

#ifdef __cplusplus
extern "C" {
#endif

struct InputHandler;

/*!
** @struct CloseBar
*/
typedef struct CloseBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Button hide;	
	/* references */
	psy_Property* property;
	psy_EventDriverCmd cmd;
	char* cmd_section;	
	bool custom;
} CloseBar;

void closebar_init(CloseBar*, psy_ui_Component* parent, psy_Property*);
void closebar_init_cmd(CloseBar*, psy_ui_Component* parent,
	const char* section, psy_EventDriverCmd);

void closebar_set_cmd(CloseBar*, const char* section,
	psy_EventDriverCmd);
void closebar_set_property(CloseBar*, psy_Property*);
void closebar_set_custom_mode(CloseBar*);

INLINE psy_ui_Component* closebar_base(CloseBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* CLOSEBAR_H */
