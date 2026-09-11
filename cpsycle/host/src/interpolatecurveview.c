/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "interpolatecurveview.h"
/* audio */
#include <sequencecmds.h>
/* dsp */
#include <interpolate.h>
/* container */
#include <trackercmds.h>
/* platform */
#include "../../detail/portable.h"


/* InterpolateCurveState */

/* implementation */
void interpolatecurvestate_init(InterpolateCurveState* self)
{	
	assert(self);
	
	self->opcount_ = 0;
	psy_property_init_type(&self->minval_, "", PSY_PROPERTY_TYPE_INTEGER);
	psy_property_set_hint(&self->minval_, PSY_PROPERTY_HINT_EDITHEX);
	psy_property_init_type(&self->maxval_, "", PSY_PROPERTY_TYPE_INTEGER);
	psy_property_set_hint(&self->maxval_, PSY_PROPERTY_HINT_EDITHEX);	
	self->maxval_.item.max = self->minval_.item.max = 0xFFFF;
	self->minval_.item.value.i = 0xFF00;
	self->maxval_.item.value.i = 0xFFFF;
	psy_property_init_type(&self->tweak_, "", PSY_PROPERTY_TYPE_CHOICE);
	psy_property_append_int(&self->tweak_, "---",
		psy_audio_NOTECOMMANDS_EMPTY, 0, 0);
	psy_property_append_int(&self->tweak_, "twk",
		psy_audio_NOTECOMMANDS_TWEAK, 0, 0);
	psy_property_append_int(&self->tweak_, "tws",
		psy_audio_NOTECOMMANDS_TWEAK_SLIDE, 0, 0);
	psy_property_append_int(&self->tweak_, "mcm",
		psy_audio_NOTECOMMANDS_MIDICC, 0, 0);
	psy_property_init_type(&self->curve_, "", PSY_PROPERTY_TYPE_CHOICE);
	psy_property_append_int(&self->curve_, "Linear",
		psy_audio_INTERPOLATE_MODE_LINEAR, 0, 0);
	psy_property_append_int(&self->curve_, "Hermit",
		psy_audio_INTERPOLATE_MODE_HERMITE, 0, 0);
}

void interpolatecurvestate_dispose(InterpolateCurveState* self)
{
	assert(self);

	psy_property_dispose(&self->minval_);
	psy_property_dispose(&self->maxval_);
	psy_property_dispose(&self->tweak_);
	psy_property_dispose(&self->curve_);
}

int interpolatecurvestate_tweak_type(InterpolateCurveState* self)
{
	psy_Property* p;

	assert(self);

	p = psy_property_at_choice(&self->tweak_);
	if (p) {
		return (int)psy_property_item_int(p);
	}
	return psy_audio_NOTECOMMANDS_EMPTY;
}

void interpolatecurvestate_update_range(InterpolateCurveState* self,
	psy_audio_Machine* machine, psy_audio_MachineParam* param)
{
	intptr_t minval;
	intptr_t maxval;

	assert(self);

	if (!param) {
		return;
	}
	if (machine) {
		psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
		psy_property_set_item_int(&self->minval_, 0);
		psy_property_set_item_int(&self->maxval_, maxval - minval);
	}
}

/* InterpolateCurveBox */

/* prototypes */
static void interpolatecurvebox_on_size(InterpolateCurveBox*);
static void interpolatecurvebox_on_draw(InterpolateCurveBox*,
	psy_ui_Graphics*);
static void interpolatecurvebox_draw_grid(InterpolateCurveBox*,
	psy_ui_Graphics*);
static void interpolatecurvebox_draw_grid_beat_seperators(
	InterpolateCurveBox*, psy_ui_Graphics*);
static void interpolatecurvebox_draw_grid_value_seperators(
	InterpolateCurveBox*, psy_ui_Graphics*);
static void interpolatecurvebox_draw_ruler(InterpolateCurveBox*,
	psy_ui_Graphics* g);
static void interpolatecurvebox_draw_pattern_values(InterpolateCurveBox*,
	psy_ui_Graphics*);
static void interpolatecurvebox_draw_point(InterpolateCurveBox*,
	psy_ui_Graphics*, psy_dsp_beatpos_t offset, double cpy);
static void interpolatecurvebox_on_mouse_down(InterpolateCurveBox*,
	psy_ui_MouseEvent*);
static void interpolatecurvebox_on_mouse_move(InterpolateCurveBox*,
	psy_ui_MouseEvent*);
static void interpolatecurvebox_on_mouse_up(InterpolateCurveBox*,
	psy_ui_MouseEvent*);
static void interpolatecurvebox_on_mouse_wheel(InterpolateCurveBox*,
	psy_ui_MouseEvent*);
static void interpolatecurvebox_remove_point(InterpolateCurveBox*);
static void interpolatecurvebox_insert_point(InterpolateCurveBox*);
static void interpolatecurvebox_change_point(InterpolateCurveBox*,
	uintptr_t value);
static void interpolatecurvebox_reset_tweak(InterpolateCurveBox*);
static psy_audio_SequenceCursor interpolatecurvebox_make_cursor(
	InterpolateCurveBox*, double x);
static void interpolatecurvebox_output(InterpolateCurveBox*,
	uintptr_t value, psy_dsp_beatpos_t position);
static uintptr_t interpolatecurvebox_tweak_value(InterpolateCurveBox*,
	double y);
static void interpolatecurvebox_set_tweak_value(InterpolateCurveBox*,
	psy_audio_PatternEvent* ev, uintptr_t value);
static psy_dsp_beatpos_t interpolatecurvebox_tweak_position(InterpolateCurveBox*,
	double x);
static void interpolatecurvebox_update_curve_type(InterpolateCurveBox*);
static void interpolatecurvebox_on_selection_changed(InterpolateCurveBox*,
	PatternSelection* sender);	
static double interpolatecurvebox_beat_to_px(const InterpolateCurveBox*,
	psy_dsp_beatpos_t pos);
static double interpolatecurvebox_value_to_px(const InterpolateCurveBox*,
	double value);
static double interpolatecurvebox_scale_x(const InterpolateCurveBox*);
static double interpolatecurvebox_scale_y(const InterpolateCurveBox*);
static double interpolatecurvebox_center_line_px(const InterpolateCurveBox*);
static void interpolatecurvebox_interpolate_point(InterpolateCurveBox*,
	psy_audio_BlockSelection block, psy_audio_InterpolateMode,
	intptr_t val0, intptr_t val1, intptr_t val2, intptr_t val3);
