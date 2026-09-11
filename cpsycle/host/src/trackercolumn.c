/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackercolumn.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* platform */
#include "../../detail/portable.h"

static const char* notetostr(psy_audio_PatternEvent ev,
	psy_dsp_NotesTabMode notes_tab_mode, bool showemptydate)
{
	static const char* emptynotestr = "- - -";

	if (ev.note != psy_audio_NOTECOMMANDS_EMPTY || !showemptydate) {
		return psy_dsp_notetostr(ev.note, notes_tab_mode);
	}
	return emptynotestr;
}

/* prototypes */
static void trackercolumn_on_draw(TrackerColumn*, psy_ui_Graphics*);
static void trackercolumn_draw_track_events(TrackerColumn*, psy_ui_Graphics*);
static  TrackerColumnFlags trackercolumn_columnflags(TrackerColumn*,
	uintptr_t line, psy_dsp_beatpos_t seqoffset);
static void trackercolumn_draw_entry(TrackerColumn*, psy_ui_Graphics*,
	psy_audio_PatternEntry*, double y, TrackerColumnFlags, TrackDef*);
static void trackercolumn_draw_digit(TrackerColumn*, psy_ui_Graphics*,
	psy_ui_RealPoint cp, const char* str);
static void trackercolumn_draw_resize_bar(TrackerColumn*, psy_ui_Graphics*);
static void trackercolumn_on_mouse_down(TrackerColumn*, psy_ui_MouseEvent*);
static void trackercolumn_on_mouse_move(TrackerColumn*, psy_ui_MouseEvent*);
static void trackercolumn_on_mouse_up(TrackerColumn*, psy_ui_MouseEvent*);
static void trackercolumn_on_align(TrackerColumn*);
static void trackercolumn_on_preferred_size(TrackerColumn*,
	const psy_ui_Size* limit, psy_ui_Size* rv);
static void trackercolumn_update_cursor(TrackerColumn*, double position);
static bool trackercolumn_is_over_note(const TrackerColumn*, double position);
static bool trackercolumn_is_over_column(const TrackerColumn*, double position);
static bool trackercolumn_in_selection(TrackerColumn*, uintptr_t track,
	psy_dsp_beatpos_t offset_abs);
static void trackercolumn_set_style(TrackerColumn*, psy_ui_Graphics*,
	psy_ui_Style*, uintptr_t noteindex, bool multicolumn);

/* vtable */
static psy_ui_ComponentVtable trackercolumn_vtable;
static bool trackercolumn_vtable_initialized = FALSE;

static void trackercolumn_vtable_init(TrackerColumn* self)
{
	if (!trackercolumn_vtable_initialized) {
		trackercolumn_vtable = *(self->component.vtable);
		trackercolumn_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			trackercolumn_on_draw;
		trackercolumn_vtable.onalign =
			(psy_ui_fp_component)
			trackercolumn_on_align;
		trackercolumn_vtable.onpreferredsize =
			(psy_ui_fp_component_on_preferred_size)
			trackercolumn_on_preferred_size;
		trackercolumn_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			trackercolumn_on_mouse_down;
		trackercolumn_vtable.on_mouse_move =
			(psy_ui_fp_component_on_mouse_event)
			trackercolumn_on_mouse_move;
		trackercolumn_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			trackercolumn_on_mouse_up;
		trackercolumn_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &trackercolumn_vtable);
}

/* implementation */
void trackercolumn_init(TrackerColumn* self, psy_ui_Component* parent,
	uintptr_t index, TrackerState* state, psy_audio_Sequencer* sequencer)
{
	assert(self);
	assert(state);
	assert(sequencer);
	
	psy_ui_component_init(&self->component, parent, NULL);
	trackercolumn_vtable_init(self);			
	self->state = state;
	self->sequencer = sequencer;	
	self->track = index;
	self->draw_restore_fg_colour = psy_ui_colour_white();
	self->draw_restore_bg_colour = psy_ui_colour_black();
	psy_ui_realsize_init(&self->size);
	psy_ui_realsize_init(&self->digitsize);	
	psy_ui_component_set_align(trackercolumn_base(self), psy_ui_ALIGN_LEFT);
	psy_ui_component_set_encoding(trackercolumn_base(self), PSY_ENCODING_NONE);
}

