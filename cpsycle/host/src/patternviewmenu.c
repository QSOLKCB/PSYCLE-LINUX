/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewmenu.h"

#ifdef PSYCLE_USE_PATTERN_VIEW

/* host */
#include "cmdsgeneral.h"
#include "styles.h"
/* ui */
#include <uiapp.h>
#include <uibutton.h>
#include <trackercmds.h>


/* prototypes */
static psy_ui_Component* patternmenu_add(PatternMenu*, psy_ui_Component* parent,
	const char* label, intptr_t cmd_id);
static void patternmenu_on_cmd(PatternMenu*, psy_ui_Component* sender);
static void paragraph(psy_ui_Component* c);

/* implementation */
void patternmenu_init(PatternMenu* self, psy_ui_Component* parent,
	psy_ui_Component* pattern, psy_audio_Player* player)
{		
	assert(self);		

	psy_ui_scroller_init(&self->scroller, parent, NULL, NULL);	
	psy_ui_component_set_align(patternmenu_base(self), psy_ui_ALIGN_RIGHT);
	psy_ui_component_set_style_type(patternmenu_base(self), STYLE_SIDE_VIEW);
	psy_ui_component_hide(patternmenu_base(self));
	psy_ui_component_init(&self->pane, &self->scroller.pane, NULL);	
	psy_ui_component_set_overflow(&self->pane, psy_ui_OVERFLOW_VSCROLL);	
	psy_ui_component_set_scroll_step(&self->pane,
		psy_ui_size_make_em(0.0, 1.0));		
	psy_ui_scroller_set_client(&self->scroller, &self->pane);		
	psy_ui_component_set_align(&self->pane, psy_ui_ALIGN_FIXED);	
		
	psy_ui_component_set_default_align(&self->pane, psy_ui_ALIGN_TOP,		
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 2.0));
	patternmenu_add(self, &self->pane, "<", CMD_PATTERN_MENU);
	self->first = patternmenu_add(self, &self->pane, "edit.cut", CMD_BLOCKCUT);
	patternmenu_add(self, &self->pane, "edit.copy", CMD_BLOCKCOPY);
	patternmenu_add(self, &self->pane, "edit.paste", CMD_BLOCKPASTE);
	patternmenu_add(self, &self->pane, "edit.mixpaste", CMD_BLOCKMIX);
	patternmenu_add(self, &self->pane, "edit.delete", CMD_BLOCKDELETE);	
	paragraph(patternmenu_add(self, &self->pane,
		"edit.searchreplace", CMD_BLOCKTRANSFORM));		
	paragraph(patternmenu_add(self, &self->pane,
		"edit.blockinterpolate", CMD_BLOCKINTERPOLATE));
	patternmenu_add(self, &self->pane,
		"edit.blockinterpolatecurve", CMD_BLOCKINTERPOLATECURVE);	
	patternmenu_add(self, &self->pane,
		"edit.blocksetmachine", CMD_BLOCKSETMACHINE);
	patternmenu_add(self, &self->pane,
		"edit.blocksetinstr", CMD_BLOCKSETINSTR);		
	psy_ui_label_init_text(&self->note_transposition, &self->pane,
		"edit.transpose");
	psy_ui_component_set_margin(psy_ui_label_base(&self->note_transposition),
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 3.0));
	psy_ui_component_init(&self->transpose, &self->pane, NULL);
	psy_ui_component_set_margin(&self->transpose,
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 2.0));
	psy_ui_component_set_align_expand(&self->transpose, psy_ui_HEXPAND);	
	psy_ui_component_set_default_align(&self->transpose, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));	
	patternmenu_add(self, &self->transpose,
		"edit.transposeblockdec", CMD_TRANSPOSEBLOCKDEC);
	patternmenu_add(self, &self->transpose,
		"edit.transposeblockinc", CMD_TRANSPOSEBLOCKINC);
	patternmenu_add(self, &self->transpose,
		"edit.transposeblockdec12", CMD_TRANSPOSEBLOCKDEC12);
	patternmenu_add(self, &self->transpose,
		"edit.transposeblockinc12", CMD_TRANSPOSEBLOCKINC12);		
	paragraph(patternmenu_add(self, &self->pane,
		"edit.blockswingfill", CMD_BLOCKSWINGFILL));
	patternmenu_add(self, &self->pane,
		"edit.trackswingfill", CMD_BLOCKTRKSWINGFILL);		
	paragraph(patternmenu_add(self, &self->pane,
		"edit.importpsb", CMD_PATTERN_IMPORT));
	patternmenu_add(self, &self->pane,
		"edit.exportpsb", CMD_PATTERN_EXPORT);	
	paragraph(patternmenu_add(self, &self->pane,
		"edit.properties", CMD_IMM_INFOPATTERN));
	patternmenu_add(self, &self->pane,
		"edit.tracknames", CMD_IMM_INFOTRACKNAMES);
	noteselect_init(&self->note_select_, &self->pane, pattern, player);
	psy_ui_component_set_tab_index(noteselect_base(&self->note_select_), 0);
}

void patternmenu_focus_first(PatternMenu* self)
{
	assert(self);

	if (self->first) {
		psy_ui_component_set_focus(self->first);
	}
}

psy_ui_Component* patternmenu_add(PatternMenu* self, psy_ui_Component* parent,
	const char* label, intptr_t cmd_id)
{
	psy_ui_Button* rv;
	
	assert(self);
	
	rv = psy_ui_button_alloc_init(parent);
	if (rv) {
		psy_ui_button_set_text(rv, label);
		psy_ui_button_connect(rv, self, patternmenu_on_cmd);
		psy_ui_component_set_tab_index(psy_ui_button_base(rv), 0);
		psy_ui_component_set_id(psy_ui_button_base(rv), cmd_id);
		psy_ui_button_set_text_alignment(rv,
			psy_ui_ALIGNMENT_CENTER_VERTICAL | psy_ui_ALIGNMENT_LEFT);
		psy_ui_component_prevent_app_focus_out(psy_ui_button_base(rv));
		return psy_ui_button_base(rv);
	}
	return NULL;
}

void patternmenu_on_cmd(PatternMenu* self, psy_ui_Component* sender)
{
	psy_EventDriverCmd cmd;
	const char*	section;
	
	assert(self);
	
	cmd = psy_eventdrivercmd_make_cmd(psy_ui_component_id(sender));	
	if (cmd.id == CMD_IMM_INFOPATTERN ||
		cmd.id == CMD_IMM_INFOTRACKNAMES) {
		section = "general";
	} else {
		section = "edit";
	}
	inputhandler_send(psy_ui_app_input_handler(psy_ui_app()), section, cmd);
	if (cmd.id == CMD_BLOCKINTERPOLATECURVE) {
		psy_ui_component_hide_align(patternmenu_base(self));
		psy_ui_component_invalidate(psy_ui_component_parent(
			patternmenu_base(self)));
	}
}

void paragraph(psy_ui_Component* c)
{
	assert(c);
	
	psy_ui_component_set_margin(c, psy_ui_margin_make_em(1.0, 0.0, 0.0, 2.0));	
}

#endif /* PSYCLE_USE_PATTERN_VIEW */
