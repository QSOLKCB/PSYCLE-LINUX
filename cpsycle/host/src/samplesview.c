/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesview.h"
/* host */
#include "styles.h"
/* audio */
#include <exclusivelock.h>
#include <instruments.h>
#include <songio.h>
/* std */
#include <math.h>
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"


/* SamplesViewButtons */

/* implementation */
void samplesviewbuttons_init(SamplesViewButtons* self, psy_ui_Component* parent)
{	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_padding(&self->component,	
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 0.0));	
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	psy_ui_button_init_text(&self->load, &self->component, "file.load");
	psy_ui_button_init_text(&self->save, &self->component, "file.save");
	psy_ui_button_init_text(&self->duplicate, &self->component,
		"edit.duplicate");
	psy_ui_button_init_text(&self->del, &self->component, "edit.delete");	
}


/* SamplesView */

/* prototypes */
static void samplesview_on_destroyed(SamplesView*);
static void samplesview_on_samples_box_changed(SamplesView*, psy_ui_Component* sender);
static void samplesview_on_load_sample(SamplesView*, psy_ui_Component* sender);
static void samplesview_on_save_sample_button(SamplesView*, psy_ui_Component* sender);
static void samplesview_on_save_sample_change(SamplesView*,
	psy_FileSelect* sender);
static void samplesview_save_sample(SamplesView*, const char* path);
static void samplesview_on_delete_sample(SamplesView*, psy_ui_Component* sender);
static void samplesview_on_duplicate_sample(SamplesView*, psy_ui_Component* sender);
static void samplesview_on_song_changed(SamplesView*,
	psy_audio_Player* sender);
static void samplesview_on_instrument_slot_changed(SamplesView*,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex* slot);
static uintptr_t samplesview_freesampleslot(SamplesView*, uintptr_t startslot,
	uintptr_t maxslots);
static void samplesview_on_sample_modified(SamplesView*, SampleEditor* sender,
	psy_audio_Sample*);
static void samplesview_on_note_tab_mode(SamplesView*,
	psy_Property* property);
static void samplesview_on_resampler_quality_changed(SamplesView*,
	psy_ui_ComboBox* sender);
static void samplesview_on_select_section(SamplesView*,
	psy_ui_Component* sender, uintptr_t section, uintptr_t options);
static void samplesview_on_client_tabbar_changed(SamplesView*,
	psy_ui_TabBar* sender, uintptr_t index);
static uintptr_t samplesview_section(const SamplesView*);

/* vtable */
static psy_ui_ComponentVtable samplesview_vtable;
static bool samplesview_vtable_initialized = FALSE;