TrackerColumn* trackercolumn_alloc(void)
{
	return (TrackerColumn*)malloc(sizeof(TrackerColumn));
}

TrackerColumn* trackercolumn_alloc_init(psy_ui_Component* parent,
	uintptr_t index, TrackerState* state, psy_audio_Sequencer* sequencer)
{
	TrackerColumn* rv;

	rv = trackercolumn_alloc();
	if (rv) {
		trackercolumn_init(rv, parent, index, state, sequencer);
		psy_ui_component_deallocate_after_destroyed(trackercolumn_base(rv));
	}
	return rv;
}

void trackercolumn_on_draw(TrackerColumn* self, psy_ui_Graphics* g)
{	
	trackercolumn_draw_track_events(self, g);
	if (trackdrag_trackactive(&self->state->track_config->resize,
			self->track)) {
		trackercolumn_draw_resize_bar(self, g);
	}	
}

void trackercolumn_draw_track_events(TrackerColumn* self, psy_ui_Graphics* g)
{	
	psy_List** events;
	psy_List* p;
	double cpy;	
	uintptr_t line;	
	TrackDef* trackdef;
	uintptr_t lpb;
		
	events = trackereventtable_track(&self->state->track_events,
		self->track);
	trackdef = trackerconfig_trackdef(self->state->track_config, self->track);	
	self->draw_restore_fg_colour = psy_ui_component_colour(&self->component);
	self->draw_restore_bg_colour = psy_ui_component_background_colour(&self->component);
	lpb = self->state->pv->cursor.lpb;
	if (patternviewstate_single_mode(self->state->pv)) {
		cpy = trackerstate_beat_to_px(self->state, 
				self->state->track_events.top);
		line = (uintptr_t)(psy_dsp_beatpos_real(self->state->track_events.top)
			* lpb);
	}
	for (p = *events; p != NULL; p = p->next) {
		TrackerEventPair* pair;
		
		pair = (TrackerEventPair*)p->entry;
		if (!patternviewstate_single_mode(self->state->pv)) {
			cpy = trackerstate_beat_to_px(self->state, 
				psy_dsp_beatpos_add(pair->offset, pair->seqoffset));
			line = psy_dsp_beatpos_line(pair->offset, lpb);
		}		
		trackercolumn_draw_entry(self, g, pair->entry,
			cpy, trackercolumn_columnflags(self, line,
				pair->seqoffset), trackdef);
		if (patternviewstate_single_mode(self->state->pv)) {
			cpy += self->state->beat_convert.line_px;
			++line;
		}
	}	
}

TrackerColumnFlags trackercolumn_columnflags(TrackerColumn* self,
	uintptr_t line, psy_dsp_beatpos_t seqoffset)
{
	TrackerColumnFlags rv;
	uintptr_t line_abs;
	uintptr_t lpb;
		
	assert(self);
	
	lpb = psy_audio_sequencecursor_lpb(&self->state->pv->cursor);		
	if (self->state->draw_beat_highlights) {
		
		rv.beat = ((line) % lpb) == 0;
		rv.beat4 = ((line) % (lpb * 4)) == 0;		
		rv.mid = self->state->midline &&
			(line == trackerstate_mid_line(self->state,
			psy_ui_component_scroll_top_px(psy_ui_component_parent(
				&self->component)),
				self->size.height));
	} else {
		rv.beat = rv.beat4 = rv.mid = FALSE;
	}
	line_abs = psy_dsp_beatpos_line(seqoffset, lpb) + line;		
	rv.cursor = (self->track == self->state->pv->cursor.channel_) &&
		(self->state->pv->cursor.abs_line == line_abs);
	rv.selection = trackercolumn_in_selection(self, self->track,
		psy_dsp_beatpos_make_lines(line_abs, lpb, psy_dsp_DEFAULT_PPQ));
	rv.playbar = self->state->draw_playbar &&
		psy_audio_hostsequencertime_playing(&self->sequencer->hostseqtime) &&		
		(self->sequencer->hostseqtime.currplaycursor.abs_line == line_abs);
	rv.focus = TRUE;
	return rv;
}