static psy_audio_BlockSelection interpolatecurvebox_make_block(InterpolateCurveBox*,
	psy_dsp_beatpos_t start_offset, psy_dsp_beatpos_t end_offset,
	psy_dsp_beatpos_t start_seqoffset, psy_dsp_beatpos_t end_seqoffset,
	uintptr_t start_order, uintptr_t end_order);
static void interpolataecurvebox_on_curve(InterpolateCurveBox*, psy_Property* sender);
static void interpolatecurvebox_on_timer(InterpolateCurveBox*,
	uintptr_t timer_id);
	
/* vtable */
static psy_ui_ComponentVtable interpolatecurvebox_vtable;
static bool interpolatecurvebox_vtable_initialized = FALSE;

static void interpolatecurvebox_vtable_init(InterpolateCurveBox* self)
{
	assert(self);
	
	if (!interpolatecurvebox_vtable_initialized) {
		interpolatecurvebox_vtable = *(self->component.vtable);		
		interpolatecurvebox_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			interpolatecurvebox_on_draw;
		interpolatecurvebox_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			interpolatecurvebox_on_mouse_down;
		interpolatecurvebox_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			interpolatecurvebox_on_mouse_move;
		interpolatecurvebox_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			interpolatecurvebox_on_mouse_up;
		interpolatecurvebox_vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			interpolatecurvebox_on_mouse_wheel;
		interpolatecurvebox_vtable.onsize =
			(psy_ui_fp_component)
			interpolatecurvebox_on_size;
		interpolatecurvebox_vtable.on_timer =
			(psy_ui_fp_component_on_timer)
			interpolatecurvebox_on_timer;
		interpolatecurvebox_vtable_initialized = TRUE;
	}
	self->component.vtable = &interpolatecurvebox_vtable;
}

/* implementation */
void interpolatecurvebox_init(InterpolateCurveBox* self,
	psy_ui_Component* parent, InterpolateCurveState* interpolate_state,
	PatternViewState* pv_state, Workspace* workspace)
{
	assert(self);
	assert(pv_state);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	interpolatecurvebox_vtable_init(self);	
	self->interpolate_state_ = interpolate_state;
	self->pv_state_ = pv_state;
	self->workspace_ = workspace;
	self->range_ = psy_dsp_beatpos_zero();
	self->tweak_node_ = NULL;
	self->tweak_pattern_ = NULL;
	self->spacing_ = psy_ui_margin_make_px(5.0, 10.0, 5.0, 5.0);
	self->cx_ = 10.0;
	self->cy_ = 10.0;
	psy_ui_component_set_aligner(&self->component, NULL);
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(0.0, 8.0));
	psy_signal_connect(&self->pv_state_->selection.signal_changed,
		self, interpolatecurvebox_on_selection_changed);
	psy_property_connect(&self->interpolate_state_->curve_,
		self, interpolataecurvebox_on_curve);
	psy_ui_component_start_timer(interpolatecurvebox_base(self), 0, 50);
}

void interpolatecurvebox_on_draw(InterpolateCurveBox* self,
	psy_ui_Graphics* g)
{
	assert(self);

	interpolatecurvebox_draw_grid(self, g);
	interpolatecurvebox_draw_ruler(self, g);
	interpolatecurvebox_draw_pattern_values(self, g);	
}

void interpolatecurvebox_draw_grid(InterpolateCurveBox* self,
	psy_ui_Graphics* g)
{	
	assert(self);

	if (psy_dsp_beatpos_not_zero(self->range_)) {
		psy_ui_Style* style;		

		style = psy_ui_style(STYLE_ENVELOPE_GRID);		
		psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
		interpolatecurvebox_draw_grid_beat_seperators(self, g);
		interpolatecurvebox_draw_grid_value_seperators(self, g);
	}
}

void interpolatecurvebox_draw_grid_beat_seperators(InterpolateCurveBox* self,
	psy_ui_Graphics* g)
{
	psy_dsp_beatpos_t cursor_position;
	psy_dsp_beatpos_t i;
	psy_dsp_beatpos_t bpl;
	psy_ui_Style* style;
	psy_ui_Style* style_cursor;
	double spacing_top;

	assert(self);

	style = psy_ui_style(STYLE_ENVELOPE_GRID);	
	style_cursor = psy_ui_style(STYLE_PV_CURSOR);
	cursor_position = psy_dsp_beatpos_sub(
		psy_audio_sequencecursor_offset(&self->pv_state_->cursor),
		psy_audio_sequencecursor_offset(
			&self->pv_state_->selection.block.topleft));
	bpl = psy_audio_sequencecursor_bpl(&self->pv_state_->cursor);
	spacing_top = psy_ui_value_px(&self->spacing_.top,
		psy_ui_component_textmetric(&self->component), NULL);
	for (i = psy_dsp_beatpos_zero();
			psy_dsp_beatpos_less(i, self->range_);
			i = psy_dsp_beatpos_add(i, bpl)) {
		double cpx;
		
		if (psy_dsp_beatpos_testrange(i, cursor_position, bpl)) {
			psy_ui_graphics_set_colour(g, psy_ui_style_background_colour(
				style_cursor));
		} else {
			psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
		}
		cpx = interpolatecurvebox_beat_to_px(self, i);
		psy_ui_drawline(g, psy_ui_realpoint_make(cpx, spacing_top),
			psy_ui_realpoint_make(cpx, spacing_top + self->cy_));
	}
}

void interpolatecurvebox_draw_grid_value_seperators(InterpolateCurveBox* self,
	psy_ui_Graphics* g)
{			
	psy_ui_Style* style;
	double spacing_top;
	double spacing_left;
	double smallstep;
	double j;

	assert(self);

	style = psy_ui_style(STYLE_ENVELOPE_GRID);
	spacing_top = psy_ui_value_px(&self->spacing_.top,
		psy_ui_component_textmetric(&self->component), NULL);
	spacing_left = psy_ui_value_px(&self->spacing_.left,
		psy_ui_component_textmetric(&self->component), NULL);
	smallstep = 0.1;
	psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
	for (j = 0; j <= 1.0; j += smallstep) {
		double cpy;

		cpy = (self->cy_ - j * self->cy_) + spacing_top;
		psy_ui_drawline(g, psy_ui_realpoint_make(spacing_left, cpy),
			psy_ui_realpoint_make(self->cx_ + spacing_left, cpy));
	}
}

