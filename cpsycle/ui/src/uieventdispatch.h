/* This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_EVENTDISPATCH_H
#define psy_ui_EVENTDISPATCH_H

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

struct psy_ui_Component;
struct psy_ui_Event;

/*
** @struct psy_ui_EventDispatch
*/
typedef struct psy_ui_EventDispatch {	
	bool handle_double_click_;
	uintptr_t last_button_;
	uintptr_t last_button_timestamp_;
	intptr_t accum_wheel_delta_;
	intptr_t delta_per_line_;
} psy_ui_EventDispatch;

void psy_ui_eventdispatch_init(psy_ui_EventDispatch*);
void psy_ui_eventdispatch_dispose(psy_ui_EventDispatch*);

INLINE void psy_ui_eventdispatch_handle_double_click(
	psy_ui_EventDispatch* self)
{
	self->handle_double_click_ = TRUE;
}

INLINE void psy_ui_eventdispatch_disable_handle_double_click(
	psy_ui_EventDispatch* self)
{
	self->handle_double_click_ = FALSE;
}

void psy_ui_eventdispatch_send(psy_ui_EventDispatch*,
	struct psy_ui_Component*, struct psy_ui_Event*);

INLINE void psy_ui_eventdispatch_set_wheel_delta_per_line(
	psy_ui_EventDispatch* self, intptr_t deltaperline)
{
	self->delta_per_line_ = deltaperline;
}

INLINE intptr_t psy_ui_eventdispatch_wheel_delta_per_line(
	const psy_ui_EventDispatch* self)
{
	return self->delta_per_line_;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_EVENTDISPATCH_H */