bool trackercolumn_in_selection(TrackerColumn* self, uintptr_t track,
	psy_dsp_beatpos_t offset_abs)
{	
	if (!psy_audio_blockselection_valid(&self->state->pv->selection.block)) {
		return FALSE;
	}
	if (!psy_audio_blockselection_test_track(&self->state->pv->selection.block, track)) {
		return FALSE;
	}				
	if (psy_dsp_beatpos_greater_equal(offset_abs,
			self->state->track_events.selection_top_abs) &&
		psy_dsp_beatpos_less(offset_abs,
			self->state->track_events.selection_bottom_abs)) {
		return TRUE;
	}
	return FALSE;
}

void trackercolumn_draw_entry(TrackerColumn* self, psy_ui_Graphics* g,
	psy_audio_PatternEntry* entry, double y, TrackerColumnFlags columnflags,
	TrackDef* trackdef)
{
	uintptr_t noteindex;	
	psy_ui_RealPoint cp;		
	psy_audio_PatternEventNode* curr;
	char* emptystr;
	psy_audio_PatternEvent emptyevent;	

	if (!entry) {
		entry = &self->state->empty;
	}		
	cp = psy_ui_realpoint_make(0.0, y);
	emptystr = (self->state->show_empty_data) ? "." : "";
	curr = psy_audio_patternentry_begin(entry);
	psy_audio_patternevent_init(&emptyevent);	
	for (noteindex = 0; noteindex < trackdef_visinotes(trackdef); ++noteindex) {
		psy_audio_PatternEvent* ev;
		TrackerColumnFlags currcolumnflags;
		uintptr_t column;		
		const char* notestr;		
		psy_ui_RealPoint cp_leftedge;
		bool is_tweak;
		
		if (curr) {
			ev = (psy_audio_PatternEvent*)curr->entry;
			curr = curr->next;
		} else {
			ev = &emptyevent;
		}		
		is_tweak = ev->note >= psy_audio_NOTECOMMANDS_TWEAK &&
			ev->note <= psy_audio_NOTECOMMANDS_TWEAK_SLIDE;
		cp_leftedge = cp;
		cp_leftedge.x += self->state->track_config->textleftedge;
		/* draw note */
		if (!trackdef->multicolumn || noteindex == 0) {
			column = 0;
			currcolumnflags = columnflags;
			currcolumnflags.cursor = columnflags.cursor &&
				(self->state->pv->cursor.column == 0) &&
				(self->state->pv->cursor.noteindex == noteindex);
			trackercolumn_set_style(self, g, trackerstate_column_style(
				self->state, currcolumnflags, psy_audio_patternentry_track(
					entry)), noteindex, trackdef->multicolumn);			
			notestr = notetostr(*ev, psy_dsp_NOTESTAB_A440, FALSE);
			psy_ui_graphics_textout_rectangle(g,
				cp_leftedge,
				psy_ui_ETO_OPAQUE | psy_ui_ETO_CLIPPED,
				psy_ui_realrectangle_make(cp, psy_ui_realsize_make(
					self->digitsize.width * 3.0, self->digitsize.height)),
				notestr, psy_strlen(notestr));
			cp.x += trackdef_column_width(trackdef, 0, self->digitsize.width);
		}
		/* draw digit columns */
		if (trackdef->multicolumn && noteindex > 0) {
			column = PATTERNEVENT_COLUMN_CMD;
		} else {
			column = 1;
		}
		for (; column < trackdef_num_columns(trackdef); ++column) {
			TrackColumnDef* coldef;
			uintptr_t digit;
			uintptr_t value;
			bool empty;
			uintptr_t num;
			const char* digitstr = NULL;

			coldef = trackdef_column_def(trackdef, column);
			if (!coldef) {
				continue;
			}			
			value = trackdef_event_value(trackdef, column, ev);
			if (is_tweak && coldef->ignore_empty_at_twk) {
				empty = FALSE;
			} else {
				empty = (coldef->emptyvalue == value);
			}
			if (column > PATTERNEVENT_COLUMN_VOL &&
					psy_audio_patternevent_tweakvalue(ev) != 0) {
				empty = FALSE;
			}
			if (empty) {
				digitstr = emptystr;
			}
			
			currcolumnflags.cursor = columnflags.cursor &&
				(column == self->state->pv->cursor.column) &&
				(self->state->pv->cursor.noteindex == noteindex);
			for (num = coldef->numdigits, digit = 0; digit < num;
					++digit, cp.x += self->digitsize.width) {
				currcolumnflags = columnflags;
				currcolumnflags.cursor = columnflags.cursor &&
					(self->state->pv->cursor.column == column) &&
					(self->state->pv->cursor.digit == digit) &&
					(self->state->pv->cursor.noteindex == noteindex);
				trackercolumn_set_style(self, g, trackerstate_column_style(
					self->state, currcolumnflags, psy_audio_patternentry_track(
						entry)), noteindex, trackdef->multicolumn);
				if (!empty) {
					digitstr = hex_tab[((value >> ((num - digit - 1) * 4)) & 0x0F)];
				}
				trackercolumn_draw_digit(self, g, cp, digitstr);
			}
			cp.x += coldef->marginright;
		}
	}
}

