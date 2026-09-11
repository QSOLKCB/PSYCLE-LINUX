/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequencecmds.h"
/* local */
#include "exclusivelock.h"
#include "exclusivelock.h"
/* dsp */
#include <interpolate.h>
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* psy_audio_SequenceInsertCommand */

/* prototypes */
static void psy_audio_sequenceinsertcommand_dispose(
	psy_audio_SequenceInsertCommand*);
static void psy_audio_sequenceinsertcommand_execute(
	psy_audio_SequenceInsertCommand*, uintptr_t param);
static void psy_audio_sequenceinsertcommand_revert(
	psy_audio_SequenceInsertCommand*);

/* vtable */
static psy_CommandVtable psy_audio_sequenceinsertcommand_vtable;
static bool psy_audio_sequenceinsertcommand_vtable_initialized = FALSE;

static void psy_audio_sequenceinsertcommand_vtable_init(
	psy_audio_SequenceInsertCommand* self)
{
	if (!psy_audio_sequenceinsertcommand_vtable_initialized) {
		psy_audio_sequenceinsertcommand_vtable = *(self->command.vtable);
		psy_audio_sequenceinsertcommand_vtable.dispose =
			(psy_fp_command)
			psy_audio_sequenceinsertcommand_dispose;
		psy_audio_sequenceinsertcommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequenceinsertcommand_execute;
		psy_audio_sequenceinsertcommand_vtable.revert =
			(psy_fp_command)
			psy_audio_sequenceinsertcommand_revert;
		psy_audio_sequenceinsertcommand_vtable_initialized = TRUE;
	}
}

/* implementation */
psy_audio_SequenceInsertCommand* psy_audio_sequenceinsertcommand_alloc(	
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex index, uintptr_t patidx)
{
	psy_audio_SequenceInsertCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceInsertCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequenceinsertcommand_vtable_init(rv);
		rv->command.vtable = &psy_audio_sequenceinsertcommand_vtable;
		rv->sequence = sequence;
		rv->selection = selection;
		psy_audio_sequenceselection_init(&rv->restoreselection);
		rv->index = index;
		rv->patidx = patidx;				
	}
	return rv;
}

void psy_audio_sequenceinsertcommand_dispose(psy_audio_SequenceInsertCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);
}

void psy_audio_sequenceinsertcommand_execute(psy_audio_SequenceInsertCommand* self,
	uintptr_t param)
{	
	psy_audio_SequenceCursor cursor;	

	assert(self);
	assert(self->sequence);
	
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_insert(self->sequence, self->index, self->patidx);	
	self->index = psy_audio_orderindex_make(self->index.track,
		self->index.order + 1);	
	cursor = psy_audio_sequence_cursor(self->sequence);
	psy_audio_sequencecursor_set_order_index(&cursor, self->index);	
	psy_audio_sequence_set_cursor(self->sequence, cursor);	
}

void psy_audio_sequenceinsertcommand_revert(psy_audio_SequenceInsertCommand* self)
{	
	psy_audio_sequence_remove(self->sequence, self->index);
	self->index = psy_audio_orderindex_make(
		self->index.track, self->index.order - 1);	
	psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
	psy_audio_sequenceselection_select_first(self->selection, self->index);	
}

/*
** psy_audio_SequenceSampleInsertCommand
*/
/* prototypes */
static void psy_audio_sequencesampleinsertcommand_dispose(psy_audio_SequenceSampleInsertCommand*);
static void psy_audio_sequencesampleinsertcommand_execute(psy_audio_SequenceSampleInsertCommand*,
	uintptr_t param);
static void psy_audio_sequencesampleinsertcommand_revert(psy_audio_SequenceSampleInsertCommand*);
/* vtable */
static psy_CommandVtable psy_audio_sequencesampleinsertcommand_vtable;
static bool psy_audio_sequencesampleinsertcommand_vtable_initialized = FALSE;

static void psy_audio_sequencesampleinsertcommand_vtable_init(psy_audio_SequenceSampleInsertCommand* self)
{
	if (!psy_audio_sequencesampleinsertcommand_vtable_initialized) {
		psy_audio_sequencesampleinsertcommand_vtable = *(self->command.vtable);
		psy_audio_sequencesampleinsertcommand_vtable.dispose =
			(psy_fp_command)
			psy_audio_sequencesampleinsertcommand_dispose;
		psy_audio_sequencesampleinsertcommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequencesampleinsertcommand_execute;
		psy_audio_sequencesampleinsertcommand_vtable.revert =
			(psy_fp_command)
			psy_audio_sequencesampleinsertcommand_revert;
		psy_audio_sequencesampleinsertcommand_vtable_initialized = TRUE;
	}
}
// implementation
psy_audio_SequenceSampleInsertCommand* psy_audio_sequencesampleinsertcommand_alloc(
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex index, psy_audio_SampleIndex sampleindex)
{
	psy_audio_SequenceSampleInsertCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceSampleInsertCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequencesampleinsertcommand_vtable_init(rv);
		rv->command.vtable = &psy_audio_sequencesampleinsertcommand_vtable;
		rv->sequence = sequence;
		rv->selection = selection;
		psy_audio_sequenceselection_init(&rv->restoreselection);
		rv->index = index;
		rv->sampleindex = sampleindex;
	}
	return rv;
}

void psy_audio_sequencesampleinsertcommand_dispose(psy_audio_SequenceSampleInsertCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);	
}

void psy_audio_sequencesampleinsertcommand_execute(psy_audio_SequenceSampleInsertCommand* self,
	uintptr_t param)
{
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_insert_sample(self->sequence, self->index, self->sampleindex);
	self->index = psy_audio_orderindex_make(
		self->index.track, self->index.order + 1);
	psy_audio_sequenceselection_select_first(self->selection, self->index);
	// psy_audio_sequenceselection_seteditposition(self->selection, self->index);
}

void psy_audio_sequencesampleinsertcommand_revert(psy_audio_SequenceSampleInsertCommand* self)
{
	psy_audio_sequence_remove(self->sequence, self->index);
	self->index = psy_audio_orderindex_make(
		self->index.track, self->index.order - 1);
	psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
	psy_audio_sequenceselection_select_first(self->selection, self->index);
	// psy_audio_sequenceselection_seteditposition(self->selection, self->index);
}

/*
** psy_audio_SequenceMarkerInsertCommand
*/
/* prototypes */
static void psy_audio_sequencemarkerinsertcommand_dispose(psy_audio_SequenceMarkerInsertCommand*);
static void psy_audio_sequencemarkerinsertcommand_execute(psy_audio_SequenceMarkerInsertCommand*,
	uintptr_t param);
static void psy_audio_sequencemarkerinsertcommand_revert(psy_audio_SequenceMarkerInsertCommand*);
/* vtable */
static psy_CommandVtable psy_audio_sequencemarkerinsertcommand_vtable;
static bool psy_audio_sequencemarkerinsertcommand_vtable_initialized = FALSE;

static void psy_audio_sequencemarkerinsertcommand_vtable_init(psy_audio_SequenceMarkerInsertCommand* self)
{
	if (!psy_audio_sequencemarkerinsertcommand_vtable_initialized) {
		psy_audio_sequencemarkerinsertcommand_vtable = *(self->command.vtable);
		psy_audio_sequencemarkerinsertcommand_vtable.dispose =
			(psy_fp_command)
			psy_audio_sequencemarkerinsertcommand_dispose;
		psy_audio_sequencemarkerinsertcommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequencemarkerinsertcommand_execute;
		psy_audio_sequencemarkerinsertcommand_vtable.revert =
			(psy_fp_command)
			psy_audio_sequencemarkerinsertcommand_revert;
		psy_audio_sequencemarkerinsertcommand_vtable_initialized = TRUE;
	}
}

/* implementation */
psy_audio_SequenceMarkerInsertCommand* psy_audio_sequencemarkerinsertcommand_alloc(
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection,
	psy_audio_OrderIndex index, const char* text)
{
	psy_audio_SequenceMarkerInsertCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceMarkerInsertCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequencemarkerinsertcommand_vtable_init(rv);
		rv->command.vtable = &psy_audio_sequencemarkerinsertcommand_vtable;
		rv->sequence = sequence;
		rv->selection = selection;
		psy_audio_sequenceselection_init(&rv->restoreselection);
		rv->index = index;
		rv->text = psy_strdup(text);
	}
	return rv;
}

