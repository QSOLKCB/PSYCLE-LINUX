/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "trackerview.h"

#ifdef PSYCLE_USE_TRACKERVIEW

/* host */
#include "styles.h"
/* audio */
#include <pattern.h>
/* platform */
#include "../../detail/portable.h"


/* TrackerConfig */

/* implementation */
void trackdrag_init(TrackDrag* self)
{
	self->active = FALSE;
	self->track = psy_INDEX_INVALID;
	psy_ui_realsize_init(&self->size);
}

void trackdrag_start(TrackDrag* self, uintptr_t track, double width)
{
	self->track = track;
	self->size.width = width;
	self->active = TRUE;
}

void trackdrag_stop(TrackDrag* self)
{
	self->active = FALSE;
}

bool trackdrag_active(const TrackDrag* self)
{
	return (self->active);
}

bool trackdrag_trackactive(const TrackDrag* self, uintptr_t track)
{
	return (self->active && self->track == track);
}


/* TrackerConfig */

/* implementation */
void trackconfig_init(TrackConfig* self, bool wideinst)
{
	self->textleftedge = 2;	
	self->flatsize = psy_ui_value_make_ew(1.6);
	trackdrag_init(&self->resize);
	psy_table_init(&self->trackconfigs);
	trackconfig_init_columns(self, wideinst);
}

void trackconfig_dispose(TrackConfig* self)
{
	psy_table_dispose_all(&self->trackconfigs, (psy_fp_disposefunc)NULL);
}

void trackconfig_init_columns(TrackConfig* self, bool wideinst)
{
	trackdef_init(&self->trackdef);
	if (wideinst) {
		self->trackdef.inst.numdigits = 4;
		self->trackdef.inst.numchars = 4;
	}
}

double trackconfig_width(const TrackConfig* self, uintptr_t track,
	const psy_ui_TextMetric* tm)
{
	const TrackDef* trackdef;
	double rv;
	double cmdparam;
	uintptr_t column;

	trackdef = trackerconfig_trackdef_const(self, track);
	cmdparam = 0.0;
	for (column = 0, rv = 0.0; column < trackdef_num_columns(trackdef);
			++column) {
		double width;

		width = trackdef_column_width(trackdef, column,
			psy_ui_value_px(&self->flatsize, tm, NULL));
		if (column >= PATTERNEVENT_COLUMN_CMD) {
			cmdparam += width;
		}
		rv += width;
	}
	if (trackdef->multicolumn) {
		return rv + (trackdef->visinotes - 1) * cmdparam + 1.0;
	}
	return trackdef->visinotes * rv + 1.0;
}

double trackconfig_width_cmdparam(const TrackConfig* self,
	const psy_ui_TextMetric* tm)
{
	const TrackDef* trackdef;
	double rv;
	uintptr_t column;

	trackdef = trackerconfig_trackdef_const(self, psy_INDEX_INVALID);
	for (column = PATTERNEVENT_COLUMN_CMD, rv = 0.0;
			column < trackdef_num_columns(trackdef);
			++column) {
		rv += trackdef_column_width(trackdef, column,
			psy_ui_value_px(&self->flatsize, tm, NULL));
	}
	return rv;
}

TrackDef* trackconfig_insert_trackdef(TrackConfig* self, uintptr_t track,
	uintptr_t numnotes)
{
	TrackDef* rv;

	rv = trackdef_allocinit();
	if (rv) {
		trackdef_init(rv);
		rv->visinotes = numnotes;
		psy_table_insert(&self->trackconfigs, track, rv);
	}
	return rv;
}

void trackconfig_settrack(TrackConfig* self, uintptr_t track,
	uintptr_t numnotes)
{
	TrackDef* trackdef;

	numnotes = psy_max(1, numnotes);
	trackdef = trackerconfig_trackdef(self, track);
	if (trackdef != &self->trackdef) {
		trackdef->visinotes = numnotes;
	} else {
		trackconfig_insert_trackdef(self, track, numnotes);
	}
}

void trackconfig_setmulticolumn(TrackConfig* self, uintptr_t track, bool multi)
{
	TrackDef* trackdef;
	
	trackdef = trackerconfig_trackdef(self, track);
	if (multi && (trackdef == &self->trackdef)) {
		trackdef = trackconfig_insert_trackdef(self, track, 1);
	}
	trackdef->multicolumn = multi;
}

TrackDef* trackerconfig_trackdef(TrackConfig* self, uintptr_t track)
{
	TrackDef* rv;

	rv = (TrackDef*)psy_table_at(&self->trackconfigs, track);
	if (!rv) {
		return &self->trackdef;
	}
	return rv;
}

TrackDef* trackerconfig_trackdef_const(const TrackConfig* self, uintptr_t track)
{
	return trackerconfig_trackdef((TrackConfig*)self, track);
}