void interpolatecurvebox_draw_ruler(InterpolateCurveBox* self, psy_ui_Graphics* g)
{	
	psy_dsp_beatpos_t i;
	psy_dsp_beatpos_t bpl;
	psy_dsp_beatpos_t step;
	psy_dsp_beatpos_t res;	
	psy_ui_Style* style;	
	const psy_ui_TextMetric* tm;
	double text_y;
	double spacing_top;

	assert(self);

	style = psy_ui_style(STYLE_ENVELOPE_RULER);	
	psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));	
	psy_ui_drawline(g, psy_ui_realpoint_make(0, self->cy_),
		psy_ui_realpoint_make(self->cx_, self->cy_));
	psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(style));
	bpl = psy_audio_sequencecursor_bpl(&self->pv_state_->cursor);
	res = psy_dsp_beatpos_make_real(1.0, psy_dsp_DEFAULT_PPQ);
	step = psy_dsp_beatpos_mul(res, bpl);	
	tm = psy_ui_component_textmetric(&self->component);
	text_y = self->cy_ - (tm->tmHeight * 1.5);
	spacing_top = psy_ui_value_px(&self->spacing_.top,
		psy_ui_component_textmetric(&self->component), NULL);
	for (i = psy_dsp_beatpos_zero();
			psy_dsp_beatpos_less_equal(i, self->range_);
			i = psy_dsp_beatpos_add(i, bpl)) {
		double cpx;		
				
		cpx = interpolatecurvebox_beat_to_px(self, i);
		psy_ui_drawline(g, psy_ui_realpoint_make(cpx, self->cy_ - 5.0),
			psy_ui_realpoint_make(cpx, self->cy_));
		if (psy_dsp_beatpos_is_zero(psy_dsp_beatpos_mod(i, res))) {
			char text[64];
			psy_ui_RealPoint cp;
			
			psy_snprintf(text, 64, "%d", (int)psy_dsp_beatpos_line(i,
				psy_audio_sequencecursor_lpb(&self->pv_state_->cursor)));
			if (psy_dsp_beatpos_equal(i, self->range_)) {
				cp = psy_ui_realpoint_make(cpx - 3.5 * (double)tm->tmAveCharWidth, text_y);
			} else {
				cp = psy_ui_realpoint_make(cpx + 4.0, text_y);				
			}
			psy_ui_graphics_textout(g, cp, text, psy_strlen(text));				
			psy_ui_drawline(g,
				psy_ui_realpoint_make(cpx, spacing_top + self->cy_ - 10.0),
				psy_ui_realpoint_make(cpx, spacing_top + self->cy_));
		}
	}
}

void interpolatecurvebox_draw_pattern_values(InterpolateCurveBox* self,
	psy_ui_Graphics* g)
{
	psy_audio_Sequence* sequence;
	psy_audio_PatternNode* currnode;	
	psy_audio_SequenceTrackIterator ite;
	psy_dsp_beatpos_t start;
	psy_dsp_beatpos_t end;
	psy_dsp_beatpos_t offset;
	psy_dsp_beatpos_t seqoffset;
	psy_dsp_beatpos_t lastoffset;	
	intptr_t val0;
	intptr_t val1;
	intptr_t val2;
	intptr_t val3;
	double lastcurveval;
	psy_audio_InterpolateMode interpolate_mode;
	psy_audio_BlockSelection selection;
	psy_ui_Style* style;
	
	assert(self);
	
	selection = self->pv_state_->selection.block;
	if (!psy_audio_blockselection_valid(&selection)) {
		return;
	}
	sequence = patternviewstate_sequence(self->pv_state_);
	if (!sequence) {
		return;
	}
	style = psy_ui_style(STYLE_ENVELOPE_CURVE);	
	psy_audio_sequencetrackiterator_init(&ite);
	offset = start = psy_audio_sequencecursor_offset_abs
		(&selection.topleft, sequence);
	lastoffset = psy_dsp_beatpos_zero();
	end = psy_audio_sequencecursor_offset_abs(&selection.bottomright,
		sequence);
	if (psy_dsp_beatpos_greater(end, psy_audio_sequencecursor_bpl(
			&selection.topleft))) {
		end = psy_dsp_beatpos_sub(end, psy_audio_sequencecursor_bpl(
			&selection.topleft));
	}	
	psy_audio_sequence_begin(sequence, self->pv_state_->cursor.order_index.track,
		start, &ite);		
	currnode = ite.patternnode;
	seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);	
	val0 = val1 = val2 = val3 = psy_property_item_int(&self->interpolate_state_->minval_);
	lastcurveval = (double)val1;
	interpolate_mode = psy_audio_INTERPOLATE_MODE_LINEAR;
	while (currnode) {
		psy_audio_PatternEntry* curr;
		psy_audio_PatternEvent* currevent;		

		curr = psy_audio_patternnode_entry(currnode);
		currevent = psy_audio_patternentry_front(curr);						
		if (self->pv_state_->cursor.channel_ == psy_audio_patternentry_track(curr)) {
			offset = psy_dsp_beatpos_add(seqoffset,
				psy_audio_patternentry_offset(curr));
			if (psy_dsp_beatpos_greater(offset, end)) {
				break;
			}
			offset = psy_dsp_beatpos_sub(offset, start);						
			val0 = val1;
			val1 = val2;			
			val2 = val3 = (currevent->parameter) | (currevent->cmd << 8);
			psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
			if (!psy_dsp_beatpos_equal(lastoffset, offset)) {
				double cpy;

				cpy = interpolatecurvebox_value_to_px(self, (double)val2);					
				if (interpolate_mode == psy_audio_INTERPOLATE_MODE_LINEAR) {
					psy_ui_drawline(g,
						psy_ui_realpoint_make(
							interpolatecurvebox_beat_to_px(self, lastoffset),
							interpolatecurvebox_value_to_px(self, lastcurveval)),
						psy_ui_realpoint_make(
							interpolatecurvebox_beat_to_px(self, offset),							
							cpy));
				} else {
					intptr_t i;
					double distance;
					double x;

					x = interpolatecurvebox_beat_to_px(self, lastoffset),
					distance = interpolatecurvebox_beat_to_px(self,
						psy_dsp_beatpos_sub(offset, lastoffset));
					for (i = 1; i < distance; i++) {
						double curveval;
						
						curveval = psy_dsp_hermite_curve_interpolate(
							val0, val1, val2, val3, i, (intptr_t)distance, 0,
							TRUE);
						psy_ui_drawline(g,
							psy_ui_realpoint_make(x + i - 1,
								interpolatecurvebox_value_to_px(self, lastcurveval)),
							psy_ui_realpoint_make(x + i,
								interpolatecurvebox_value_to_px(self, curveval)));
						lastcurveval = curveval;
					}
				}
				interpolatecurvebox_draw_point(self, g, offset, cpy);
			}
			if (interpolate_mode == psy_audio_INTERPOLATE_MODE_LINEAR) {	
				lastcurveval = (double)val2;
			}
			lastoffset = offset;
			interpolate_mode = psy_audio_patternentry_interpolate_mode(curr);
		}		
		if (patternviewstate_single_mode(self->pv_state_)) {
			psy_audio_patternnode_next(&currnode);
		} else {
			psy_audio_sequencetrackiterator_inc(&ite);
			currnode = ite.patternnode;
			seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);
		}		
	}		
	offset = psy_dsp_beatpos_sub(end, start);
	if (!psy_dsp_beatpos_equal(lastoffset, offset)) {
		psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
		psy_ui_drawline(g,
			psy_ui_realpoint_make(
				interpolatecurvebox_beat_to_px(self, lastoffset),
				interpolatecurvebox_value_to_px(self, lastcurveval)),
			psy_ui_realpoint_make(
				interpolatecurvebox_beat_to_px(self, offset),
				interpolatecurvebox_value_to_px(self, (double)
					psy_property_item_int(&self->interpolate_state_->minval_))));
	}
	psy_audio_sequencetrackiterator_dispose(&ite);
}

