/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEQLISTVIEW_H)
#define SEQLISTVIEW_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_SEQVIEW

/* host */
#include "seqviewstate.h"
#include "workspace.h"
/* ui */
#include <uicomponent.h>

#ifdef __cplusplus
extern "C" {
#endif

struct SeqView;

/* SeqViewTrack */

typedef struct SeqViewTrack {
	/*! @extends  */
	psy_ui_Component component;
	uintptr_t trackindex;
	/* references */
	SeqViewState* state;
	psy_audio_SequenceTrack* track;
} SeqViewTrack;

void seqviewtrack_init(SeqViewTrack*, psy_ui_Component* parent,
	uintptr_t trackindex, SeqViewState*);

SeqViewTrack* seqviewtrack_alloc(void);
SeqViewTrack* seqviewtrack_allocinit(psy_ui_Component* parent,
	uintptr_t trackindex, SeqViewState*);

/* SeqViewItem */

typedef struct SeqViewItem {
	/*! @extends  */
	psy_ui_Component component;	
	psy_audio_OrderIndex order_index;
	/* references */
	psy_audio_SequenceEntry* seqentry;	
	psy_audio_Sequencer* sequencer;
	SeqViewState* state;
} SeqViewItem;

void seqviewitem_init(SeqViewItem*, psy_ui_Component* parent,
	psy_audio_SequenceEntry*, psy_audio_OrderIndex, SeqViewState*);

SeqViewItem* seqviewitem_alloc(void);
SeqViewItem* seqviewitem_allocinit(psy_ui_Component* parent,
	psy_audio_SequenceEntry*, psy_audio_OrderIndex, SeqViewState*);

/* SeqviewList */

typedef struct SeqviewList {
	/*! @extends  */
	psy_ui_Component component;	
	/* references */
	SeqViewState* state;
	Workspace* workspace;
} SeqviewList;

void seqviewlist_init(SeqviewList*, psy_ui_Component* parent, SeqViewState*,
	Workspace*);

void seqviewlist_rename(SeqviewList*);
void seqviewlist_build(SeqviewList*);
void seqviewlist_on_pattern_name_changed(SeqviewList*, psy_audio_Patterns*,
	uintptr_t slot);
void seqviewlist_set_song(SeqviewList*, psy_audio_Song*);
uintptr_t seqviewlist_visible_lines(const SeqviewList*);
void seqviewlist_set_top_order(SeqviewList*, uintptr_t order);
uintptr_t seqviewlist_top_order(const SeqviewList*);
void seqlistview_on_sequence_select(SeqviewList*,
	psy_audio_SequenceSelection* sender, psy_audio_OrderIndex*);

INLINE psy_ui_Component* seqviewlist_base(SeqviewList* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_SEQVIEW */

#endif /* SEQLISTVIEW_H */
