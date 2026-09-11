// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net

#include "../../detail/prefix.h"

#include "sequencehostcmds.h"
// host
#include "workspace.h"
// audio
#include <exclusivelock.h>
#include <sequencecmds.h>
// platform
#include "../../detail/portable.h"
#include "../../detail/trace.h"

// SequenceCmds
// implementation
void sequencecmds_init(SequenceCmds* self, Workspace* workspace)
{
	self->workspace = workspace;
	self->player = &self->workspace->player_;
	sequencecmds_update(self);
}

void sequencecmds_newentry(SequenceCmds* self,
	psy_audio_SequenceEntryType type)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);
		switch (type) {
		case psy_audio_SEQUENCEENTRY_PATTERN: {
			psy_audio_Pattern* newpattern;
			uintptr_t patidx;

			newpattern = psy_audio_pattern_alloc_init();
			/* change length to default lines */
			psy_audio_pattern_set_length(newpattern,
				psy_dsp_beatpos_make_lines(
					psy_audio_pattern_num_default_lines(),
					psy_audio_player_lpb(workspace_player(self->workspace)),
					psy_dsp_DEFAULT_PPQ));
			patidx = psy_audio_patterns_append(self->patterns, newpattern);
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&psy_audio_sequenceinsertcommand_alloc(self->sequence,
					&self->workspace->song->sequence_.selection,
					psy_audio_sequenceselection_first(
						&self->workspace->song->sequence_.selection),
					patidx)->command);
			break; }
		case psy_audio_SEQUENCEENTRY_SAMPLE: {			
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&psy_audio_sequencesampleinsertcommand_alloc(self->sequence,
					&self->workspace->song->sequence_.selection,
					psy_audio_sequenceselection_first(
						&self->workspace->song->sequence_.selection),
					self->workspace->song->samples_.selected)->command);
			break; }
		case psy_audio_SEQUENCEENTRY_MARKER: {
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&psy_audio_sequencemarkerinsertcommand_alloc(self->sequence,
					&self->workspace->song->sequence_.selection,
					psy_audio_sequenceselection_first(
						&self->workspace->song->sequence_.selection),
					"Untitled")->command);
			break; }
		default:
			break;
		}
	}
}

void sequencecmds_insert_entry(SequenceCmds* self,
	psy_audio_SequenceEntryType type)
{
	if (workspace_song(self->workspace)) {		
		sequencecmds_update(self);
		switch (type) {
		case psy_audio_SEQUENCEENTRY_PATTERN: {
			psy_audio_SequencePatternEntry* entry;

			entry = (psy_audio_SequencePatternEntry*)
				psy_audio_sequence_entry(self->sequence,
				psy_audio_sequenceselection_first(
					&self->workspace->song->sequence_.selection));
			if (entry) {
				psy_undoredo_execute(&self->workspace->undo_redo_,
					&psy_audio_sequenceinsertcommand_alloc(self->sequence,
						&self->workspace->song->sequence_.selection,
						psy_audio_sequenceselection_first(
							&self->workspace->song->sequence_.selection),
						entry->patternslot)->command);
			}
			break; }
		case psy_audio_SEQUENCEENTRY_MARKER: {
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&psy_audio_sequencemarkerinsertcommand_alloc(self->sequence,
					&self->workspace->song->sequence_.selection,
					psy_audio_sequenceselection_first(
						&self->workspace->song->sequence_.selection),
					"Untitled")->command);
			break; }
		default:
			break;
		}
	}
}

void sequencecmds_cloneentry(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		psy_audio_Pattern* pattern;

		sequencecmds_update(self);
		pattern = psy_audio_sequence_pattern(self->sequence,
			psy_audio_sequenceselection_first(&self->workspace->song->sequence_.selection));
		if (pattern) {
			psy_audio_Pattern* newpattern;
			uintptr_t patidx;

			newpattern = psy_audio_pattern_clone(pattern);
			patidx = psy_audio_patterns_append(self->patterns, newpattern);
			psy_undoredo_execute(&self->workspace->undo_redo_,
				&psy_audio_sequenceinsertcommand_alloc(self->sequence,
					&self->workspace->song->sequence_.selection,
					psy_audio_sequenceselection_first(&self->workspace->song->sequence_.selection),
					patidx)->command);
		}
	}
}