void interpolatecurvebox_draw_point(InterpolateCurveBox* self,
	psy_ui_Graphics* g, psy_dsp_beatpos_t offset, double cpy)
{		
	psy_ui_Style* style;
	psy_ui_RealSize ptsize;
	psy_ui_RealSize ptsize2;	

	assert(self);

	ptsize = psy_ui_realsize_make(5.0, 5.0);
	ptsize2 = psy_ui_realsize_make(2.5, 2.5);
	style = psy_ui_style(STYLE_ENVELOPE_POINT);
	psy_ui_graphics_set_colour(g, psy_ui_style_colour(style));
	psy_ui_graphics_draw_solid_rectangle(g, 
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(
				interpolatecurvebox_beat_to_px(self, offset) - ptsize2.width,
				cpy - ptsize2.height),
			ptsize),
		psy_ui_style_colour(style));
}

void interpolatecurvebox_on_mouse_down(InterpolateCurveBox* self,
	psy_ui_MouseEvent* ev)
{	
	psy_audio_PatternNode* prev;
		
	assert(self);
	
	if (patternviewstate_sequence(self->pv_state_)) {
		psy_audio_sequence_set_cursor(patternviewstate_sequence(
			self->pv_state_), interpolatecurvebox_make_cursor(self,
				psy_ui_mouseevent_offset(ev).x));
	}
	self->tweak_pattern_ = patternviewstate_pattern(self->pv_state_);
	if (!self->tweak_pattern_) {
		return;
	}	
	self->tweak_node_ = psy_audio_pattern_findnode_cursor(self->tweak_pattern_,
		self->pv_state_->cursor, &prev);
	if (self->tweak_node_) {
		if (psy_ui_mouseevent_button(ev) == 2) {
			interpolatecurvebox_remove_point(self);						
		} else {
			interpolatecurvebox_change_point(self,
				interpolatecurvebox_tweak_value(self,
					psy_ui_mouseevent_offset(ev).y));
			interpolatecurvebox_update_curve_type(self);
		}		
	} else if (psy_ui_mouseevent_button(ev) == 1) {			
		psy_audio_PatternEvent pat_ev;
		psy_audio_PatternEntry insert;
			
		psy_audio_patternevent_init(&pat_ev);
		interpolatecurvebox_set_tweak_value(self, &pat_ev,
			interpolatecurvebox_tweak_value(self, psy_ui_mouseevent_offset(ev).y));
		psy_audio_patternentry_init(&insert);
		psy_audio_patternentry_set_event(&insert, pat_ev, 0);		
		self->tweak_node_ = psy_audio_pattern_insert(self->tweak_pattern_,
			prev,
			psy_audio_sequencecursor_channel(&self->pv_state_->cursor),
			psy_audio_sequencecursor_offset(&self->pv_state_->cursor),
			&insert);
		psy_audio_patternentry_dispose(&insert);
		interpolatecurvebox_output(self, pat_ev.parameter,
			psy_audio_sequencecursor_offset(&self->pv_state_->cursor));
		psy_ui_component_invalidate(&self->component);
	} else {
		interpolatecurvebox_reset_tweak(self);
	}	
	if (self->tweak_node_) {
		psy_ui_component_capture(&self->component);		
	}
	psy_ui_mouseevent_stop_propagation(ev);
}


void interpolatecurvebox_on_mouse_move(InterpolateCurveBox* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (self->tweak_node_) {
		interpolatecurvebox_change_point(self,
			interpolatecurvebox_tweak_value(self,
				psy_ui_mouseevent_offset(ev).y));		
		psy_audio_sequence_tweak(self->pv_state_->sequence);
	}
	interpolatecurvebox_output(self, interpolatecurvebox_tweak_value(
		self, psy_ui_mouseevent_offset(ev).y),
		interpolatecurvebox_tweak_position(self, psy_ui_mouseevent_offset(ev).x));
	psy_ui_mouseevent_stop_propagation(ev);
}

void interpolatecurvebox_on_mouse_up(InterpolateCurveBox* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);
	
	psy_ui_component_release_capture(&self->component);
	interpolatecurvebox_insert_point(self);	
	interpolatecurvebox_reset_tweak(self);
	psy_ui_mouseevent_stop_propagation(ev);
}