void trackconfig_resize(TrackConfig* self, uintptr_t track, double width,
	const psy_ui_TextMetric* tm)
{
	if (trackdrag_active(&self->resize)) {
		TrackDef* trackdef;
		double basewidth;
		uintptr_t numnotes;

		trackdef = trackerconfig_trackdef(self, track);
		basewidth = trackdef_defaulttrackwidth(trackdef,
			psy_ui_value_px(&self->flatsize, tm, NULL));
		width = psy_max(0.0, width);
		self->resize.size.width = width;
		self->resize.track = track;
		if (trackdef->multicolumn) {
			numnotes = (uintptr_t)psy_max(1.0,
				psy_max(0.0, (width - (basewidth -
					trackconfig_width_cmdparam(self, tm)))) /
				trackconfig_width_cmdparam(self, tm));
		} else {
			numnotes = (uintptr_t)(psy_max(1.0,
				(psy_max(0.0, self->resize.size.width)) /
				basewidth) + 0.9);
		}
		trackconfig_settrack(self, track, numnotes);
	}
}

void trackconfig_track_position(TrackConfig* self, uintptr_t track,
	double x, const psy_ui_TextMetric* tm,
	uintptr_t* rv_column, uintptr_t* rv_digit,
	uintptr_t* rv_noteindex)
{
	TrackDef* trackdef;
	double cpx;
	double flatsize;

	*rv_column = *rv_digit = 0;
	flatsize = psy_ui_value_px(&self->flatsize, tm, NULL);
	trackdef = trackerconfig_trackdef(self, track);	
	if (trackdef->visinotes == 1) {
		cpx = 0.0;
		*rv_noteindex = 0;
	} else {
		double singletrackwidth;
		
		singletrackwidth = trackdef_defaulttrackwidth(trackdef, flatsize);
		if (trackdef->multicolumn) {
			if (x < singletrackwidth) {
				cpx = 0.0;
				*rv_noteindex = 0;
			} else {
				double cmdparamwidth;
				
				*rv_column = 4;
				cmdparamwidth =
					trackdef_column_width(trackdef, *rv_column, flatsize) +
					trackdef_column_width(trackdef, *rv_column + 1, flatsize);				
				*rv_noteindex = (uintptr_t)((x - singletrackwidth) /
					cmdparamwidth);
				cpx = singletrackwidth + (*rv_noteindex) * cmdparamwidth;
				++(*rv_noteindex);
			}
		} else {		
			*rv_noteindex = (uintptr_t)(x / singletrackwidth);		
			cpx = singletrackwidth * (*rv_noteindex);
		}
	}	
	while (*rv_column < trackdef_num_columns(trackdef) &&
		cpx + trackdef_column_width(trackdef, *rv_column,
			flatsize) < x) {
		cpx += trackdef_column_width(trackdef, *rv_column, flatsize);
		++(*rv_column);
	}
	*rv_digit = (uintptr_t)((x - cpx) / flatsize);	
	if (*rv_digit >= trackdef_num_digits(trackdef, *rv_column)) {
		*rv_digit = trackdef_num_digits(trackdef, *rv_column) - 1;
	}
}


/* TrackColumnDef */
static void enter_digit(int digit, int newval, unsigned char* val);

/* implementation */
void trackcolumndef_init(TrackColumnDef* self, uintptr_t numdigits,
	uintptr_t numchars, double marginright, int wrapeditcolumn,
	int wrapclearcolumn, int emptyvalue, bool ignore_empty_at_twk)
{
	self->numdigits = numdigits;
	self->numchars = numchars;
	self->marginright = marginright;
	self->wrapeditcolumn = wrapeditcolumn;
	self->wrapclearcolumn = wrapclearcolumn;
	self->emptyvalue = emptyvalue;
	self->ignore_empty_at_twk = ignore_empty_at_twk;
}

/* TrackDef */
void trackdef_init(TrackDef* self)
{
	self->visinotes = 1;
	self->multicolumn = FALSE;
	trackcolumndef_init(&self->note, 1, 3, 1,
		PATTERNEVENT_COLUMN_NOTE, PATTERNEVENT_COLUMN_NOTE, 0xFF, FALSE);
	trackcolumndef_init(&self->inst, 2, 2, 1,
		PATTERNEVENT_COLUMN_INST, PATTERNEVENT_COLUMN_INST,
		psy_audio_NOTECOMMANDS_INST_EMPTY, TRUE);
	trackcolumndef_init(&self->mach, 2, 2, 1,
		PATTERNEVENT_COLUMN_MACH, PATTERNEVENT_COLUMN_MACH,
		psy_audio_NOTECOMMANDS_EMPTY, TRUE);
	trackcolumndef_init(&self->vol, 2, 2, 1,
		PATTERNEVENT_COLUMN_VOL, PATTERNEVENT_COLUMN_VOL,
		psy_audio_NOTECOMMANDS_VOL_EMPTY, FALSE);
	trackcolumndef_init(&self->cmd, 2, 2, 0,
		PATTERNEVENT_COLUMN_NONE, PATTERNEVENT_COLUMN_CMD, 0x00, TRUE);
	trackcolumndef_init(&self->param, 2, 2, 1,
		PATTERNEVENT_COLUMN_CMD, PATTERNEVENT_COLUMN_PARAM, 0x00, TRUE);
}

