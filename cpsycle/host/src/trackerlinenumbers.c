/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackerlinenumbers.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* platform */
#include "../../detail/portable.h"


/* TrackerLineNumbers */

/* prototypes */
static void trackerlinenumbers_connect_configuration(TrackerLineNumbers*);
static void trackerlinenumbers_on_draw(TrackerLineNumbers*, psy_ui_Graphics*);
static TrackerColumnFlags trackerlinennumbers_column_flags(TrackerLineNumbers*,
	intptr_t line, intptr_t seqline);
static void trackerlinennumbers_draw_text(TrackerLineNumbers*, psy_ui_Graphics*,
	const char* format, double y, double width, const char* text);
static void trackerlinenumbers_invalidate_cursor_internal(TrackerLineNumbers*,
	const psy_audio_SequenceCursor*);
static void trackerlinenumbers_on_align(TrackerLineNumbers*);
static void trackerlinenumbers_on_preferred_size(TrackerLineNumbers*,
	psy_ui_Size* limit, psy_ui_Size* rv);
static void trackerlinenumbers_update_size(TrackerLineNumbers*);
static void trackerlinenumbers_configure(TrackerLineNumbers*);
static void trackerlinenumbers_on_show_beat_offset(TrackerLineNumbers*,
	psy_Property* sender);
static void trackerlinenumbers_on_show_cursor(TrackerLineNumbers*,
	psy_Property* sender);
static void trackerlinenumbers_on_show_in_hex(TrackerLineNumbers*,
	psy_Property* sender);
static void trackerlinennumbers_save_colours(TrackerLineNumbers*,
	psy_ui_Graphics*);	
static void trackerlinennumbers_set_colours(TrackerLineNumbers*,
	psy_ui_Graphics*, psy_ui_Style*);
static void trackerlinennumbers_text(TrackerLineNumbers*, intptr_t line,
	intptr_t pat_idx, char* rv);	

/* vtable */
static psy_ui_ComponentVtable trackerlinenumbers_vtable;
static bool trackerlinenumbers_vtable_initialized = FALSE;

static void trackerlinenumbers_vtable_init(TrackerLineNumbers* self)
{
	if (!trackerlinenumbers_vtable_initialized) {
		trackerlinenumbers_vtable = *(self->component.vtable);
		trackerlinenumbers_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			trackerlinenumbers_on_draw;
		trackerlinenumbers_vtable.onalign =
			(psy_ui_fp_component)
			trackerlinenumbers_on_align;
		trackerlinenumbers_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			trackerlinenumbers_on_preferred_size;
		trackerlinenumbers_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component,
		&trackerlinenumbers_vtable);
}

/* implementation */
void trackerlinenumbers_init(TrackerLineNumbers* self,
	psy_ui_Component* parent, TrackerState* state)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	trackerlinenumbers_vtable_init(self);	
	self->state = state;	
	self->show_in_hex = TRUE;
	self->show_beat = FALSE;
	self->draw_cursor = TRUE;
	self->draw_restore_fg_colour = psy_ui_colour_white();
	self->draw_restore_bg_colour = psy_ui_colour_black();	
	psy_ui_realsize_init(&self->size);	
	psy_audio_sequencecursor_init(&self->old_cursor);
	trackerlinenumbers_update_format(self);			
	psy_ui_component_set_scroll_step_height(&self->component,
		state->line_height);
	psy_ui_component_set_scroll_redraw(&self->component,
		psy_ui_SCROLLREDRAW_NONE);
	trackerlinenumbers_connect_configuration(self);
	trackerlinenumbers_configure(self);	
}

void trackerlinenumbers_connect_configuration(TrackerLineNumbers* self)
{
	psy_Configuration* cfg;
	
	assert(self);
	
	cfg = self->state->pv->patconfig;
	psy_configuration_connect(cfg, "beatoffset",
		self, trackerlinenumbers_on_show_beat_offset);
	psy_configuration_connect(cfg, "linenumberscursor",
		self, trackerlinenumbers_on_show_cursor);
	psy_configuration_connect(cfg, "linenumbersinhex",
		self, trackerlinenumbers_on_show_in_hex);	
}

