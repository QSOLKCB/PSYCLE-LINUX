/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesbox.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void samplesbox_on_destroyed(SamplesBox*);
static void samplesbox_build_sample_list(SamplesBox*);
static void samplesbox_build_subsample_list(SamplesBox*, uintptr_t slot,
	bool create);
static void samplesbox_on_sample_insert(SamplesBox*, psy_ui_Component* sender,
	psy_audio_SampleIndex*);
static void samplesbox_on_sample_removed(SamplesBox*, psy_ui_Component* sender,
	psy_audio_SampleIndex*);
static void samplesbox_on_sample_list_changed(SamplesBox*,
	psy_ui_ListBox* sender);
static void samplesbox_on_subsample_list_changed(SamplesBox*,
	psy_ui_ListBox* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(SamplesBox* self)
{
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			samplesbox_on_destroyed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void samplesbox_init(SamplesBox* self, psy_ui_Component* parent,
	psy_audio_Samples* samples)
{	
	psy_ui_Margin margin;
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	psy_ui_margin_init_em(&margin, 0.0, 0.0, 1.0, 0.0);	
	psy_ui_label_init(&self->header, &self->component);
	psy_ui_label_set_text(&self->header, "samplesview.groupsfirstsample");
	psy_ui_label_set_char_number(&self->header, 25);
	psy_ui_component_set_margin(&self->header.component, margin);		
	psy_ui_component_set_align(&self->header.component, psy_ui_ALIGN_TOP);	
	psy_ui_listbox_init(&self->samplelist, &self->component);	
	psy_ui_component_set_align(&self->samplelist.component, psy_ui_ALIGN_CLIENT);	
	psy_ui_listbox_init(&self->subsamplelist, &self->component);
	psy_ui_component_set_maximum_size(&self->subsamplelist.component,
		psy_ui_size_make_em(0.0, 10.0));
	psy_ui_component_set_minimum_size(&self->subsamplelist.component,
		psy_ui_size_make_em(0.0, 10.0));
	psy_ui_component_set_align(&self->subsamplelist.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_label_init_text(&self->group, &self->component,
		"samplesview.groupsamples");
	psy_ui_component_set_align(&self->group.component, psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_margin(&self->group.component, margin);	
	psy_signal_init(&self->signal_changed);
	psy_ui_component_set_margin(&self->samplelist.component, margin);
	samplesbox_set_samples(self, samples);	
	psy_signal_connect(&self->samplelist.signal_selchanged, self,
		samplesbox_on_sample_list_changed);
	psy_signal_connect(&self->subsamplelist.signal_selchanged, self,
		samplesbox_on_subsample_list_changed);	
	self->restore_view = psy_INDEX_INVALID;
}

void samplesbox_on_destroyed(SamplesBox* self)
{	
	psy_signal_dispose(&self->signal_changed);
}

void samplesbox_build_sample_list(SamplesBox* self)
{				
	uintptr_t slot = 0;
	char text[40];
	
	psy_ui_listbox_clear(&self->samplelist);	
	for ( ; slot < 256; ++slot) {
		psy_audio_Sample* sample;

		sample = self->samples ? psy_audio_samples_at(self->samples,
			psy_audio_sampleindex_make(slot, 0)) : 0;
		if (sample) {
			psy_snprintf(text, 20, "%02X*:%s", slot,
				psy_audio_sample_name(sample));
		} else {
			psy_snprintf(text, 20, "%02X:%s", slot, "");
		}
		psy_ui_listbox_add_text(&self->samplelist, text);
	}	
}

void samplesbox_build_subsample_list(SamplesBox* self, uintptr_t slot,
	bool create)
{	
	uintptr_t subslot = 0;
	char text[40];
	
	if (create) {
		psy_ui_listbox_clear(&self->subsamplelist);
	}
	for ( ;subslot < 256; ++subslot) {
		psy_audio_Sample* sample;		

		sample = self->samples
			? psy_audio_samples_at(self->samples,
				psy_audio_sampleindex_make(slot, subslot))
			: 0;
		if (sample != 0) {
			psy_snprintf(text, 20, "%02X:%s", subslot,
				psy_audio_sample_name(sample));
		} else {
			psy_snprintf(text, 20, "%02X:%s", subslot, "");
		}
		if (create) {
			psy_ui_listbox_add_text(&self->subsamplelist, text);
		} else {
			psy_ui_listbox_set_text(&self->subsamplelist, text, subslot);
		}
	}	
}

void samplesbox_on_sample_list_changed(SamplesBox* self,
	psy_ui_ListBox* sender)
{
	samplesbox_build_subsample_list(self, psy_ui_listbox_cur_sel(sender),
		FALSE);
	psy_ui_listbox_set_cur_sel(&self->subsamplelist, 0);	
	psy_signal_emit(&self->signal_changed, self, 0);
}

void samplesbox_on_subsample_list_changed(SamplesBox* self,
	psy_ui_ListBox* sender)
{		
	psy_signal_emit(&self->signal_changed, self, 0);	
}

void samplesbox_on_sample_insert(SamplesBox* self, psy_ui_Component* sender,
	psy_audio_SampleIndex* index)
{
	samplesbox_build_sample_list(self);
	samplesbox_build_subsample_list(self, index->slot, FALSE);
}

void samplesbox_on_sample_removed(SamplesBox* self, psy_ui_Component* sender,
	psy_audio_SampleIndex* index)
{
	samplesbox_build_sample_list(self);
	samplesbox_build_subsample_list(self, index->slot, FALSE);
}

void samplesbox_set_samples(SamplesBox* self, psy_audio_Samples* samples)
{
	self->samples = samples;
	samplesbox_build_sample_list(self);
	samplesbox_build_subsample_list(self, 0, TRUE);
	psy_ui_listbox_set_cur_sel(&self->samplelist, 0);
	psy_ui_listbox_set_cur_sel(&self->subsamplelist, 0);	
	if (self->samples) {
		psy_signal_connect(&samples->signal_insert, self,
			samplesbox_on_sample_insert);	
		psy_signal_connect(&samples->signal_removed, self,
			samplesbox_on_sample_removed);	
	}
}

psy_audio_SampleIndex samplesbox_selected(SamplesBox* self)
{
	psy_audio_SampleIndex rv;

	rv.slot = psy_ui_listbox_cur_sel(&self->samplelist);
	rv.subslot = psy_ui_listbox_cur_sel(&self->subsamplelist);
	return rv;	
}

void samplesbox_select(SamplesBox* self, psy_audio_SampleIndex index)
{
	psy_audio_SampleIndex currindex;
	
	currindex = samplesbox_selected(self);
	if (currindex.slot != index.slot) {
		psy_ui_listbox_set_cur_sel(&self->samplelist, index.slot);
	}
	if (currindex.subslot != index.subslot) {
		psy_ui_listbox_set_cur_sel(&self->subsamplelist, index.subslot);
	}
}