void interpolatecurvebox_on_mouse_wheel(InterpolateCurveBox* self,
	psy_ui_MouseEvent* ev)
{
	assert(self);

	if (!patternviewstate_sequence(self->pv_state_)) {
		return;
	}	
	if (psy_ui_mouseevent_delta(ev) != 0) {
		psy_audio_SequenceCursor cursor;
		uintptr_t value;
		psy_audio_PatternNode* prev;
		
		cursor = interpolatecurvebox_make_cursor(self,
			psy_ui_mouseevent_offset(ev).x);
		if (!psy_audio_sequencecursor_equal(&cursor, &self->pv_state_->cursor)) {
			psy_audio_sequence_set_cursor(patternviewstate_sequence(
				self->pv_state_), cursor);
		}
		self->tweak_pattern_ = patternviewstate_pattern(self->pv_state_);
		if (!self->tweak_pattern_) {
			return;
		}	
		self->tweak_node_ = psy_audio_pattern_findnode_cursor(
			self->tweak_pattern_, self->pv_state_->cursor, &prev);
		if (self->tweak_node_) {
			psy_audio_PatternEntry* entry;
			psy_audio_PatternEvent* pat_ev;

			entry = psy_audio_patternnode_entry(self->tweak_node_);
			pat_ev = psy_audio_patternentry_front(entry);								
			value = psy_audio_patternevent_tweakvalue(pat_ev);
			interpolatecurvebox_update_curve_type(self);
		} else {
			return;
		}		
		if (psy_ui_mouseevent_delta(ev) > 0) {
			value += 1;
		} else {
			if (value > 0) {
				value -= 1;
			}
		}
		value = psy_max(
			(uintptr_t)psy_property_item_int(&self->interpolate_state_->minval_),
				psy_min((uintptr_t)psy_property_item_int(&self->interpolate_state_->maxval_),
				value));
		interpolatecurvebox_change_point(self, value);			
		psy_audio_sequence_tweak(self->pv_state_->sequence);
		interpolatecurvebox_reset_tweak(self);
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void interpolatecurvebox_remove_point(InterpolateCurveBox* self)
{
	assert(self);

	if (self->tweak_node_ && self->tweak_pattern_) {
		psy_undoredo_execute(self->pv_state_->undo_redo,
			&removecommand_allocinit(self->tweak_pattern_,
				self->pv_state_->cursor, self->pv_state_->sequence)->command);
		interpolatecurvebox_reset_tweak(self);
		psy_ui_component_invalidate(&self->component);				
	}
}

void interpolatecurvebox_insert_point(InterpolateCurveBox* self)
{
	assert(self);

	if (self->tweak_pattern_ && self->tweak_node_) {
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* pat_ev;

		entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
		pat_ev = psy_audio_patternentry_front(entry);
		psy_undoredo_execute(self->pv_state_->undo_redo,
			&insertcommand_allocinit(self->tweak_pattern_,
				self->pv_state_->cursor, *pat_ev, psy_dsp_beatpos_zero(),
				self->pv_state_->sequence)->command);
		psy_ui_component_invalidate(&self->component);
	}
}

void interpolatecurvebox_change_point(InterpolateCurveBox* self, uintptr_t value)
{
	assert(self);

	if (self->tweak_node_) {
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* pat_ev;

		entry = psy_audio_patternnode_entry(self->tweak_node_);
		pat_ev = psy_audio_patternentry_front(entry);
		interpolatecurvebox_set_tweak_value(self, pat_ev, value);
		psy_ui_component_invalidate(&self->component);
	}
}

void interpolatecurvebox_update_curve_type(InterpolateCurveBox* self)
{
	assert(self);

	if (self->tweak_node_) {
		psy_audio_PatternEntry* entry;		

		entry = psy_audio_patternnode_entry(self->tweak_node_);
		if (psy_audio_patternentry_interpolate_mode(entry) == psy_audio_INTERPOLATE_MODE_LINEAR) {
			psy_property_set_item_int(&self->interpolate_state_->curve_, 0);
		} else {
			psy_property_set_item_int(&self->interpolate_state_->curve_, 1);
		}		
	}
}

void interpolatecurvebox_reset_tweak(InterpolateCurveBox* self)
{
	assert(self);

	self->tweak_pattern_ = NULL;
	self->tweak_node_ = NULL;
}

psy_audio_SequenceCursor interpolatecurvebox_make_cursor(
	InterpolateCurveBox* self, double x)
{
	psy_audio_SequenceCursor rv;	

	assert(self);
	
	rv = self->pv_state_->selection.block.topleft;
	psy_audio_sequencecursor_set_offset(&rv,
		interpolatecurvebox_tweak_position(self, x));
	return rv;
}

void interpolatecurvebox_output(InterpolateCurveBox* self, uintptr_t value,
	psy_dsp_beatpos_t position)
{
	psy_Logger* logger;	
	
	assert(self);

	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {
		char text[64];
					
		psy_snprintf(text, 64, "Tweak Parameter %X at %.2f", (int)value,
			psy_dsp_beatpos_real(position));
		psy_logger_output(logger, text);
	}			
}

uintptr_t interpolatecurvebox_tweak_value(InterpolateCurveBox* self,
	double y)
{		
	intptr_t tmp;
	uintptr_t rv;
				
	assert(self);
		
	tmp = (intptr_t)((self->cy_ + psy_ui_value_px(&self->spacing_.top,
			psy_ui_component_textmetric(&self->component), NULL) - y) /
		interpolatecurvebox_scale_y(self)) + 
		psy_property_item_int(&self->interpolate_state_->minval_);
	if (tmp >= 0) {
		rv = (uintptr_t)tmp;
	} else {
		rv = psy_property_item_int(&self->interpolate_state_->minval_);
	}
	if (rv < (uintptr_t)psy_property_item_int(&self->interpolate_state_->minval_)) {
		rv = (uintptr_t)psy_property_item_int(&self->interpolate_state_->minval_);;
	} else if (rv > (uintptr_t)psy_property_item_int(&self->interpolate_state_->maxval_)) {
		rv = (uintptr_t)psy_property_item_int(&self->interpolate_state_->maxval_);
	}		
	return rv;
}

void interpolatecurvebox_set_tweak_value(InterpolateCurveBox* self,
	psy_audio_PatternEvent* ev, uintptr_t value)
{
	assert(self);

	ev->parameter = value & 0xFF;
	ev->cmd = (value >> 8) & 0xFF;
	interpolatecurvebox_output(self, value, psy_audio_sequencecursor_offset(
		&self->pv_state_->cursor));
}

psy_dsp_beatpos_t interpolatecurvebox_tweak_position(InterpolateCurveBox* self,
	double x)
{
	assert(self);

	return psy_dsp_beatpos_add(
		psy_audio_sequencecursor_offset(
			&self->pv_state_->selection.block.topleft),
		psy_dsp_beatpos_quantize(
			psy_dsp_beatpos_make_real(
				psy_max(0.0,
					(x + interpolatecurvebox_center_line_px(self) -
						psy_ui_value_px(&self->spacing_.left,
							psy_ui_component_textmetric(&self->component), NULL)) /
					interpolatecurvebox_scale_x(self)),
				psy_dsp_DEFAULT_PPQ),
			psy_audio_sequencecursor_lpb(&self->pv_state_->cursor)));
}

double interpolatecurvebox_beat_to_px(const InterpolateCurveBox* self,
	psy_dsp_beatpos_t pos)
{	
	assert(self);
	
	return psy_dsp_beatpos_real(pos) * interpolatecurvebox_scale_x(self) +
		psy_ui_value_px(&self->spacing_.left, psy_ui_component_textmetric(
			&self->component), NULL);
}

double interpolatecurvebox_center_line_px(const InterpolateCurveBox* self)
{
	assert(self);

	return (interpolatecurvebox_scale_x(self) /
		psy_audio_sequencecursor_lpb(&self->pv_state_->cursor) / 2);
}

double interpolatecurvebox_scale_x(const InterpolateCurveBox* self)
{		
	assert(self);
		
	return self->cx_ / psy_dsp_beatpos_real(self->range_);
}

double interpolatecurvebox_value_to_px(const InterpolateCurveBox* self,
	double value)
{	
	assert(self);
	
	return self->cy_ - (value - psy_property_item_int(
			&self->interpolate_state_->minval_)) *
		interpolatecurvebox_scale_y(self) +
		psy_ui_value_px(&self->spacing_.top, psy_ui_component_textmetric(
			&self->component), NULL);
}

double interpolatecurvebox_scale_y(const InterpolateCurveBox* self)
{
	assert(self);
			
	return self->cy_ / (double)(psy_property_item_int
		(&self->interpolate_state_->maxval_) -
		psy_property_item_int(&self->interpolate_state_->minval_));
}

void interpolatecurvebox_interpolate(InterpolateCurveBox* self)
{
	psy_audio_Sequence* sequence;
	psy_audio_PatternNode* currnode;	
	psy_audio_SequenceTrackIterator ite;
	psy_dsp_beatpos_t bpl;
	psy_dsp_beatpos_t start;
	psy_dsp_beatpos_t end;
	psy_dsp_beatpos_t offset;
	psy_dsp_beatpos_t seqoffset;
	psy_dsp_beatpos_t lastseqoffset;
	psy_dsp_beatpos_t lastoffset;	
	uintptr_t lastorder;
	psy_audio_InterpolateMode mode;	
	intptr_t val0;
	intptr_t val1;
	intptr_t val2;
	intptr_t val3;		
	psy_audio_SequenceEntry* seq_entry;
	
	assert(self);
		
	if (!psy_audio_blockselection_valid(&self->pv_state_->selection.block)) {
		return;
	}
	sequence = patternviewstate_sequence(self->pv_state_);
	if (!sequence) {
		return;
	}	
	psy_audio_sequencetrackiterator_init(&ite);
	bpl = psy_audio_sequencecursor_bpl(&self->pv_state_->cursor);
	lastoffset = offset = start = psy_audio_sequencecursor_offset_abs(
		&self->pv_state_->selection.block.topleft, sequence);
	end = psy_audio_sequencecursor_offset_abs(
		&self->pv_state_->selection.block.bottomright, sequence);
	psy_audio_sequence_begin(sequence, self->pv_state_->cursor.order_index.track,
		start, &ite);		
	currnode = ite.patternnode;
	seq_entry = psy_audio_sequencetrackiterator_entry(&ite);
	if (!seq_entry) {
		return;
	}
	lastorder = seq_entry->row;
	lastseqoffset = seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);	
	val1 = val2 = psy_property_item_int(&self->interpolate_state_->minval_);
	mode = psy_audio_INTERPOLATE_MODE_LINEAR;
	while (currnode) {
		psy_audio_PatternEntry* curr;
		psy_audio_PatternEvent* currevent;		

		curr = psy_audio_patternnode_entry(currnode);
		currevent = psy_audio_patternentry_front(curr);		
		if (self->pv_state_->cursor.channel_ == psy_audio_patternentry_track(curr)) {
			offset = psy_dsp_beatpos_add(psy_audio_patternentry_offset(curr),
				seqoffset);
			val0 = val1;
			val1 = val2;
			val2 = val3 = psy_min(psy_property_item_int(&self->interpolate_state_->maxval_),
				psy_max(psy_property_item_int(&self->interpolate_state_->minval_),
				psy_audio_patternevent_tweakvalue(currevent)));
			if (!psy_dsp_beatpos_equal(offset, lastoffset)) {				
				interpolatecurvebox_interpolate_point(self,
					interpolatecurvebox_make_block(self,
						lastoffset, psy_dsp_beatpos_add(offset, bpl),
						lastseqoffset, seqoffset,
						lastorder, seq_entry->row),
					mode, val0, val1, val2, val3);
			}										
			mode = psy_audio_patternentry_interpolate_mode(curr);
			lastoffset = offset;
			lastseqoffset = seqoffset;
			lastorder = seq_entry->row;
		}		
		if (patternviewstate_single_mode(self->pv_state_)) {
			psy_audio_patternnode_next(&currnode);
		} else {
			psy_audio_sequencetrackiterator_inc(&ite);
			seq_entry = psy_audio_sequencetrackiterator_entry(&ite);
			if (!seq_entry) {
				break;
			}
			currnode = ite.patternnode;
			seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);			
		}		
	}	
	if (!psy_dsp_beatpos_equal(lastoffset, end)) {
		val0 = val1;
		val1 = val2;
		val2 = val3 = psy_property_item_int(&self->interpolate_state_->minval_);
		interpolatecurvebox_interpolate_point(self,
			interpolatecurvebox_make_block(self,
				lastoffset, end,
				lastseqoffset, seqoffset,
				lastorder, 
				self->pv_state_->selection.block.bottomright.order_index.order),
			mode, val0, val1, val2, val3);		
	}	
	psy_audio_sequencetrackiterator_dispose(&ite);
	psy_ui_component_invalidate(&self->component);
}

