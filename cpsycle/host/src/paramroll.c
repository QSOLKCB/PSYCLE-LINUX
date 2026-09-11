/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "paramroll.h"

#ifdef PSYCLE_USE_PIANOROLL

/* audio */
#include <sequencecmds.h>
/* platform */
#include "../../detail/portable.h"


/* ParamRuler */

/* prototypes */
static void paramruler_on_draw(ParamRuler*, psy_ui_Graphics*);

/* vtable */
static psy_ui_ComponentVtable paramruler_vtable;
static bool paramruler_vtable_initialized = FALSE;

static void paramruler_table_init(ParamRuler* self)
{
	assert(self);

	if (!paramruler_vtable_initialized) {
		paramruler_vtable = *(self->component.vtable);
		paramruler_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			paramruler_on_draw;		
		paramruler_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &paramruler_vtable);	
}

/* implementation */
void paramruler_init(ParamRuler* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	paramruler_table_init(self);
}

void paramruler_on_draw(ParamRuler* self, psy_ui_Graphics* g)
{
	psy_ui_RealSize size;
	uintptr_t i;
	uintptr_t step;
	uintptr_t maxval;	
	
	size = psy_ui_component_size_px(&self->component);	
	maxval = 256;
	step = maxval / 8;
	for (i = 0; i < maxval; i += step) {
		double cpy;
		double w;
		
		cpy = i * (size.height / (double)maxval);
		if (i == 0x80) {
			w = size.width;
		} else {
			w = size.width / 2;
		}
		psy_ui_drawline(g, psy_ui_realpoint_make(0, cpy),
			psy_ui_realpoint_make(w, cpy));		
	}
}

/* ParamDraw */

/* prototypes */
static void paramdraw_on_draw(ParamDraw*, psy_ui_Graphics*);
static void paramdraw_draw_mid_line(ParamDraw*, psy_ui_Graphics*);
static void paramdraw_draw_entries(ParamDraw*, psy_ui_Graphics*);
static void paramdraw_on_preferred_size(ParamDraw*, const psy_ui_Size* limit,
	psy_ui_Size* rv);
static void paramdraw_on_track_display(ParamDraw*, psy_Property* sender);
static void paramdraw_on_mouse_down(ParamDraw*, psy_ui_MouseEvent*);
static void paramdraw_on_mouse_up(ParamDraw*, psy_ui_MouseEvent*);
static void paramdraw_on_mouse_move(ParamDraw*, psy_ui_MouseEvent*);
static uint8_t paramdraw_tweak_value(ParamDraw*, psy_ui_MouseEvent*);
static void paramdraw_output(ParamDraw*, uint8_t value);
static void paramdraw_on_mouse_wheel(ParamDraw*, psy_ui_MouseEvent*);
static psy_audio_SequenceCursor paramdraw_make_cursor(ParamDraw*, double x);

/* vtable */
static psy_ui_ComponentVtable paramdraw_vtable;
static bool paramdraw_vtable_initialized = FALSE;

static void paramdraw_vtable_init(ParamDraw* self)
{
	assert(self);

	if (!paramdraw_vtable_initialized) {
		paramdraw_vtable = *(self->component.vtable);
		paramdraw_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			paramdraw_on_draw;
		paramdraw_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			paramdraw_on_preferred_size;
		paramdraw_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			paramdraw_on_mouse_down;
		paramdraw_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			paramdraw_on_mouse_up;
		paramdraw_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			paramdraw_on_mouse_move;
		paramdraw_vtable.on_mouse_wheel =
			(psy_ui_fp_component_on_mouse_event)
			paramdraw_on_mouse_wheel;
		paramdraw_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &paramdraw_vtable);	
}

/* implementation */
void paramdraw_init(ParamDraw* self, psy_ui_Component* parent,
	PianoGridState* state)
{
	psy_ui_Path path;
	assert(self);
	assert(state);	
	
	psy_ui_component_init(&self->component, parent, NULL);
	paramdraw_vtable_init(self);	
	self->state_ = state;	
	self->tweak_pattern_ = NULL;
	self->tweak_node_ = NULL;	
	psy_property_connect(&self->state_->track_display,
		self, paramdraw_on_track_display);	
	psy_ui_svg_init(&self->svg);
	psy_ui_path_init(&path);
	psy_ui_path_move_to(&path, psy_ui_realpoint_make(100.0, 100.0));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(300.0, 100.0));
	psy_ui_path_line_to(&path, psy_ui_realpoint_make(200.0, 300.0));
	psy_ui_path_close(&path);
	psy_ui_path_set_fill(&path, psy_ui_colour_red());
	psy_ui_path_set_stroke(&path, psy_ui_colour_blue());
	psy_ui_svg_add(&self->svg, &path);
}