void trackerlinenumbers_update_format(TrackerLineNumbers* self)
{	
	static const char* format_hex_seqstart = "%.2X %.2X";
	static const char* format_hex = "%X";

	static const char* format_seqstart = "%X %i";
	static const char* format = "%i";

	assert(self);
	
	if (self->show_in_hex) {		
		self->format = format_hex;
		self->format_seqstart = format_hex_seqstart;		
	} else {		
		self->format = format;
		self->format_seqstart = format_seqstart;		
	}
}

void trackerlinenumbers_on_draw(TrackerLineNumbers* self, psy_ui_Graphics* g)
{	
	psy_ui_RealSize size;
	psy_ui_RealRectangle g_clip;
	BeatClip clip;	
	uintptr_t line;
	uintptr_t num_clip_lines;
	intptr_t seqline;
	uintptr_t num_entry_lines;	
	uintptr_t count;	
	uintptr_t patidx;	
	psy_audio_SequenceTrackIterator ite;
	double cpy;	
		
	assert(self);
			
	if (!patternviewstate_sequence(self->state->pv)) {
		return;
	}		
	psy_ui_graphics_set_encoding(g, PSY_ENCODING_NONE);
	/* prepare clipping */
	g_clip = psy_ui_graphics_cliprect(g);
	beatclip_init(&clip, &self->state->beat_convert, g_clip.top,
		g_clip.bottom);
	clip.end = psy_dsp_beatpos_min(clip.end, patternviewstate_length(
		self->state->pv));	
	line = beatline_beat_to_line(&self->state->pv->beat_line, clip.begin);
	num_clip_lines = beatline_beat_to_line(&self->state->pv->beat_line,
		clip.end) - line;
	cpy = beatconvert_beat_to_px(&self->state->beat_convert, clip.begin);		
	if (patternviewstate_single_mode(self->state->pv)) {		
		seqline = beatline_beat_to_line(&self->state->pv->beat_line,
			psy_audio_sequencecursor_seqoffset(&self->state->pv->cursor,
			self->state->pv->sequence));
		num_entry_lines = beatline_beat_to_line(&self->state->pv->beat_line,
			patternviewstate_length(self->state->pv));
		patidx = 0;
	} else {
		psy_audio_sequencetrackiterator_init(&ite);
		psy_audio_sequence_begin(self->state->pv->sequence,			
			self->state->pv->cursor.order_index.track, clip.begin, &ite);
		patidx = psy_audio_sequencetrackiterator_pattern_index(&ite);		
		seqline = beatline_beat_to_line(&self->state->pv->beat_line,
			psy_audio_sequencetrackiterator_seq_offset(&ite));
		num_entry_lines = beatline_beat_to_line(&self->state->pv->beat_line,
			psy_audio_sequencetrackiterator_entry_length(&ite));
		line -= seqline;		
	}	
	/* prepare draw */
	trackerlinennumbers_save_colours(self, g);
	size = psy_ui_component_scroll_size_px(&self->component);
	/* draw lines */	
	count = 0;	
	while (count <= num_clip_lines) {						
		if (line < num_entry_lines) {
			char text[64];
			
			trackerlinennumbers_text(self, line, patidx, text);		
			trackerlinennumbers_set_colours(self, g, trackerstate_column_style(
				self->state, trackerlinennumbers_column_flags(self, line,
				seqline), 0));
			trackerlinennumbers_draw_text(self, g, text, cpy, size.width, text);
		}
		cpy += self->state->beat_convert.line_px;
		++line;
		++count;		
		if (!patternviewstate_single_mode(self->state->pv)) {			
			if (line >= num_entry_lines) {
				if (psy_audio_sequencetrackiterator_has_next_entry(&ite)) {
					psy_audio_sequencetrackiterator_inc_entry(&ite);					
					seqline = beatline_beat_to_line(&self->state->pv->beat_line,
						psy_audio_sequencetrackiterator_seq_offset(&ite));
					line = 0;
					cpy = (line + seqline) * self->state->beat_convert.line_px;
					num_entry_lines = beatline_beat_to_line(
						&self->state->pv->beat_line,						
						psy_audio_sequencetrackiterator_entry_length(&ite));					
					patidx = psy_audio_sequencetrackiterator_pattern_index(&ite);					
				} else {
					break;
				}
			}
		}
	}
	if (!patternviewstate_single_mode(self->state->pv)) {
		psy_audio_sequencetrackiterator_dispose(&ite);
	}		
}