void interpolatecurvebox_interpolate_point(InterpolateCurveBox* self,
	psy_audio_BlockSelection block,	psy_audio_InterpolateMode mode,
	intptr_t val0, intptr_t val1, intptr_t val2, intptr_t val3)
{	
	psy_audio_Sequence* sequence;

	assert(self);
	
	sequence = patternviewstate_sequence(self->pv_state_);
	if (!sequence) {
		return;
	}
	if (psy_dsp_beatpos_greater(
			psy_audio_sequencecursor_offset_abs(&block.bottomright, sequence),
			psy_dsp_beatpos_add(
				psy_audio_sequencecursor_offset_abs(&block.topleft, sequence),
			psy_audio_sequencecursor_bpl(&self->pv_state_->cursor)))) {
		psy_audio_PatternBlockCmd cmd;
		psy_Command* interpolate_cmd;
								
		if (mode == psy_audio_INTERPOLATE_MODE_LINEAR) {	
			psy_audio_Machine* machine;
			uintptr_t param;
			
			machine = psy_audio_machines_selected_machine(
				psy_audio_song_machines(self->workspace_->song));
			if (machine) {
				param = psy_audio_machine_param_selected(machine);
			} else {
				param = 0;
			}
			interpolate_cmd = psy_audio_patternlinearinterpolatecmd_base(
				psy_audio_patternlinearinterpolatecmd_alloc_init(
					psy_audio_linearinterpolate_make(sequence, block,
						psy_property_item_int(&self->interpolate_state_->minval_),
						psy_property_item_int(&self->interpolate_state_->maxval_),
						interpolatecurvestate_tweak_type(self->interpolate_state_),
						(int)param, (int)psy_audio_machines_selected(
							psy_audio_song_machines(self->workspace_->song)))));
		} else {						
			interpolate_cmd = psy_audio_patternhermiteinterpolatecmd_base(
				psy_audio_patternhermiteinterpolatecmd_alloc_init(
					psy_audio_hermiteinterpolate_make(sequence, block)));
		}
		if (interpolate_cmd) {
			psy_audio_patternblockcmd_init(&cmd, sequence, block, TRUE,
				interpolate_cmd);
			psy_command_execute(psy_audio_patternblockcmd_base(&cmd), 0);
			psy_command_dispose(psy_audio_patternblockcmd_base(&cmd));
		}
	}	
}