TrackDef* trackdef_alloc(void)
{
	return (TrackDef*)malloc(sizeof(TrackDef));
}

TrackDef* trackdef_allocinit(void)
{
	TrackDef* rv;

	rv = trackdef_alloc();
	if (rv) {
		trackdef_init(rv);
	}
	return rv;
}

void trackdef_deallocate(TrackDef* self)
{
	free(self);
}


/* TrackerEventTable */
void trackereventtable_init(TrackerEventTable* self)
{
	psy_table_init_keysize(&self->tracks, 67);
	self->selection_top_abs = self->selection_bottom_abs =
		psy_dsp_beatpos_zero();
}

void trackereventtable_dispose(TrackerEventTable* self)
{
	psy_TableIterator it;

	assert(self);

	for (it = psy_table_begin(&self->tracks);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_List** track;

		track = (psy_List**)psy_tableiterator_value(&it);
		psy_list_deallocate(track, NULL);
		free(psy_tableiterator_value(&it));
	}
	psy_table_dispose(&self->tracks);
}

void trackereventtable_clear_events(TrackerEventTable* self)
{
	psy_TableIterator it;

	assert(self);

	for (it = psy_table_begin(&self->tracks);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_List** track;

		track = (psy_List**)psy_tableiterator_value(&it);
		psy_list_deallocate(track, NULL);
		*track = NULL;
	}
}

psy_List** trackereventtable_track(TrackerEventTable* self, uintptr_t index)
{
	psy_List** rv;

	rv = (psy_List**)psy_table_at(&self->tracks, index);
	if (!rv) {
		rv = (psy_List**)malloc(sizeof(psy_List**));
		assert(rv);
		*rv = NULL;
		psy_table_insert(&self->tracks, index, (void*)rv);
	}
	return rv;
}

void trackereventtable_append(TrackerEventTable* self, uintptr_t track,
	psy_dsp_beatpos_t rel_offset, psy_dsp_beatpos_t seq_offset,
	psy_audio_PatternEntry* entry)
{
	psy_List** track_events;

	track_events = trackereventtable_track(self, track);	
	psy_list_append(track_events, trackereventpair_allocinit(
		rel_offset, seq_offset, entry));	
}

TrackerEventPair* trackereventpair_allocinit(psy_dsp_beatpos_t offset,
	psy_dsp_beatpos_t seqoffset, psy_audio_PatternEntry* entry)
{
	TrackerEventPair* rv;

	rv = (TrackerEventPair*)malloc(sizeof(TrackerEventPair));
	assert(rv);
	rv->offset = offset;
	rv->seqoffset = seqoffset;
	rv->entry = entry;
	return rv;
}

void trackereventtable_prepare_selection(TrackerEventTable* self,
	psy_audio_Sequence* sequence, const psy_audio_BlockSelection* selection)
{
	psy_audio_SequenceEntry* top_entry;
	psy_audio_SequenceEntry* bottom_entry;

	self->selection_top_abs = self->selection_bottom_abs =
		psy_dsp_beatpos_zero();
	if (!psy_audio_blockselection_valid(selection)) {
		return;
	}
	top_entry = psy_audio_sequence_entry(sequence,
		selection->topleft.order_index);
	if (!top_entry) {
		return;
	}
	bottom_entry = psy_audio_sequence_entry(sequence,
		selection->bottomright.order_index);
	if (!bottom_entry) {
		return;
	}
	self->selection_top_abs = psy_dsp_beatpos_add(
		psy_audio_sequenceentry_offset(top_entry),
		psy_audio_sequencecursor_offset(&selection->topleft));
	self->selection_bottom_abs = psy_dsp_beatpos_add(
		psy_audio_sequenceentry_offset(bottom_entry),
		psy_audio_sequencecursor_offset(&selection->bottomright));
}

/* TrackerState */


/* implementation */
void trackerstate_init(TrackerState* self, TrackConfig* track_config,
	PatternViewState* pvstate, Workspace* workspace)
{
	assert(self);
	assert(pvstate);

	self->track_config = track_config;
	self->pv = pvstate;
	self->workspace = workspace;
	beatconvert_init(&self->beat_convert, &self->pv->beat_line, 19);
	self->draw_beat_highlights = TRUE;
	self->draw_playbar = TRUE;
	self->show_empty_data = FALSE;
	self->midline = FALSE;
	self->scroll_cursor = FALSE;	
	trackereventtable_init(&self->track_events);
	psy_audio_patternentry_init(&self->empty);
	self->line_height = psy_ui_value_make_eh(1.0);
}

void trackerstate_dispose(TrackerState* self)
{
	trackereventtable_dispose(&self->track_events);
	psy_audio_patternentry_dispose(&self->empty);
}

double trackerstate_trackwidth(const TrackerState* self, uintptr_t track,
	const psy_ui_TextMetric* tm)
{
	assert(self);
	assert(self->track_config);

	if (self->track_config->resize.active &&
			self->track_config->resize.track == track) {
		return self->track_config->resize.size.width;
	}
	return trackconfig_width(self->track_config, track, tm);
}