void sequencecmds_delentry(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		bool playing;
		sequencecmds_update(self);
		
		playing = psy_audio_player_playing(&self->workspace->player_);
		psy_audio_player_stop(&self->workspace->player_);
		psy_undoredo_execute(&self->workspace->undo_redo_,
			&psy_audio_sequenceremovecommand_alloc(self->sequence,
				&self->workspace->song->sequence_.selection)->command);
		if (playing) {
			psy_audio_exclusivelock_enter();		
			psy_audio_player_set_position(&self->workspace->player_,
				psy_audio_player_position(&self->workspace->player_));
			psy_audio_player_start(&self->workspace->player_);
			psy_audio_exclusivelock_leave();
		}
	}
}

void sequencecmds_incpattern(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);

		psy_undoredo_execute(&self->workspace->undo_redo_,
			&psy_audio_sequencechangepatterncommand_alloc(self->sequence,
				&self->workspace->song->sequence_.selection, 1)->command);
	}
}

void sequencecmds_decpattern(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);

		psy_undoredo_execute(&self->workspace->undo_redo_,
			&psy_audio_sequencechangepatterncommand_alloc(self->sequence,
				&self->workspace->song->sequence_.selection, -1)->command);
	}
}

void sequencecmds_changepattern(SequenceCmds* self, uintptr_t step)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);

		psy_undoredo_execute(&self->workspace->undo_redo_,
			&psy_audio_sequencechangepatterncommand_alloc(self->sequence,
				&self->workspace->song->sequence_.selection, step)->command);
	}
}

void sequencecmds_update(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		self->patterns = &workspace_song(self->workspace)->patterns_;
		self->sequence = &workspace_song(self->workspace)->sequence_;
	} else {
		self->patterns = NULL;
		self->sequence = NULL;
	}
}

void sequencecmds_copy(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);
		psy_audio_sequencepaste_copy(
			&self->workspace->sequencepaste,
			self->sequence,
			&self->workspace->song->sequence_.selection);
	}
}

void sequencecmds_paste(SequenceCmds* self)
{
	assert(self);

	if (workspace_song(self->workspace)) {
		psy_List* p;		

		sequencecmds_update(self);
		for (p = self->workspace->sequencepaste.nodes; p != NULL; psy_list_next(&p)) {
			psy_audio_Order* order;
			psy_audio_OrderIndex insertposition;
			psy_audio_SequencePatternEntry* newentry;

			order = (psy_audio_Order*)psy_list_entry(p);
			if (order->entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
				insertposition = psy_audio_sequenceselection_first(&self->workspace->song->sequence_.selection);
				insertposition.order += order->index.order;
				insertposition.track += order->index.track;
				psy_undoredo_execute(&self->workspace->undo_redo_,
					&psy_audio_sequenceinsertcommand_alloc(self->sequence,
						&self->workspace->song->sequence_.selection,
						insertposition,
						((psy_audio_SequencePatternEntry*)order->entry)->patternslot)->command);
				newentry = (psy_audio_SequencePatternEntry*)
					psy_audio_sequence_entry(self->sequence, insertposition);
				if (newentry) {
					*newentry = *((psy_audio_SequencePatternEntry*)(order->entry));
				}
			}
		}
	}
}

