/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqedittracks.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "styles.h"


/* prototypes */
static void seqeditortracks_on_mouse_down(SeqEditorTracks*, psy_ui_MouseEvent*);
static void seqeditortracks_on_mouse_up(SeqEditorTracks*, psy_ui_MouseEvent*);

/* vtable */
static psy_ui_ComponentVtable seqeditortracks_vtable;
static bool seqeditortracks_vtable_initialized = FALSE;

static void seqeditortracks_vtable_init(SeqEditorTracks* self)
{
	if (!seqeditortracks_vtable_initialized) {
		seqeditortracks_vtable = *(self->component.vtable);		
		seqeditortracks_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			seqeditortracks_on_mouse_down;
		seqeditortracks_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			seqeditortracks_on_mouse_up;		
		seqeditortracks_vtable_initialized = TRUE;
	}
	self->component.vtable = &seqeditortracks_vtable;
}

/* implementation */
void seqeditortracks_init(SeqEditorTracks* self, psy_ui_Component* parent,
	SeqEditState* state, Workspace* workspace)
{		
	psy_ui_component_init(&self->component, parent, NULL);
	seqeditortracks_vtable_init(self);
	self->workspace = workspace;
	self->state = state;	
	psy_ui_component_set_style_type(&self->component, STYLE_SEQEDT_TRACKS);
	psy_ui_component_set_scroll_step(&self->component,
		psy_ui_size_make_em(8.0, 0.0));	
	psy_ui_component_set_wheel_scroll(&self->component, 1);	
	psy_ui_component_set_overflow(&self->component, psy_ui_OVERFLOW_SCROLL);	
	psy_ui_component_set_default_align(&self->component,
		psy_ui_ALIGN_TOP, psy_ui_margin_zero());	
	seqeditortracks_build(self);
}

void seqeditortracks_build(SeqEditorTracks* self)
{
	psy_audio_Sequence* sequence;	
	
	psy_ui_component_clear(&self->component);
	sequence = seqeditstate_sequence(self->state);	
	if (sequence) {
		uintptr_t t;
		uintptr_t width;

		width = psy_audio_sequence_width(sequence);
		for (t = 0; t < width ; ++t) {
			psy_audio_SequenceTrack* seqtrack;
			SeqEditTrack* seqedittrack;
						
			seqtrack = psy_audio_sequence_track_at(sequence, t);			
			seqedittrack = seqedittrack_allocinit(&self->component,
				self->state, seqtrack, t);			
		}		
	}	
}

void seqeditortracks_on_mouse_down(SeqEditorTracks* self,
	psy_ui_MouseEvent* ev)
{
	if (self->state->dragstatus == SEQEDIT_DRAG_REMOVE) {
		sequencecmds_delentry(self->state->cmds);
		self->state->dragstatus = SEQEDIT_DRAG_NONE;
		psy_ui_mouseevent_stop_propagation(ev);
	} else if ((self->state->dragtype & SEQEDIT_DRAGTYPE_REORDER)
			== SEQEDIT_DRAGTYPE_REORDER) {		
	}
}

void seqeditortracks_on_mouse_up(SeqEditorTracks* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);	
	self->state->cmd = SEQEDTCMD_NONE;
	self->state->seqentry = NULL;
	self->state->dragseqpos = psy_audio_orderindex_make_invalid();
	self->state->dragstatus = SEQEDIT_DRAG_NONE;
	psy_ui_mouseevent_stop_propagation(ev);
}

#endif /* PSYCLE_USE_SEQEDITOR */