uintptr_t trackerstate_px_to_track(const TrackerState* self, double x,
	const psy_ui_TextMetric* tm)
{
	double currx = 0;
	uintptr_t rv = 0;

	while (rv < patternviewstate_num_song_tracks(self->pv)) {
		currx += trackerstate_trackwidth(self, rv, tm);
		if (currx > x) {
			break;
		}
		++rv;
	}
	return rv;
}

double trackerstate_base_width(TrackerState* self, uintptr_t track,
	const psy_ui_TextMetric* tm)
{
	return trackdef_basewidth(trackerconfig_trackdef(self->track_config,
		track), psy_ui_value_px(&self->track_config->flatsize, tm, NULL)) + 1;
}

uintptr_t trackdef_num_digits(TrackDef* self, uintptr_t column)
{
	TrackColumnDef* coldef;

	coldef = trackdef_column_def(self, column);
	return ((coldef)
		? coldef->numdigits
		: 0);
}

uintptr_t trackdef_num_columns(const TrackDef* self)
{
	return PATTERNEVENT_COLUMN_END;
}

psy_audio_PatternEvent trackdef_setevent_digit(TrackDef* self,
	uintptr_t column, uintptr_t digit, const psy_audio_PatternEvent* ev,
	uintptr_t digitvalue)
{
	uintptr_t value;
	uintptr_t num;
	ptrdiff_t pos;
	uint8_t* data;
	psy_audio_PatternEvent rv;
	TrackColumnDef* columndef;	

	assert(ev);

	rv = *ev;
	columndef = trackdef_column_def(self, column);
	if (!columndef) {
		return *ev;
	}
	num = trackdef_num_digits(self, column);
	value = psy_audio_patternevent_value(ev, column);
	if (value == trackdef_emptyvalue(self, column)) {		
		value = 0;		
	}
	pos = num / 2 - digit / 2 - 1;
	data = (uint8_t*)&value + pos;
	enter_digit(digit % 2, (uint8_t)digitvalue, data);
	if ((column == 1) && (num == 2) && ((value & 0xFF) == 0xFF)) {
		value = trackdef_emptyvalue(self, column);
	}
	psy_audio_patternevent_setvalue(&rv, column, value);
	return rv;
}

void enter_digit(int digit, int newval, unsigned char* val)
{
	if (digit == 0) {
		*val = (*val & 0x0F) | ((newval & 0x0F) << 4);
	} else if (digit == 1) {
		*val = (*val & 0xF0) | (newval & 0x0F);
	}
}

uintptr_t trackdef_event_value(TrackDef* self, uintptr_t column,
	const psy_audio_PatternEvent* ev)
{
	if (column < PATTERNEVENT_COLUMN_END) {
		switch (column) {
		case PATTERNEVENT_COLUMN_NOTE:
			return ev->note;
		case PATTERNEVENT_COLUMN_INST:
			return ev->inst;
		case PATTERNEVENT_COLUMN_MACH:
			return ev->mach;
		case PATTERNEVENT_COLUMN_VOL:
			return ev->vol;
		case PATTERNEVENT_COLUMN_CMD:
			return ev->cmd;
		case PATTERNEVENT_COLUMN_PARAM:
			return ev->parameter;
		default:
			break;
		}
	}
	return 0;
}

uintptr_t trackdef_value(TrackDef* self, uintptr_t column,
	const psy_audio_PatternEntry* entry)
{
	uintptr_t rv;

	if (column < PATTERNEVENT_COLUMN_CMD) {
		rv = trackdef_event_value(self, column,
			psy_audio_patternentry_front_const(entry));
	} else {
		uintptr_t c;
		uintptr_t num;
		const psy_audio_PatternEventNode* p;

		column = column - 4;
		num = column / 2;
		c = 0;
		p = psy_audio_patternentry_begin_const(entry);
		while (p && c < num) {
			p = p->next;
			++c;
		}
		if (p) {
			const psy_audio_PatternEvent* ev;

			ev = (psy_audio_PatternEvent*)p->entry;
			assert(ev);
			if ((column % 2) == 0) {
				rv = ev->cmd;
			} else {
				rv = ev->parameter;
			}
		} else {
			rv = 0;
		}
	}
	return rv;
}

uintptr_t trackdef_emptyvalue(TrackDef* self, uintptr_t column)
{
	TrackColumnDef* coldef;

	coldef = trackdef_column_def(self, column);
	return coldef ? coldef->emptyvalue : 0;
}

double trackdef_basewidth(const TrackDef* self, double textwidth)
{
	double rv = 0;
	uintptr_t column;

	for (column = 0; column < 4; ++column) {
		rv += trackdef_column_width(self, column, textwidth);
	}
	return rv;
}

double trackdef_defaulttrackwidth(const TrackDef* self, double textwidth)
{
	double rv = 0;
	uintptr_t column;

	for (column = 0; column < PATTERNEVENT_COLUMN_END; ++column) {
		rv += trackdef_column_width(self, column, textwidth);
	}
	return rv;
}

