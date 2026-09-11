/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesviewheader.h"
/* host */
#include "samplesview.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void samplesviewheader_init_base(SamplesViewHeader*);
static void samplesviewheader_init_name(SamplesViewHeader*);
static void samplesviewheader_init_navigation(SamplesViewHeader*);
static void samplesviewheader_init_sample_rate(SamplesViewHeader*);
static void samplesviewheader_init_num_frames(SamplesViewHeader*);
static void samplesviewheader_init_channels(SamplesViewHeader*);
static void samplesviewheader_on_prev(SamplesViewHeader*,
	psy_ui_Component* sender);
static void samplesviewheader_on_next(SamplesViewHeader*,
	psy_ui_Component* sender);
static void samplesviewheader_on_name_changed(SamplesViewHeader*,
	psy_ui_Edit* sender);
static char* samplesviewheader_channel_str(const SamplesViewHeader*,
	char* rv);
static char* samplesviewheader_samplerate_str(const SamplesViewHeader*,
	char* rv);
static char* samplesviewheader_num_frames_str(const SamplesViewHeader*,
	char* rv);

/* implementation */
void samplesviewheader_init(SamplesViewHeader* self, psy_ui_Component* parent,
	psy_audio_Instruments* instruments, struct SamplesView* view)
{			
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	self->view_ = view;
	self->instruments_ = instruments;
	samplesviewheader_init_base(self);
	samplesviewheader_init_name(self);
	samplesviewheader_init_navigation(self);
	samplesviewheader_init_sample_rate(self);
	samplesviewheader_init_num_frames(self);
	samplesviewheader_init_channels(self);
}

void samplesviewheader_init_base(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_component_set_style_type(&self->component, STYLE_VIEW_HEADER);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
}

void samplesviewheader_init_name(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_label_init_text(&self->name_desc_, &self->component,
		"samplesview.samplename");
	psy_ui_edit_init(&self->name_, &self->component);
	psy_ui_edit_set_char_number(&self->name_, 20.0);
	psy_signal_connect(&self->name_.signal_change, self,
		samplesviewheader_on_name_changed);
}

void samplesviewheader_init_navigation(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_button_init_resource_connect(&self->prev_, &self->component,
		"img.icon-less", self, samplesviewheader_on_prev);
	psy_ui_button_init_resource_connect(&self->next_, &self->component,
		"img.icon-more", self, samplesviewheader_on_next);
}

void samplesviewheader_init_sample_rate(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_label_init_text(&self->sample_rate_desc_, &self->component,
		"samplesview.samplerate");
	psy_ui_edit_init(&self->sample_rate_edit_, &self->component);
	psy_ui_edit_set_char_number(&self->sample_rate_edit_, 8.0);
}

void samplesviewheader_init_num_frames(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_label_init_text(&self->num_frames_desc_, &self->component,
		"samplesview.samples");
	psy_ui_label_init(&self->num_frames_, &self->component);
	psy_ui_label_prevent_translation(&self->num_frames_);
	psy_ui_label_set_char_number(&self->num_frames_, 10.0);
}

void samplesviewheader_init_channels(SamplesViewHeader* self)
{
	assert(self);

	psy_ui_label_init(&self->channels_, &self->component);
	psy_ui_label_prevent_translation(&self->channels_);
	psy_ui_label_set_text(&self->channels_, "");
	psy_ui_label_set_char_number(&self->channels_, 7.0);
}

void samplesviewheader_set_sample(SamplesViewHeader* self,
	psy_audio_Sample* sample)
{
	char text[64];

	assert(self);

	self->sample_ = sample;
	if (self->sample_) {
		psy_ui_edit_set_text(&self->name_, psy_audio_sample_name(
			self->sample_));
	} else {
		psy_ui_edit_set_text(&self->name_, "");
	}
	psy_ui_edit_set_text(&self->sample_rate_edit_,
		samplesviewheader_samplerate_str(self, text));	
	psy_ui_label_set_text(&self->num_frames_,
		samplesviewheader_num_frames_str(self, text));	
	psy_ui_label_set_text(&self->channels_,
		samplesviewheader_channel_str(self, text));
	if (self->sample_) {
		psy_ui_component_enable_input(&self->component, psy_ui_RECURSIVE);
	} else {
		psy_ui_component_prevent_input(&self->component, psy_ui_RECURSIVE);
	}
}

char* samplesviewheader_samplerate_str(const SamplesViewHeader* self, char* rv)
{
	assert(self);

	if (self->sample_) {
		psy_snprintf(rv, 64, "%d", (int)psy_audio_sample_sample_rate(
			self->sample_));
	} else {
		psy_snprintf(rv, 64, "%s", "");
	}
	return rv;
}

char* samplesviewheader_num_frames_str(const SamplesViewHeader* self, char* rv)
{
	assert(self);

	if (self->sample_) {
		psy_snprintf(rv, 64, "%d", (int)psy_audio_sample_num_frames(
			self->sample_));
	} else {
		psy_snprintf(rv, 64, "%s", "");
	}
	return rv;
}

char* samplesviewheader_channel_str(const SamplesViewHeader* self, char* rv)
{
	assert(self);

	if (self->sample_) {
		switch (psy_audio_sample_num_channels(self->sample_)) {
		case 0:
			psy_snprintf(rv, 64, "%s", "");
			break;
		case 1:
			psy_snprintf(rv, 64, "%s", "Mono");
			break;
		case 2:
			psy_snprintf(rv, 64, "%s", "Stereo");
			break;
		default:
			psy_snprintf(rv, 64, "%d Chs",
				psy_audio_sample_num_channels(self->sample_));
			break;
		}
	} else {
		psy_snprintf(rv, 64, "%s", "");
	}
	return rv;
}

void samplesviewheader_on_name_changed(SamplesViewHeader* self,
	psy_ui_Edit* sender)
{
	assert(self);

	if (self->sample_) {
		char text[40];
		psy_audio_SampleIndex index;

		index = samplesbox_selected(&self->view_->samplesbox);
		psy_audio_sample_set_name(self->sample_, psy_ui_edit_text(sender));
		if (index.subslot == 0) {
			psy_snprintf(text, 20, "%02X:%s", 
			(int)index.slot, psy_audio_sample_name(self->sample_));
			psy_ui_listbox_set_text(&self->view_->samplesbox.samplelist, text,
				index.slot);
		}
		psy_snprintf(text, 20, "%02X:%s", 
			(int)index.subslot, psy_audio_sample_name(self->sample_));
		psy_ui_listbox_set_text(&self->view_->samplesbox.subsamplelist, text,
			index.subslot);
	}
}

void samplesviewheader_on_prev(SamplesViewHeader* self,
	psy_ui_Component* sender)
{
	assert(self);
}

void samplesviewheader_on_next(SamplesViewHeader* self,
	psy_ui_Component* sender)
{
	assert(self);
}
