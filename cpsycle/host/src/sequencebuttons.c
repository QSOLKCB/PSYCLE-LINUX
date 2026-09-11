/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequencebuttons.h"
/* host */
#include "styles.h"


/* prototypes */
static void sequencebuttons_on_more(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_new_entry(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_insert_entry(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_clone_entry(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_del_entry(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_inc_pattern(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_dec_pattern(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_copy(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_onpaste(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_clear(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_rename(SequenceButtons*,
	psy_ui_Button* sender);
static void sequencebuttons_on_edit_accept(SequenceButtons*,
	psy_ui_TextArea* sender);
static void sequencebuttons_on_edit_reject(SequenceButtons*,
	psy_ui_TextArea* sender);
static void sequencebuttons_add(SequenceButtons*, psy_ui_Button*,
	psy_ui_Component* parent, const char* text);

/* implementation */
void sequencebuttons_init(SequenceButtons* self, psy_ui_Component* parent,
	SequenceCmds* cmds)
{
	assert(self);

	self->cmds = cmds;
	self->restore_focus = NULL;
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_init(&self->standard, &self->component, NULL);
	psy_ui_component_set_align(&self->standard, psy_ui_ALIGN_TOP);
	psy_ui_component_prevent_app_focus_out(&self->component);
	/* row 0 */
	psy_ui_component_init(&self->row0, &self->standard, NULL);
	psy_ui_component_set_align(&self->row0, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->row0, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_prevent_app_focus_out(&self->row0);
	psy_ui_component_set_margin(&self->row0,
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));

	sequencebuttons_add(self, &self->incpattern, &self->row0, "+");
	sequencebuttons_add(self, &self->insertentry, &self->row0, "seqview.ins");
	sequencebuttons_add(self, &self->decpattern, &self->row0, "-");
	/* row 1 */
	psy_ui_component_init(&self->row1, &self->standard, NULL);
	psy_ui_component_prevent_app_focus_out(&self->row1);
	psy_ui_component_set_margin(&self->row1,
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));
	psy_ui_component_set_align(&self->row1, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->row1, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));

	sequencebuttons_add(self, &self->newentry, &self->row1, "seqview.new");
	sequencebuttons_add(self, &self->cloneentry, &self->row1, "seqview.clone");
	sequencebuttons_add(self, &self->delentry, &self->row1, "seqview.del");
	/* more/less */
	psy_ui_component_init(&self->rowmore, &self->standard, NULL);
	psy_ui_component_set_margin(&self->rowmore,
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));
	psy_ui_component_set_align(&self->rowmore, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->rowmore, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_prevent_app_focus_out(&self->rowmore);

	psy_ui_button_init(&self->moreless, &self->rowmore);
	psy_ui_button_set_svg(&self->moreless, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
	psy_ui_button_set_text(&self->moreless, "seqview.more");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->moreless), 0);
	/* more block */
	psy_ui_component_init(&self->block, &self->component, NULL);
	psy_ui_component_hide(&self->block);
	psy_ui_component_prevent_app_focus_out(&self->block);
	psy_ui_component_set_align(&self->block, psy_ui_ALIGN_TOP);
	psy_ui_component_set_padding(&self->block,
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));
	psy_ui_component_init(&self->row2, &self->block, NULL);
	psy_ui_component_prevent_app_focus_out(&self->row2);
	psy_ui_component_set_margin(&self->row2,
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 0.0));
	psy_ui_component_set_align(&self->row2, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->row2, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	/* clear rename copy */
	sequencebuttons_add(self, &self->clear, &self->row2, "seqview.clear");
	sequencebuttons_add(self, &self->rename, &self->row2, "seqview.rename");
	psy_ui_component_enable_app_focus_out(psy_ui_button_base(&self->rename));
	sequencebuttons_add(self, &self->copy, &self->row2, "seqview.copy");
	/* rename edit */
	psy_ui_edit_init(&self->edit, &self->component);
	psy_ui_component_set_margin(psy_ui_edit_base(&self->edit),
		psy_ui_margin_make_em(0.5, 0.0, 1.0, 0.0));
	psy_ui_component_set_align(psy_ui_edit_base(&self->edit),
		psy_ui_ALIGN_TOP);
	psy_ui_component_hide(psy_ui_edit_base(&self->edit));
	/* row3 */
	psy_ui_component_init(&self->row3, &self->block, NULL);
	psy_ui_component_prevent_app_focus_out(&self->row3);
	psy_ui_component_set_align(&self->row3, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->row3, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	sequencebuttons_add(self, &self->paste, &self->row3, "seqview.paste");
	
	psy_signal_connect(&self->newentry.signal_clicked, self,
		sequencebuttons_on_new_entry);
	psy_signal_connect(&self->insertentry.signal_clicked, self,
		sequencebuttons_on_insert_entry);
	psy_signal_connect(&self->cloneentry.signal_clicked, self,
		sequencebuttons_on_clone_entry);
	psy_signal_connect(&self->delentry.signal_clicked, self,
		sequencebuttons_on_del_entry);
	psy_signal_connect(&self->incpattern.signal_clicked, self,
		sequencebuttons_on_inc_pattern);
	psy_signal_connect(&self->decpattern.signal_clicked, self,
		sequencebuttons_on_dec_pattern);
	psy_signal_connect(&self->copy.signal_clicked, self,
		sequencebuttons_on_copy);
	psy_signal_connect(&self->paste.signal_clicked, self,
		sequencebuttons_onpaste);
	psy_signal_connect(&self->clear.signal_clicked, self,
		sequencebuttons_on_clear);
	psy_signal_connect(&self->moreless.signal_clicked, self,
		sequencebuttons_on_more);
	psy_signal_connect(&self->edit.signal_accept, self,
		sequencebuttons_on_edit_accept);
	psy_signal_connect(&self->edit.signal_reject, self,
		sequencebuttons_on_edit_reject);
	psy_signal_connect(&self->rename.signal_clicked, self,
		sequencebuttons_on_rename);
}

void sequencebuttons_add(SequenceButtons* self,
	psy_ui_Button* button,
	psy_ui_Component* parent,
	const char* text)
{
	assert(self);

	psy_ui_button_init_text(button, parent, text);
	psy_ui_component_set_tab_index(psy_ui_button_base(button), 0);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(button));
	psy_ui_button_set_char_number(button, 12.0);
	psy_ui_component_set_style_types(psy_ui_button_base(button),
		STYLE_SEQVIEW_BUTTON, STYLE_SEQVIEW_BUTTON_HOVER,
		STYLE_SEQVIEW_BUTTON_SELECT, psy_INDEX_INVALID);
}

void sequencebuttons_on_more(SequenceButtons* self,
	psy_ui_Button* sender)
{
	assert(self);

	if (psy_ui_component_visible(&self->block)) {
		psy_ui_component_hide(&self->block);		
		psy_ui_button_set_svg(&self->moreless, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
		psy_ui_button_set_text(&self->moreless, "seqview.more");
		
	} else {
		psy_ui_button_set_svg(&self->moreless, psy_ui_app_svg(psy_ui_app(), "img.icon-less"));
		psy_ui_button_set_text(&self->moreless, "seqview.less");
		psy_ui_component_show(&self->block);		
	}
	psy_ui_component_align_invalidate(psy_ui_component_parent(
		&self->component));	
}

void sequencebuttons_on_new_entry(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_newentry(self->cmds, psy_audio_SEQUENCEENTRY_PATTERN);
}

void sequencebuttons_on_insert_entry(SequenceButtons* self,
	psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_insert_entry(self->cmds, psy_audio_SEQUENCEENTRY_PATTERN);
}

void sequencebuttons_on_clone_entry(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_cloneentry(self->cmds);
}

void sequencebuttons_on_del_entry(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_delentry(self->cmds);
}

void sequencebuttons_on_inc_pattern(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_incpattern(self->cmds);
}

void sequencebuttons_on_dec_pattern(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_decpattern(self->cmds);
}

void sequencebuttons_on_copy(SequenceButtons* self, psy_ui_Button* sender)
{	
	assert(self);

	sequencecmds_copy(self->cmds);
}

void sequencebuttons_onpaste(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	sequencecmds_paste(self->cmds);
}

void sequencebuttons_on_clear(SequenceButtons* self, psy_ui_Button* sender)
{	
	assert(self);

	workspace_confirm_seq_clear(self->cmds->workspace);
}

void sequencebuttons_on_rename(SequenceButtons* self, psy_ui_Button* sender)
{
	assert(self);

	if (!psy_ui_component_visible(psy_ui_edit_base(&self->edit))) {
		psy_audio_Pattern* pattern;

		psy_ui_edit_enable_input_field(&self->edit);
		
		pattern = psy_audio_sequence_pattern(self->cmds->sequence,
			psy_audio_sequenceselection_first(
				&self->cmds->workspace->song->sequence_.selection));
		if (pattern) {
			self->restore_focus = psy_ui_app_focus(psy_ui_app());
			psy_ui_edit_set_text(&self->edit, psy_audio_pattern_name(pattern));
			psy_ui_edit_select(&self->edit, 0, -1);
			psy_ui_component_show(psy_ui_edit_base(&self->edit));
			psy_ui_component_align_invalidate(psy_ui_component_parent(
				&self->component));			
			psy_ui_component_set_focus(psy_ui_textpane_base(&self->edit.pane));
		} else {
			psy_Logger* logger;
		
			logger = psy_ui_component_logger(&self->component,
				psy_ui_LOG_STATUS);
			if (logger) {					
				psy_logger_output(logger, "No SequenceEntry selected");
			}
		}
	}
}

void sequencebuttons_on_edit_accept(SequenceButtons* self, psy_ui_TextArea* sender)
{
	assert(self);

	if (self->cmds->sequence) {
		psy_audio_SequenceEntry* entry;

		entry = (self->cmds->sequence)
			? psy_audio_sequence_entry(self->cmds->sequence,
				psy_audio_sequenceselection_first(
					&self->cmds->workspace->song->sequence_.selection))
			: NULL;
		if (entry && entry->type == psy_audio_SEQUENCEENTRY_PATTERN) {
			psy_audio_Pattern* pattern;
			psy_audio_SequencePatternEntry* seqpatternentry;

			seqpatternentry = (psy_audio_SequencePatternEntry*)entry;
			pattern = psy_audio_patterns_at(
				self->cmds->patterns,
				seqpatternentry->patternslot);
			if (pattern) {
				psy_audio_pattern_set_name(pattern, psy_ui_edit_text(
					&self->edit));
			}
		}
	}
	psy_ui_component_hide(&self->edit.component);
	psy_ui_component_align(psy_ui_component_parent(&self->component));
	psy_ui_component_invalidate(psy_ui_component_parent(&self->component));	
	if (self->restore_focus) {
		psy_ui_component_set_focus(self->restore_focus);
		self->restore_focus = NULL;
	}
}

void sequencebuttons_on_edit_reject(SequenceButtons* self, psy_ui_TextArea* sender)
{
	assert(self);

	psy_ui_component_hide(&self->edit.component);
	psy_ui_component_align_invalidate(psy_ui_component_parent(
		&self->component));	
	if (self->restore_focus) {
		psy_ui_component_set_focus(self->restore_focus);
		self->restore_focus = NULL;
	}
}