void paramdraw_on_draw(ParamDraw* self, psy_ui_Graphics* g)
{
	/* psy_ui_SVGDraw svg_draw; */
	assert(self);
	
	paramdraw_draw_mid_line(self, g);
	paramdraw_draw_entries(self, g);
	/* psy_ui_svgdraw_init(&svg_draw, &self->svg); */
	/* psy_ui_svgdraw_draw(&svg_draw, g, psy_ui_realpoint_make(0.0, 0.0)); */
}

void paramdraw_draw_mid_line(ParamDraw* self, psy_ui_Graphics* g)
{
	psy_ui_RealSize size;
	uintptr_t i;	
	uintptr_t maxval;	
	double cpy;
	psy_ui_Colour restore;
	psy_ui_Colour colour;
	
	assert(self);

	restore = psy_ui_component_colour(&self->component);
	colour = psy_ui_colour_make(0x333333);	
	psy_ui_graphics_set_colour(g, colour);
	size = psy_ui_component_size_px(&self->component);	
	maxval = 256;	
	i = 0x80;	
	cpy = i * (size.height / (double)maxval);		
	psy_ui_drawline(g, psy_ui_realpoint_make(0, cpy),
		psy_ui_realpoint_make(size.width, cpy));
	psy_ui_graphics_set_colour(g, restore);
}	

void paramdraw_draw_entries(ParamDraw* self, psy_ui_Graphics* g)
{
	psy_audio_Sequence* sequence;
	psy_audio_PatternNode* currnode;	
	psy_audio_SequenceTrackIterator ite;
	psy_dsp_beatpos_t seqoffset;	
	bool track_display;
	psy_ui_RealSize size;
	psy_ui_RealSize pt_size;	
	uintptr_t maxval;
	uint8_t param;
	double px;
	
	assert(self);
	
	sequence = patternviewstate_sequence(self->state_->pv);
	if (!sequence) {
		return;
	}				
	size = psy_ui_component_scroll_size_px(&self->component);
	maxval = 256;
	param = 0x0C;
	px = size.height / (double)maxval;
		
	psy_audio_sequencetrackiterator_init(&ite);
	psy_audio_sequence_begin(sequence,			
		self->state_->pv->cursor.order_index.track,
		(patternviewstate_single_mode(self->state_->pv)
		? psy_audio_sequencecursor_seqoffset(&self->state_->pv->cursor,
			patternviewstate_sequence(self->state_->pv))
		: psy_dsp_beatpos_zero()), &ite);
	currnode = ite.patternnode;
	seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);
	pt_size = psy_ui_realsize_make(5.0, 5.0);
	track_display = (pianogridstate_track_display(self->state_) ==
		PIANOROLL_TRACK_DISPLAY_CURRENT);
	while (currnode) {
		psy_audio_PatternEntry* curr;
		psy_audio_PatternEvent* currevent;		

		curr = (psy_audio_PatternEntry*)psy_list_entry(currnode);			
		currevent = psy_audio_patternentry_front(curr);
		if ((currevent->cmd == param) && ((!track_display) || (
				psy_audio_sequencecursor_channel(&self->state_->pv->cursor) ==
				psy_audio_patternentry_track(curr)))) {
			psy_dsp_beatpos_t offset;
			double cpy;
					
			cpy = (maxval - currevent->parameter) * px;
			offset = psy_audio_patternentry_offset(curr);
			if (!patternviewstate_single_mode(self->state_->pv)) {
				offset = psy_dsp_beatpos_add(offset, seqoffset);
			}
			psy_ui_graphics_draw_rectangle(g, psy_ui_realrectangle_make(
				psy_ui_realpoint_make(pianogridstate_raw_beat_to_px(
					self->state_, offset), cpy),
				pt_size));			
		}
		if (patternviewstate_single_mode(self->state_->pv)) {			
			psy_audio_patternnode_next(&currnode);
		} else {
			psy_audio_sequencetrackiterator_inc(&ite);
			currnode = ite.patternnode;
			seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);
		}		
	}
	psy_audio_sequencetrackiterator_dispose(&ite);
}

void paramdraw_on_preferred_size(ParamDraw* self, const psy_ui_Size* limit,
	psy_ui_Size* rv)
{
	assert(self);

	rv->width = psy_ui_value_make_px(pianogridstate_beat_to_px(self->state_,
		patternviewstate_length(self->state_->pv)));
	if (limit) {
		rv->height = limit->height;
	}
}

void paramdraw_on_track_display(ParamDraw* self, psy_Property* sender)
{
	psy_ui_component_invalidate(&self->component);
}