void psy_audio_sequencemarkerinsertcommand_dispose(
	psy_audio_SequenceMarkerInsertCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);
	free(self->text);
	self->text = NULL;
}

void psy_audio_sequencemarkerinsertcommand_execute(
	psy_audio_SequenceMarkerInsertCommand* self, uintptr_t param)
{
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_insert_marker(self->sequence, self->index, self->text);
	self->index = psy_audio_orderindex_make(
		self->index.track, self->index.order + 1);
	psy_audio_sequenceselection_select_first(self->selection, self->index);
}

void psy_audio_sequencemarkerinsertcommand_revert(
	psy_audio_SequenceMarkerInsertCommand* self)
{
	psy_audio_sequence_remove(self->sequence, self->index);
	self->index = psy_audio_orderindex_make(
		self->index.track, self->index.order - 1);
	psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
	psy_audio_sequenceselection_select_first(self->selection, self->index);
}


/* psy_audio_SequenceRemoveCommand */

/* prototypes */
static void psy_audio_sequenceremovecommand_dispose(
	psy_audio_SequenceRemoveCommand*);
static void psy_audio_sequenceremovecommand_execute(
	psy_audio_SequenceRemoveCommand*, uintptr_t param);
static void psy_audio_sequenceremovecommand_revert(
	psy_audio_SequenceRemoveCommand*);

/* vtable */
static psy_CommandVtable psy_audio_sequenceremovecommand_vtable;
static bool psy_audio_sequenceremovecommand_vtable_initialized = FALSE;

static void psy_audio_sequenceremovecommand_vtable_init(
	psy_audio_SequenceRemoveCommand* self)
{
	if (!psy_audio_sequenceremovecommand_vtable_initialized) {
		psy_audio_sequenceremovecommand_vtable = *(self->command.vtable);
		psy_audio_sequenceremovecommand_vtable.dispose =
			(psy_fp_command)
			psy_audio_sequenceremovecommand_dispose;
		psy_audio_sequenceremovecommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequenceremovecommand_execute;
		psy_audio_sequenceremovecommand_vtable.revert =
			(psy_fp_command)
			psy_audio_sequenceremovecommand_revert;
		psy_audio_sequenceremovecommand_vtable_initialized = TRUE;
	}
}

/* implementation */
psy_audio_SequenceRemoveCommand* psy_audio_sequenceremovecommand_alloc(
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection)
{
	psy_audio_SequenceRemoveCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceRemoveCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequenceremovecommand_vtable_init(rv);
		rv->command.vtable = &psy_audio_sequenceremovecommand_vtable;
		rv->sequence = sequence;
		rv->selection = selection;
		psy_audio_sequenceselection_init(&rv->restoreselection);		
		psy_audio_sequence_init(&rv->restoresequence, sequence->patterns, sequence->samples);		
	}
	return rv;
}

void psy_audio_sequenceremovecommand_dispose(psy_audio_SequenceRemoveCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);
	psy_audio_sequence_dispose(&self->restoresequence);
}

void psy_audio_sequenceremovecommand_execute(psy_audio_SequenceRemoveCommand* self,
	uintptr_t param)
{
	psy_audio_OrderIndex editposition;	
	psy_audio_SequenceCursor cursor;
	psy_dsp_beatpos_t pattern_offset;

	assert(self);
	assert(self->sequence);

	pattern_offset = psy_audio_sequencecursor_offset(&self->sequence->cursor);
	cursor = psy_audio_sequence_cursor(self->sequence);
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_copy(&self->restoresequence, self->sequence);	
	psy_audio_exclusivelock_enter();
	editposition = psy_audio_sequenceselection_first(self->selection);	
	psy_audio_sequence_remove_selection(self->sequence, self->selection);	
	if (psy_audio_sequence_track_size(self->sequence, 0) == 0) {
		psy_audio_sequence_insert(self->sequence,
			psy_audio_orderindex_make(0, 0), 0);
	}
	psy_audio_exclusivelock_leave();
	psy_audio_sequenceselection_clear(self->selection);
	if (editposition.order >= psy_audio_sequence_track_size(self->sequence,
			editposition.track)) {
		editposition.order = psy_audio_sequence_track_size(self->sequence,
			editposition.track);
		if (editposition.order > 0) {
			--editposition.order;
		}
	}
	psy_audio_sequenceselection_select_first(self->selection, editposition);
	psy_audio_sequencecursor_set_order_index(&cursor, editposition);	
	psy_audio_sequencecursor_set_offset(&cursor, pattern_offset);
	psy_audio_sequence_set_cursor(self->sequence, cursor);	
}

void psy_audio_sequenceremovecommand_revert(psy_audio_SequenceRemoveCommand* self)
{
	psy_audio_exclusivelock_enter();	
	psy_audio_sequence_copy(self->sequence, &self->restoresequence);
	psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
//	psy_audio_sequenceselection_update(self->selection);		
	psy_audio_exclusivelock_leave();
}


/* psy_audio_SequenceClearCommand */

/* prototypes */
static void psy_audio_sequenceclearcommand_execute(psy_audio_SequenceClearCommand*,
	uintptr_t param);

/* vtable */
static psy_CommandVtable psy_audio_sequenceclearcommand_vtable;
static bool psy_audio_sequenceclearcommand_vtable_initialized = FALSE;

static void psy_audio_sequenceclearcommand_vtable_init(psy_audio_SequenceClearCommand* self)
{
	if (!psy_audio_sequenceclearcommand_vtable_initialized) {
		psy_audio_sequenceclearcommand_vtable = *(self->command.vtable);		
		psy_audio_sequenceclearcommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequenceclearcommand_execute;		
		psy_audio_sequenceclearcommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_sequenceclearcommand_vtable;
}

/* implementation */
psy_audio_SequenceClearCommand* psy_audio_sequenceclearcommand_alloc(
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection)
{
	psy_audio_SequenceClearCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceClearCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequenceclearcommand_vtable_init(rv);		
		rv->sequence = sequence;
		rv->selection = selection;
		psy_audio_sequenceselection_init(&rv->restoreselection);
		psy_audio_sequence_init(&rv->restoresequence, sequence->patterns, sequence->samples);
	}
	return rv;
}

void psy_audio_sequenceclearcommand_dispose(psy_audio_SequenceClearCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);
	psy_audio_sequence_dispose(&self->restoresequence);	
}

void psy_audio_sequenceclearcommand_execute(psy_audio_SequenceClearCommand* self,
	uintptr_t param)
{	
	psy_audio_SequenceCursor cursor;

	psy_audio_sequencecursor_init(&cursor);
	psy_audio_sequence_set_cursor(self->sequence, cursor);
	psy_audio_exclusivelock_enter();
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_copy(&self->restoresequence, self->sequence);
	psy_audio_sequence_clear(self->sequence);	
	psy_audio_sequence_append_track(self->sequence,
		psy_audio_sequencetrack_alloc_init());
	psy_audio_sequence_insert(self->sequence,
		psy_audio_orderindex_make(0, 0), 0);
	psy_audio_sequence_set_cursor(self->sequence, cursor);
	psy_audio_exclusivelock_leave();
}

void psy_audio_sequenceclearcommand_revert(psy_audio_SequenceClearCommand* self)
{	
	psy_audio_exclusivelock_enter();
	psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
	psy_audio_sequence_copy(self->sequence, &self->restoresequence);
	psy_audio_sequence_resetpatterns(self->sequence);
	psy_audio_sequenceselection_select_first(self->selection,
		psy_audio_sequenceselection_first(self->selection));
	psy_audio_exclusivelock_leave();
}


/* psy_audio_SequenceChangePatternCommand */

/* prototypes */
static void psy_audio_sequencechangepatterncommand_execute(
	psy_audio_SequenceChangePatternCommand*, uintptr_t param);

/* vtable */
static psy_CommandVtable psy_audio_sequencechangepatterncommand_vtable;
static bool psy_audio_sequencechangepatterncommand_vtable_initialized = FALSE;