void trackercolumn_set_style(TrackerColumn* self, psy_ui_Graphics* g,
	psy_ui_Style* style, uintptr_t noteindex, bool multicolumn)
{
	psy_ui_Colour fg;
	psy_ui_Colour bg;
			
	assert(self);
	
	if (!style) {
		return;
	}	
	if (style->background.colour.mode.transparent) {
		bg = self->draw_restore_bg_colour;						
	} else {
		bg = psy_ui_style_background_colour(style);
	}
	if (style->colour.mode.transparent) {
		fg = self->draw_restore_fg_colour;						
	} else {
		fg = psy_ui_style_colour(style);
	}
	if (noteindex > 0) {
		double m;
	
		if (multicolumn) {
			m = 0.95;
		} else {
			m = psy_max(0.1, (1.0 - (double)noteindex * 0.02));
		}
		psy_ui_colour_mul_rgb(&fg, m, m, m);
		psy_ui_colour_mul_rgb(&bg, m, m, m);					
	}
	psy_ui_set_background_colour(g, bg);
	psy_ui_graphics_set_text_colour(g, fg);	
}

void trackercolumn_draw_digit(TrackerColumn* self, psy_ui_Graphics* g,
	psy_ui_RealPoint cp, const char* str)
{
	psy_ui_RealPoint cp_leftedge;

	cp_leftedge = cp;
	cp_leftedge.x += self->state->track_config->textleftedge;
	psy_ui_graphics_textout_rectangle(g, cp_leftedge,
		psy_ui_ETO_OPAQUE | psy_ui_ETO_CLIPPED,
			psy_ui_realrectangle_make(cp, self->digitsize), str, psy_strlen(str));
}

void trackercolumn_draw_resize_bar(TrackerColumn* self, psy_ui_Graphics* g)
{
	TrackDef* trackdef;

	trackdef = trackerconfig_trackdef(self->state->track_config,
		self->track);
	if (trackdef->multicolumn) {
		psy_ui_graphics_draw_solid_rectangle(g,
			psy_ui_realrectangle_make(
				psy_ui_realpoint_make(self->size.width - 3, 0),
				psy_ui_realsize_make(3.0, self->size.height)),
			psy_ui_colour_white());
	} else {
		double notewidth;
		
		notewidth = trackdef_column_width(trackdef, 0,
			psy_ui_value_px(&self->state->track_config->flatsize,
				psy_ui_component_textmetric(&self->component), NULL));
		psy_ui_graphics_draw_solid_rectangle(g,
			psy_ui_realrectangle_make(
				psy_ui_realpoint_make(notewidth - 3, 0),
				psy_ui_realsize_make(3.0, self->size.height)),
			psy_ui_colour_white());
	}
}

