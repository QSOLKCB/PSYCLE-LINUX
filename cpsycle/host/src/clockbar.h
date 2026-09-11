/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CLOCKBAR_H)
#define CLOCKBAR_H

/* ui */
#include "uilabel.h"

/* std */
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 
** @struct ClockBar
** @brief Displays the edit time of a song
*/

typedef struct ClockBar {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Label header_;
	psy_ui_Label position_;
	time_t start_;
	bool display_minutes_;
} ClockBar;

void clockbar_init(ClockBar*, psy_ui_Component* parent);

void clockbar_reset(ClockBar*);
void clockbar_start(ClockBar*);
void clockbar_restart(ClockBar*);
void clockbar_stop(ClockBar*);


INLINE void clockbar_display_minutes(ClockBar* self)
{
	self->display_minutes_ = TRUE;
}

INLINE psy_ui_Component* clockbar_base(ClockBar* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* CLOCKBAR_H */