static void psy_audio_sequencechangepatterncommand_vtable_init(
	psy_audio_SequenceChangePatternCommand* self)
{
	if (!psy_audio_sequencechangepatterncommand_vtable_initialized) {
		psy_audio_sequencechangepatterncommand_vtable = *(self->command.vtable);		
		psy_audio_sequencechangepatterncommand_vtable.execute =
			(psy_fp_command_param)
			psy_audio_sequencechangepatterncommand_execute;
		psy_audio_sequencechangepatterncommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_sequencechangepatterncommand_vtable;
}

/* implementation */
psy_audio_SequenceChangePatternCommand* psy_audio_sequencechangepatterncommand_alloc(
	psy_audio_Sequence* sequence, psy_audio_SequenceSelection* selection, intptr_t step)
{
	psy_audio_SequenceChangePatternCommand* rv;

	rv = malloc(sizeof(psy_audio_SequenceChangePatternCommand));
	if (rv) {
		psy_command_init(&rv->command);
		psy_audio_sequencechangepatterncommand_vtable_init(rv);		
		rv->sequence = sequence;
		rv->selection = selection;
		rv->step = step;
		rv->success = FALSE;
		psy_audio_sequenceselection_init(&rv->restoreselection);
		psy_audio_sequence_init(&rv->restoresequence, sequence->patterns,
			sequence->samples);
	}
	return rv;
}

void psy_audio_sequencechangepatterncommand_dispose(
	psy_audio_SequenceChangePatternCommand* self)
{
	psy_audio_sequenceselection_dispose(&self->restoreselection);
	psy_audio_sequence_dispose(&self->restoresequence);	
}

void psy_audio_sequencechangepatterncommand_execute(
	psy_audio_SequenceChangePatternCommand* self,
	uintptr_t param)
{		
	psy_audio_SequenceSelectionIterator ite;

	assert(self);

	self->success = FALSE;
	psy_audio_sequenceselection_copy(&self->restoreselection, self->selection);
	psy_audio_sequence_copy(&self->restoresequence, self->sequence);
	psy_audio_exclusivelock_enter();	
	ite = psy_audio_sequenceselection_begin(self->selection);
	for (; ite != NULL; psy_list_next(&ite)) {
		psy_audio_OrderIndex* orderindex;
		psy_audio_SequenceEntry* seqentry;
		psy_audio_SequencePatternEntry* seqpatternentry;

		orderindex = ite->entry;
		assert(orderindex);
		seqentry = psy_audio_sequence_entry(self->sequence, *orderindex);		
		if (!seqentry || seqentry->type != psy_audio_SEQUENCEENTRY_PATTERN) {
			return;
		}
		seqpatternentry = (psy_audio_SequencePatternEntry*)seqentry;
		if ((self->step > 0 || (self->step < 0 &&
				seqpatternentry->patternslot >= (uintptr_t)(self->step * (-1))))) {
			psy_audio_SequenceCursor cursor;

			cursor = self->sequence->cursor;
			psy_audio_sequence_setpatternindex(self->sequence,
				*orderindex, seqpatternentry->patternslot + self->step);			
			psy_audio_sequence_set_cursor(self->sequence, cursor);
			self->success = TRUE;
		}
	}	
	psy_audio_exclusivelock_leave();	
}

void psy_audio_sequencechangepatterncommand_revert(
	psy_audio_SequenceChangePatternCommand* self)
{	
	psy_audio_exclusivelock_enter();
	if (self->success) {
		psy_audio_sequence_copy(self->sequence, &self->restoresequence);
		psy_audio_sequenceselection_copy(self->selection, &self->restoreselection);
//		psy_audio_sequenceselection_update(self->selection);
		self->success = FALSE;
	}
	psy_audio_exclusivelock_leave();
}


/* psy_audio_PatternRemoveCmd */

/* prototypes */
static void psy_audio_patternremovecmd_execute(psy_audio_PatternRemoveCmd*,
	SequenceTraverseParams*);

/* vtable */
static psy_CommandVtable psy_audio_patternremovecmd_vtable;
static bool psy_audio_patternremovecmd_vtable_initialized = FALSE;

static void psy_audio_patternremovecmd_vtable_init(
	psy_audio_PatternRemoveCmd* self)
{
	assert(self);
	
	if (!psy_audio_patternremovecmd_vtable_initialized) {
		psy_audio_patternremovecmd_vtable = *(self->command.vtable);		
		psy_audio_patternremovecmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternremovecmd_execute;		
		psy_audio_patternremovecmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternremovecmd_vtable;
}

/* implementation */
void psy_audio_patternremovecmd_init(psy_audio_PatternRemoveCmd* self)
{
	assert(self);
	
	psy_command_init(&self->command);
	psy_audio_patternremovecmd_vtable_init(self);
}

void psy_audio_patternremovecmd_execute(psy_audio_PatternRemoveCmd* self,
	SequenceTraverseParams* params)
{
	assert(self);
						
	if (params->has_event && params->pattern && params->node) {
		params->prev_node = params->node->prev;
		params->prev_set = TRUE;
		psy_audio_pattern_remove(params->pattern, params->node);		
	}	
}

psy_audio_PatternRemoveCmd* psy_audio_patternremovecmd_alloc_init(void)
{
	psy_audio_PatternRemoveCmd* rv;

	rv = (psy_audio_PatternRemoveCmd*)malloc(sizeof(
		psy_audio_PatternRemoveCmd));
	if (rv) {
		psy_audio_patternremovecmd_init(rv);
	}
	return rv;
}

/* psy_audio_PatternTransposeCmd */

/* prototypes */
static void psy_audio_patterntransposecmd_dispose(
	psy_audio_PatternTransposeCmd* self);
static void psy_audio_patterntransposecmd_execute(
	psy_audio_PatternTransposeCmd*, SequenceTraverseParams*);
static void psy_audio_patterntransposecmd_revert(
	psy_audio_PatternTransposeCmd*);

/* vtable */
static psy_CommandVtable psy_audio_patterntransposecmd_vtable;
static bool psy_audio_patterntransposecmd_vtable_initialized = FALSE;

static void psy_audio_patterntransposecmd_vtable_init(
	psy_audio_PatternTransposeCmd* self)
{
	if (!psy_audio_patterntransposecmd_vtable_initialized) {
		psy_audio_patterntransposecmd_vtable = *(self->command.vtable);
		psy_audio_patterntransposecmd_vtable.dispose =
			(psy_fp_command)
			psy_audio_patterntransposecmd_dispose;
		psy_audio_patterntransposecmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patterntransposecmd_execute;
		psy_audio_patterntransposecmd_vtable.revert =
			(psy_fp_command)
			psy_audio_patterntransposecmd_revert;	
		psy_audio_patterntransposecmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patterntransposecmd_vtable;
}

void psy_audio_patterntransposecmd_init(
	psy_audio_PatternTransposeCmd* self,
	intptr_t offset)
{
	psy_command_init(&self->command);
	psy_audio_patterntransposecmd_vtable_init(self);
	self->offset = offset;
	psy_table_init(&self->nodemark);
}

psy_audio_PatternTransposeCmd* psy_audio_patterntransposecmd_alloc_init(
	intptr_t offset)
{
	psy_audio_PatternTransposeCmd* rv;

	rv = (psy_audio_PatternTransposeCmd*)malloc(sizeof(
		psy_audio_PatternTransposeCmd));
	if (rv) {
		psy_audio_patterntransposecmd_init(rv, offset);
	}
	return rv;
}

void psy_audio_patterntransposecmd_dispose(
		psy_audio_PatternTransposeCmd* self)
{
	assert(self);
	
	psy_table_dispose(&self->nodemark);
}

void psy_audio_patterntransposecmd_execute(psy_audio_PatternTransposeCmd*
	self, SequenceTraverseParams* params)
{
	assert(self);	

	if (!psy_table_exists(&self->nodemark, (uintptr_t)params->node)) {
		psy_audio_PatternEntry* entry;

		entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
		if (psy_audio_patternentry_front(entry)->note <
				psy_audio_NOTECOMMANDS_RELEASE) {
			if (psy_audio_patternentry_front(entry)->note + self->offset < 0) {
				psy_audio_patternentry_front(entry)->note = 0;
			} else {
				psy_audio_patternentry_front(entry)->note = (uint8_t)
					psy_min(255, psy_audio_patternentry_front(entry)->note + 
						self->offset);
			}
			if (psy_audio_patternentry_front(entry)->note >=
					psy_audio_NOTECOMMANDS_RELEASE) {
				psy_audio_patternentry_front(entry)->note =
					psy_audio_NOTECOMMANDS_RELEASE - 1;
			}
		}
		psy_table_insert(&self->nodemark, (uintptr_t)params->node, 0);
		psy_audio_pattern_inc_opcount(params->pattern);
	}
}

void psy_audio_patterntransposecmd_revert(
	psy_audio_PatternTransposeCmd* self)
{
	psy_table_clear(&self->nodemark);
}

/* psy_audio_PatternCopyCmd */

/* prototypes */
static void psy_audio_patterncopycmd_execute(psy_audio_PatternCopyCmd*,
	SequenceTraverseParams*);

/* vtable */
static psy_CommandVtable psy_audio_patterncopycmd_vtable;
static bool psy_audio_patterncopycmd_vtable_initialized = FALSE;

static void psy_audio_patterncopycmd_vtable_init(
	psy_audio_PatternCopyCmd* self)
{
	assert(self);
	
	if (!psy_audio_patterncopycmd_vtable_initialized) {
		psy_audio_patterncopycmd_vtable = *(self->command.vtable);
		psy_audio_patterncopycmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patterncopycmd_execute;
		psy_audio_patterncopycmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patterncopycmd_vtable;
}

/* implementation */
void psy_audio_patterncopycmd_init(psy_audio_PatternCopyCmd* self,
	psy_audio_Pattern* dest, psy_dsp_beatpos_t abs_offset,
	intptr_t trackoffset)
{
	assert(self);
	
	psy_command_init(&self->command);
	psy_audio_patterncopycmd_vtable_init(self);
	self->dest = dest;
	self->prev = 0;
	self->abs_offset = abs_offset;
	self->trackoffset = trackoffset;
}

void psy_audio_patterncopycmd_execute(psy_audio_PatternCopyCmd* self,
	SequenceTraverseParams* params)
{
	assert(self);

	if (params->pattern && params->node && params->seq_node) {
		psy_audio_PatternEntry* entry;
		psy_audio_SequenceEntry* seq_entry;
							
		entry = psy_audio_patternnode_entry(params->node);
		seq_entry = (psy_audio_SequenceEntry*)params->seq_node->entry;
		self->prev = psy_audio_pattern_insert(self->dest, self->prev,
			psy_audio_patternentry_track(entry) - self->trackoffset,
			psy_dsp_beatpos_sub(psy_dsp_beatpos_add(
				psy_audio_sequenceentry_offset(seq_entry),
					psy_audio_patternentry_offset(entry)),
				self->abs_offset), entry);
	}
}

psy_audio_PatternSearchReplaceMode
psy_audio_patternsearchreplacemode_make(
	int searchnote, int searchinst, int searchmach, int replnote,
	int replinst, int replmach, bool repltweak)
{
	psy_audio_PatternSearchReplaceMode mode;

	mode.notereference = (uintptr_t)(searchnote & 0xFF);
	mode.instreference = (uintptr_t)(searchinst & 0xFF);
	mode.machreference = (uintptr_t)(searchmach & 0xFF);
	mode.notereplace = (uintptr_t)(replnote & 0xFF);
	mode.instreplace = (uintptr_t)(replinst & 0xFF);
	mode.machreplace = (uintptr_t)(replmach & 0xFF);
	mode.tweakreplace = psy_audio_NOTECOMMANDS_EMPTY;

	// In search: 1001 empty, 1002 non-empty, 1003 all, other -> exact match
	switch (searchnote) {
	case 1001: mode.notematcher = psy_audio_patternsearchreplacemode_matchesempty; break;
	case 1002: mode.notematcher = psy_audio_patternsearchreplacemode_matchesnonempty; break;
	case 1003: mode.notematcher = psy_audio_patternsearchreplacemode_matchesall; break;
	default: mode.notematcher = psy_audio_patternsearchreplacemode_matchesequal; break;
	}
	switch (searchinst) {
	case 1001: mode.instmatcher = psy_audio_patternsearchreplacemode_matchesempty; break;
	case 1002: mode.instmatcher = psy_audio_patternsearchreplacemode_matchesnonempty; break;
	case 1003: mode.instmatcher = psy_audio_patternsearchreplacemode_matchesall; break;
	default: mode.instmatcher = psy_audio_patternsearchreplacemode_matchesequal; break;
	}
	switch (searchmach) {
	case 1001: mode.machmatcher = psy_audio_patternsearchreplacemode_matchesempty; break;
	case 1002: mode.machmatcher = psy_audio_patternsearchreplacemode_matchesnonempty; break;
	case 1003: mode.machmatcher = psy_audio_patternsearchreplacemode_matchesall; break;
	default: mode.machmatcher = psy_audio_patternsearchreplacemode_matchesequal; break;
	}
	// in replace: 1001 set empty, 1002 -> keep existing, other -> replace value
	switch (replnote) {
	case 1001: mode.notereplacer = psy_audio_patternsearchreplacemode_replacewithempty; break;
	case 1002: mode.notereplacer = psy_audio_patternsearchreplacemode_replacewithcurrent; break;
	default: mode.notereplacer = psy_audio_patternsearchreplacemode_replacewithnewval; break;
	}
	switch (replinst) {
	case 1001: mode.instreplacer = psy_audio_patternsearchreplacemode_replacewithempty; break;
	case 1002: mode.instreplacer = psy_audio_patternsearchreplacemode_replacewithcurrent; break;
	default: mode.instreplacer = psy_audio_patternsearchreplacemode_replacewithnewval; break;
	}
	switch (replmach) {
	case 1001: mode.machreplacer = psy_audio_patternsearchreplacemode_replacewithempty; break;
	case 1002: mode.machreplacer = psy_audio_patternsearchreplacemode_replacewithcurrent; break;
	default: mode.machreplacer = psy_audio_patternsearchreplacemode_replacewithnewval; break;
	}
	mode.tweakreplacer = (repltweak)
		? psy_audio_patternsearchreplacemode_replacewithempty
		: psy_audio_patternsearchreplacemode_replacewithcurrent;
	return mode;
}

/* psy_audio_PatternSearchCmd */

/* prototypes */
static void psy_audio_patternsearchcmd_dispose(psy_audio_PatternSearchCmd*);
static void psy_audio_patternsearchcmd_execute(psy_audio_PatternSearchCmd*,
	SequenceTraverseParams*);
static void psy_audio_patternsearchcmd_revert(psy_audio_PatternSearchCmd*);

/* vtable */
static psy_CommandVtable psy_audio_patternsearchcmd_vtable;
static bool psy_audio_patternsearchcmd_vtable_initialized = FALSE;

static void psy_audio_patternsearchcmd_vtable_init(
	psy_audio_PatternSearchCmd* self)
{
	assert(self);
	
	if (!psy_audio_patternsearchcmd_vtable_initialized) {
		psy_audio_patternsearchcmd_vtable = *(self->command.vtable);
		psy_audio_patternsearchcmd_vtable.dispose =
			(psy_fp_command)
			psy_audio_patternsearchcmd_dispose;
		psy_audio_patternsearchcmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternsearchcmd_execute;
		psy_audio_patternsearchcmd_vtable.revert =
			(psy_fp_command)
			psy_audio_patternsearchcmd_revert;		
		psy_audio_patternsearchcmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternsearchcmd_vtable;
}

void psy_audio_patternsearchcmd_init(psy_audio_PatternSearchCmd* self,
	psy_audio_PatternSearchReplaceMode search_pattern, bool replace)
{
	psy_command_init(&self->command);
	psy_audio_patternsearchcmd_vtable_init(self);
	self->search_pattern = search_pattern;	
	self->replace = replace;
	self->result = psy_audio_sequencecursor_make_invalid();
}

void psy_audio_patternsearchcmd_dispose(
	psy_audio_PatternSearchCmd* self)
{
	assert(self);
	
}

psy_audio_PatternSearchCmd* psy_audio_patternsearchcmd_alloc_init(
	psy_audio_PatternSearchReplaceMode search_pattern, bool replace)
{
	psy_audio_PatternSearchCmd* rv;

	rv = (psy_audio_PatternSearchCmd*)malloc(sizeof(
		psy_audio_PatternSearchCmd));
	if (rv) {
		psy_audio_patternsearchcmd_init(rv, search_pattern, replace);
	}
	return rv;
}

void psy_audio_patternsearchcmd_execute(psy_audio_PatternSearchCmd*
	self, SequenceTraverseParams* params)
{
	psy_audio_PatternEntry* entry;	
	psy_audio_PatternEvent* ev;
	
	assert(self);	
	
	if (!params->seq_node || !params->node || !params->track) {
		return;
	}
	entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
	ev = psy_audio_patternentry_front(entry);
	if (!ev) {
		return;
	}	
	if (psy_audio_patternsearchreplacemode_match(&self->search_pattern, *ev)) {		
		if (self->replace) {
			ev->note = (uint8_t)self->search_pattern.notereplacer(ev->note,
				self->search_pattern.notereplace);
			ev->inst = (uint16_t)self->search_pattern.instreplacer(ev->inst,
				self->search_pattern.instreplace);
			ev->mach = (uint8_t)self->search_pattern.machreplacer(ev->mach,
				self->search_pattern.machreplace);
			psy_audio_pattern_inc_opcount(params->pattern);
		} else {			
			psy_audio_PatternEntry* e;
			psy_audio_SequenceEntry* seqentry;
			
			params->stop = TRUE;
			seqentry = (psy_audio_SequenceEntry*)params->seq_node->entry;
			e = psy_audio_patternnode_entry(params->node);			
			self->result = psy_audio_sequencecursor_make_all(
				psy_audio_orderindex_make(
					params->track_idx,
					seqentry->row),
				psy_audio_patternentry_track(e),
				psy_audio_patternentry_offset(e),
				(uint8_t)self->search_pattern.notereference);
		}
	}	
}

psy_audio_SequenceCursor psy_audio_patternsearchcmd_result(
	const psy_audio_PatternSearchCmd* self)
{	
	assert(self);
	
	return self->result;	
}

void psy_audio_patternsearchcmd_revert(psy_audio_PatternSearchCmd* self)
{
	assert(self);
	
	self->result = psy_audio_sequencecursor_make_invalid();	
}

/* psy_audio_PatternSwingFillCmd */

/* prototypes */
static void psy_audio_patternswingfillcmd_execute(
	psy_audio_PatternSwingFillCmd*, SequenceTraverseParams*);
	
/* vtable */
static psy_CommandVtable psy_audio_patternswingfillcmd_vtable;
static bool psy_audio_patternswingfillcmd_vtable_initialized = FALSE;

static void psy_audio_patternswingfillcmd_vtable_init(
	psy_audio_PatternSwingFillCmd* self)
{
	if (!psy_audio_patternswingfillcmd_vtable_initialized) {
		psy_audio_patternswingfillcmd_vtable = *(self->command.vtable);		
		psy_audio_patternswingfillcmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternswingfillcmd_execute;		
		psy_audio_patternswingfillcmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternswingfillcmd_vtable;
}

void psy_audio_patternswingfillcmd_init(psy_audio_PatternSwingFillCmd*
	self, psy_audio_SwingFill swingfill)
{
	assert(self);
	
	psy_command_init(&self->command);
	psy_audio_patternswingfillcmd_vtable_init(self);
	self->swingfill = swingfill;	
	self->twopi = 2.0 * psy_dsp_PI;
	self->var = (swingfill.variance / 100.0);
	self->step = self->twopi / (swingfill.width);
	self->index = swingfill.phase * self->twopi / 360;
	self->dcoffs = 0;
	/*
	** remember we are at each speed for the length of time it takes to
	** do one tick this approximately calculates the offset	
	*/
	if (self->swingfill.offset) {
		double swing = 0.0;
		uintptr_t l;
		
		for (l = 0; l < (uintptr_t)self->swingfill.width; ++l) {
			double val;
			
			val = ((sin(self->index) * self->var *
				self->swingfill.tempo) + self->swingfill.tempo);
			swing += (val / self->swingfill.tempo) *
				(val / self->swingfill.tempo);
			self->index += self->step;
		}
		self->dcoffs = ((swing - self->swingfill.width) *
			self->swingfill.tempo) / self->swingfill.width;
	}	
}

psy_audio_PatternSwingFillCmd* psy_audio_patternswingfillcmd_alloc_init(
	psy_audio_SwingFill swingfill)
{
	psy_audio_PatternSwingFillCmd* rv;

	rv = (psy_audio_PatternSwingFillCmd*)malloc(sizeof(
		psy_audio_PatternSwingFillCmd));
	if (rv) {
		psy_audio_patternswingfillcmd_init(rv, swingfill);
	}
	return rv;
}

void psy_audio_patternswingfillcmd_execute(psy_audio_PatternSwingFillCmd*
	self, SequenceTraverseParams* params)
{
	psy_audio_PatternEntry* entry;	
	psy_audio_PatternEvent* ev;
	int val;
	
	assert(self);	
	
	if (!params->seq_node || !params->track || !params->pattern) {
		return;
	}	
	if (!params->node) {
		psy_audio_PatternEntry insert;
						
		psy_audio_patternentry_init(&insert);
		params->prev_set = TRUE;
		params->prev_node = psy_audio_pattern_insert(params->pattern,
			params->prev_node, params->channel,
			psy_dsp_beatpos_sub(params->offset, params->seqoffset),				
			&insert);
		psy_audio_patternentry_dispose(&insert);
		if (!params->prev_node) {
			return;
		}
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->prev_node);		
	} else {
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
	}
	ev = psy_audio_patternentry_front(entry);
	if (!ev) {
		return;
	}	
	/*
	** -0x20; // ***** proposed change to ffxx command to allow
	**  moreuseable range since the tempo bar only uses this range anyway..
	*/
	val = (int)((((sin(self->index) * self->var *
		self->swingfill.tempo) + self->swingfill.tempo) + self->dcoffs)
		+ 0.5f);
	if (val < 1) {
		val = 1;
	} else if (val > 255) {
		val = 255;
	}
	self->index += self->step;						
	ev->cmd = 0xff;
	ev->parameter = (unsigned char)(val);
}


/* psy_audio_LinearInterpolate */
psy_audio_LinearInterpolate psy_audio_linearinterpolate_make(
	psy_audio_Sequence* sequence, psy_audio_BlockSelection block,
	uintptr_t minval, uintptr_t maxval,
	int twk_type, int twk_inst, int twk_mac)
{
	psy_audio_LinearInterpolate rv;
	intptr_t startval;
	intptr_t endval;	
	psy_audio_SequenceCursor begin;
	psy_audio_SequenceCursor end;
	uintptr_t beginline;
	uintptr_t endline;	
	psy_audio_PatternEvent ev;	
	
	begin = block.topleft;
	end = block.bottomright;
	rv.minval = minval;
	rv.maxval = maxval;
	rv.beginoffset = psy_audio_sequencecursor_offset_abs(&block.topleft,
		sequence);
	beginline = (uintptr_t)(
		psy_dsp_beatpos_real(psy_audio_sequencecursor_offset_abs(&begin,
			sequence)) * begin.lpb);
	endline = psy_dsp_beatpos_line(
		psy_audio_sequencecursor_offset_abs(&end, sequence),
		psy_audio_sequencecursor_lpb(&end));		
	ev = psy_audio_sequence_pattern_event_at_cursor(sequence, begin);
	startval = psy_max(minval, psy_min(maxval,
		psy_audio_patternevent_tweakvalue(&ev)));
	end.offset = psy_dsp_beatpos_sub(
		psy_audio_sequencecursor_offset(&end),
		psy_audio_sequencecursor_bpl(&sequence->cursor));
	--end.channel_;
	ev = psy_audio_sequence_pattern_event_at_cursor(sequence, end);	
	endval = psy_max(minval, psy_min(maxval,
		psy_audio_patternevent_tweakvalue(&ev)));
	rv.step = (endval - startval) / (double)(endline - beginline - 1);
	rv.startval = startval;
	rv.endval = endval;
	rv.twk_type = twk_type;
	rv.twk_inst = twk_inst;
	rv.twk_mac = twk_mac;
	return rv;
}

/* psy_audio_PatternLinearInterpolateCmd */

/* prototypes */
static void psy_audio_patternlinearinterpolatecmd_execute(
	psy_audio_PatternLinearInterpolateCmd*, SequenceTraverseParams*);
	
/* vtable */
static psy_CommandVtable psy_audio_patternlinearinterpolatecmd_vtable;
static bool psy_audio_patternlinearinterpolatecmd_vtable_initialized = FALSE;

static void psy_audio_patternlinearinterpolatecmd_vtable_init(
	psy_audio_PatternLinearInterpolateCmd* self)
{
	if (!psy_audio_patternlinearinterpolatecmd_vtable_initialized) {
		psy_audio_patternlinearinterpolatecmd_vtable = *(self->command.vtable);		
		psy_audio_patternlinearinterpolatecmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternlinearinterpolatecmd_execute;		
		psy_audio_patternlinearinterpolatecmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternlinearinterpolatecmd_vtable;
}

void psy_audio_patternlinearinterpolatecmd_init(
	psy_audio_PatternLinearInterpolateCmd* self,
	psy_audio_LinearInterpolate interpolate)
{
	assert(self);
	
	psy_command_init(&self->command);
	psy_audio_patternlinearinterpolatecmd_vtable_init(self);
	self->interpolate = interpolate;	
}

psy_audio_PatternLinearInterpolateCmd*
psy_audio_patternlinearinterpolatecmd_alloc_init(
	psy_audio_LinearInterpolate interpolate)
{
	psy_audio_PatternLinearInterpolateCmd* rv;

	rv = (psy_audio_PatternLinearInterpolateCmd*)malloc(sizeof(
		psy_audio_PatternLinearInterpolateCmd));
	if (rv) {
		psy_audio_patternlinearinterpolatecmd_init(rv, interpolate);
	}
	return rv;
}

void psy_audio_patternlinearinterpolatecmd_execute(psy_audio_PatternLinearInterpolateCmd*
	self, SequenceTraverseParams* params)
{
	psy_audio_PatternEntry* entry;	
	psy_audio_PatternEvent* ev;	
	uintptr_t line;
	uintptr_t value;	
	
	assert(self);	
	
	if (!params->seq_node || !params->track || !params->pattern) {
		return;
	}	
	if (!params->node) {				
		psy_audio_PatternEntry insert;
						
		psy_audio_patternentry_init(&insert);		
		params->prev_node = psy_audio_pattern_insert(params->pattern,
			params->prev_node, params->channel,
			psy_dsp_beatpos_sub(params->offset, params->seqoffset),				
			&insert);
		params->prev_set = TRUE;
		psy_audio_patternentry_dispose(&insert);
		if (!params->prev_node) {
			return;
		}
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->prev_node);		
	} else {
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
	}
	ev = psy_audio_patternentry_front(entry);
	if (!ev) {
		return;
	}		
	line = (uintptr_t)(
		psy_dsp_beatpos_real(
			psy_dsp_beatpos_sub(
				params->offset,
				self->interpolate.beginoffset)) /		
		psy_dsp_beatpos_real(params->bpl));
	value = (int)((self->interpolate.step * (line) +
		self->interpolate.startval));
	if (self->interpolate.twk_type != psy_audio_NOTECOMMANDS_EMPTY) {
		ev->note = self->interpolate.twk_type;
		ev->inst = self->interpolate.twk_inst;
		ev->mach = self->interpolate.twk_mac;
	}
	psy_audio_patternevent_set_tweak_value(ev, (uint16_t)value);
	psy_audio_pattern_inc_opcount(params->pattern);
}

/* psy_audio_HermiteInterpolate */
psy_audio_HermiteInterpolate psy_audio_hermiteinterpolate_make(
	psy_audio_Sequence* sequence, psy_audio_BlockSelection block)
{
	psy_audio_HermiteInterpolate rv;
	intptr_t startval;
	intptr_t endval;	
	psy_audio_SequenceCursor begin;
	psy_audio_SequenceCursor end;
	uintptr_t beginline;
	uintptr_t endline;	
	psy_audio_PatternEvent ev;	
		
	begin = block.topleft;
	end = block.bottomright;
	rv.begin = psy_audio_sequencecursor_offset_abs(&block.topleft, sequence);
	beginline = psy_dsp_beatpos_line(psy_audio_sequencecursor_offset_abs(
		&begin, sequence), begin.lpb);
	endline = psy_dsp_beatpos_line(psy_audio_sequencecursor_offset_abs(
		&end, sequence), end.lpb);		
	if (endline > beginline) {
		rv.distance = endline - beginline - 1;
	} else {
		rv.distance = 0;
	}
	ev = psy_audio_sequence_pattern_event_at_cursor(sequence, begin);
	startval = psy_audio_patternevent_tweakvalue(&ev);	
	end.offset = psy_dsp_beatpos_sub(
		psy_audio_sequencecursor_offset(&end),
		psy_audio_sequencecursor_bpl(&sequence->cursor));
	--end.channel_;
	ev = psy_audio_sequence_pattern_event_at_cursor(sequence, end);	
	endval = psy_audio_patternevent_tweakvalue(&ev);	
	rv.val0 = rv.val1 = startval;	
	rv.val2 = rv.val2 = endval;	
	return rv;
}

/* psy_audio_PatternHermiteInterpolateCmd */

/* prototypes */
static void psy_audio_patternhermiteinterpolatecmd_execute(
	psy_audio_PatternHermiteInterpolateCmd*, SequenceTraverseParams*);
	
/* vtable */
static psy_CommandVtable psy_audio_patternhermiteinterpolatecmd_vtable;
static bool psy_audio_patternhermiteinterpolatecmd_vtable_initialized = FALSE;

static void psy_audio_patternhermiteinterpolatecmd_vtable_init(
	psy_audio_PatternHermiteInterpolateCmd* self)
{
	if (!psy_audio_patternhermiteinterpolatecmd_vtable_initialized) {
		psy_audio_patternhermiteinterpolatecmd_vtable = *(self->command.vtable);		
		psy_audio_patternhermiteinterpolatecmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternhermiteinterpolatecmd_execute;		
		psy_audio_patternhermiteinterpolatecmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternhermiteinterpolatecmd_vtable;
}

void psy_audio_patternhermiteinterpolatecmd_init(
	psy_audio_PatternHermiteInterpolateCmd* self,
	psy_audio_HermiteInterpolate interpolate)
{
	assert(self);
	
	psy_command_init(&self->command);
	psy_audio_patternhermiteinterpolatecmd_vtable_init(self);
	self->interpolate = interpolate;	
}

psy_audio_PatternHermiteInterpolateCmd*
psy_audio_patternhermiteinterpolatecmd_alloc_init(
	psy_audio_HermiteInterpolate interpolate)
{
	psy_audio_PatternHermiteInterpolateCmd* rv;

	rv = (psy_audio_PatternHermiteInterpolateCmd*)malloc(sizeof(
		psy_audio_PatternHermiteInterpolateCmd));
	if (rv) {
		psy_audio_patternhermiteinterpolatecmd_init(rv, interpolate);
	}
	return rv;
}

void psy_audio_patternhermiteinterpolatecmd_execute(psy_audio_PatternHermiteInterpolateCmd*
	self, SequenceTraverseParams* params)
{
	psy_audio_PatternEntry* entry;	
	psy_audio_PatternEvent* ev;	
	
	assert(self);	
	
	if (!params->seq_node || !params->track || !params->pattern) {
		return;
	}	
	if (!params->node) {		
		psy_audio_PatternEntry insert;
						
		psy_audio_patternentry_init(&insert);		
		params->prev_node = psy_audio_pattern_insert(params->pattern,
			params->prev_node, params->channel,
			psy_dsp_beatpos_sub(params->offset, params->seqoffset),				
			&insert);
		psy_audio_patternentry_dispose(&insert);
		if (!params->prev_node) {
			return;
		}
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->prev_node);		
	} else {
		entry = (psy_audio_PatternEntry*)psy_list_entry(params->node);
	}
	ev = psy_audio_patternentry_front(entry);
	if (!ev) {
		return;
	}	
	psy_audio_patternevent_set_tweak_value(ev, (uint16_t)
		psy_dsp_hermite_curve_interpolate(
			self->interpolate.val0, self->interpolate.val1,
			self->interpolate.val2, self->interpolate.val3,
			psy_dsp_beatpos_line(psy_dsp_beatpos_sub(params->offset,
				self->interpolate.begin), params->lpb),
				self->interpolate.distance, 0, TRUE));
	psy_audio_pattern_inc_opcount(params->pattern);
}


/* psy_audio_PatternPasteCmd */

/* prototypes */
static void psy_audio_patternpastecmd_execute(psy_audio_PatternPasteCmd*,
	SequenceTraverseParams*);

/* vtable */
static psy_CommandVtable psy_audio_patternpastecmd_vtable;
static bool psy_audio_patternpastecmd_vtable_initialized = FALSE;

static void psy_audio_patternpastecmd_vtable_init(
	psy_audio_PatternPasteCmd* self)
{
	if (!psy_audio_patternpastecmd_vtable_initialized) {
		psy_audio_patternpastecmd_vtable = *(self->command.vtable);
		psy_audio_patternpastecmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternpastecmd_execute;
		psy_audio_patternpastecmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternpastecmd_vtable;
}

void psy_audio_patternpastecmd_init(psy_audio_PatternPasteCmd* self,
	psy_audio_Pattern* src, psy_audio_SequenceCursor begin, bool mix)
{
	assert(self);
	assert(src);

	psy_command_init(&self->command);
	psy_audio_patternpastecmd_vtable_init(self);
	self->src = src;
	self->begin = begin;
	self->curr = psy_audio_pattern_begin(src);
	self->mix = mix;
}

psy_audio_PatternPasteCmd*
psy_audio_patternpastecmd_alloc_init(psy_audio_Pattern* src,
	psy_audio_SequenceCursor cursor, bool mix)
{
	psy_audio_PatternPasteCmd* rv;

	rv = (psy_audio_PatternPasteCmd*)malloc(sizeof(psy_audio_PatternPasteCmd));
	if (rv) {
		psy_audio_patternpastecmd_init(rv, src, cursor, mix);
	}
	return rv;
}

void psy_audio_patternpastecmd_execute(psy_audio_PatternPasteCmd*
	self, SequenceTraverseParams* params)
{		
	psy_audio_PatternEntry* paste_entry;

	assert(self);

	if (!self->curr || !params->seq_node || !params->track || !params->pattern) {
		return;
	}
	paste_entry = psy_audio_patternnode_entry(self->curr);
	if ((psy_audio_sequencecursor_channel(&self->begin) +
			psy_audio_patternentry_track(paste_entry)) != params->channel) {
		return;
	}
	if (!psy_dsp_beatpos_testrange(params->offset, psy_dsp_beatpos_add(
			psy_audio_sequencecursor_offset(&self->begin),
			psy_audio_patternentry_offset(paste_entry)), params->bpl)) {
		return;
	}	
	if (params->node) {		
		psy_audio_PatternEntry* entry;

		entry = psy_audio_patternnode_entry(params->node);
		*psy_audio_patternentry_front(entry) = *psy_audio_patternentry_front(paste_entry);		
	} else {				
		params->prev_node = psy_audio_pattern_insert(params->pattern,
			params->prev_node, params->channel,
			psy_dsp_beatpos_sub(params->offset, params->seqoffset),
			paste_entry);
		params->prev_set = TRUE;
	}		
	self->curr = self->curr->next;
	
	psy_audio_pattern_inc_opcount(params->pattern);
}


/* psy_audio_PatternBlockCmd */

/* prototypes */
static void psy_audio_patternblockcmd_dispose(
	psy_audio_PatternBlockCmd*);
static void psy_audio_patternblockcmd_execute(psy_audio_PatternBlockCmd*,
	SequenceTraverseParams*);
static void psy_audio_patternblockcmd_revert(
	psy_audio_PatternBlockCmd*);

/* vtable */
static psy_CommandVtable psy_audio_patternblockcmd_vtable;
static bool psy_audio_patternblockcmd_vtable_initialized = FALSE;

static void psy_audio_patternblockcmd_vtable_init(
	psy_audio_PatternBlockCmd* self)
{
	assert(self);

	if (!psy_audio_patternblockcmd_vtable_initialized) {
		psy_audio_patternblockcmd_vtable = *(self->command.vtable);
		psy_audio_patternblockcmd_vtable.dispose =
			(psy_fp_command)
			psy_audio_patternblockcmd_dispose;
		psy_audio_patternblockcmd_vtable.execute =
			(psy_fp_command_param)
			psy_audio_patternblockcmd_execute;
		psy_audio_patternblockcmd_vtable.revert =
			(psy_fp_command)
			psy_audio_patternblockcmd_revert;
		psy_audio_patternblockcmd_vtable_initialized = TRUE;
	}
	self->command.vtable = &psy_audio_patternblockcmd_vtable;
}

void psy_audio_patternblockcmd_init(psy_audio_PatternBlockCmd* self,
	psy_audio_Sequence* sequence, psy_audio_BlockSelection block,
	bool grid, psy_Command* cmd)
{
	assert(self);
	assert(sequence);

	psy_command_init(&self->command);
	psy_audio_patternblockcmd_vtable_init(self);
	self->sequence = sequence;
	self->block = block;
	self->grid = grid;
	self->cmd = cmd;
	psy_audio_pattern_init(&self->restore);
}

void psy_audio_patternblockcmd_dispose(psy_audio_PatternBlockCmd* self)
{
	assert(self);

	if (self->cmd) {
		psy_command_dispose(self->cmd);
		free(self->cmd);
		self->cmd = NULL;
	}
	psy_audio_pattern_dispose(&self->restore);
}

psy_audio_PatternBlockCmd* psy_audio_patternblockcmd_alloc_init(
	psy_audio_Sequence* sequence, psy_audio_BlockSelection block,
	bool grid, psy_Command* cmd)
{
	psy_audio_PatternBlockCmd* rv;

	rv = (psy_audio_PatternBlockCmd*)malloc(sizeof(
		psy_audio_PatternBlockCmd));
	if (rv) {
		psy_audio_patternblockcmd_init(rv, sequence, block, grid, cmd);
	}
	return rv;
}

void psy_audio_patternblockcmd_execute(psy_audio_PatternBlockCmd*
	self, SequenceTraverseParams* params)
{
	assert(self);

	if (!self->cmd) {
		return;
	}
	psy_audio_sequence_block_copy_pattern(self->sequence, self->block,
		&self->restore);
	if (self->grid) {
		psy_audio_sequence_block_grid_traverse(self->sequence,
			self->block, self->cmd);
	}
	else {
		psy_audio_sequence_block_traverse(self->sequence, self->block,
			self->cmd);
	}
}

void psy_audio_patternblockcmd_revert(psy_audio_PatternBlockCmd* self)
{
	psy_audio_PatternRemoveCmd rm_cmd;

	/* clear block */
	psy_audio_patternremovecmd_init(&rm_cmd);
	psy_audio_sequence_block_traverse(self->sequence, self->block,
		psy_audio_patternremovecmd_base(&rm_cmd));
	psy_command_dispose(psy_audio_patternremovecmd_base(&rm_cmd));
	/* insert */
	{
		psy_audio_SequenceTrackIterator it;
		psy_audio_PatternNode* p;
		psy_audio_PatternNode* prev = 0;
		psy_dsp_beatpos_t dstoffset;
		psy_dsp_beatpos_t srcoffset;
		psy_dsp_beatpos_t currseqoffset;
		intptr_t trackoffset;
		psy_audio_Pattern* dest;
		psy_audio_SequenceEntry* seqentry;
		psy_dsp_beatpos_t bpl;

		bpl = psy_audio_sequencecursor_bpl(&self->sequence->cursor);
		psy_audio_exclusivelock_enter();
		psy_audio_sequencetrackiterator_init(&it);
		psy_audio_sequence_begin(self->sequence, 0,
			psy_audio_sequencecursor_offset_abs(&self->block.topleft,
				self->sequence),
			&it);
		dstoffset = psy_audio_sequencecursor_offset(&self->block.topleft);
		srcoffset = psy_dsp_beatpos_zero();
		trackoffset = self->block.topleft.channel_;
		if (it.sequencentrynode) {
			seqentry = (psy_audio_SequenceEntry*)it.sequencentrynode->entry;
		}
		else {
			seqentry = NULL;
			return;
		}
		p = psy_audio_pattern_begin(&self->restore);
		currseqoffset = psy_audio_sequenceentry_offset(seqentry);
		while (p != NULL) {
			psy_audio_PatternEntry* pasteentry;
			psy_audio_PatternNode* node;
			psy_audio_Pattern* pattern;
			psy_dsp_beatpos_t pattern_dest_offset;

			pasteentry = psy_audio_patternnode_entry(p);
			pattern_dest_offset = psy_dsp_beatpos_add(
				psy_audio_patternentry_offset(pasteentry),
				dstoffset);
			while (it.sequencentrynode &&
				!(psy_dsp_beatpos_real(pattern_dest_offset) >=
					psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
						seqentry)) - psy_dsp_beatpos_real(currseqoffset) &&
					psy_dsp_beatpos_real(pattern_dest_offset) <
					(psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
						seqentry)) - psy_dsp_beatpos_real(currseqoffset) +
						psy_dsp_beatpos_real(psy_audio_sequenceentry_length(
							seqentry))))) {
				psy_audio_sequencetrackiterator_inc_entry(&it);
				if (it.sequencentrynode) {
					seqentry = (psy_audio_SequenceEntry*)it.sequencentrynode->entry;
					pattern_dest_offset = psy_dsp_beatpos_make_real(
						psy_dsp_beatpos_real(pattern_dest_offset) -
						(psy_dsp_beatpos_real(psy_audio_sequenceentry_offset(
							seqentry)) - psy_dsp_beatpos_real(currseqoffset)),
						psy_dsp_DEFAULT_PPQ);
					currseqoffset = psy_audio_sequenceentry_offset(seqentry);
					dstoffset = psy_dsp_beatpos_neg(psy_audio_patternentry_offset(pasteentry));
				}
				else {
					seqentry = NULL;
				}
			}
			pattern = psy_audio_sequencetrackiterator_pattern(&it);
			if (!it.sequencentrynode || !pattern) {
				break;
			}
			dest = pattern;
			node = psy_audio_pattern_find_node(dest,
				psy_audio_patternentry_track(pasteentry) + trackoffset,
				pattern_dest_offset,
				bpl,
				&prev);
			if (node) {
				psy_audio_PatternEntry* entry;

				entry = psy_audio_patternnode_entry(node);
				*psy_audio_patternentry_front(entry) =
					*psy_audio_patternentry_front(pasteentry);
			}
			else {
				psy_audio_pattern_insert(dest,
					prev,
					psy_audio_patternentry_track(pasteentry) + trackoffset,
					pattern_dest_offset,
					pasteentry);
			}
			p = p->next;
		}
		psy_audio_sequencetrackiterator_dispose(&it);
		psy_audio_exclusivelock_leave();
	}
	/* clean up */
	psy_audio_pattern_dispose(&self->restore);
	psy_audio_pattern_init(&self->restore);
	self->cmd->vtable->revert(self->cmd);
}


/* pattern cmds */

/* InsertCommand */

/* prototypes */
static void insertcommand_execute(InsertCommand*, uintptr_t param);
static void insertcommand_revert(InsertCommand*);

/* vtable */
static psy_CommandVtable insertcommand_vtable;
static bool insertcommand_vtable_initialized = FALSE;

static void insertcommand_vtable_init(InsertCommand* self)
{
	assert(self);

	if (!insertcommand_vtable_initialized) {
		insertcommand_vtable = *(self->command.vtable);
		insertcommand_vtable.execute =
			(psy_fp_command_param)
			insertcommand_execute;
		insertcommand_vtable.revert =
			(psy_fp_command)
			insertcommand_revert;
		insertcommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &insertcommand_vtable;
}

/* implementation */
InsertCommand* insertcommand_allocinit(psy_audio_Pattern* pattern,
	psy_audio_SequenceCursor cursor, psy_audio_PatternEvent event,
	psy_dsp_beatpos_t duration, psy_audio_Sequence* sequence)
{
	InsertCommand* rv;

	rv = (InsertCommand*)malloc(sizeof(InsertCommand));
	if (rv) {
		psy_command_init(&rv->command);
		insertcommand_vtable_init(rv);
		rv->cursor = cursor;
		rv->event = event;
		rv->pattern = pattern;
		rv->sequence = sequence;
		rv->duration = duration;
	}
	return rv;
}

void insertcommand_execute(InsertCommand* self, uintptr_t param)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	self->oldevent = psy_audio_pattern_event_at_cursor(self->pattern,
		self->cursor);
	psy_audio_pattern_set_event_at_cursor(self->pattern, self->cursor,
		self->event);
	if (!psy_dsp_beatpos_is_zero(self->duration)) {
		psy_audio_PatternEvent release;

		psy_audio_patternevent_clear(&release);
		release.note = psy_audio_NOTECOMMANDS_RELEASE;
		self->cursor_at_noteoff = self->cursor;
		self->cursor_at_noteoff.offset = psy_dsp_beatpos_add(self->cursor.offset,
			self->duration);
		self->oldevent_at_noteoff = psy_audio_pattern_event_at_cursor(
			self->pattern, self->cursor_at_noteoff);
		psy_audio_pattern_set_event_at_cursor(self->pattern,
			self->cursor_at_noteoff, release);
	}
	psy_audio_exclusivelock_leave();
	if (self->sequence) {
		psy_audio_sequence_set_cursor(self->sequence, self->cursor);
	}
}

void insertcommand_revert(InsertCommand* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_pattern_set_event_at_cursor(self->pattern, self->cursor,
		self->oldevent);
	if (!psy_dsp_beatpos_is_zero(self->duration)) {
		psy_audio_pattern_set_event_at_cursor(self->pattern,
			self->cursor_at_noteoff, self->oldevent_at_noteoff);
	}
	psy_audio_exclusivelock_leave();
	if (self->sequence) {
		psy_audio_sequence_set_cursor(self->sequence, self->cursor);
	}
}


/* RemoveCommand */

/* prototypes */
static void removecommand_execute(RemoveCommand*, uintptr_t param);
static void removecommand_revert(RemoveCommand*);
/* vtable */
static psy_CommandVtable removecommandcommand_vtable;
static bool removecommandcommand_vtable_initialized = FALSE;

static void removecommandcommand_vtable_init(RemoveCommand* self)
{
	assert(self);

	if (!removecommandcommand_vtable_initialized) {
		removecommandcommand_vtable = *(self->command.vtable);
		removecommandcommand_vtable.execute =
			(psy_fp_command_param)
			removecommand_execute;
		removecommandcommand_vtable.revert =
			(psy_fp_command)
			removecommand_revert;
		removecommandcommand_vtable_initialized = TRUE;
	}
	self->command.vtable = &removecommandcommand_vtable;
}

/* implementation */
RemoveCommand* removecommand_allocinit(psy_audio_Pattern* pattern,
	psy_audio_SequenceCursor cursor, psy_audio_Sequence* sequence)
{
	RemoveCommand* rv;

	rv = (RemoveCommand*)malloc(sizeof(RemoveCommand));
	if (rv) {
		psy_command_init(&rv->command);
		removecommandcommand_vtable_init(rv);
		rv->cursor = cursor;
		rv->pattern = pattern;
		rv->sequence = sequence;
	}
	return rv;
}

void removecommand_execute(RemoveCommand* self, uintptr_t param)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	self->oldevent = psy_audio_pattern_event_at_cursor(self->pattern,
		self->cursor);
	psy_audio_pattern_set_event_at_cursor(self->pattern,
		self->cursor, psy_audio_patternevent_zero());
	psy_audio_exclusivelock_leave();
	if (self->sequence) {
		psy_audio_sequence_set_cursor(self->sequence, self->cursor);
	}
}

void removecommand_revert(RemoveCommand* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_pattern_set_event_at_cursor(self->pattern,
		self->cursor, self->oldevent);
	psy_audio_exclusivelock_leave();
	if (self->sequence) {
		psy_audio_sequence_set_cursor(self->sequence, self->cursor);
	}
}