void trackerlinennumbers_text(TrackerLineNumbers* self, intptr_t line,
	intptr_t pat_idx, char* rv)
{
	assert(self);
	
	if (line < 0) {
		rv[0] = '\0';
	} else if (!patternviewstate_single_mode(self->state->pv) && line == 0) {			
		psy_snprintf(rv, 64, self->format_seqstart, (int)pat_idx, (int)line);
	} else {
		psy_snprintf(rv, 64, self->format, (int)line);
	}
}

void trackerlinennumbers_save_colours(TrackerLineNumbers* self,
	psy_ui_Graphics* g)
{
	assert(self);
	
	self->draw_restore_fg_colour = psy_ui_component_colour(&self->component);
	self->draw_restore_bg_colour = psy_ui_component_background_colour(
		&self->component);
}

void trackerlinennumbers_set_colours(TrackerLineNumbers* self,
	psy_ui_Graphics* g, psy_ui_Style* style)
{
	assert(self);
	
	if (style) {
		if (style->background.colour.mode.transparent) {
			psy_ui_set_background_colour(g, self->draw_restore_bg_colour);
		} else {
			psy_ui_set_background_colour(g, psy_ui_style_background_colour(style));
		}
		if (style->colour.mode.transparent) {
			psy_ui_graphics_set_text_colour(g, self->draw_restore_fg_colour);
		} else {
			psy_ui_graphics_set_text_colour(g, psy_ui_style_colour(style));
		}
	}
}

TrackerColumnFlags trackerlinennumbers_column_flags(TrackerLineNumbers* self,
	intptr_t line, intptr_t seqline)
{
	TrackerColumnFlags rv;	
	uintptr_t line_abs;
	uintptr_t lpb;
	psy_audio_HostSequencerTime seqtime;

	assert(self);
		
	seqtime = self->state->pv->player->sequencer.hostseqtime;
	lpb = psy_audio_sequencecursor_lpb(&self->state->pv->cursor);	
	line_abs = seqline + line;		
	rv.playbar = seqtime.playing && (seqtime.currplaycursor.abs_line ==
		line_abs);
	rv.mid = 0;	
	rv.cursor = (self->state->pv->cursor.abs_line == line_abs);	
	rv.beat = (line % lpb) == 0;
	rv.beat4 = (line % (lpb * 4)) == 0;
	rv.selection = 0;
	return rv;
}

