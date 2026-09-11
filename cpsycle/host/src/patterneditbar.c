/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patterneditbar.h"
/* host */
#include "styles.h"


/* prototypes */
static void patterneditbar_init_button(PatternEditBar*, psy_ui_Button*,
	const char* svg, uintptr_t mode);
static void patterneditbar_on_set_duration(PatternEditBar*,
	psy_ui_Button* sender);
static void patterneditbar_on_set_select(PatternEditBar*,
	psy_ui_Button* sender);
static psy_dsp_beatpos_t patterneditbar_duration(const PatternEditBar*,
	PatternEditorNoteMode);
 static PatternEditorNoteMode patterneditbar_mode(const PatternEditBar*,
	psy_dsp_beatpos_t duration);

/* implementation */
void patterneditbar_init(PatternEditBar* self, psy_ui_Component* parent,
	PianoGridState* gridstate, PatternViewState* state, Workspace* workspace)
{
	assert(self);
	assert(state);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	self->gridstate = gridstate;
	self->state = state;	
	self->workspace_ = workspace;	
	psy_ui_component_set_default_align(patterneditbar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_margin_zero());
	psy_ui_component_set_align_expand(patterneditbar_base(self), psy_ui_HEXPAND);
	patterneditbar_init_button(self, &self->endless, "img.endless",
		PATTERN_EDT_NOTE_MODE_ENDLESS);
	patterneditbar_init_button(self, &self->semibreve, "img.semi-breve",
		PATTERN_EDT_NOTE_MODE_SEMI_BREVE);
	patterneditbar_init_button(self, &self->minim, "img.minim",
		PATTERN_EDT_NOTE_MODE_MINIM);
	patterneditbar_init_button(self, &self->minim_dot, "img.minim-dot",
		PATTERN_EDT_NOTE_MODE_MINIM_DOT);
	patterneditbar_init_button(self, &self->crotchet, "img.crotchet",
		PATTERN_EDT_NOTE_MODE_CROTCHET);
	patterneditbar_init_button(self, &self->crotchet_dot, "img.crotchet-dot",
		PATTERN_EDT_NOTE_MODE_CROTCHET_DOT);
	patterneditbar_init_button(self, &self->icon_quaver, "img.quaver",
		PATTERN_EDT_NOTE_MODE_QUAVER);
	patterneditbar_init_button(self, &self->quaver_dot, "img.quaver-dot",
		PATTERN_EDT_NOTE_MODE_QUAVER_DOT);
	patterneditbar_init_button(self, &self->semiquaver, "img.semi-quaver",
		PATTERN_EDT_NOTE_MODE_SEMI_QUAVER);
	patterneditbar_init_button(self, &self->select, "img.select",
		PATTERN_EDT_NOTE_MODE_SELECT);
	patterneditbar_update_selection(self);
}

void patterneditbar_init_button(PatternEditBar* self, psy_ui_Button* button,
	const char* svg, uintptr_t mode)
{
	assert(self);

	psy_ui_button_init_resource_connect(button, patterneditbar_base(self),
		svg, self, patterneditbar_on_set_duration);	
	psy_ui_component_set_id(psy_ui_button_base(button), mode);
}

void patterneditbar_on_set_duration(PatternEditBar* self,
	psy_ui_Button* sender)
{
	assert(self);
					
	self->state->insert_duration = patterneditbar_duration(self, psy_ui_component_id(
		psy_ui_button_base(sender)));
	self->gridstate->select_mode = FALSE;
	patterneditbar_update_selection(self);
}

void patterneditbar_on_set_select(PatternEditBar* self,
	psy_ui_Button* sender)
{
	assert(self);
	
	self->gridstate->select_mode = TRUE;	
	patterneditbar_update_selection(self);
	psy_ui_button_highlight(sender);
}

psy_dsp_beatpos_t patterneditbar_duration(const PatternEditBar* self,
	PatternEditorNoteMode mode)
{
	assert(self);
	
	switch (mode) {
	case PATTERN_EDT_NOTE_MODE_ENDLESS:
		return psy_dsp_beatpos_zero();		
	case PATTERN_EDT_NOTE_MODE_SEMI_BREVE:
		return psy_dsp_beatpos_make_real(4.0, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_MINIM:
		return psy_dsp_beatpos_make_real(2.0, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_MINIM_DOT:
		return psy_dsp_beatpos_make_real(3.0, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_CROTCHET:
		return psy_dsp_beatpos_make_real(1.0, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_CROTCHET_DOT:
		return psy_dsp_beatpos_make_real(1.5, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_QUAVER:
		return psy_dsp_beatpos_make_real(0.5, psy_dsp_DEFAULT_PPQ);		
	case PATTERN_EDT_NOTE_MODE_QUAVER_DOT:
		return psy_dsp_beatpos_make_real(0.75, psy_dsp_DEFAULT_PPQ);
	case PATTERN_EDT_NOTE_MODE_SEMI_QUAVER:
		return psy_dsp_beatpos_make_real(0.25, psy_dsp_DEFAULT_PPQ);		
	default:
		return psy_dsp_beatpos_zero();		
	}
}

PatternEditorNoteMode patterneditbar_mode(const PatternEditBar* self,
	psy_dsp_beatpos_t duration)
{
	assert(self);
		
	if (psy_dsp_beatpos_equal(duration, psy_dsp_beatpos_zero())) {
		return PATTERN_EDT_NOTE_MODE_ENDLESS;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(4.0, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_SEMI_BREVE;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(2.0, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_MINIM;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(3.0, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_MINIM_DOT;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(1.0, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_CROTCHET;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(1.5, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_CROTCHET_DOT;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(0.5, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_QUAVER;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(0.75, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_QUAVER_DOT;
	} else if (psy_dsp_beatpos_equal(duration, 
			psy_dsp_beatpos_make_real(0.25, psy_dsp_DEFAULT_PPQ))) {
		return PATTERN_EDT_NOTE_MODE_SEMI_QUAVER;
	}
	return PATTERN_EDT_NOTE_MODE_ENDLESS;
}

void patterneditbar_update_selection(PatternEditBar* self)	
{
	PatternEditorNoteMode curr_mode;
	psy_ui_Button* buttons[] = {
		&self->endless,
		&self->semibreve,
		&self->minim,
		&self->minim_dot,
		&self->crotchet,
		&self->crotchet_dot,
		&self->icon_quaver,
		&self->quaver_dot,
		&self->semiquaver,
		&self->select,
		NULL
	};
	uintptr_t i;
	
	assert(self);
		
	curr_mode = patterneditbar_mode(self, self->state->insert_duration);
	for (i = 0; buttons[i] != NULL; ++i) {
		psy_ui_Button* button;
		
		button = buttons[i];
		if (curr_mode == psy_ui_component_id(psy_ui_button_base(button))) {
			psy_ui_button_highlight(button);
		} else {
		 	psy_ui_button_disable_highlight(button);
		}
	}
}