double trackdef_column_width(const TrackDef* self, intptr_t column, double textwidth)
{
	const TrackColumnDef* coldef;

	coldef = trackdef_column_def((TrackDef*)self, column);
	return ((coldef)
		? coldef->numchars * textwidth + coldef->marginright
		: 0.0);
}

double trackdef_marginright(TrackDef* self, intptr_t column)
{
	TrackColumnDef* coldef;

	coldef = trackdef_column_def(self, column);
	return ((coldef) ? coldef->marginright : 0.0);
}

TrackColumnDef* trackdef_column_def(TrackDef* self, intptr_t column)
{
	switch (column) {
	case PATTERNEVENT_COLUMN_NOTE:
		return &self->note;
	case PATTERNEVENT_COLUMN_INST:
		return &self->inst;
	case PATTERNEVENT_COLUMN_MACH:
		return &self->mach;
	case PATTERNEVENT_COLUMN_VOL:
		return &self->vol;
	case PATTERNEVENT_COLUMN_CMD:
		return &self->cmd;
	case PATTERNEVENT_COLUMN_PARAM:
		return &self->param;
	default:
		return NULL;
	}
}

void trackerstate_track_clip(TrackerState* self,
	const psy_ui_RealRectangle* clip, const psy_ui_TextMetric* tm,
	uintptr_t* rv_left, uintptr_t* rv_right)
{
	assert(self);

	*rv_left = trackerstate_px_to_track(self, clip->left, tm);
	*rv_right = trackerstate_px_to_track(self, clip->right, tm) + 1;
	if (*rv_right > patternviewstate_num_song_tracks(self->pv)) {
		*rv_right = patternviewstate_num_song_tracks(self->pv);
	}
}

psy_audio_SequenceCursor trackerstate_checkcursorbounds(TrackerState* self,
	psy_audio_SequenceCursor cursor)
{
	psy_audio_SequenceCursor rv;

	rv = cursor;
	if (psy_dsp_beatpos_less(rv.offset, psy_dsp_beatpos_zero())) {
		rv.offset = psy_dsp_beatpos_zero();
	} else {
		if (patternviewstate_pattern(self->pv)) {
			if (psy_dsp_beatpos_greater_equal(rv.offset,
					patternviewstate_length(self->pv))) {
				rv.offset = patternviewstate_length(self->pv);
			}
		} else {
			rv.offset = psy_dsp_beatpos_zero();;
		}
	}
	if (rv.channel_ < 0) {
		rv.channel_ = 0;
	} else if (rv.channel_ >= patternviewstate_num_song_tracks(self->pv)) {
		rv.channel_ = patternviewstate_num_song_tracks(self->pv);
	}
	return rv;
}

psy_audio_SequenceCursor trackerstate_make_cursor(TrackerState* self,
	psy_ui_RealPoint pt, uintptr_t index, const psy_ui_TextMetric* tm)
{
	psy_audio_SequenceCursor rv;
	psy_audio_Sequence* sequence;
	psy_dsp_beatpos_t offset;
	psy_audio_OrderIndex order_index;

	rv = self->pv->cursor;
	sequence = patternviewstate_sequence(self->pv);
	if (!sequence) {
		return rv;
	}
	offset = trackerstate_px_to_beat(self, pt.y);
	if (!patternviewstate_single_mode(self->pv)) {
		psy_audio_SequenceEntry* seq_entry;

		order_index = psy_audio_orderindex_make(rv.order_index.track,
			psy_audio_sequence_order(sequence, rv.order_index.track, offset));
		seq_entry = psy_audio_sequence_entry(sequence, order_index);
		if (seq_entry) {
			offset = psy_dsp_beatpos_sub(offset, psy_audio_sequenceentry_offset(
				seq_entry));
		} else {
			return rv;
		}
	} else {
		order_index = rv.order_index;
	}
	psy_audio_sequencecursor_set_offset(&rv, offset);
	psy_audio_sequencecursor_set_order_index(&rv, order_index);
	rv.channel_ = index;
	trackconfig_track_position(self->track_config, rv.channel_, pt.x, tm,
		&rv.column, &rv.digit, &rv.noteindex);	
	rv.key = sequence->cursor.key;
	return rv;
}

psy_ui_Style* trackerstate_column_style(TrackerState* self,
	TrackerColumnFlags flags, uintptr_t track)
{
	psy_ui_Style* rv;

	// numtracks = patternviewstate_num_song_tracks(self->pv);
	rv = NULL;
	if (flags.cursor != FALSE) {
		rv = psy_ui_style(STYLE_PV_CURSOR);
	} else if (flags.playbar) {
		rv = psy_ui_style(STYLE_PV_PLAYBAR);
	} else if (flags.selection) {
		if (flags.beat4) {
			rv = psy_ui_style(STYLE_PV_ROW4BEAT_SELECT);
		} else if (flags.beat) {
			rv = psy_ui_style(STYLE_PV_ROWBEAT_SELECT);
		} else {
			rv = psy_ui_style(STYLE_PV_ROW_SELECT);
		}
	} else if (flags.mid) {
		if (flags.cursor != FALSE) {
			rv = psy_ui_style(STYLE_PV_CURSOR);
		} else {
			rv = psy_ui_style(STYLE_PV_PLAYBAR);
		}
	} else {
		if (flags.beat4) {
			rv = psy_ui_style(STYLE_PV_ROW4BEAT);
		} else if (flags.beat) {
			rv = psy_ui_style(STYLE_PV_ROWBEAT);
		} else {
			rv = psy_ui_style(STYLE_PV_ROW);
		}
	}
	if (rv) {
		return rv;
	}
	return psy_ui_style(STYLE_PATTERNVIEW);
}

