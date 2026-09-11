/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQEDITORSTATE_H)
#define SEQEDITORSTATE_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_SEQEDITOR

/* host */
#include "sequencehostcmds.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SEQEDIT_PROPERTIESVIEW 1000

typedef enum SeqEditItemType {
	SEQEDITITEM_NONE,
	SEQEDITITEM_PATTERN,
	SEQEDITITEM_SAMPLE,
	SEQEDITITEM_MARKER,
	SEQEDITITEM_TIMESIG,
	SEQEDITITEM_LOOP
} SeqEditItemType;

typedef enum {
	SEQEDIT_DRAGTYPE_UNDEFINED = 0,
	SEQEDIT_DRAGTYPE_MOVE      = 1,
	SEQEDIT_DRAGTYPE_REORDER   = 2
} SeqEditorDragType;

typedef enum {	
	SEQEDIT_DRAG_NONE    = 0,
	SEQEDIT_DRAG_START   = 1,
	SEQEDIT_DRAG_MOVE    = 2,
	SEQEDIT_DRAG_REORDER = 3,
	SEQEDIT_DRAG_LENGTH  = 4,
	SEQEDIT_DRAG_REMOVE  = 5
} SeqEditorDragStatus;

typedef enum {
	SEQEDTCMD_NONE = 0,
	SEQEDTCMD_NEWTRACK = 1,
	SEQEDTCMD_DELTRACK = 2	
} SeqEdtCmd;

typedef struct SeqEditState {
	/* signals */	
	psy_Signal signal_itemselected;
	psy_Signal signal_timesigchanged;
	psy_Signal signal_loopchanged;	
	double pxperbeat;
	double defaultpxperbeat;
	double tracks_scroll_left;
	double propertiesview_desc_column_width;
	psy_ui_Value line_height;
	psy_ui_Value defaultlineheight;	
	psy_dsp_beatpos_t cursorposition;
	bool updatecursorposition;
	bool drawcursor;
	bool cursoractive;
	bool drawpatternevents;	
	bool showpatternnames;
	/* drag */
	SeqEditorDragType dragtype;
	SeqEditorDragStatus dragstatus;	
	psy_dsp_beatpos_t dragposition;
	psy_audio_OrderIndex dragseqpos;	
	SeqEdtCmd cmd;
	psy_audio_SequenceEntryType inserttype;	
	/* references */
	psy_audio_SequenceEntry* seqentry;
	Workspace* workspace;	
	SequenceCmds* cmds;	
	psy_ui_Component* view;
} SeqEditState;

void seqeditstate_init(SeqEditState*, SequenceCmds*, psy_ui_Component* view);
void seqeditstate_dispose(SeqEditState*);

psy_audio_Sequence* seqeditstate_sequence(SeqEditState*);
const psy_audio_Sequence* seqeditstate_sequence_const(const SeqEditState*);
psy_audio_Patterns* seqeditstate_patterns(SeqEditState*);
void seqeditstate_outputstatusposition(SeqEditState*);

INLINE psy_audio_OrderIndex seqeditstate_editposition(const SeqEditState* self)
{	
	if (self->workspace->song) {
		return self->workspace->song->sequence_.cursor.order_index;
	}
	return psy_audio_orderindex_make(0, 0);
}

INLINE double seqeditstate_beat_to_px(const SeqEditState* self,
	psy_dsp_beatpos_t position)
{
	assert(self);

	return floor(psy_dsp_beatpos_real(position) * self->pxperbeat);
}

INLINE psy_ui_Value seqeditstate_preferredwidth(const SeqEditState* self)
{
	assert(self);

	if (seqeditstate_sequence_const(self)) {
		return psy_ui_value_make_px(seqeditstate_beat_to_px(self,
			psy_audio_sequence_duration(seqeditstate_sequence_const(self))) +
			400.0);
	}
	return psy_ui_value_make_px(seqeditstate_beat_to_px(self, 
		psy_dsp_beatpos_make_real(400.0, psy_dsp_DEFAULT_PPQ)));
}

INLINE psy_dsp_beatpos_t seqeditstate_pxtobeat(const
	SeqEditState* self, double px)
{
	assert(self);

	return psy_dsp_beatpos_make_real(px / (double)self->pxperbeat,
		psy_dsp_DEFAULT_PPQ);
}

INLINE void seqeditstate_set_cursor(SeqEditState* self,
	psy_dsp_beatpos_t position)
{
	assert(self);

	position = psy_dsp_beatpos_max(psy_dsp_beatpos_zero(), position);
	if (psy_dsp_beatpos_not_equal(self->cursorposition, position)) {
		self->cursorposition = position;
		seqeditstate_outputstatusposition(self);		
	}
}

psy_dsp_beatpos_t seqeditstate_quantize(const SeqEditState*,
	psy_dsp_beatpos_t position);

INLINE psy_ui_Value seqeditstate_line_height(const SeqEditState* self)
{
	return self->line_height;
}

psy_audio_PatternNode* seqeditstate_node(SeqEditState*, psy_ui_RealPoint,
	psy_dsp_beatpos_t d1, psy_dsp_beatpos_t d2, psy_audio_PatternNode** prev);
psy_audio_Pattern* seqeditstate_globalpattern(SeqEditState*);
void seqeditstate_edit(SeqEditState*, psy_ui_Component* parent,
	psy_ui_RealPoint cp, double width, const char* text);

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_SEQEDITOR */

#endif /* SEQEDITORSTATE_H */
