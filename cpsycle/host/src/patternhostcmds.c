/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternhostcmds.h"
/* host*/
#include "patternviewstate.h"
/* file */
#include <fileselect.h>
/* audio */
#include <exclusivelock.h>
#include <patternio.h>
#include <sequencecmds.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* prototypes */
static void patterncmds_on_pattern_load_changed(PatternCmds*,
	psy_FileSelect* sender);
static void patterncmds_on_pattern_save_changed(PatternCmds*,
	psy_FileSelect* sender);
static void patterncmds_block_transpose(PatternCmds*, intptr_t offset);
static void patterncmds_move_cursor_when_paste(PatternCmds*);
static void patterncmds_block_interpolate(PatternCmds*);
static void patterncmds_block_delete(PatternCmds*, psy_audio_BlockSelection);
static void patterncmds_block_paste(PatternCmds*, psy_audio_SequenceCursor,
	bool mix);
static void patterncmds_block_mix_paste(PatternCmds*);
static void patterncmds_block_copy(PatternCmds*, psy_audio_BlockSelection);
static void patterncmds_block_cut(PatternCmds*);
static void patterncmds_row_delete(PatternCmds*);
static void patterncmds_row_insert(PatternCmds*);
static void patterncmds_select_machine(PatternCmds*);
static void patterncmds_block_change_machine(PatternCmds*);
static void patterncmds_block_change_instrument(PatternCmds*);	
static void patterncmds_import_pattern(PatternCmds*);
static void patterncmds_export_pattern(PatternCmds*);
static bool patterncmds_check_block(const PatternCmds*,
	const psy_audio_BlockSelection*);

/* implementation */
void patterncmds_init(PatternCmds* self, PatternViewState* state,
	psy_DiskOp* disk_op)
{
	assert(self);
	assert(state);
	
	self->state = state;
	self->disk_op = disk_op;	
}

bool patterncmds_handle_edt_command(PatternCmds* self, psy_EventDriverCmd cmd)
{
	assert(self);	
	
	switch (cmd.id) {
	case CMD_BLOCKSTART:
		patternselection_start_drag(&self->state->selection,
			*patternviewstate_cursor(self->state));
		patternselection_stop_drag(&self->state->selection);		
		return TRUE;
	case CMD_BLOCKEND:
		patternselection_drag(&self->state->selection,
			self->state->cursor, self->state->sequence);
		patternselection_stop_drag(&self->state->selection);
		return TRUE;
	case CMD_BLOCKUNMARK:
		patternselection_disable(&self->state->selection);		
		return TRUE;
	case CMD_BLOCKCUT:		
		patterncmds_block_cut(self);
		return TRUE;
	case CMD_BLOCKCOPY:
		patterncmds_block_copy(self, self->state->selection.block);
		return TRUE;
	case CMD_BLOCKPASTE:
		patterncmds_block_paste(self, self->state->cursor, FALSE);
		patterncmds_move_cursor_when_paste(self);
		return TRUE;
	case CMD_BLOCKMIX:
		patterncmds_block_mix_paste(self);
		patterncmds_move_cursor_when_paste(self);
		return TRUE;
	case CMD_BLOCKINTERPOLATE:
		patterncmds_block_interpolate(self);
		return TRUE;
	case CMD_BLOCKSETMACHINE:		
		patterncmds_block_change_machine(self);
		return TRUE;
	case CMD_BLOCKSETINSTR:		
		patterncmds_block_change_instrument(self);
		return TRUE;
	case CMD_BLOCKDELETE:
		patterncmds_block_delete(self, self->state->selection.block);
		return TRUE;
	case CMD_TRANSPOSEBLOCKDEC:
		patterncmds_block_transpose(self, -1);		
		return TRUE;
	case CMD_TRANSPOSEBLOCKINC:
		patterncmds_block_transpose(self, 1);		
		return TRUE;
	case CMD_TRANSPOSEBLOCKDEC12:
		patterncmds_block_transpose(self, -12);		
		return TRUE;
	case CMD_TRANSPOSEBLOCKINC12:
		patterncmds_block_transpose(self, 12);		
		return TRUE;	
	case CMD_UNDO:
		if (self->state->undo_redo) {
			psy_undoredo_undo(self->state->undo_redo);
		}
		return TRUE;
	case CMD_REDO:
		if (self->state->undo_redo) {
			psy_undoredo_redo(self->state->undo_redo);
		}
		return TRUE;
	case CMD_SELECTALL:
		patternselection_select_all(&self->state->selection,
			self->state->sequence, patternviewstate_single_mode(self->state));
		return TRUE;
	case CMD_SELECTCOL:
		patternselection_select_col(&self->state->selection,
			self->state->sequence, patternviewstate_single_mode(self->state));
		return TRUE;
	case CMD_SELECTBAR:
		patternselection_select_bar(&self->state->selection,
			self->state->sequence, patternviewstate_single_mode(self->state));
		return TRUE;
	case CMD_SELECTMACHINE: 
		patterncmds_select_machine(self);		
		return TRUE;
	case CMD_ROWINSERT: 
		patterncmds_row_insert(self);		
		return TRUE;
	case CMD_ROWDELETE:
		patterncmds_row_delete(self);
		return TRUE;
	case CMD_PATTERN_IMPORT:	
		patterncmds_import_pattern(self);
		return TRUE;
	case CMD_PATTERN_EXPORT:	
		patterncmds_export_pattern(self);
		return TRUE;
	default:
		return FALSE;
	}	
}

