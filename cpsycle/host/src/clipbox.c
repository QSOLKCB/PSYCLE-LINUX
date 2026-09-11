/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "clipbox.h"
/* host */
#include "styles.h"
/* audio */
#include <machine.h>
/* dsp */
#include <operations.h>


static bool check_peak(float peak)
{
	return ((peak >= 32767.f) || (peak < -32768.f));
}

/* prototypes */
static void clipbox_on_timer(ClipBox*, uintptr_t timerid);
static void clipbox_on_mouse_down(ClipBox*, psy_ui_MouseEvent*);
static bool clipbox_check(ClipBox*);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(ClipBox* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(self->component.vtable);		
		vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			clipbox_on_mouse_down;
		vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			clipbox_on_timer;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void clipbox_init(ClipBox* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);
	self->machine_ = NULL;
	psy_ui_component_set_style_type(clipbox_base(self), STYLE_CLIPBOX);
	psy_ui_component_set_style_type_select(clipbox_base(self),
		STYLE_CLIPBOX_SELECT);	
	psy_ui_component_start_timer(clipbox_base(self), 0, 100);
}

void clipbox_on_timer(ClipBox* self, uintptr_t timerid)
{	
	assert(self);
	
	if (!clipbox_activated(self) && clipbox_check(self)) {		
		clipbox_activate(self);					
	}
}

bool clipbox_check(ClipBox* self)
{
	psy_audio_Buffer * memory;
	
	assert(self);
	
	if (!self->machine_) {
		return FALSE;
	}
	memory = psy_audio_machine_buffermemory(self->machine_);
	if (memory && psy_audio_buffer_num_frames(memory) > 0) {
		if (psy_audio_buffer_num_channels(memory) > 0 &&
				check_peak(dsp.maxvol(memory->samples[0],
					psy_audio_buffer_num_frames(memory)))) {
			return TRUE;
		}
		if (psy_audio_buffer_num_channels(memory) > 1 &&
				check_peak(dsp.maxvol(memory->samples[1],
					psy_audio_buffer_num_frames(memory)))) {
			return TRUE;
		}
	}
	return FALSE;
}

void clipbox_on_mouse_down(ClipBox* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	clipbox_deactivate(self);	
}