void trackerlinennumbers_draw_text(TrackerLineNumbers* self, psy_ui_Graphics* g,
	const char* format, double y, double width, const char* text)
{
	uintptr_t numdigits;
	uintptr_t maxdigits;
	uintptr_t startdigit;
	double blankspace;	
	char digit[2];
	psy_ui_RealRectangle r;
	uintptr_t c;
	const psy_ui_TextMetric* tm;
	double flat_size;

	assert(self);
	
	digit[1] = '\0';
	tm = psy_ui_component_textmetric(&self->component);
	numdigits = psy_strlen(text);
	if (numdigits == 0) {
		return;
	}
	flat_size = psy_ui_value_px(&self->state->track_config->flatsize, tm, NULL);
	maxdigits = (uintptr_t)(width / flat_size);
	if (maxdigits == 0) {
		return;
	}	
	startdigit = maxdigits - numdigits;
	if (numdigits < maxdigits) {
		--startdigit;
	}
	for (c = 0; c < maxdigits; ++c) {
		psy_ui_RealPoint cp;		
		
		if (c >= startdigit && c < startdigit + numdigits) {
			digit[0] = text[c - startdigit];
		} else {
			digit[0] = ' ';
		}
		cp = psy_ui_realpoint_make(c * flat_size, y);
		if (c == maxdigits - 1) {
			r = psy_ui_realrectangle_make(cp,
				psy_ui_realsize_make(self->size.width - cp.x - 1,
					self->state->beat_convert.line_px - 1));
		} else {
			r = psy_ui_realrectangle_make(cp,
				psy_ui_realsize_make(flat_size,
					self->state->beat_convert.line_px - 1));
		}
		psy_ui_graphics_textout_rectangle(g, cp, psy_ui_ETO_OPAQUE | psy_ui_ETO_CLIPPED,
			r, digit, psy_strlen(digit));
	}
	r.left += flat_size;
	blankspace = (width - r.left) - 4;
	if (blankspace > 0) {
		r = psy_ui_realrectangle_make(
				psy_ui_realpoint_make(r.left, y),
			psy_ui_realsize_make(
				blankspace, self->state->beat_convert.line_px - 1));
		digit[0] = ' ';
		psy_ui_graphics_textout_rectangle(g, psy_ui_realrectangle_topleft(&r),
			psy_ui_ETO_OPAQUE | psy_ui_ETO_CLIPPED, r,
			digit, psy_strlen(digit));
	}
}

void trackerlinenumbers_invalidate_cursor_internal(TrackerLineNumbers* self,
	const psy_audio_SequenceCursor* cursor)
{		
	intptr_t line;
	
	assert(self);
	
	line = beatline_beat_to_line(&self->state->pv->beat_line,
		(patternviewstate_single_mode(self->state->pv))
		? psy_audio_sequencecursor_offset(cursor)
		: psy_audio_sequencecursor_offset_abs(cursor,
			self->state->pv->sequence));		
	psy_ui_component_invalidate_rect(&self->component,
		psy_ui_realrectangle_make(psy_ui_realpoint_make(
			0.0, self->state->beat_convert.line_px * line),
			psy_ui_realsize_make(self->size.width,
			self->state->beat_convert.line_px)));
}

void trackerlinenumbers_invalidate_cursor(TrackerLineNumbers* self)
{
	assert(self);
	
	trackerlinenumbers_invalidate_cursor_internal(self, &self->old_cursor);
	trackerlinenumbers_invalidate_cursor_internal(self,
		&self->state->pv->cursor);
	self->old_cursor = self->state->pv->cursor;
}

void trackerlinenumbers_invalidate_playbar(TrackerLineNumbers* self)
{	
	double minval;
	double maxval;
	psy_ui_RealSize size;

	assert(self);
	
	trackerstate_playbar_update_range(self->state,
		&self->state->pv->player->sequencer.hostseqtime,
		&minval, &maxval);
	if (maxval == 0.0) {
		return;
	}	
	size = psy_ui_component_scroll_size_px(&self->component);
	psy_ui_component_invalidate_rect(&self->component, 
		psy_ui_realrectangle_make(
			psy_ui_realpoint_make(0.0, minval),
			psy_ui_realsize_make(
				size.width,
				maxval - minval + self->state->beat_convert.line_px)));
		
}

void trackerlinenumbers_on_align(TrackerLineNumbers* self)
{
	assert(self);
	
	trackerlinenumbers_update_size(self);
}

void trackerlinenumbers_on_preferred_size(TrackerLineNumbers* self,
	psy_ui_Size* limit, psy_ui_Size* rv)
{	
	double width;

	assert(self);
		
	width = 5.0;
	if (self->show_in_hex) {
	}
	if (self->show_beat) {
		width += 5.0;
	}
	if (!patternviewstate_single_mode(self->state->pv)) {
		width += 1.0;	
	}
	rv->width = psy_ui_mul_values(psy_ui_value_make_ew(width),
		self->state->track_config->flatsize,
		psy_ui_component_textmetric(&self->component),
		NULL);
	rv->height = psy_ui_value_make_px(
		patternviewstate_numlines(self->state->pv) *
		self->state->beat_convert.line_px);
}

