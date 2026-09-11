/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqeditline.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "styles.h"
/* audio */
#include <exclusivelock.h>
/* platform */
#include "../../detail/portable.h"

/* SeqEditorLine */

void seqeditorline_init(SeqEditorLine* self, psy_ui_Component* parent,
	SeqEditState* state)
{
	assert(self);
	assert(state);

	psy_ui_component_init(seqeditorline_base(self), parent, NULL);
	self->state = state;
	psy_ui_component_set_preferred_size(seqeditorline_base(self),
		psy_ui_size_make(psy_ui_value_make_px(1.0),
			psy_ui_value_make_eh(10.0)));
	psy_ui_component_set_position(seqeditorline_base(self),
		psy_ui_rectangle_make(psy_ui_point_zero(), psy_ui_size_zero()));
}

SeqEditorLine* seqeditorline_alloc(void)
{
	return (SeqEditorLine*)malloc(sizeof(SeqEditorLine));
}

SeqEditorLine* seqeditorline_allocinit(psy_ui_Component* parent,
	SeqEditState* state)
{
	SeqEditorLine* rv;

	rv = seqeditorline_alloc();
	if (rv) {
		seqeditorline_init(rv, parent, state);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void seqeditorline_updateposition(SeqEditorLine* self,
	psy_dsp_beatpos_t position)
{
	double position_px;
	psy_ui_RealRectangle new_position;
	psy_ui_RealRectangle update_position;
	psy_ui_RealSize parentsize;

	assert(self);

	position_px = seqeditstate_beat_to_px(self->state, position) -
		self->state->tracks_scroll_left;
	update_position = psy_ui_component_position(&self->component);
	parentsize = psy_ui_component_clientsize_px(
		psy_ui_component_parent(&self->component));
	if (parentsize.height != psy_ui_realrectangle_height(&update_position) ||
		position_px != update_position.left) {
		new_position = psy_ui_realrectangle_make(
			psy_ui_realpoint_make(position_px, 0.0),
			psy_ui_realsize_make(1.0, parentsize.height));
		psy_ui_component_set_position(&self->component,
			psy_ui_rectangle_make_px(&new_position));
		psy_ui_realrectangle_union(&update_position, &new_position);
		psy_ui_component_invalidate_rect(
			psy_ui_component_parent(&self->component),
			update_position);
	}
}

/* SeqEditorPlayLine */

/* prototypes */
static void seqeditorplayline_on_mouse_down(SeqEditorPlayline*,
	psy_ui_MouseEvent*);
static void seqeditorplayline_on_mouse_move(SeqEditorPlayline*,
	psy_ui_MouseEvent*);
static void seqeditorplayline_on_mouse_up(SeqEditorPlayline*,
	psy_ui_MouseEvent*);

/* vtable */
static psy_ui_ComponentVtable seqeditorplayline_vtable;
static bool seqeditorplayline_vtable_initialized = FALSE;

static void seqeditorplayline_vtable_init(SeqEditorPlayline* self)
{
	if (!seqeditorplayline_vtable_initialized) {
		seqeditorplayline_vtable =
			*(seqeditorline_base(&self->seqeditorline)->vtable);
		seqeditorplayline_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			seqeditorplayline_on_mouse_down;
		seqeditorplayline_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			seqeditorplayline_on_mouse_move;
		seqeditorplayline_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			seqeditorplayline_on_mouse_up;
	}
	seqeditorline_base(&self->seqeditorline)->vtable =
		&seqeditorplayline_vtable;
}

/* implementation */
void seqeditorplayline_init(SeqEditorPlayline* self, psy_ui_Component* parent,
	SeqEditState* state)
{
	assert(self);
	assert(state);

	seqeditorline_init(&self->seqeditorline, parent, state);
	seqeditorplayline_vtable_init(self);
	self->drag = FALSE;
	self->dragbase = 0.0;
	psy_ui_component_set_style_type(seqeditorplayline_base(self),
		STYLE_SEQEDT_LINE_PLAY);
}

SeqEditorPlayline* seqeditorplayline_alloc(void)
{
	return (SeqEditorPlayline*)malloc(sizeof(SeqEditorPlayline));
}

SeqEditorPlayline* seqeditorplayline_allocinit(
	psy_ui_Component* parent, SeqEditState* state)
{
	SeqEditorPlayline* rv;

	rv = seqeditorplayline_alloc();
	if (rv) {
		seqeditorplayline_init(rv, parent, state);
		psy_ui_component_deallocate_after_destroyed(
			seqeditorline_base(&rv->seqeditorline));
	}
	return rv;
}

void seqeditorplayline_on_mouse_down(SeqEditorPlayline* self,
	psy_ui_MouseEvent* ev)
{
	psy_ui_RealRectangle position;

	assert(self);

	position = psy_ui_component_position(
		seqeditorline_base(&self->seqeditorline));
	self->drag = TRUE;
	self->dragbase = position.left;
	psy_ui_component_capture(seqeditorline_base(&self->seqeditorline));
	psy_ui_component_set_cursor(seqeditorline_base(&self->seqeditorline),
		psy_ui_CURSORSTYLE_COL_RESIZE);
	psy_ui_mouseevent_stop_propagation(ev);
}

void seqeditorplayline_on_mouse_move(SeqEditorPlayline* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	psy_ui_component_set_cursor(seqeditorline_base(&self->seqeditorline),
		psy_ui_CURSORSTYLE_COL_RESIZE);
	if (self->drag) {
		psy_audio_exclusivelock_enter();
		psy_audio_player_set_position(
			&self->seqeditorline.state->workspace->player_,
			psy_dsp_beatpos_real(
				psy_dsp_beatpos_max(
					psy_dsp_beatpos_zero(),
					seqeditstate_pxtobeat(self->seqeditorline.state,
						self->dragbase + psy_ui_mouseevent_pt(ev).x))));
		psy_audio_exclusivelock_leave();
		seqeditorplayline_update(self);
		self->dragbase = psy_ui_component_position(
			seqeditorline_base(&self->seqeditorline)).left;
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void seqeditorplayline_on_mouse_up(SeqEditorPlayline* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	self->drag = FALSE;
	self->dragbase = 0.0;
	psy_ui_component_release_capture(seqeditorline_base(&self->seqeditorline));
	psy_ui_mouseevent_stop_propagation(ev);
}

void seqeditorplayline_update(SeqEditorPlayline* self)
{
	assert(self);

	seqeditorline_updateposition(&self->seqeditorline,
		psy_dsp_beatpos_make_real(
			psy_audio_player_position(workspace_player(
				self->seqeditorline.state->workspace)),
			psy_dsp_DEFAULT_PPQ));
}

#endif /* PSYCLE_USE_SEQEDITOR */
