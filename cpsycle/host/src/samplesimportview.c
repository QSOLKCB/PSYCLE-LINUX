/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesimportview.h"
/* host */
#include "samplesview.h"
/* audio */
#include <songio.h>


/* SamplesImportView */

/* prototypes */
static void samplesimportview_on_destroyed(SamplesImportView*);
static void samplesimportview_on_load_song_button(SamplesImportView*,
	psy_ui_Component* sender);
static void samplesimportview_on_load_changed(SamplesImportView*,
	psy_FileSelect* sender);
static void samplesimportview_load_song(SamplesImportView*,
	const char* path);	
static void samplesimportview_on_copy(SamplesImportView*,
	psy_ui_Component* sender);
static void samplesimportview_on_samplesbox_changed(SamplesImportView*,
	SamplesBox* sender);

/* vtable */
static psy_ui_ComponentVtable samplesimportview_vtable;
static bool samplesimportview_vtable_initialized = FALSE;

static void samplesimportview_vtable_init(SamplesImportView* self)
{
	if (!samplesimportview_vtable_initialized) {
		samplesimportview_vtable = *(self->component.vtable);		
		samplesimportview_vtable.on_destroyed =
			(psy_ui_fp_component)
			samplesimportview_on_destroyed;		
		samplesimportview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component,
		&samplesimportview_vtable);
}

/* implementation */
void samplesimportview_init(SamplesImportView* self,
	psy_ui_Component* parent, SamplesView* view, Workspace* workspace)
{		
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	samplesimportview_vtable_init(self);
	self->view = view;
	self->source = 0;
	self->workspace = workspace;
	psy_ui_component_init(&self->header, &self->component, NULL);
	psy_ui_component_set_align(&self->header, psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->header, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_padding(&self->header,
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 0.0));
	psy_ui_label_init_text(&self->label, &self->header, "samplesview.source");
	psy_ui_label_init_text(&self->songname, &self->header,
		"samplesview.nosongloaded");
	psy_ui_label_set_char_number(&self->songname, 30);	
	psy_ui_button_init_text_connect(&self->browse, &self->header,
		"samplesview.songselect", self, samplesimportview_on_load_song_button);
	psy_ui_checkbox_init_text(&self->wave_prev_, &self->header, "Preview");
	psy_ui_checkbox_check(&self->wave_prev_);
	/* bar */
	psy_ui_component_init(&self->bar, &self->component, NULL);
	psy_ui_component_set_align(&self->bar, psy_ui_ALIGN_LEFT);		
	psy_ui_component_set_minimum_size(&self->bar, psy_ui_size_make_em(12, 1));
	psy_ui_button_init_text(&self->add, &self->bar, "samplesview.copy");	
	psy_ui_component_set_align(&self->add.component, psy_ui_ALIGN_CENTER);
	psy_signal_connect(&self->add.signal_clicked, self,
		samplesimportview_on_copy);
	 /* samplesbox */
	samplesbox_init(&self->samplesbox, &self->component, NULL);
	psy_ui_component_set_align(&self->samplesbox.component,
		psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->samplesbox.signal_changed, self,
		samplesimportview_on_samplesbox_changed);
	/* samplebox */
	wavebox_init(&self->samplebox, &self->component, workspace);
	psy_ui_component_set_align(&self->samplebox.component,
		psy_ui_ALIGN_BOTTOM);	
}

void samplesimportview_on_destroyed(SamplesImportView* self)
{	
	assert(self);
	
	if (self->source) {
		psy_audio_song_deallocate(self->source);
		self->source = NULL;
	}	
}

void samplesimportview_on_load_song_button(SamplesImportView* self,
	psy_ui_Component* sender)
{
	psy_FileSelect load;
	
	assert(self);
			
	psy_fileselect_init(&load);
	psy_fileselect_connect_change(&load, self,
		(psy_fp_fileselect_change)samplesimportview_on_load_changed);
	psy_fileselect_set_title(&load, psy_ui_translate("samplesview.loadsong"));
	psy_fileselect_set_default_extension(&load,
		psy_audio_songfile_standardloadfilter());
	psy_fileselect_add_filter(&load, "psy", "*.psy");
	// psy_audio_songfile_loadfilter()
	workspace_disk_op(self->workspace, &load, NULL, NULL);		
	psy_fileselect_dispose(&load);
}

void samplesimportview_on_load_changed(SamplesImportView* self,
	psy_FileSelect* sender)
{
	assert(self);
	
	samplesimportview_load_song(self, psy_fileselect_value(sender));
	workspace_restore_view(self->workspace);	
}

void samplesimportview_load_song(SamplesImportView* self,
	const char* path)
{	
	psy_audio_SongReader song_reader;
	
	assert(self);

	if (self->source) {
		psy_audio_song_deallocate(self->source);			
	}	
	self->source = psy_audio_song_alloc_init(
		&self->workspace->player_.machinefactory);
	psy_audio_songreader_init(&song_reader, self->source, NULL, FALSE);	
	psy_audio_songreader_load(&song_reader, path);
	if (!song_reader.err) {
		psy_ui_label_set_text(&self->songname,
			psy_audio_song_title(self->source));
		samplesbox_set_samples(&self->samplesbox, &self->source->samples_);
	} else {
		psy_ui_label_set_text(&self->songname,
			"No source song loaded");
	}
	psy_audio_songreader_dispose(&song_reader);	
}

void samplesimportview_on_copy(SamplesImportView* self,
	psy_ui_Component* sender) {
	psy_audio_SampleIndex src;
	psy_audio_SampleIndex dst;
	psy_audio_Sample* sample;
	psy_audio_Sample* samplecopy;
	psy_audio_Instrument* instrument;
	
	assert(self);
	
	src = samplesbox_selected(&self->samplesbox);
	dst = samplesbox_selected(&self->view->samplesbox);	
	sample = psy_audio_samples_at(&self->source->samples_, src);
	if (sample) {
		samplecopy = psy_audio_sample_clone(sample);
		psy_audio_samples_insert(&workspace_song(self->workspace)->samples_, samplecopy,
			dst);
		instrument = (psy_audio_Instrument*)malloc(sizeof(psy_audio_Instrument));		
		psy_audio_instrument_init(instrument);
		psy_audio_instrument_set_name(instrument, psy_audio_sample_name(samplecopy));
		psy_audio_instrument_setindex(instrument, dst.slot);

		psy_audio_instruments_insert(&workspace_song(self->workspace)->instruments_, instrument,
			psy_audio_instrumentindex_make(0, dst.slot));
		samplesview_set_sample(self->view, dst);
		/*signal_prevent(&workspace_song(self->workspace)->instruments.signal_slotchange,
			self->view, OnInstrumentSlotChanged);
		instruments_changeslot(&workspace_song(self->workspace)->instruments, dstslot);
		signal_enable(&workspace_song(self->workspace)->instruments.signal_slotchange, self,
			OnInstrumentSlotChanged);	*/
		psy_ui_component_invalidate(&self->view->component);
	}
}

void samplesimportview_on_samplesbox_changed(SamplesImportView* self,
	SamplesBox* sender)
{	
	psy_audio_Sample* sample;
	
	assert(self);

	if (self->source) {						
		sample = psy_audio_samples_at(&self->source->samples_,
			samplesbox_selected(sender));
	} else {
		sample = NULL;
	}
	if (sample && psy_ui_checkbox_checked(&self->wave_prev_)) {
		psy_audio_player_wave_prev_play_sample(&self->workspace->player_,
			sample);
	}
	wavebox_setsample(&self->samplebox, sample, 0);
}