void patterncmds_block_change_instrument(PatternCmds* self)
{
	psy_audio_BlockSelection block;
		
	assert(self);
				
	block = self->state->selection.block;
	if (psy_audio_blockselection_valid(&block)) {		
		psy_audio_InstrumentIndex inst;
	
		inst = psy_audio_instruments_selected(
			&self->state->player->song->instruments_);
		if (inst.subslot == psy_INDEX_INVALID) {
			return;
		}
		if (!patternviewstate_sequence(self->state)) {		
			return;
		}		
		psy_undoredo_execute(self->state->undo_redo,
		psy_audio_patternblockcmd_base(
			psy_audio_patternblockcmd_alloc_init(
				patternviewstate_sequence(self->state),
				block, FALSE,
				psy_audio_patternsearchcmd_base(
					psy_audio_patternsearchcmd_alloc_init(
						psy_audio_patternsearchreplacemode_make(
						psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* note */
						psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* inst */
						psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* mac */
						psy_audio_PATTERNREPLACESELECTOR_KEEP,  /* note */				
						(uint8_t)inst.subslot,                  /* repl inst */
						psy_audio_PATTERNREPLACESELECTOR_KEEP,  /* mac */
						FALSE), TRUE)))));	
		patternviewstate_invalidate(self->state);			
	}
}

void patterncmds_block_change_machine(PatternCmds* self)
{
	psy_audio_BlockSelection block;
	uintptr_t mac;
		
	assert(self);
										
	block = self->state->selection.block;
	if (!patterncmds_check_block(self, &block)) {
		return;
	}
	mac = psy_audio_machines_selected(&self->state->player->song->machines_);
	if (mac == psy_INDEX_INVALID) {
		return;
	}	
	psy_undoredo_execute(self->state->undo_redo,
	psy_audio_patternblockcmd_base(
		psy_audio_patternblockcmd_alloc_init(
			patternviewstate_sequence(self->state),
			block, FALSE,
			psy_audio_patternsearchcmd_base(
				psy_audio_patternsearchcmd_alloc_init(
					psy_audio_patternsearchreplacemode_make(
					psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* note */
					psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* inst */
					psy_audio_PATTERNSEARCHSELECTOR_ALL,    /* mac */
					psy_audio_PATTERNREPLACESELECTOR_KEEP,  /* note */
					psy_audio_PATTERNREPLACESELECTOR_KEEP,  /* inst */
					(uint8_t)mac,                           /* replace mach */		
					FALSE), TRUE)))));	
	patternviewstate_invalidate(self->state);			
}

