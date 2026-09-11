/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_TERMINAL_H
#define psy_ui_TERMINAL_H

/* local */
#include "uilabel.h"
#include "uiscroller.h"
/* container */
#include <logger.h>
#include <stringbuffer.h>
/* thread */
#include <lock.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_ui_Terminal
*/

typedef struct psy_ui_Terminal {
	/*! @extends  */
	psy_ui_Component component;
	/* extends */
	psy_Logger logger;	
	/*! @internal */
	psy_ui_Scroller scroller_;
	psy_ui_Label output_;
	psy_StringBuffer buffer_;
	int last_status_;
	bool scroll_to_bottom_;
} psy_ui_Terminal;

void psy_ui_terminal_init(psy_ui_Terminal*, psy_ui_Component* parent);

/* output synchronized with ui thread */
void psy_ui_terminal_output(psy_ui_Terminal*, const char* text);
void psy_ui_terminal_output_warn(psy_ui_Terminal*, const char* text);
void psy_ui_terminal_output_error(psy_ui_Terminal*, const char* text);
void psy_ui_terminal_clear(psy_ui_Terminal*);
void psy_ui_terminal_flush(psy_ui_Terminal*);

INLINE psy_ui_Component* psy_ui_terminal_base(psy_ui_Terminal* self)
{
	return &self->component;
}

INLINE psy_Logger* psy_ui_terminal_logger_base(psy_ui_Terminal* self)
{
	return &self->logger;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_TERMINAL_H */
