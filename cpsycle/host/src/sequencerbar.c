/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequencerbar.h"
/* host */
#include "styles.h"
/* ui */
#include "uiapp.h"


/* prototypes */
static void sequencerbar_on_destroyed(SequencerBar*);
static void sequencerbar_init_setting_button(SequencerBar*, psy_ui_Button*,
	psy_Configuration*, const char* key);
static void sequencerbar_init_bench_button(SequencerBar*, psy_ui_Button*,
	psy_Configuration*, const char* key, const char* label);
static void sequencerbar_on_more(SequencerBar*, psy_ui_Button* sender);
static void sequencerbar_on_size(SequencerBar*);
static void sequencerbar_reparent_setting_items(SequencerBar*,
	uintptr_t num_main);

/* vtable */
static psy_ui_ComponentVtable sequencerbar_vtable;
static bool sequencerbar_vtable_initialized = FALSE;

static void sequencerbar_vtable_init(SequencerBar* self)
{
	assert(self);

	if (!sequencerbar_vtable_initialized) {
		sequencerbar_vtable = *(self->component.vtable);		
		sequencerbar_vtable.on_destroyed =
			(psy_ui_fp_component)
			sequencerbar_on_destroyed;
		sequencerbar_vtable.onsize =
			(psy_ui_fp_component)
			sequencerbar_on_size;		
		sequencerbar_vtable_initialized = TRUE;
	}
	self->component.vtable = &sequencerbar_vtable;
}

/* implementation */
void sequencerbar_init(SequencerBar* self, psy_ui_Component* parent,
	psy_Configuration* misc, psy_Configuration* general)
{
	assert(self);
	assert(misc);
	assert(general);
	
	psy_ui_component_init(&self->component, parent, NULL);
	sequencerbar_vtable_init(self);
	psy_ui_component_prevent_app_focus_out(&self->component);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 0.25, 0.0));
	psy_ui_component_init(&self->main_settings_, &self->component, NULL);
	psy_ui_component_set_default_align(&self->main_settings_, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 0.25, 0.0));
	self->setting_items_ = NULL;	
	sequencerbar_init_setting_button(self, &self->follow_song_, misc, "followsong");
	sequencerbar_init_setting_button(self, &self->show_names_, general, "showpatternnames");
	sequencerbar_init_setting_button(self, &self->record_note_off_, misc, "recordnoteoff");
	sequencerbar_init_setting_button(self, &self->record_tweak_, misc, "record-tweak");
	sequencerbar_init_setting_button(self, &self->multi_channel_audition_, misc, "multikey");
	sequencerbar_init_setting_button(self, &self->allow_notes_to_effect_, misc, "notestoeffects");
	self->num_more_ = 0;
	self->doresize = FALSE;
	psy_ui_button_init_text_connect(&self->more_, &self->component,
		"...", self, sequencerbar_on_more);
	psy_ui_component_hide(psy_ui_button_base(&self->more_));
	psy_ui_component_init(&self->more_settings_, &self->component, NULL);
	psy_ui_component_set_default_align(&self->more_settings_, psy_ui_ALIGN_TOP,
		psy_ui_margin_make_em(0.0, 0.0, 0.25, 0.0));
	psy_ui_component_hide(&self->more_settings_);	
	/* bench */
	psy_ui_component_init(&self->view_buttons_, &self->component, NULL);
	psy_ui_component_prevent_app_focus_out(&self->view_buttons_);
	psy_ui_component_set_align(&self->view_buttons_, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_margin(&self->view_buttons_,
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_default_align(&self->view_buttons_, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());	
	sequencerbar_init_bench_button(self, &self->toggle_seq_edit_, general,
		"bench.showsequenceedit", "seqview.showseqeditor");	
	sequencerbar_init_bench_button(self, &self->toggle_step_seq_, general,
		"bench.showstepsequencer", "seqview.showstepsequencer");	
	sequencerbar_init_bench_button(self, &self->toggle_kbd_, general,
		"bench.showpianokbd", "seqview.showpianokbd");	
	/* prevent focus */
	psy_ui_component_prevent_app_focus_out_recursive(&self->component);		
}

void sequencerbar_on_destroyed(SequencerBar* self)
{
	assert(self);

	psy_list_free(self->setting_items_);
	self->setting_items_ = NULL;
}

void sequencerbar_init_setting_button(SequencerBar* self, psy_ui_Button* button,
	psy_Configuration* cfg, const char* key)
{
	assert(self);

	psy_ui_button_init(button, &self->main_settings_);
	psy_ui_button_set_svg(button, psy_ui_app_svg(psy_ui_app(), "img.check-off"));
	psy_ui_button_set_svg_selected(button, psy_ui_app_svg(psy_ui_app(), "img.check-on"));
	psy_ui_button_exchange(button, psy_configuration_at(cfg, key));
	psy_ui_button_set_text_alignment(button, (psy_ui_Alignment)
		(psy_ui_ALIGNMENT_LEFT | psy_ui_ALIGNMENT_CENTER_VERTICAL));
	psy_list_append(&self->setting_items_, button);
}

void sequencerbar_init_bench_button(SequencerBar* self, psy_ui_Button* button,
	psy_Configuration* cfg, const char* key, const char* label)
{
	assert(self);

	psy_ui_button_init(button, &self->view_buttons_);
	psy_ui_button_set_svg(button, psy_ui_app_svg(psy_ui_app(), "img.icon-more"));
	psy_ui_button_set_svg_selected(button, psy_ui_app_svg(psy_ui_app(), "img.icon-less"));
	psy_ui_button_set_text(button, label);
	psy_ui_button_exchange(button, psy_configuration_at(cfg, key));
	psy_ui_button_set_text_alignment(button, psy_ui_ALIGNMENT_LEFT);
	psy_ui_component_prevent_app_focus_out(psy_ui_button_base(button));
}

void sequencerbar_on_more(SequencerBar* self, psy_ui_Button* sender)
{
	assert(self);

	psy_ui_component_toggle_visibility(&self->more_settings_);
	psy_ui_component_align_invalidate(psy_ui_component_parent(&self->component));
}

void sequencerbar_on_size(SequencerBar* self)
{	
	psy_ui_RealSize psize;
	const psy_ui_TextMetric* tm;
	double q;
	uintptr_t num_more;

	assert(self);
	
	psize = psy_ui_component_scroll_size_px(
		psy_ui_component_parent(&self->component));
	tm = psy_ui_component_textmetric(&self->component);	
	q = (tm->tmHeight * 5) / psize.height;
	num_more = self->num_more_;
	if (q > 0.12) {
		sequencerbar_reparent_setting_items(self, 1);
		psy_ui_component_show(psy_ui_button_base(&self->more_));
	} else {
		sequencerbar_reparent_setting_items(self, psy_INDEX_INVALID);
		psy_ui_component_hide(psy_ui_button_base(&self->more_));
	}
	if (num_more != self->num_more_) {	
		self->doresize = TRUE;
	}
}

void sequencerbar_reparent_setting_items(SequencerBar* self,
	uintptr_t num_main)
{
	psy_List* p;
	uintptr_t i;
	
	i = 0;
	for (p = self->setting_items_;
			(p != NULL) && (i < num_main);
			p = p->next, ++i) {
		psy_ui_Component* component;

		component = (psy_ui_Component*)p->entry;
		psy_ui_component_set_parent(component, &self->main_settings_);
	}
	self->num_more_ = 0;
	for (; p != NULL; p = p->next) {
		psy_ui_Component* component;

		component = (psy_ui_Component*)p->entry;
		psy_ui_component_set_parent(component, &self->more_settings_);
		++self->num_more_;
	}
}