void patterncmds_select_machine(PatternCmds* self)
{
	assert(self);
	
	if (patternviewstate_sequence(self->state)) {
		psy_audio_PatternEvent ev;
		
		ev = psy_audio_sequence_pattern_event_at_cursor(
			patternviewstate_sequence(self->state),
			self->state->cursor);		
		if (!psy_audio_patternevent_empty(&ev)) {
			psy_audio_machines_select(
				psy_audio_song_machines(self->state->player->song),
				ev.mach);
			psy_audio_instruments_select(
				psy_audio_song_instruments(self->state->player->song),				
				psy_audio_instrumentindex_make(0, ev.inst));
		}
	}
}

void patterncmds_row_insert(PatternCmds* self)
{
	psy_audio_Pattern* pattern;		
	psy_audio_BlockSelection block;
	psy_audio_SequenceCursor position;	
	
	assert(self);

	pattern = patternviewstate_pattern(self->state);
	if (!pattern) {
		return;
	}		
	position = self->state->cursor;
	psy_audio_sequencecursor_set_offset(&position, psy_audio_pattern_length(
		pattern));
	psy_audio_sequencecursor_set_channel(&position, 
		psy_audio_sequencecursor_channel(&self->state->cursor) + 1);
	psy_audio_blockselection_init_all(&block, self->state->cursor,
		position);		
	position = self->state->cursor;	
	patterncmds_block_copy(self, block);		
	patterncmds_block_delete(self, block);	
	psy_audio_sequencecursor_set_offset(&position,
		psy_dsp_beatpos_add(self->state->cursor.offset,
			psy_audio_sequencecursor_bpl(&self->state->cursor)));
	patterncmds_block_paste(self, position, FALSE);
}

void patterncmds_row_delete(PatternCmds* self)
{
	psy_audio_Pattern* pattern;
	psy_audio_BlockSelection block;
	psy_audio_SequenceCursor begin;
	psy_audio_SequenceCursor end;

	assert(self);

	pattern = patternviewstate_pattern(self->state);
	if (!pattern) {
		return;
	}
	if (psy_dsp_beatpos_is_zero(psy_audio_sequencecursor_offset(
			&self->state->cursor))) {
		return;
	}
	begin = self->state->cursor;	
	end = begin;
	end.offset = psy_audio_pattern_length(pattern);
	psy_audio_sequencecursor_inc_channel(&end);		
	psy_audio_blockselection_init_all(&block, begin, end);
	patterncmds_block_copy(self, block);
	begin.offset = psy_dsp_beatpos_sub(begin.offset,
		psy_audio_sequencecursor_bpl(&begin));
	psy_audio_blockselection_init_all(&block, begin, end);
	patterncmds_block_delete(self, block);
	patterncmds_block_paste(self, block.topleft, FALSE);	
}

void patterncmds_import_pattern(PatternCmds* self)
{
	assert(self);
	
	if (self->disk_op) {
		psy_FileSelect load;
			
		psy_fileselect_init(&load);
		psy_fileselect_connect_change(&load, self, (psy_fp_fileselect_change)
			patterncmds_on_pattern_load_changed);
		psy_fileselect_set_title(&load, "Import Pattern");
		psy_fileselect_set_default_extension(&load, "*.psb");
		psy_fileselect_add_filter(&load, "PSB", "*.psb");
		// psy_audio_songfile_loadfilter()
		psy_diskop_execute(self->disk_op, &load, NULL, NULL);		
		psy_fileselect_dispose(&load);
	}
}

void patterncmds_on_pattern_load_changed(PatternCmds* self,
	psy_FileSelect* sender)
{
	assert(self);
		
	// psy_audio_patternio_load(self->pattern,
	//	psy_fileselect_value(sender), self->bpl);	
	// workspace_select_view(self->workspace, viewindex_make_all(
	//	VIEW_ID_PATTERNS, 0, 0, psy_INDEX_INVALID));
}