void trackerlinenumbers_update_size(TrackerLineNumbers* self)
{	
	assert(self);
	
	self->size = psy_ui_component_scroll_size_px(&self->component);		
}

void trackerlinenumbers_show_cursor(TrackerLineNumbers* self)
{	
	assert(self);
	
	self->draw_cursor = TRUE;
	psy_ui_component_invalidate(&self->component);
}

void trackerlinenumbers_hide_cursor(TrackerLineNumbers* self)
{
	assert(self);
	
	self->draw_cursor = FALSE;
	psy_ui_component_invalidate(&self->component);
}

void trackerlinenumbers_show_in_hex(TrackerLineNumbers* self)
{
	assert(self);
	
	self->show_in_hex = TRUE;
	trackerlinenumbers_update_format(self);
	psy_ui_component_invalidate(&self->component);
}

void trackerlinenumbers_show_in_decimal(TrackerLineNumbers* self)
{
	assert(self);
	
	self->show_in_hex = FALSE;
	trackerlinenumbers_update_format(self);
	psy_ui_component_invalidate(&self->component);
}

void trackerlinenumbers_configure(TrackerLineNumbers* self)
{
	psy_Configuration* cfg;

	assert(self);
	
	cfg = self->state->pv->patconfig;
	if (psy_configuration_value_bool(cfg, "beatoffset", FALSE)) {
		trackerlinenumbers_show_beat(self);		
	} else {
		trackerlinenumbers_hide_beat(self);		
	}
	if (psy_configuration_value_bool(cfg, "linenumberscursor", FALSE)) {
		trackerlinenumbers_show_cursor(self);
	} else {
		trackerlinenumbers_hide_cursor(self);
	}
	if (psy_configuration_value_bool(cfg, "linenumbersinhex", FALSE)) {
		trackerlinenumbers_show_in_hex(self);
	} else {
		trackerlinenumbers_show_in_decimal(self);
	}
}

void trackerlinenumbers_on_show_beat_offset(TrackerLineNumbers* self,
	psy_Property* sender)
{
	psy_ui_Component* align_parent;

	assert(self);

	if (psy_property_item_bool(sender)) {
		trackerlinenumbers_show_beat(self);
	} else {
		trackerlinenumbers_hide_beat(self);
	}
	align_parent = psy_ui_component_parent(psy_ui_component_parent(
		psy_ui_component_parent(&self->component)));
	if (align_parent) {
		psy_ui_component_align(align_parent);
		psy_ui_component_invalidate(align_parent);
	}
}

void trackerlinenumbers_on_show_in_hex(TrackerLineNumbers* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		trackerlinenumbers_show_in_hex(self);
	} else {
		trackerlinenumbers_show_in_decimal(self);
	}	
}

void trackerlinenumbers_on_show_cursor(TrackerLineNumbers* self,
	psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		trackerlinenumbers_show_cursor(self);
	} else {
		trackerlinenumbers_hide_cursor(self);
	}
}


/* TrackerLineNumberView */

/* prototypes */
static void trackerlinenumberview_on_scrolled(TrackerLineNumberView*,
	psy_ui_Component* sender, double dx, double dy);

/* implementation */
void trackerlinenumberview_init(TrackerLineNumberView* self,
	psy_ui_Component* parent, TrackerState* state)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	trackerlinenumbers_init(&self->linenumbers, &self->component, state);
	psy_ui_component_set_align(&self->linenumbers.component,
		psy_ui_ALIGN_FIXED);
	psy_signal_connect(&self->linenumbers.component.signal_scrolled, self,
		trackerlinenumberview_on_scrolled);
}

void trackerlinenumberview_set_scroll_top(TrackerLineNumberView* self,
	psy_ui_Value top)
{
	assert(self);
	
	psy_ui_component_set_scroll_top(&self->linenumbers.component, top);
}

void trackerlinenumberview_on_scrolled(TrackerLineNumberView* self,
	psy_ui_Component* sender, double dx, double dy)	
{
	assert(self);
		
	trackerstate_scroll_to(self->linenumbers.state, sender, dx, dy);		
}

#endif /* PSYCLE_USE_TRACKERVIEW */