void paramdraw_on_mouse_down(ParamDraw* self, psy_ui_MouseEvent* ev)
{	
	psy_audio_PatternNode* prev;
		
	assert(self);
	
	if (patternviewstate_sequence(self->state_->pv)) {		
		psy_audio_sequence_set_cursor(patternviewstate_sequence(
			self->state_->pv), paramdraw_make_cursor(self,
				psy_ui_mouseevent_offset(ev).x));
	}
	self->tweak_pattern_ = patternviewstate_pattern(self->state_->pv);
	if (!self->tweak_pattern_) {
		return;
	}	
	self->tweak_node_ = psy_audio_pattern_findnode_cursor(self->tweak_pattern_,
		self->state_->pv->cursor, &prev);
	if (self->tweak_node_) {
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* pat_ev;

		entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
		pat_ev = psy_audio_patternentry_front(entry);		
		if (psy_ui_mouseevent_button(ev) == 2) {
			pat_ev->cmd = 0x00;
			pat_ev->parameter = 0x00;
			self->tweak_pattern_ = NULL;
			self->tweak_node_ = NULL;			
		} else {
			pat_ev->cmd = 0x0C;
			pat_ev->parameter = paramdraw_tweak_value(self, ev);			
			paramdraw_output(self, pat_ev->parameter);
		}
		psy_ui_component_invalidate(&self->component);
	} else if (psy_ui_mouseevent_button(ev) == 1) {			
		psy_audio_PatternEvent pat_ev;
		psy_audio_PatternEntry insert;
			
		psy_audio_patternevent_init(&pat_ev);
		pat_ev.cmd = 0x0C;
		pat_ev.parameter = paramdraw_tweak_value(self, ev);
		psy_audio_patternentry_init(&insert);
		psy_audio_patternentry_set_event(&insert, pat_ev, 0);
		self->tweak_node_ = psy_audio_pattern_insert(self->tweak_pattern_,
			prev,
			psy_audio_sequencecursor_channel(&self->state_->pv->cursor),
			psy_audio_sequencecursor_offset(&self->state_->pv->cursor),
			&insert);
		psy_audio_patternentry_dispose(&insert);
		paramdraw_output(self, pat_ev.parameter);
		psy_ui_component_invalidate(&self->component);
	} else {
		self->tweak_pattern_ = NULL;
		self->tweak_node_ = NULL;	
	}	
	if (self->tweak_node_) {
		psy_ui_component_capture(&self->component);		
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

psy_audio_SequenceCursor paramdraw_make_cursor(ParamDraw* self, double x)
{
	psy_audio_SequenceCursor rv;	
		
	assert(self);
	
	rv = self->state_->pv->cursor;
	psy_audio_sequencecursor_set_offset(&rv, pianogridstate_px_to_beat(
		self->state_, x));
	return rv;
}

void paramdraw_on_mouse_move(ParamDraw* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (self->tweak_node_) {
		uint8_t value;
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* pat_ev;
		
		entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
		pat_ev = psy_audio_patternentry_front(entry);
		value = paramdraw_tweak_value(self, ev);
		if (pat_ev->parameter != value) {			
			pat_ev->parameter = value;
			psy_audio_sequence_tweak(self->state_->pv->sequence);			
			psy_ui_component_invalidate(&self->component);
			paramdraw_output(self, value);
		}		
	} else if (psy_ui_mouseevent_button(ev) == 0) {
		psy_audio_SequenceCursor cursor;
		psy_audio_Pattern* pattern;
		psy_audio_PatternNode* prev;
		psy_audio_PatternNode* node;
		
		cursor = paramdraw_make_cursor(self, psy_ui_mouseevent_offset(ev).x);
		pattern = patternviewstate_pattern(self->state_->pv);
		if (!pattern) {
			return;
		}	
		node = psy_audio_pattern_findnode_cursor(pattern, cursor,
			&prev);
		if (node) {
			psy_audio_PatternEntry* entry;
			psy_audio_PatternEvent* pat_ev;

			entry = (psy_audio_PatternEntry*)node->entry;
			pat_ev = psy_audio_patternentry_front(entry);
			if (pat_ev->cmd == 0x0C) {
				paramdraw_output(self, pat_ev->parameter);
			} else {
				paramdraw_output(self, paramdraw_tweak_value(self, ev));				
			}
		} else {
			paramdraw_output(self, paramdraw_tweak_value(self, ev));
		}
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void paramdraw_on_mouse_up(ParamDraw* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	psy_ui_component_release_capture(&self->component);
	if (self->tweak_pattern_ && self->tweak_node_) {
		psy_audio_PatternEntry* entry;
		psy_audio_PatternEvent* pat_ev;
		
		entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
		pat_ev = psy_audio_patternentry_front(entry);				
		psy_undoredo_execute(self->state_->pv->undo_redo,
			&insertcommand_allocinit(self->tweak_pattern_,
				self->state_->pv->cursor, *pat_ev, psy_dsp_beatpos_zero(),
				self->state_->pv->sequence)->command);
		psy_ui_component_invalidate(&self->component);		
	}
	self->tweak_pattern_ = NULL;
	self->tweak_node_ = NULL;
	psy_ui_mouseevent_stop_propagation(ev);
}

void paramdraw_on_mouse_wheel(ParamDraw* self, psy_ui_MouseEvent* ev)
{
	if (!patternviewstate_sequence(self->state_->pv)) {		
		return;
	}	
	if (psy_ui_mouseevent_delta(ev) != 0) {
		psy_audio_SequenceCursor cursor;
		intptr_t value;
		psy_audio_PatternNode* prev;
		
		cursor = paramdraw_make_cursor(self, psy_ui_mouseevent_offset(ev).x);
		if (!psy_audio_sequencecursor_equal(&cursor, &self->state_->pv->cursor)) {						
			psy_audio_sequence_set_cursor(patternviewstate_sequence(
				self->state_->pv), cursor);
		}
		self->tweak_pattern_ = patternviewstate_pattern(self->state_->pv);
		if (!self->tweak_pattern_) {
			return;
		}	
		self->tweak_node_ = psy_audio_pattern_findnode_cursor(self->tweak_pattern_,
			self->state_->pv->cursor, &prev);
		if (self->tweak_node_) {			
			psy_audio_PatternEntry* entry;
			psy_audio_PatternEvent* pat_ev;

			entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
			pat_ev = psy_audio_patternentry_front(entry);					
			if (pat_ev->cmd == 0x0C) {
				value = pat_ev->parameter;
			} else {
				return;
			}			
		} else {
			return;
		}	
		if (psy_ui_mouseevent_delta(ev) > 0) {
			value += 1;
		} else {
			value -= 1;
		}
		value = psy_max(0, psy_min(0xFF, value));
		if (self->tweak_node_) {			
			psy_audio_PatternEntry* entry;
			psy_audio_PatternEvent* pat_ev;

			entry = (psy_audio_PatternEntry*)self->tweak_node_->entry;
			pat_ev = psy_audio_patternentry_front(entry);					
			if (pat_ev->cmd == 0x0C) {							
				pat_ev->parameter = (uint8_t)value;
				psy_audio_sequence_tweak(self->state_->pv->sequence);			
				psy_ui_component_invalidate(&self->component);
				paramdraw_output(self, pat_ev->parameter);
			}			
		}		
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void paramdraw_output(ParamDraw* self, uint8_t value)
{
	psy_Logger* logger;	
			
	logger = psy_ui_component_logger(&self->component, psy_ui_LOG_STATUS);
	if (logger) {
		char text[64];
					
		psy_snprintf(text, 64, "Tweak Cmd 0C Parameter %X", (int)value);
		psy_logger_output(logger, text);
	}			
}

uint8_t paramdraw_tweak_value(ParamDraw* self, psy_ui_MouseEvent* ev)
{
	psy_ui_RealSize size;				
	uintptr_t maxval;
	/* uint8_t cmd; */
	double px;
	intptr_t val;		
		
	size = psy_ui_component_scroll_size_px(&self->component);
	maxval = 256;
	/* cmd = 0x0C; */
	px = size.height / (double)maxval;
	val = (intptr_t)(psy_ui_mouseevent_offset(ev).y / px);
	if (val < 0) {
		val = 255;
	} else if (val < 256) {
		val = maxval - val;
	} else {
		val = 0;
	}
	return (uint8_t)val;
}

/* ParamRoll */

/* implementation */
void paramroll_init(ParamRoll* self, psy_ui_Component* parent,
	PianoGridState* state)
{
	assert(self);
	assert(state);
	
	psy_ui_component_init(&self->component, parent, NULL);
	/* left */
	psy_ui_component_init(&self->left_, &self->component, NULL);
	psy_ui_component_set_align(&self->left_, psy_ui_ALIGN_LEFT);
	psy_ui_component_set_preferred_size(&self->left_,
		psy_ui_size_make_em(10.0, 4.0));
	paramruler_init(&self->ruler_, &self->left_);
	psy_ui_component_set_align(paramruler_base(&self->ruler_),
		psy_ui_ALIGN_CLIENT);
	/* pane */
	psy_ui_component_init(&self->pane_, &self->component, NULL);
	psy_ui_component_set_align(&self->pane_, psy_ui_ALIGN_CLIENT);
	paramdraw_init(&self->draw_, &self->pane_, state);
	psy_ui_component_set_align(paramdraw_base(&self->draw_),
		psy_ui_ALIGN_FIXED);
}

#endif /* PSYCLE_USE_PIANOROLL */