void patterncmds_export_pattern(PatternCmds* self)
{	
	assert(self);
	
	if (!self->disk_op) {
		psy_FileSelect save;
		
		psy_fileselect_init(&save);
		psy_fileselect_connect_change(&save, self, (psy_fp_fileselect_change)
			patterncmds_on_pattern_save_changed);
		psy_fileselect_set_title(&save, "Export Pattern");
		psy_fileselect_set_default_extension(&save, "*.psb");
		psy_fileselect_add_filter(&save, "PSB", "*.psb");
		// psy_audio_songfile_loadfilter()
		psy_diskop_execute(self->disk_op, &save, NULL, NULL);		
		psy_fileselect_dispose(&save);
	}
}

void patterncmds_on_pattern_save_changed(PatternCmds* self,
	psy_FileSelect* sender)
{
	// psy_audio_patternio_save(self->pattern,
	// 	psy_fileselect_value(sender), self->bpl,
	// 	self->numtracks);
	// workspace_select_view(self->workspace, viewindex_make_all(
	//	VIEW_ID_PATTERNS, 0, 0, psy_INDEX_INVALID));
}

void patterncmds_block_interpolate(PatternCmds* self)
{	
	assert(self);
	
	if (patterncmds_check_block(self, &self->state->selection.block)) {	
		psy_undoredo_execute(self->state->undo_redo,
			psy_audio_patternblockcmd_base(
				psy_audio_patternblockcmd_alloc_init(
					patternviewstate_sequence(self->state),
					self->state->selection.block, TRUE,
					psy_audio_patternlinearinterpolatecmd_base(
						psy_audio_patternlinearinterpolatecmd_alloc_init(
							psy_audio_linearinterpolate_make(
								patternviewstate_sequence(self->state),
								self->state->selection.block,
								0, 0xFF,
								psy_audio_NOTECOMMANDS_EMPTY,
								psy_audio_NOTECOMMANDS_INST_EMPTY,
								psy_audio_NOTECOMMANDS_EMPTY))))));
		patternviewstate_invalidate(self->state);
	}
}

void patterncmds_block_transpose(PatternCmds* self, intptr_t offset)
{	
	assert(self);
	
	if (patterncmds_check_block(self, &self->state->selection.block)) {
		psy_undoredo_execute(self->state->undo_redo,
			psy_audio_patternblockcmd_base(
				psy_audio_patternblockcmd_alloc_init(
					patternviewstate_sequence(self->state),					
					self->state->selection.block, FALSE,
					psy_audio_patterntransposecmd_base(
						psy_audio_patterntransposecmd_alloc_init(offset)))));
		patternviewstate_invalidate(self->state);
	}
}

void patterncmds_block_delete(PatternCmds* self, psy_audio_BlockSelection
	block)
{
	assert(self);
	
	if (patterncmds_check_block(self, &block)) {
		psy_undoredo_execute(self->state->undo_redo,
			psy_audio_patternblockcmd_base(
				psy_audio_patternblockcmd_alloc_init(
					patternviewstate_sequence(self->state),					
					block, FALSE,
					psy_audio_patternremovecmd_base(
						psy_audio_patternremovecmd_alloc_init()))));
		patternviewstate_invalidate(self->state);
	}	
}

void patterncmds_block_mix_paste(PatternCmds* self)
{
	assert(self);
	
	patterncmds_block_paste(self, self->state->cursor, TRUE);	
}

void patterncmds_block_paste(PatternCmds* self,
	psy_audio_SequenceCursor cursor, bool mix)
{
	assert(self);

	if (self->state->sequence && !psy_audio_pattern_empty(
			&self->state->patternpaste)) {
		psy_audio_BlockSelection block;

		block = psy_audio_blockselection_make(cursor, cursor);		
		block.bottomright.offset = psy_dsp_beatpos_add(
			cursor.offset, psy_audio_pattern_length(
				&self->state->patternpaste));
		psy_audio_sequencecursor_set_channel(&block.bottomright,
			psy_audio_sequencecursor_channel(&block.bottomright) +
			psy_audio_pattern_max_song_tracks(&self->state->patternpaste));
		psy_undoredo_execute(self->state->undo_redo,
			psy_audio_patternblockcmd_base(
				psy_audio_patternblockcmd_alloc_init(
					patternviewstate_sequence(self->state),
					block, TRUE,
					psy_audio_patternpastecmd_base(
						psy_audio_patternpastecmd_alloc_init(															
							&self->state->patternpaste, cursor, mix)))));
		patternviewstate_invalidate(self->state);
	}
}