/* TrackerClipCmd */

typedef struct TrackerClipCmd {	
	psy_Command command;
	TrackerEventTable* track_events;	
} TrackerClipCmd;

/* prototypes */
static void trackerclipcmd_execute(TrackerClipCmd*,
	SequenceTraverseParams*);

/* vtable */
static psy_CommandVtable trackerclipcmd_vtable;
static bool trackerclipcmd_vtable_initialized = FALSE;

static void trackerclipcmd_vtable_init(
	TrackerClipCmd* self)
{
	if (!trackerclipcmd_vtable_initialized) {
		trackerclipcmd_vtable = *(self->command.vtable);	
		trackerclipcmd_vtable.execute =
			(psy_fp_command_param)
			trackerclipcmd_execute;		
		trackerclipcmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &trackerclipcmd_vtable;
}

static void trackerclipcmd_init(TrackerClipCmd* self,
	TrackerEventTable* track_events)
{
	assert(self);
	
	psy_command_init(&self->command);
	trackerclipcmd_vtable_init(self);
	self->track_events = track_events;	
}

void trackerclipcmd_execute(TrackerClipCmd* self,
	SequenceTraverseParams* params)
{		
	psy_audio_PatternEntry* entry;
	
	if (params->node) {	
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
	} else {
		entry = NULL;
	}		
	/* printf("%d %f\n %f\n", (int)params->channel, 
		psy_dsp_beatpos_sub(params->offset, params->seqoffset),
		psy_dsp_beatpos_real(params->offset)); */
	trackereventtable_append(self->track_events, params->channel,
		psy_dsp_beatpos_sub(params->offset, params->seqoffset),
		params->seqoffset, entry);	
}


/*! @todo speed slows down in singlemode at increasing order rows  */
#ifdef USE_CLIP_CMD
void trackerstate_update_clip_events(TrackerState* self,
	psy_ui_RealRectangle* g_clip, const psy_ui_TextMetric* tm)
{
	BeatClip clip;
	psy_dsp_beatpos_t offset;	
	psy_audio_BlockSelection block;
	psy_audio_OrderIndex first;
	TrackerClipCmd cmd;

	assert(self);

	trackereventtable_clear_events(&self->track_events);
	if (!patternviewstate_sequence(self->pv)) {
		return;
	}	
	beatclip_init(&clip, &self->beat_convert, g_clip->top,
		g_clip->bottom);
	self->track_events.top = offset = clip.begin;
	trackerstate_track_clip(self, g_clip, tm, &self->track_events.left,
		&self->track_events.right);	
	if (patternviewstate_single_mode(self->pv)) {
		psy_audio_SequenceEntry* entry;
		psy_dsp_beatpos_t seq_offset;
		
		entry = psy_audio_sequence_entry(self->pv->sequence,
			self->pv->cursor.order_index);
		if (!entry) {
			return;
		}
		seq_offset = psy_audio_sequenceentry_offset(entry);
		offset = psy_dsp_beatpos_add(offset, seq_offset);
		clip.end = psy_dsp_beatpos_add(clip.end, seq_offset);		
		first = self->pv->cursor.order_index;
	} else {
		first = psy_audio_orderindex_make(
			self->pv->cursor.order_index.track, 0);
	}
	clip.end = psy_dsp_beatpos_add(clip.end, psy_audio_sequencecursor_bpl(
			&self->pv->cursor));
	trackerclipcmd_init(&cmd, &self->track_events);
	psy_audio_sequence_block_grid_traverse(
		patternviewstate_sequence(self->pv),
		psy_audio_blockselection_make(
			psy_audio_sequencecursor_make(first,
				self->track_events.left, clip.begin),
			psy_audio_sequencecursor_make(first,
				self->track_events.right, clip.end)),
		&cmd.command);
	psy_command_dispose(&cmd.command);	
}
#endif

/*! todo relpace with USE_CLIP_CMD */
void trackerstate_update_clip_events(TrackerState* self,
	psy_ui_RealRectangle* g_clip, const psy_ui_TextMetric* tm)
{
	uintptr_t track;
	BeatClip clip;
	psy_dsp_beatpos_t offset;
	psy_dsp_beatpos_t seqoffset;
	psy_dsp_beatpos_t length;
	psy_dsp_beatpos_t bpl;
	psy_audio_SequenceTrackIterator ite;	

	assert(self);
	
	if (!patternviewstate_pattern(self->pv)) {
		return;
	}
	trackereventtable_clear_events(&self->track_events);
	beatclip_init(&clip, &self->beat_convert, g_clip->top, g_clip->bottom);
	offset = clip.begin;
	trackerstate_track_clip(self, g_clip, tm, &self->track_events.left,
		&self->track_events.right);
	psy_audio_sequencetrackiterator_init(&ite);
	if (patternviewstate_single_mode(self->pv)) {
		psy_audio_sequence_begin(self->pv->sequence,
			self->pv->cursor.order_index.track,
			psy_dsp_beatpos_add(offset,
				psy_audio_sequencecursor_seqoffset(&self->pv->cursor,
					self->pv->sequence)),
			&ite);
	} else {
		psy_audio_sequence_begin(self->pv->sequence,
			self->pv->cursor.order_index.track, offset,
			&ite);
	}
	length = psy_audio_sequencetrackiterator_entry_length(&ite);
	seqoffset = psy_audio_sequencetrackiterator_seq_offset(&ite);	
	self->track_events.top = offset;
	if (patternviewstate_single_mode(self->pv)) {
		offset = psy_dsp_beatpos_add(offset, seqoffset);
		clip.end = psy_dsp_beatpos_add(clip.end, seqoffset);
	}
	bpl = psy_audio_sequencecursor_bpl(&self->pv->cursor);
	while (psy_dsp_beatpos_less_equal(offset, clip.end)) {
		bool fill;
		psy_dsp_beatpos_t line_end;
		psy_dsp_beatpos_t rel_offset;
		psy_dsp_beatpos_t seq_entry_end;

		rel_offset = psy_dsp_beatpos_sub(offset, seqoffset);
		seq_entry_end = psy_dsp_beatpos_add(seqoffset, length);
		line_end = psy_dsp_beatpos_add(offset, bpl);
		fill = !(psy_dsp_beatpos_greater_equal(offset, seqoffset) &&
			psy_dsp_beatpos_less(offset, seq_entry_end)) ||
			!ite.patternnode;
		/* fill line */
		for (track = self->track_events.left; track < self->track_events.right;
				++track) {
			bool has_event;

			has_event = FALSE;
			while (!fill && ite.patternnode &&
				psy_audio_patternentry_track(psy_audio_sequencetrackiterator_pattern_entry(&ite)) <= track &&
				psy_dsp_beatpos_testrange(
					psy_audio_sequencetrackiterator_offset(&ite), offset, bpl))
			{
				psy_audio_PatternEntry* entry;

				entry = psy_audio_sequencetrackiterator_pattern_entry(&ite);
				if (psy_audio_patternentry_track(entry) == track) {
					trackereventtable_append(&self->track_events, track,
						rel_offset, seqoffset, entry);					
					psy_list_next(&ite.patternnode);
					has_event = TRUE;
					break;
				}
				psy_list_next(&ite.patternnode);
			}
			if (!has_event) {
				if (psy_dsp_beatpos_less(offset, seq_entry_end)) {
					trackereventtable_append(&self->track_events, track,
						rel_offset, seqoffset, NULL);
				}				
			} else if (ite.patternnode && ((psy_audio_PatternEntry*)(
					ite.patternnode->entry))->track_ <= track) {
				fill = TRUE;
			}
		}
		/* skip remaining events of the line */		
		while (ite.patternnode && (psy_dsp_beatpos_less(
				psy_audio_sequencetrackiterator_offset(&ite), line_end))) {
			psy_list_next(&ite.patternnode);
		}
		/* advance offset */
		offset = psy_dsp_beatpos_add(offset, bpl);		
		if (psy_dsp_beatpos_greater_equal(offset, seq_entry_end)) {
			psy_audio_SequenceEntry* seq_entry;
						
			psy_audio_sequencetrackiterator_inc_entry(&ite);
			seq_entry = psy_audio_sequencetrackiterator_entry(&ite);
			if (seq_entry) {				
				offset = seqoffset = psy_audio_sequenceentry_offset(seq_entry);
				length = psy_audio_sequenceentry_length(seq_entry);
			} else {
				break;
			}
		}
	}
	psy_audio_sequencetrackiterator_dispose(&ite);
}

void trackerstate_update_textmetric(TrackerState* self,
	const psy_ui_Font* font)
{
	assert(self);
	assert(font);

	beatconvert_set_line_px(&self->beat_convert, psy_ui_value_px(
		&self->line_height, psy_ui_font_textmetric(font), NULL));
}

double trackerstate_blit_start(TrackerState* self, bool top)
{
	psy_audio_Sequence* sequence;
	psy_audio_SequenceCursor last_cursor;
	psy_audio_SequenceCursor curr_cursor;
	psy_dsp_beatpos_t last_offset;
	psy_dsp_beatpos_t curr_offset;
	double last;
	double curr;

	assert(self);

	sequence = self->pv->sequence;
	if (!sequence) {
		return 0.0;
	}
	if (self->scroll_cursor) {
		last_cursor = sequence->lastcursor;
		curr_cursor = sequence->cursor;
	} else {
		last_cursor = self->pv->player->sequencer.hostseqtime.lastplaycursor;
		curr_cursor = self->pv->player->sequencer.hostseqtime.currplaycursor;
	}
	if (patternviewstate_single_mode(self->pv)) {
		last_offset = psy_audio_sequencecursor_offset(&last_cursor);
		curr_offset = psy_audio_sequencecursor_offset(&curr_cursor);
	} else {
		last_offset = psy_audio_sequencecursor_offset_abs(&last_cursor,
			sequence);
		curr_offset = psy_audio_sequencecursor_offset_abs(&curr_cursor,
			sequence);
	}
	last = trackerstate_beat_to_px(self, last_offset);
	curr = trackerstate_beat_to_px(self, curr_offset);
	if (top) {
		return psy_min(last, curr);
	}
	return psy_max(last, curr);
}

void trackerstate_scroll_to(TrackerState* self, psy_ui_Component* component,
	double dx, double dy)
{
	assert(self);

	if (!psy_ui_component_draw_visible(component)) {
		return;
	}
	if (self->midline) {
		psy_ui_component_invalidate(component);
	} else if (self->pv->selection.selecting) {
		/*psy_ui_RealRectangle r;

		r = psy_ui_realrectangle_make(
			psy_ui_realpoint_make(
				psy_ui_component_scroll_left_px(component),
				psy_ui_component_scroll_top_px(component)),
		psy_ui_component_scroll_size_px(psy_ui_component_parent(component)));
		psy_ui_component_scroll_to(component, dx, dy, &r);
		psy_ui_component_invalidate_scroll_diff(component);*/
		psy_ui_component_invalidate(component);
		psy_ui_component_invalidate_scroll_diff(component);
	} else {
		psy_ui_RealSize size;
		double top;
		double blit_start;
		bool limit_vertical_blit;
		psy_ui_RealRectangle r;

		size = psy_ui_component_scroll_size_px(psy_ui_component_parent(
			component));
		top = blit_start = 0.0;
		limit_vertical_blit = (dy != 0.0) && ((self->scroll_cursor) ||
			(trackerstate_playing_following_song(self)));
		if (limit_vertical_blit) {
			if (dy < 0) {
				blit_start = trackerstate_blit_start(self, TRUE);
				size.height = blit_start - psy_ui_component_scroll_top_px(
					component);
			} else {
				top = trackerstate_blit_start(self, FALSE) -
					psy_ui_component_scroll_top_px(component);
				size.height -= top;
			}
		}
		r = psy_ui_realrectangle_make(
			psy_ui_realpoint_make(
				psy_ui_component_scroll_left_px(component),
				psy_ui_component_scroll_top_px(component) + top),
			size);
		psy_ui_component_scroll_to(component, psy_ui_realpoint_make(dx, dy), &r);
		if (limit_vertical_blit) {
		if (dy < 0) {
				psy_ui_RealSize invalidate_size;

				invalidate_size = psy_ui_component_scroll_size_px(
					psy_ui_component_parent(component));
				invalidate_size.height -= size.height;
				r = psy_ui_realrectangle_make(
					psy_ui_realpoint_make(
						psy_ui_component_scroll_left_px(component),
						blit_start),
					invalidate_size);
			} else {
				size.height = top;
				r = psy_ui_realrectangle_make(
						psy_ui_component_scroll_px(component),
						size);
			}
			psy_ui_component_invalidate_rect(component, r);
			psy_ui_component_invalidate_scroll_diff(component);
		}
	}
}

void trackerstate_playbar_update_range(TrackerState* self,
	psy_audio_HostSequencerTime* seqtime,
	double* rv_minval, double* rv_maxval)
{
	psy_dsp_beatpos_t last;
	double last_px;
	psy_dsp_beatpos_t curr;
	double curr_px;		

	assert(self);
	assert(seqtime);
	assert(rv_minval);
	assert(rv_maxval);

	if (!self->pv->sequence) {
		*rv_minval = *rv_maxval = 0.0;
		return;
	}
	last = seqtime->lastplaycursor.offset;
	if (!patternviewstate_single_mode(self->pv)) {
		last = psy_dsp_beatpos_add(last,
			psy_audio_sequencecursor_seqoffset(
				&seqtime->lastplaycursor,
				self->pv->sequence));
	}
	last_px = trackerstate_beat_to_px(self, last);
	curr = seqtime->currplaycursor.offset;
	if (!patternviewstate_single_mode(self->pv)) {
		curr = psy_dsp_beatpos_add(curr,
			psy_audio_sequencecursor_seqoffset(
				&seqtime->currplaycursor,
				self->pv->sequence));
	}
	curr_px = trackerstate_beat_to_px(self, curr);
	*rv_minval = psy_min(last_px, curr_px);
	*rv_maxval = psy_max(last_px, curr_px);	
}

#endif /* PSYCLE_USE_TRACKERVIEW */