psy_audio_BlockSelection interpolatecurvebox_make_block(InterpolateCurveBox* self,
	psy_dsp_beatpos_t start_offset, psy_dsp_beatpos_t end_offset,
	psy_dsp_beatpos_t start_seqoffset, psy_dsp_beatpos_t end_seqoffset,
	uintptr_t start_order, uintptr_t end_order)
{	
	psy_audio_SequenceCursor start;
	psy_audio_SequenceCursor end;

	assert(self);

	start = self->pv_state_->selection.block.topleft;
	end = self->pv_state_->selection.block.bottomright;
	start.offset = psy_dsp_beatpos_sub(start_offset, start_seqoffset);
	end.offset = psy_dsp_beatpos_sub(end_offset, end_seqoffset);
	start.order_index.order = start_order;
	end.order_index.order = end_order;
	return psy_audio_blockselection_make(start, end);	
}

void interpolatecurvebox_on_selection_changed(InterpolateCurveBox* self,
	PatternSelection* sender)
{
	psy_audio_Sequence* sequence;

	assert(self);	
	
	sequence = patternviewstate_sequence(self->pv_state_);
	if (!sequence || !psy_audio_blockselection_valid(&sender->block)) {
		self->range_ = psy_dsp_beatpos_zero();
	} else {
		self->range_ = psy_dsp_beatpos_sub(
			psy_audio_sequencecursor_offset_abs(
				&sender->block.bottomright, sequence),
			psy_audio_sequencecursor_offset_abs(
				&sender->block.topleft, sequence));
		if (psy_dsp_beatpos_greater(self->range_, psy_audio_sequencecursor_bpl(
				&sender->block.topleft))) {
			self->range_ = psy_dsp_beatpos_sub(self->range_,
				psy_audio_sequencecursor_bpl(&sender->block.topleft));
		}
	}
	psy_ui_component_invalidate(interpolatecurvebox_base(self));	
}

void interpolatecurvebox_on_size(InterpolateCurveBox* self)
{
	psy_ui_RealSize size;
	const psy_ui_TextMetric* tm;

	assert(self);

	size = psy_ui_component_scroll_size_px(&self->component);
	tm = psy_ui_component_textmetric(&self->component);
	self->cx_ = size.width - psy_ui_value_px(&self->spacing_.left, tm, NULL) -
		psy_ui_value_px(&self->spacing_.right, tm, NULL);
	self->cy_ = size.height - psy_ui_value_px(&self->spacing_.top, tm, NULL) -
		psy_ui_value_px(&self->spacing_.bottom, tm, NULL);
}

void interpolataecurvebox_on_curve(InterpolateCurveBox* self,
	psy_Property* sender)
{
	psy_audio_SequenceCursor cursor;	
	psy_audio_PatternNode* prev;

	assert(self);

	cursor = self->pv_state_->cursor;
	cursor.channel_ = self->pv_state_->selection.block.topleft.channel_;
	self->tweak_pattern_ = patternviewstate_pattern(self->pv_state_);
	if (!self->tweak_pattern_) {
		return;
	}
	self->tweak_node_ = psy_audio_pattern_findnode_cursor(self->tweak_pattern_,
		cursor, &prev);
	if (self->tweak_node_) {
		psy_audio_PatternEntry* entry;		

		entry = psy_audio_patternnode_entry(self->tweak_node_);
		switch (psy_property_item_int(sender)) {
		case 0:
			psy_audio_patternentry_set_interpolate_mode(entry,
				psy_audio_INTERPOLATE_MODE_LINEAR);
			break;
		case 1:
			psy_audio_patternentry_set_interpolate_mode(entry,
				psy_audio_INTERPOLATE_MODE_HERMITE);
			break;
		default:
			break;
		}
	}
	interpolatecurvebox_reset_tweak(self);
	psy_ui_component_invalidate(&self->component);
}

void interpolatecurvebox_on_timer(InterpolateCurveBox* self,
	uintptr_t timer_id)
{
	psy_audio_Pattern* pattern;

	assert(self);

	if (!psy_ui_component_draw_visible(&self->component)) {
		return;
	}
	pattern = patternviewstate_pattern(self->pv_state_);
	if (pattern && (psy_audio_pattern_opcount(pattern) != self->interpolate_state_->opcount_)) {
		psy_ui_component_invalidate(interpolatecurvebox_base(self));
		self->interpolate_state_->opcount_ = psy_audio_pattern_opcount(pattern);
	} else {
		self->interpolate_state_->opcount_ = 0;
	}
}


/* InterpolateCurveBar */

/* prototypes */
static void interpolatecurvebar_init_tweak(InterpolateCurveBar*);
static void interpolatecurvebar_init_range(InterpolateCurveBar*);
static void interpolatecurvebar_init_curve(InterpolateCurveBar*);
static void interpolatecurvebar_init_interpolate(InterpolateCurveBar*);
static void interpolatecurvebar_on_minval_changed(InterpolateCurveBar*,
	psy_Property* sender);