void trackercolumn_on_mouse_down(TrackerColumn* self, psy_ui_MouseEvent* ev)
{	
	double notewidth;
	TrackDef* trackdef;

	if (psy_ui_mouseevent_button(ev) != 1) {
		return;
	}	
	trackdef = trackerconfig_trackdef(self->state->track_config, self->track);
	notewidth = trackdef_column_width(trackdef, 0,
		psy_ui_value_px(&self->state->track_config->flatsize,
			psy_ui_component_textmetric(&self->component), NULL));	
	if (psy_ui_mouseevent_pt(ev).x > notewidth - 5 &&
			psy_ui_mouseevent_pt(ev).x < notewidth) {
		trackconfig_setmulticolumn(self->state->track_config, self->track,
			FALSE);		
		trackdrag_start(&self->state->track_config->resize, self->track,
			self->size.width);
	} else if (psy_ui_mouseevent_pt(ev).x > self->size.width - 5) {
		trackconfig_setmulticolumn(self->state->track_config, self->track,
			TRUE);
		trackdrag_start(&self->state->track_config->resize, self->track,
			self->size.width);		
	} else {
		if (psy_audio_blockselection_valid(&self->state->pv->selection.block)) {
			psy_audio_blockselection_disable(&self->state->pv->selection.block);
			psy_ui_component_invalidate(psy_ui_component_parent(&self->component));
		}
		self->state->pv->selection.block.drag_base_ = trackerstate_make_cursor(
			self->state, psy_ui_mouseevent_pt(ev), self->track,
			psy_ui_component_textmetric(&self->component));
	}
	if (trackdrag_active(&self->state->track_config->resize)) {		
		psy_ui_component_capture(&self->component);
		trackconfig_resize(self->state->track_config, self->track,
			self->size.width, psy_ui_component_textmetric(&self->component));
	}	
}

void trackercolumn_on_mouse_move(TrackerColumn* self, psy_ui_MouseEvent* ev)
{			
	trackconfig_resize(self->state->track_config, self->track,
		psy_ui_mouseevent_pt(ev).x,
		psy_ui_component_textmetric(&self->component));
	trackercolumn_update_cursor(self, psy_ui_mouseevent_pt(ev).x);
}

void trackercolumn_update_cursor(TrackerColumn* self, double position)
{	
	if (trackercolumn_is_over_note(self, position) ||
			trackercolumn_is_over_column(self, position)) {
		psy_ui_component_set_cursor(&self->component,
			psy_ui_CURSORSTYLE_COL_RESIZE);
	}
}

bool trackercolumn_is_over_note(const TrackerColumn* self, double position)
{	
	return (position > self->size.width - 5);
}

bool trackercolumn_is_over_column(const TrackerColumn* self, double position)
{
	double flatsize;

	flatsize = psy_ui_value_px(&self->state->track_config->flatsize,
		psy_ui_component_textmetric(&self->component), NULL);
	return (position >= flatsize * 3 - 5 &&
		position < flatsize * 3);
}

void trackercolumn_on_mouse_up(TrackerColumn* self, psy_ui_MouseEvent* ev)
{
	psy_ui_component_release_capture(&self->component);	
	trackconfig_resize(self->state->track_config, self->track,
		psy_ui_mouseevent_pt(ev).x,
		psy_ui_component_textmetric(&self->component));
}

void trackercolumn_on_align(TrackerColumn* self)
{	
	self->size = psy_ui_component_scroll_size_px(&self->component);
	self->digitsize = psy_ui_realsize_make(
		psy_ui_value_px(&self->state->track_config->flatsize,
			psy_ui_component_textmetric(&self->component), NULL),
		self->state->beat_convert.line_px - 1);
}

void trackercolumn_on_preferred_size(TrackerColumn* self,
	const psy_ui_Size* limit, psy_ui_Size* rv)
{		
	psy_ui_size_setpx(rv, trackerstate_trackwidth(self->state, self->track,
		psy_ui_component_textmetric(&self->component)),
		patternviewstate_numlines(self->state->pv) *
		self->state->beat_convert.line_px);
}

#endif /* PSYCLE_USE_TRACKERVIEW */
