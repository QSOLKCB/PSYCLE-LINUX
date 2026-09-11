/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"

#include "transformpatternview.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* audio */
#include <sequencecmds.h>
#include <sequencecursor.h>
/* platform */
#include "../../detail/portable.h"

static const char notes[12][3] = {
	"C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-"
};
static const char* empty = "Empty";
static const char* nonempty = "Nonempty";
static const char* all = "All";
static const char* same = "Same";
static const char* off = "off";
static const char* twk = "twk";
static const char* tws = "tws";
static const char* mcm = "mcm";

/* prototypes */
static void transformpatternview_init_search(TransformPatternView*);
static void transformpatternview_init_replace(TransformPatternView*);
static void transformpatternview_init_search_on(TransformPatternView*);
static void transformpatternview_init_actions(TransformPatternView*);
static void transformpatternview_init_selection(TransformPatternView*);
static void transformpatternview_set_selection_mode(TransformPatternView*,
	TransformSelectionMode index);
void transformpatternview_on_selection_mode(TransformPatternView*,
	psy_ui_Button* sender);
static void transformpatternview_on_search(TransformPatternView*);
static void transformpatternview_on_replace(TransformPatternView*);
static void transformpatternview_search(TransformPatternView*,
	bool replace);
static psy_audio_BlockSelection transformpatternview_block(
	const TransformPatternView*);
static psy_audio_PatternSearchReplaceMode transformpatternview_search_pattern(
	TransformPatternView*);