void sequencecmds_clear(SequenceCmds* self)
{
	if (self->sequence) {		
		sequencecmds_update(self);
		assert(self->sequence);
		psy_audio_player_stop(&self->workspace->player_);
		psy_audio_exclusivelock_enter();		
		workspace_clear_sequence_paste(self->workspace);
		/* no undo / redo */
		psy_audio_patterns_clear(self->sequence->patterns);
		psy_audio_patterns_insert(self->sequence->patterns, 0,
			psy_audio_pattern_alloc_init());
		/*
		** order can be restored but not patterndata
		** psycle mfc behaviour
		*/
		psy_undoredo_execute(&self->workspace->undo_redo_,
			&psy_audio_sequenceclearcommand_alloc(self->sequence,
				&self->sequence->selection)->command);		
		psy_audio_exclusivelock_leave();		
	}
}

void sequencecmds_append_track(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);
		psy_audio_exclusivelock_enter();
		psy_audio_sequence_append_track(self->sequence,
			psy_audio_sequencetrack_alloc_init());
		psy_audio_exclusivelock_leave();		
		psy_audio_sequence_set_cursor(self->sequence,
			psy_audio_sequencecursor_make(psy_audio_orderindex_make(
					psy_audio_sequence_width(&self->workspace->song->sequence_) - 1,
					psy_INDEX_INVALID),		
				0, psy_dsp_beatpos_zero()));
	}
}

void sequencecmds_insert_track(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		psy_audio_OrderIndex editpos;		

		sequencecmds_update(self);		
		editpos = psy_audio_sequenceselection_first(
			&self->workspace->song->sequence_.selection);
		psy_audio_exclusivelock_enter();		
		editpos.track = psy_audio_sequence_set_track(self->sequence,
			psy_audio_sequencetrack_alloc_init(), editpos.track);			
		psy_audio_exclusivelock_leave();	
		psy_audio_sequenceselection_select_first(
			&self->workspace->song->sequence_.selection, editpos);
	}
}

void sequencecmds_remove_track(SequenceCmds* self, uintptr_t trackindex)
{
	if (workspace_song(self->workspace)) {
		sequencecmds_update(self);
		if (trackindex != psy_INDEX_INVALID) {
			psy_audio_SequenceCursor cursor;

			psy_audio_exclusivelock_enter();
			psy_audio_sequence_remove_track(self->sequence, trackindex);
			psy_audio_exclusivelock_leave();
			cursor = self->sequence->cursor;
			if (trackindex >= psy_audio_sequence_width(self->sequence)) {
				psy_audio_OrderIndex index;
				
				index = cursor.order_index;
				if (psy_audio_sequence_width(self->sequence) > 0) {
					index.track = psy_audio_sequence_width(self->sequence) - 1;
				} else {
					index.track = psy_INDEX_INVALID;
				}
				if (index.order >= psy_audio_sequence_track_size(self->sequence, trackindex)) {
					if (psy_audio_sequence_track_size(self->sequence, trackindex) > 0) {
						index.order = psy_audio_sequence_track_size(self->sequence, trackindex) - 1;
					} else {
						index.order = psy_INDEX_INVALID;
					}
				}
				psy_audio_sequencecursor_set_order_index(&cursor, index);		
			}	
			psy_audio_sequence_set_cursor(self->sequence, cursor);
		}
	}
}

void sequencecmds_change_play_position(SequenceCmds* self)
{
	if (workspace_song(self->workspace)) {
		psy_audio_SequenceEntry* entry;
		psy_dsp_beatpos_t startposition;

		sequencecmds_update(self);
		entry = psy_audio_sequence_entry(self->sequence,
			psy_audio_sequenceselection_first(
				&self->workspace->song->sequence_.selection));
		psy_audio_exclusivelock_enter();
		psy_audio_sequence_set_play_selection(self->sequence,
			&self->workspace->song->sequence_.selection);
		psy_audio_exclusivelock_leave();
		if (entry) {
			startposition = entry->offset;
			psy_audio_exclusivelock_enter();
			psy_audio_player_stop(&self->workspace->player_);
			psy_audio_player_set_position(&self->workspace->player_,
				psy_dsp_beatpos_real(startposition));
			psy_audio_player_start(&self->workspace->player_);
			psy_audio_exclusivelock_leave();
		}
	}
}