void patterncmds_block_cut(PatternCmds* self)
{
	assert(self);
	
	patterncmds_block_copy(self, self->state->selection.block);
	patterncmds_block_delete(self, self->state->selection.block);	
}

void patterncmds_block_copy(PatternCmds* self,
	psy_audio_BlockSelection block)
{
	assert(self);
	
	if (patterncmds_check_block(self, &block)) {
		psy_audio_sequence_block_copy_pattern(
			patternviewstate_sequence(self->state),
			block, &self->state->patternpaste);
	}
}

void patterncmds_move_cursor_when_paste(PatternCmds* self)
{
	assert(self);		
	
	if (patternviewstate_sequence(self->state) &&
			patternviewstate_hasmovecursorwhenpaste(self->state)) {
		psy_audio_SequenceCursor cursor;

		cursor = self->state->cursor;
		psy_audio_sequencecursor_set_offset(&cursor,
			psy_dsp_beatpos_add(
				psy_audio_sequencecursor_offset(&cursor),
				psy_audio_pattern_length(&self->state->patternpaste)));
		/* todo abs */
		if (psy_dsp_beatpos_greater_equal(psy_audio_sequencecursor_offset(&cursor),
				psy_audio_pattern_length(patternviewstate_pattern(self->state)))) {
			psy_audio_sequencecursor_set_offset(&cursor,
				psy_dsp_beatpos_sub(
					psy_audio_pattern_length(patternviewstate_pattern(self->state)),
					psy_audio_sequencecursor_bpl(&cursor)));
		}
		psy_audio_sequence_set_cursor(patternviewstate_sequence(self->state),
			cursor);
	}	
}

void patterncmds_swing_fill(PatternCmds* self, psy_audio_SwingFill fill,
	bool track_mode)
{
	psy_audio_BlockSelection block;
	psy_audio_Sequence* sequence;	
		
	assert(self);
		
	sequence = patternviewstate_sequence(self->state);
	if (!sequence) {
		return;
	}		
	if (track_mode) {	
		if (patternviewstate_single_mode(self->state)) {
			block = psy_audio_sequence_block_selection_order(
			self->state->sequence, self->state->cursor.order_index);
		} else {
			block = psy_audio_sequence_block_selection_track(
				self->state->sequence,
				self->state->cursor.order_index.track);		
		}	
		if (psy_audio_blockselection_valid(&block)) {
			psy_audio_sequencecursor_set_channel(&block.topleft,
				psy_audio_sequencecursor_channel(&self->state->cursor));
			psy_audio_sequencecursor_set_channel(&block.bottomright,
				psy_audio_sequencecursor_channel(&block.topleft) + 1);
		}		
	} else {
		block = self->state->selection.block;		
		psy_audio_sequencecursor_set_channel(&block.bottomright,
			psy_audio_sequencecursor_channel(&block.topleft) + 1);
	}
	if (psy_audio_blockselection_valid(&block)) {								
		psy_undoredo_execute(self->state->undo_redo,
			psy_audio_patternblockcmd_base(
				psy_audio_patternblockcmd_alloc_init(
					self->state->sequence, block, TRUE,
					psy_audio_patternswingfillcmd_base(
					psy_audio_patternswingfillcmd_alloc_init(fill)))));		
		patternviewstate_invalidate(self->state);			
	}
}

bool patterncmds_check_block(const PatternCmds* self,
	const psy_audio_BlockSelection* block)
{
	assert(self);
	assert(block);
	
	return (patternviewstate_sequence(self->state) &&
		psy_audio_blockselection_valid(block));
}