static void interpolatecurvebar_on_maxval_changed(InterpolateCurveBar*,
	psy_Property* sender);
static void interpolatecurvebar_on_mac_param_range(InterpolateCurveBar*,
	psy_ui_Button* sender);
	
/* implementation */
void interpolatecurvebar_init(InterpolateCurveBar* self,
	psy_ui_Component* parent, InterpolateCurveState* state,
	Workspace* workspace)
{	
	assert(self);
	assert(state);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	self->state = state;
	self->workspace = workspace;
	psy_ui_component_set_default_align(interpolatecurvebar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));					
	interpolatecurvebar_init_tweak(self);
	interpolatecurvebar_init_range(self);
	interpolatecurvebar_init_curve(self);
	interpolatecurvebar_init_interpolate(self);
}

void interpolatecurvebar_init_tweak(InterpolateCurveBar* self)
{
	assert(self);

	psy_ui_combobox_init(&self->note_cmd__, interpolatecurvebar_base(self));
	psy_ui_combobox_set_char_number(&self->note_cmd__, 6.0);
	psy_ui_combobox_exchange(&self->note_cmd__, &self->state->tweak_);
}

void interpolatecurvebar_init_range(InterpolateCurveBar* self)
{
	assert(self);

	psy_ui_label_init_text(&self->minval_desc_, interpolatecurvebar_base(self),
		"Min value:");
	psy_ui_edit_init(&self->minval_, interpolatecurvebar_base(self));			
	psy_ui_edit_exchange(&self->minval_, &self->state->minval_);
	psy_ui_edit_set_char_number(&self->minval_, 10.0);
	psy_property_connect(&self->state->minval_, self,
		interpolatecurvebar_on_minval_changed);	

	psy_ui_label_init_text(&self->maxval_desc_, interpolatecurvebar_base(self),
		"Max value:");
	psy_ui_edit_init(&self->maxval_, interpolatecurvebar_base(self));
	psy_ui_edit_exchange(&self->maxval_, &self->state->maxval_);
	psy_ui_edit_set_char_number(&self->maxval_, 10.0);
	psy_property_connect(&self->state->maxval_, self,
		interpolatecurvebar_on_maxval_changed);

	psy_ui_button_init_text_connect(&self->mac_param_range_, interpolatecurvebar_base(
		self), "Set to mac param",
		self, interpolatecurvebar_on_mac_param_range);	
}

void interpolatecurvebar_init_curve(InterpolateCurveBar* self)
{
	assert(self);

	psy_ui_combobox_init(&self->curve_type_, interpolatecurvebar_base(self));
	psy_ui_combobox_set_char_number(&self->curve_type_, 15.0);
	psy_ui_combobox_exchange(&self->curve_type_, &self->state->curve_);
}

void interpolatecurvebar_init_interpolate(InterpolateCurveBar* self)
{
	assert(self);

	psy_ui_button_init_text(&self->interpolate_, interpolatecurvebar_base(self),
		"Interpolate");
	psy_ui_component_set_align(psy_ui_button_base(&self->interpolate_),
		psy_ui_ALIGN_RIGHT);
}

void interpolatecurvebar_on_minval_changed(InterpolateCurveBar* self,
	psy_Property* sender)
{
	assert(self);
		
	psy_ui_component_invalidate(&self->component);
}

void interpolatecurvebar_on_maxval_changed(InterpolateCurveBar* self,
	psy_Property* sender)
{
	assert(self);
	
	psy_ui_component_invalidate(&self->component);	
}

void interpolatecurvebar_on_mac_param_range(InterpolateCurveBar* self,
	psy_ui_Button* sender)
{
	psy_audio_Machine* machine;	

	assert(self);

	machine = psy_audio_machines_selected_machine(
		psy_audio_song_machines(self->workspace->song));
	if (machine) {		
		interpolatecurvestate_update_range(self->state, machine, 
			psy_audio_machine_tweak_parameter(machine,
				psy_audio_machine_param_selected(machine)));
	}	
}


/* InterpolateCurveView */

/* prototypes */
static void interpolatecurveview_on_destroyed(InterpolateCurveView*);
static void interpolatecurveview_on_interpolate(InterpolateCurveView*,
	psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable interpolatecurveview_vtable;
static bool interpolatecurveview_vtable_initialized = FALSE;

static void interpolatecurveview_vtable_init(InterpolateCurveView* self)
{
	assert(self);

	if (!interpolatecurveview_vtable_initialized) {
		interpolatecurveview_vtable = *(self->component.vtable);
		interpolatecurveview_vtable.on_destroyed =
			(psy_ui_fp_component)
			interpolatecurveview_on_destroyed;		
		interpolatecurveview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &interpolatecurveview_vtable);
}

/* implementation */
void interpolatecurveview_init(InterpolateCurveView* self, psy_ui_Component*
	parent, PatternViewState* pv_state, Workspace* workspace)
{	
	assert(self);
	assert(pv_state);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	interpolatecurveview_vtable_init(self);
	psy_ui_component_set_style_type(interpolatecurveview_base(self), STYLE_SIDE_VIEW);
	interpolatecurvestate_init(&self->interpolate_state_);
	closebar_init_cmd(&self->close_, interpolatecurveview_base(self),		
		"edit", psy_eventdrivercmd_make_cmd(CMD_BLOCKINTERPOLATECURVE));	
	interpolatecurvebar_init(&self->bar_, interpolatecurveview_base(self),
		&self->interpolate_state_, workspace);
	psy_ui_component_set_align(interpolatecurvebar_base(&self->bar_),
		psy_ui_ALIGN_BOTTOM);
	interpolatecurvebox_init(&self->box_, interpolatecurveview_base(self),
		&self->interpolate_state_, pv_state, workspace);
	psy_ui_component_set_align(interpolatecurvebox_base(&self->box_),
		psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->bar_.interpolate_.signal_clicked, self,
		interpolatecurveview_on_interpolate);	
}

void interpolatecurveview_on_destroyed(InterpolateCurveView* self)
{
	assert(self);

	interpolatecurvestate_dispose(&self->interpolate_state_);
}

void interpolatecurveview_on_interpolate(InterpolateCurveView* self,
	psy_ui_Component* sender)
{
	assert(self);
	
	interpolatecurvebox_interpolate(&self->box_);
}
