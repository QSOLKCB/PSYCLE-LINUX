/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "clockbar.h"

/* platform */
#include "../../detail/portable.h"

#define CLOCKBAR_REFRESHRATE 200


/* prototypes */
static void clockbar_update_label(ClockBar*);
static void clockbar_on_timer(ClockBar*, uintptr_t timer_id);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(ClockBar* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);		
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			clockbar_on_timer;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(clockbar_base(self), &vtable);
}

/* implementation */
void clockbar_init(ClockBar* self, psy_ui_Component* parent)
{	
	assert(self);
		
	psy_ui_component_init(&self->component, parent, NULL);	
	vtable_init(self);
	self->display_minutes_ = FALSE;
	self->start_ = time(NULL);
	psy_ui_component_set_align_expand(clockbar_base(self), psy_ui_HEXPAND);	
	psy_ui_label_init(&self->position_, clockbar_base(self));
	psy_ui_label_prevent_translation(&self->position_);
	psy_ui_label_set_char_number(&self->position_, 10.0);
	psy_ui_component_set_align(psy_ui_label_base(&self->position_),
		psy_ui_ALIGN_LEFT);
	psy_ui_component_set_encoding(psy_ui_label_base(&self->position_),
		PSY_ENCODING_NONE);
	clockbar_update_label(self);
}

void clockbar_update_label(ClockBar* self)
{
	time_t currtime;
	char text[80];

	assert(self);
	
	currtime = time(NULL) - self->start_;
	if (self->display_minutes_) {
		psy_snprintf(text, 40, "%02ldm %02lds",
			(int)(currtime / 60.0), (int)(currtime) % 60);
	} else {
		psy_snprintf(text, 40, "%02ld:%02ld",
			(int)(currtime / 3600.0), (int)(currtime / 60.0) % 60);
	}
#ifndef PSYCLE_DEBUG_PREVENT_TIMER_DRAW
	psy_ui_label_set_text(&self->position_, text);
#endif
}

void clockbar_reset(ClockBar* self)
{
	assert(self);
	
	self->start_ = time(NULL);
	clockbar_update_label(self);
}

void clockbar_start(ClockBar* self)
{
	assert(self);
	
	psy_ui_component_start_timer(clockbar_base(self), 0, CLOCKBAR_REFRESHRATE);
}

void clockbar_restart(ClockBar* self)
{
	assert(self);
	
	clockbar_reset(self);
	clockbar_start(self);
}

void clockbar_stop(ClockBar* self)
{
	assert(self);
	
	psy_ui_component_stop_timer(clockbar_base(self), 0);
}

void clockbar_on_timer(ClockBar* self, uintptr_t timer_id)
{
	assert(self);
	
	clockbar_update_label(self);
}