/* implementation */
void transformpatternview_init(TransformPatternView* self, psy_ui_Component*
	parent, PatternViewState* state, Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(transformpatternview_base(self), parent, NULL);
	self->state = state;
	self->workspace = workspace;
	self->selection_mode = 0;	
	psy_ui_margin_init_em(&self->sectionmargin, 0.0, 0.0, 0.0, 2.0);
	psy_ui_component_set_preferred_width(transformpatternview_base(self),
		psy_ui_value_make_ew(35.0));
	closebar_init(&self->close_bar,	transformpatternview_base(self), NULL);
	psy_ui_component_set_default_align(transformpatternview_base(self),
		psy_ui_ALIGN_TOP, psy_ui_margin_make_em(0.0, 0.0, 0.5, 2.0));	
	transformpatternview_init_search(self);
	transformpatternview_init_replace(self);
	transformpatternview_init_search_on(self);
	transformpatternview_init_actions(self);
	transformpatternview_init_selection(self);	
	psy_ui_component_set_align(transformpatternview_base(self),
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_hide(transformpatternview_base(self));
}

void transformpatternview_init_search(TransformPatternView* self)
{
	assert(self);
	
	psy_ui_component_init(&self->search, transformpatternview_base(self), NULL);
	psy_ui_component_set_default_align(&self->search,
		psy_ui_ALIGN_TOP, psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_component_init(&self->searchtop, &self->search, NULL);
	psy_ui_component_set_align(&self->searchtop, psy_ui_ALIGN_TOP);
	/* title */
	psy_ui_label_init(&self->searchtitle, &self->searchtop);
	psy_ui_label_set_text(&self->searchtitle,
		"transformpattern.searchpattern");	
	psy_ui_component_set_align(psy_ui_label_base(&self->searchtitle),
		psy_ui_ALIGN_LEFT);	
	/* Note */
	psy_ui_label_init(&self-> searchnotedesc, &self->search);
	psy_ui_label_set_text(&self->searchnotedesc, "transformpattern.note");
	psy_ui_combobox_init(&self->searchnote, &self->search);	
	/* Inst */
	psy_ui_label_init(&self->searchinstdesc, &self->search);
	psy_ui_label_set_text(&self->searchinstdesc, "transformpattern.instr");
	psy_ui_combobox_init(&self->searchinst, &self->search);
	/* Mach */
	psy_ui_label_init(&self-> searchmachdesc, &self->search);
	psy_ui_label_set_text(&self->searchmachdesc, "transformpattern.mac");	
	psy_ui_combobox_init(&self->searchmach, &self->search);
}

void transformpatternview_init_replace(TransformPatternView* self)
{
	assert(self);

	psy_ui_component_init(&self->replace, transformpatternview_base(self),
		NULL);
	psy_ui_component_set_default_align(&self->replace,
		psy_ui_ALIGN_TOP, psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_label_init(&self->replacetitle, &self->replace);
	psy_ui_label_set_text(&self->replacetitle,
		"transformpattern.replacepattern");	
	psy_ui_component_set_default_align(&self->replace, psy_ui_ALIGN_TOP,
		self->sectionmargin);
	/* Note */
	psy_ui_label_init(&self->replacenotedesc, &self->replace);
	psy_ui_label_set_text(&self->replacenotedesc, "transformpattern.note");
	psy_ui_combobox_init(&self->replacenote, &self->replace);
	/* Inst */
	psy_ui_label_init(&self->replaceinstdesc, &self->replace);
	psy_ui_label_set_text(&self->replaceinstdesc, "transformpattern.instr");
	psy_ui_combobox_init(&self->replaceinst, &self->replace);
	/* Mach */
	psy_ui_label_init(&self->replacemachdesc, &self->replace);
	psy_ui_label_set_text(&self->replacemachdesc, "transformpattern.mac");
	psy_ui_combobox_init(&self->replacemach, &self->replace);
}

void transformpatternview_init_search_on(TransformPatternView* self)
{
	assert(self);

	psy_ui_component_init(&self->searchon, transformpatternview_base(self),
		NULL);
	psy_ui_component_set_default_align(&self->searchon,
		psy_ui_ALIGN_TOP, psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_label_init_text(&self->searchontitle, &self->searchon,
		"transformpattern.searchon");
	psy_ui_component_init(&self->searchonchoice, &self->searchon, NULL);	
	psy_ui_component_set_default_align(&self->searchonchoice,
		psy_ui_ALIGN_TOP, self->sectionmargin);
	psy_ui_button_init_text_connect(&self->entire, &self->searchonchoice,
		"transformpattern.entiresong",
		self, transformpatternview_on_selection_mode);	
	psy_ui_component_set_id(psy_ui_button_base(&self->entire),
		TRANSFORM_ALL);
	psy_ui_button_set_text_alignment(&self->entire,
		psy_ui_ALIGNMENT_LEFT | psy_ui_ALIGNMENT_CENTER_VERTICAL);
	psy_ui_button_init_text_connect(&self->currpattern, &self->searchonchoice,
		"transformpattern.currentpattern",
		self, transformpatternview_on_selection_mode);
	psy_ui_component_set_id(psy_ui_button_base(&self->currpattern),
		TRANSFORM_PATTERN);
	psy_ui_button_set_text_alignment(&self->currpattern,
		psy_ui_ALIGNMENT_LEFT | psy_ui_ALIGNMENT_CENTER_VERTICAL);
	psy_ui_button_init_text_connect(&self->currselection, &self->searchonchoice,
		"transformpattern.currentselection",
		self, transformpatternview_on_selection_mode);
	psy_ui_button_set_text_alignment(&self->currselection,
		psy_ui_ALIGNMENT_LEFT | psy_ui_ALIGNMENT_CENTER_VERTICAL);
	psy_ui_component_set_id(psy_ui_button_base(&self->currselection),
		TRANSFORM_SELECTION);
	transformpatternview_set_selection_mode(self, TRANSFORM_PATTERN);
}

void transformpatternview_init_actions(TransformPatternView* self)
{
	assert(self);

	psy_ui_component_init(&self->actions, transformpatternview_base(self),
		NULL);	
	psy_ui_component_set_margin(&self->actions, psy_ui_margin_make_em(
		1.0, 0.0, 0.0, 0.0));	
	psy_ui_component_set_default_align(&self->actions, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_align_expand(&self->actions, psy_ui_HEXPAND);	
	psy_ui_button_init_text_connect(&self->dosearch, &self->actions,
		"transformpattern.search", self, transformpatternview_on_search);	
	psy_ui_button_init_text_connect(&self->doreplace, &self->actions,
		"transformpattern.replaceall", self, transformpatternview_on_replace);				
}

void transformpatternview_init_selection(TransformPatternView* self)
{
	int i;
	bool is440;

	assert(self);

	// init maps
	//Note (search and replace)
	psy_ui_combobox_add_text(&self->searchnote, all);
	psy_ui_combobox_set_item_id(&self->searchnote, 0,
		psy_audio_PATTERNSEARCHSELECTOR_ALL);
	psy_ui_combobox_add_text(&self->searchnote, empty);
		psy_ui_combobox_set_item_id(&self->searchnote, 1,
		psy_audio_PATTERNSEARCHSELECTOR_EMPTY);
	psy_ui_combobox_add_text(&self->searchnote, nonempty);
		psy_ui_combobox_set_item_id(&self->searchnote, 2,
		psy_audio_PATTERNSEARCHSELECTOR_NON_EMPTY);
	psy_ui_combobox_add_text(&self->replacenote, same);
		psy_ui_combobox_set_item_id(&self->replacenote, 0,
			psy_audio_PATTERNREPLACESELECTOR_KEEP);
	psy_ui_combobox_add_text(&self->replacenote, empty);
		psy_ui_combobox_set_item_id(&self->replacenote, 1,
		psy_audio_PATTERNREPLACESELECTOR_SET_EMPTY);	
	is440 = (psy_configuration_value_int(
		psycleconfig_patview(workspace_cfg(self->workspace)),
		"notetab", 0) == 0);	
	for (i = psy_audio_NOTECOMMANDS_C0; i <= psy_audio_NOTECOMMANDS_B9; i++) {
		char text[256];

		psy_snprintf(text, 256, "%s%d", notes[i % 12],
			(i / 12) + ((is440) ? - 1 : 0));
		psy_ui_combobox_add_text(&self->searchnote, text);
		psy_ui_combobox_set_item_id(&self->searchnote, 3 + i, i);
		psy_ui_combobox_add_text(&self->replacenote, text);
		psy_ui_combobox_set_item_id(&self->replacenote, 2 + i, i);
	}
	psy_ui_combobox_add_text(&self->searchnote, off);
		psy_ui_combobox_set_item_id(&self->searchnote, 123,
			psy_audio_NOTECOMMANDS_RELEASE);
	psy_ui_combobox_add_text(&self->searchnote, twk);
		psy_ui_combobox_set_item_id(&self->searchnote, 124,
			psy_audio_NOTECOMMANDS_TWEAK);
	psy_ui_combobox_add_text(&self->searchnote, tws);
		psy_ui_combobox_set_item_id(&self->searchnote, 125,
			psy_audio_NOTECOMMANDS_TWEAK_SLIDE);
	psy_ui_combobox_add_text(&self->searchnote, mcm);
		psy_ui_combobox_set_item_id(&self->searchnote, 126,
			psy_audio_NOTECOMMANDS_MIDICC);
	psy_ui_combobox_add_text(&self->replacenote, off);
		psy_ui_combobox_set_item_id(&self->replacenote, 122,
			psy_audio_NOTECOMMANDS_RELEASE);
	psy_ui_combobox_add_text(&self->replacenote, twk);
		psy_ui_combobox_set_item_id(&self->replacenote, 123,
			psy_audio_NOTECOMMANDS_TWEAK);
	psy_ui_combobox_add_text(&self->replacenote, tws);
		psy_ui_combobox_set_item_id(&self->replacenote, 124,
			psy_audio_NOTECOMMANDS_TWEAK_SLIDE);
	psy_ui_combobox_add_text(&self->replacenote, mcm);
		psy_ui_combobox_set_item_id(&self->replacenote, 125,
			psy_audio_NOTECOMMANDS_MIDICC);

	psy_ui_combobox_select(&self->searchnote, 0);
	psy_ui_combobox_select(&self->replacenote, 0);

	/* Inst(search and replace) */
	psy_ui_combobox_add_text(&self->searchinst, all);
	psy_ui_combobox_set_item_id(&self->searchinst, 0,
		psy_audio_PATTERNSEARCHSELECTOR_ALL);
	psy_ui_combobox_add_text(&self->searchinst, empty);
	psy_ui_combobox_set_item_id(&self->searchinst, 1,
		psy_audio_PATTERNSEARCHSELECTOR_EMPTY);
	psy_ui_combobox_add_text(&self->searchinst, nonempty);
	psy_ui_combobox_set_item_id(&self->searchinst, 2,
		psy_audio_PATTERNSEARCHSELECTOR_NON_EMPTY);
	psy_ui_combobox_add_text(&self->replaceinst, same);
	psy_ui_combobox_set_item_id(&self->replaceinst, 0,
		psy_audio_PATTERNREPLACESELECTOR_KEEP);
	psy_ui_combobox_add_text(&self->replaceinst, empty);
	psy_ui_combobox_set_item_id(&self->replaceinst, 1,
		psy_audio_PATTERNREPLACESELECTOR_SET_EMPTY);
	for (i = 0; i < 0xFF; i++) {
		char text[256];
		
		psy_snprintf(text, 256, "%02X", i);				
		psy_ui_combobox_add_text(&self->searchinst, text);
		psy_ui_combobox_set_item_id(&self->searchinst, 3 + i, i);
		psy_ui_combobox_add_text(&self->replaceinst, text);
		psy_ui_combobox_set_item_id(&self->replaceinst, 2 + i, i);
	}
	psy_ui_combobox_select(&self->searchinst, 0);
	psy_ui_combobox_select(&self->replaceinst, 0);

	/* Mach(search and replace) */
	psy_ui_combobox_add_text(&self->searchmach, all);
	psy_ui_combobox_set_item_id(&self->searchmach, 0,
		psy_audio_PATTERNSEARCHSELECTOR_ALL);
	psy_ui_combobox_add_text(&self->searchmach, empty);
	psy_ui_combobox_set_item_id(&self->searchmach, 1,
		psy_audio_PATTERNSEARCHSELECTOR_EMPTY);
	psy_ui_combobox_add_text(&self->searchmach, nonempty);
	psy_ui_combobox_set_item_id(&self->searchmach, 2,
		psy_audio_PATTERNSEARCHSELECTOR_NON_EMPTY);
	psy_ui_combobox_add_text(&self->replacemach, same);
	psy_ui_combobox_set_item_id(&self->replacemach, 0,
		psy_audio_PATTERNREPLACESELECTOR_KEEP);
	psy_ui_combobox_add_text(&self->replacemach, empty);
	psy_ui_combobox_set_item_id(&self->replacemach, 1,
		psy_audio_PATTERNREPLACESELECTOR_SET_EMPTY);
	for (i = 0; i < 0xFF; ++i) {
		char text[256];

		psy_snprintf(text, 256, "%02X", i);
		psy_ui_combobox_add_text(&self->searchmach, text);
		psy_ui_combobox_set_item_id(&self->searchmach, 3 + i, i);
		psy_ui_combobox_add_text(&self->replacemach, text);
		psy_ui_combobox_set_item_id(&self->replacemach, 2 + i, i);
	}
	psy_ui_combobox_select(&self->searchmach, 0);
	psy_ui_combobox_select(&self->replacemach, 0);
}

void transformpatternview_set_selection_mode(TransformPatternView* self,
	TransformSelectionMode index)
{
	assert(self);
	
	psy_ui_component_remove_style_state(
		psy_ui_button_base(&self->entire),
		psy_ui_STYLESTATE_SELECT);		
	psy_ui_component_remove_style_state(
		psy_ui_button_base(&self->currpattern),
		psy_ui_STYLESTATE_SELECT);		
	psy_ui_component_remove_style_state(
		psy_ui_button_base(&self->currselection),
		psy_ui_STYLESTATE_SELECT);
	switch (index)  {
	case TRANSFORM_ALL:
		psy_ui_component_add_style_state(
			psy_ui_button_base(&self->entire),
			psy_ui_STYLESTATE_SELECT);
		break;
	case TRANSFORM_PATTERN:
		psy_ui_component_add_style_state(
			psy_ui_button_base(&self->currpattern),
			psy_ui_STYLESTATE_SELECT);
		break;
	case TRANSFORM_SELECTION:
		psy_ui_component_add_style_state(
			psy_ui_button_base(&self->currselection),
			psy_ui_STYLESTATE_SELECT);
		break;
	default:
		break;
	}	
}

void transformpatternview_on_selection_mode(TransformPatternView* self,
	psy_ui_Button* sender)
{
	assert(self);
	
	transformpatternview_set_selection_mode(self, psy_ui_component_id(
		psy_ui_button_base(sender)));
}

void transformpatternview_on_search(TransformPatternView* self)
{
	transformpatternview_search(self, FALSE);
}

void transformpatternview_on_replace(TransformPatternView* self)
{
	transformpatternview_search(self, TRUE);
}

void transformpatternview_search(TransformPatternView* self,
	bool replace)
{	
	psy_audio_BlockSelection block;
		
	assert(self);
		
	block = transformpatternview_block(self);
	if (psy_audio_blockselection_valid(&block)) {
		psy_audio_Sequence* sequence;
		psy_audio_PatternSearchCmd search;
		psy_audio_SequenceCursor cursor;
			
		sequence = patternviewstate_sequence(self->state);
		if (!sequence) {
			return;
		}		
		psy_audio_patternsearchcmd_init(&search, 
			transformpatternview_search_pattern(self),
			replace);		
		psy_audio_sequence_block_traverse(sequence, block,
			psy_audio_patternsearchcmd_base(&search));
		cursor = psy_audio_patternsearchcmd_result(&search);		
		if (psy_audio_sequencecursor_valid(&cursor)) {
			psy_audio_sequence_set_cursor(sequence, cursor);				
		}
		if (replace) {
			patternviewstate_invalidate(self->state);
		}		
	}	
}

psy_audio_BlockSelection transformpatternview_block(
	const TransformPatternView* self)
{	
	psy_audio_Sequence* sequence;
	
	assert(self);
	
	sequence = patternviewstate_sequence(self->state);
	if (!sequence) {
		return psy_audio_blockselection_zero();
	}		
	switch (self->selection_mode) {
	case TRANSFORM_ALL:
		return psy_audio_sequence_block_selection_track(sequence,
			self->state->cursor.channel_);
	case TRANSFORM_PATTERN:
		return psy_audio_sequence_block_selection_order(sequence,
			self->state->cursor.order_index);				
	case TRANSFORM_SELECTION:
		return self->state->selection.block;		
	default:
		return psy_audio_blockselection_zero();		
	}
}

psy_audio_PatternSearchReplaceMode transformpatternview_search_pattern(
	TransformPatternView* self)
{
	assert(self);
	
	return  psy_audio_patternsearchreplacemode_make(
		(int)psy_ui_combobox_item_id(&self->searchnote,
			psy_ui_combobox_cur_sel(&self->searchnote)),
		(int)psy_ui_combobox_item_id(&self->searchinst,
			psy_ui_combobox_cur_sel(&self->searchinst)),
		(int)psy_ui_combobox_item_id(&self->searchmach,
			psy_ui_combobox_cur_sel(&self->searchmach)),
		(int)psy_ui_combobox_item_id(&self->replacenote,
			psy_ui_combobox_cur_sel(&self->replacenote)),
		(int)psy_ui_combobox_item_id(&self->replaceinst,
			psy_ui_combobox_cur_sel(&self->replaceinst)),
		(int)psy_ui_combobox_item_id(&self->replacemach,
			psy_ui_combobox_cur_sel(&self->replacemach)),
		FALSE);					
}

#endif /* PSYCLE_USE_PATTERN_VIEW */