static void samplesview_vtable_init(SamplesView* self)
{
	if (!samplesview_vtable_initialized) {
		samplesview_vtable = *(self->component.vtable);		
		samplesview_vtable.on_destroyed =
			(psy_ui_fp_component)
			samplesview_on_destroyed;
		samplesview_vtable.section =
			(psy_ui_fp_component_section)
			samplesview_section;
		samplesview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(samplesview_base(self), &samplesview_vtable);
}

/* implementation */
void samplesview_init(SamplesView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace* workspace)
{	
	assert(self);
		
	psy_ui_component_init(&self->component, parent, NULL);
	samplesview_vtable_init(self);
	self->workspace = workspace;
	psy_ui_component_set_id(samplesview_base(self), VIEW_ID_SAMPLES);
	psy_ui_component_set_title(samplesview_base(self), "main.samples");
	psy_ui_component_set_style_type(&self->component, STYLE_CLIENT_VIEW);	
	/* header */
	samplesviewheader_init(&self->header, &self->component,
		&workspace->song->instruments_, self);
	psy_ui_component_set_align(&self->header.component, psy_ui_ALIGN_TOP);
	/* left */
	psy_ui_component_init(&self->left, &self->component, NULL);
	psy_ui_component_set_align(&self->left, psy_ui_ALIGN_LEFT);	
	samplesviewbuttons_init(&self->buttons, &self->left);
	psy_ui_component_set_align(&self->buttons.component, psy_ui_ALIGN_TOP);
	/* tabbarparent */
	psy_ui_tabbar_init(&self->clienttabbar, tabbarparent);
	psy_ui_component_set_id(psy_ui_tabbar_base(&self->clienttabbar),
		VIEW_ID_SAMPLES);
	psy_ui_component_set_align(&self->clienttabbar.component, psy_ui_ALIGN_LEFT);
	psy_ui_component_hide(&self->clienttabbar.component);
	psy_ui_tabbar_append_tabs(&self->clienttabbar, "Properties", "Import",
		"Editor", NULL);
	psy_signal_connect(&self->clienttabbar.signal_change, self,
		samplesview_on_client_tabbar_changed);
	samplesbox_init(&self->samplesbox, &self->left, &workspace->song->samples_);
	psy_ui_component_set_align(&self->samplesbox.component,
		psy_ui_ALIGN_CLIENT);
	psy_ui_button_connect(&self->buttons.load, self,
		samplesview_on_load_sample);
	psy_ui_button_connect(&self->buttons.save, self,
		samplesview_on_save_sample_button);
	psy_ui_button_connect(&self->buttons.duplicate, self,
		samplesview_on_duplicate_sample);
	psy_ui_button_connect(&self->buttons.del, self,
		samplesview_on_delete_sample);
	/* client */
	/* right */
	psy_ui_component_init(&self->right, &self->component, NULL);
	psy_ui_component_set_align(&self->right, psy_ui_ALIGN_CLIENT);	
	psy_ui_component_set_margin(&self->right,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 2.0));
	/* client notebook */
	psy_ui_notebook_init(&self->clientnotebook, &self->right);
	psy_ui_component_set_margin(psy_ui_notebook_base(&self->clientnotebook),
		psy_ui_margin_make_em(0.30, 2.0, 0.0, 0.0));
	psy_ui_component_set_align(&self->clientnotebook.component, psy_ui_ALIGN_CLIENT);
	/* client */
	psy_ui_component_init(&self->client, &self->clientnotebook.component, NULL);
	psy_ui_tabbar_init(&self->tabbar, &self->client);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar), psy_ui_ALIGN_TOP);	
	psy_ui_tabbar_append(&self->tabbar, "General", psy_INDEX_INVALID);
	psy_ui_tabbar_append(&self->tabbar, "Vibrato", psy_INDEX_INVALID);
	psy_ui_notebook_init(&self->notebook, &self->client);
	psy_ui_component_set_margin(psy_ui_notebook_base(&self->notebook),
		psy_ui_margin_make_em(1.0, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_TOP);	
	psy_ui_notebook_connect_controller(&self->notebook,
		&self->tabbar.signal_change);
	/* GeneralView */
	samplesviewgeneral_init(&self->general, psy_ui_notebook_base(
		&self->notebook), psycleconfig_patview(workspace_cfg(self->workspace)));
	psy_ui_component_set_align(&self->general.component, psy_ui_ALIGN_TOP);
	/* VibratoView */
	samplesviewvibrato_init(&self->vibrato, psy_ui_notebook_base(&self->notebook),
		&workspace->player_);
	psy_ui_component_set_align(&self->vibrato.component, psy_ui_ALIGN_TOP);
	psy_ui_notebook_select(&self->notebook, 0);
	wavebox_init(&self->wavebox, &self->client, workspace);
	psy_ui_component_set_align(&self->wavebox.component, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_margin(&self->wavebox.component,
		psy_ui_margin_make_em(0.5, 1.0, 0.0, 0.5));	
	/* ImportView */
	samplesimportview_init(&self->songimport,
		&self->clientnotebook.component, self, workspace);
	/* WaveEditorView */
	sampleeditor_init(&self->sampleeditor, &self->clientnotebook.component,
		workspace);
	sampleeditorbar_init(&self->sampleeditor.sampleeditortbar,
		&self->right, &self->sampleeditor, self->workspace);
	psy_ui_component_set_margin(&self->sampleeditor.sampleeditortbar.component,
		psy_ui_margin_make_em(0.5, 0.0, 0.0, 0.0));
	psy_ui_component_set_align(&self->sampleeditor.sampleeditortbar.component,
		psy_ui_ALIGN_BOTTOM);
	/* LoopView */
	samplesloopedit_init(&self->waveloop, &self->right);
	psy_ui_component_set_align(&self->waveloop.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_margin(&self->waveloop.component,
		psy_ui_margin_make_em(0.0, 0.0, 0.0, 0.5));	
	psy_signal_connect(&self->samplesbox.signal_changed, self,
		samplesview_on_samples_box_changed);
	psy_signal_connect(&workspace->song->instruments_.signal_slotchange, self,
		samplesview_on_instrument_slot_changed);	
	psy_ui_notebook_select(&self->clientnotebook, 0);
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		samplesview_on_song_changed);
	psy_ui_notebook_select(&self->clientnotebook, 0);	
	samplesview_set_sample(self, psy_audio_sampleindex_make(0, 0));
	psy_signal_connect(&self->sampleeditor.signal_samplemodified, self,
		samplesview_on_sample_modified);	
	psy_ui_tabbar_select(&self->tabbar, 0);
	psy_signal_connect(&samplesview_base(self)->signal_select_section, self,
		samplesview_on_select_section);	
	samplesview_connect_status_bar(self);
	psy_configuration_connect(
		psycleconfig_patview(workspace_cfg(self->workspace)),
		"notetab", self, samplesview_on_note_tab_mode);
	psy_configuration_configure(
		psycleconfig_patview(workspace_cfg(self->workspace)),
		"notetab");
}

void samplesview_on_destroyed(SamplesView* self)
{		
	assert(self);
}

void samplesview_on_note_tab_mode(SamplesView* self, psy_Property* sender)
{
	assert(self);

	self->general.notes_tab_mode = (psy_property_item_bool(sender))
		? psy_dsp_NOTESTAB_A440
		: psy_dsp_NOTESTAB_A220;	
}

void samplesview_on_samples_box_changed(SamplesView* self, psy_ui_Component* sender)
{
	psy_audio_SampleIndex index;

	assert(self);
	
	index = samplesbox_selected(&self->samplesbox);
	samplesview_set_sample(self, index);
	if (workspace_song(self->workspace)) {
		psy_signal_disconnect(&workspace_song(self->workspace)->instruments_.signal_slotchange,
			self, samplesview_on_instrument_slot_changed);
		psy_audio_instruments_select(&workspace_song(self->workspace)->instruments_,
			psy_audio_instrumentindex_make(0, index.slot));
		psy_signal_connect(
			&workspace_song(self->workspace)->instruments_.signal_slotchange,
			self, samplesview_on_instrument_slot_changed);
	}
	psy_ui_component_invalidate(&self->wavebox.component);
}

void samplesview_on_instrument_slot_changed(SamplesView* self,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex* slot)
{
	psy_audio_SampleIndex index;

	assert(self);
	
	index = samplesbox_selected(&self->samplesbox);
	if (index.slot != slot->subslot) {
		index.subslot = 0;
	}
	index.slot = slot->subslot;
	samplesview_set_sample(self, index);
}

void samplesview_set_sample(SamplesView* self, psy_audio_SampleIndex index)
{
	psy_audio_Sample* sample;

	assert(self);
	
	sample = (workspace_song(self->workspace))
		? psy_audio_samples_at(&workspace_song(self->workspace)->samples_, index)
		: 0;
	workspace_song(self->workspace)->samples_.selected = index;
	wavebox_setsample(&self->wavebox, sample, 0);
	sampleeditor_setsample(&self->sampleeditor, sample);
	samplesviewheader_set_sample(&self->header, sample);
	samplesviewgeneral_set_sample(&self->general, sample);
	samplesviewvibrato_set_sample(&self->vibrato, sample);
	samplesloopedit_set_sample(&self->waveloop, sample);
	samplesbox_select(&self->samplesbox, index);	
}

void samplesview_connect_status_bar(SamplesView* self)
{	
	assert(self);
		
	psy_signal_connect(
		&self->sampleeditor.sampleeditortbar.visualrepresentation.signal_selchanged,
		self, samplesview_on_resampler_quality_changed);
}

void samplesview_on_load_sample(SamplesView* self,
	psy_ui_Component* sender)
{	
	assert(self);
				
	workspace_load_sample(self->workspace, samplesbox_selected(
		&self->samplesbox));
}

void samplesview_on_save_sample_button(SamplesView* self,
	psy_ui_Component* sender)
{
	psy_FileSelect save;
	
	assert(self);
			
	psy_fileselect_init(&save);
	psy_fileselect_connect_change(&save, self,
		(psy_fp_fileselect_change)samplesview_on_save_sample_change);
	psy_fileselect_set_title(&save, "Save Sample");
	psy_fileselect_set_default_extension(&save, "wav");
	psy_fileselect_add_filter(&save, "Wave", "*.wav");
	psy_fileselect_add_filter(&save, "IFF", "*.iff");
	psy_fileselect_set_directory(&save, 
		psy_configuration_value_str(
			psycleconfig_directories(workspace_cfg(self->workspace)),			
			"samples", PSYCLE_SAMPLES_DEFAULT_DIR));
	workspace_disk_op(self->workspace, NULL, &save, NULL);		
	psy_fileselect_dispose(&save);	
}

void samplesview_on_save_sample_change(SamplesView* self,
	psy_FileSelect* sender)
{
	assert(self);
	
	samplesview_save_sample(self, psy_fileselect_value(sender));
	workspace_select_view(self->workspace, viewindex_make_all(
		VIEW_ID_SAMPLES, 0, 0, psy_INDEX_INVALID));
}

void samplesview_save_sample(SamplesView* self, const char* path)
{
	assert(self);
	
	if (wavebox_sample(&self->wavebox)) {
		psy_audio_sample_save(wavebox_sample(&self->wavebox), path);
	}
}

void samplesview_on_delete_sample(SamplesView* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (workspace_song(self->workspace)) {
		psy_audio_SampleIndex index;

		index = samplesbox_selected(&self->samplesbox);
		psy_audio_exclusivelock_enter();
		psy_audio_samples_remove(&workspace_song(self->workspace)->samples_, index);
		psy_audio_instruments_remove(&workspace_song(self->workspace)->instruments_,
			psy_audio_instrumentindex_make(0, index.subslot));
		samplesview_set_sample(self, index);		
		psy_audio_exclusivelock_leave();
		psy_audio_machines_notify_aux_change(psy_audio_song_machines(
			workspace_song(self->workspace)));
	}
}

void samplesview_on_duplicate_sample(SamplesView* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (workspace_song(self->workspace)) {
		psy_audio_SampleIndex src;
		psy_audio_SampleIndex dst;

		src = samplesbox_selected(&self->samplesbox);
		dst.slot = samplesview_freesampleslot(self, src.slot, 256);
		dst.subslot = 0;
		if (dst.slot != 256) {
			psy_audio_Sample* source;

			source = psy_audio_samples_at(&workspace_song(self->workspace)->samples_,
				src);
			if (source) {
				psy_audio_Sample* copy;
				psy_audio_Instrument* instrument;

				copy = psy_audio_sample_clone(source);
				psy_audio_samples_insert(&workspace_song(self->workspace)->samples_, copy,
					dst);
				instrument = psy_audio_instrument_allocinit();
				psy_audio_instrument_set_name(instrument, psy_audio_sample_name(copy));
				psy_audio_instrument_setindex(instrument, dst.slot);
				psy_audio_instruments_insert(&workspace_song(self->workspace)->instruments_,
					instrument, psy_audio_instrumentindex_make(0, dst.slot));
				samplesview_set_sample(self, psy_audio_sampleindex_make(dst.slot, 0));
				psy_audio_machines_notify_aux_change(psy_audio_song_machines(
					workspace_song(self->workspace)));
			}
		}
	}
}

uintptr_t samplesview_freesampleslot(SamplesView* self, uintptr_t startslot,
	uintptr_t maxslots)
{
	assert(self);

	if (workspace_song(self->workspace)) {
		return psy_audio_samples_freeslot(
			&workspace_song(self->workspace)->samples_, startslot, maxslots);
	}
	return maxslots;
}

void samplesview_on_sample_modified(SamplesView* self, SampleEditor* sender,
	psy_audio_Sample* sample)
{
	assert(self);

	samplesviewheader_set_sample(&self->header, sample);
	samplesviewgeneral_set_sample(&self->general, sample);
	samplesviewvibrato_set_sample(&self->vibrato, sample);
	samplesloopedit_set_sample(&self->waveloop, sample);
}

void samplesview_on_song_changed(SamplesView* self, psy_audio_Player* sender)
{
	assert(self);

	if (sender->song) {
		psy_signal_connect(&sender->song->instruments_.signal_slotchange, self,
			samplesview_on_instrument_slot_changed);
		samplesbox_set_samples(&self->samplesbox, &sender->song->samples_);
	} else {
		samplesbox_set_samples(&self->samplesbox, NULL);
	}
	samplesview_set_sample(self, psy_audio_sampleindex_make(0, 0));
}

void samplesview_on_resampler_quality_changed(SamplesView* self,
	psy_ui_ComboBox* sender)
{
	assert(self);

	wavebox_set_quality(&self->wavebox, (psy_dsp_ResamplerQuality)
		psy_ui_combobox_cur_sel(sender));
	sampleeditor_setquality(&self->sampleeditor,
		(psy_dsp_ResamplerQuality)psy_ui_combobox_cur_sel(sender));
}

void samplesview_on_select_section(SamplesView* self, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options)
{
	assert(self);

	if (section != psy_INDEX_INVALID) {
		psy_ui_tabbar_mark(&self->clienttabbar, section);
		psy_ui_notebook_select(&self->clientnotebook, section);
	}
}

void samplesview_on_client_tabbar_changed(SamplesView* self,
	psy_ui_TabBar* sender, uintptr_t index)
{
	uintptr_t section;
	
	assert(self);
	
	switch (index) {
	case 0: section = SECTION_ID_SAMPLES_PROPERTIES; break;
	case 1: section = SECTION_ID_SAMPLES_IMPORT; break;
	case 2: section = SECTION_ID_SAMPLES_EDIT; break;
	default: section = psy_INDEX_INVALID; break;
	}
	if (section != psy_INDEX_INVALID) {
		workspace_select_view(self->workspace, viewindex_make_section(
			VIEW_ID_SAMPLES, section));
	}
}

uintptr_t samplesview_section(const SamplesView* self)
{
	assert(self);

	return psy_ui_notebook_page_index(&self->clientnotebook);
}
