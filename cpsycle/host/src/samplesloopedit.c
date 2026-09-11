/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "samplesloopedit.h"
/* ui */
#include <uiapp.h>


/* prototypes */
static intptr_t LoopTypeToComboBox(psy_audio_SampleLoopType);
static psy_audio_SampleLoopType ComboBoxToLoopType(intptr_t combobox_index);
static void samplesloopedit_samplecontloopchanged(SamplesLoopEdit*,
	psy_audio_Sample* sender);
static void samplesloopedit_samplesustainloopchanged(SamplesLoopEdit*,
	psy_audio_Sample* sender);
static void samplesloopedit_onlooptypechange(SamplesLoopEdit*,
	psy_ui_ComboBox* sender);
static void samplesloopedit_onsustainlooptypechange(SamplesLoopEdit*,
	psy_ui_ComboBox* sender);
static void samplesloopedit_looptypeenablepreventinput(SamplesLoopEdit*);
static void samplesloopedit_oneditchangedloopstart(SamplesLoopEdit*,
	psy_ui_NumberEdit* sender);
static void samplesloopedit_oneditchangedloopend(SamplesLoopEdit*,
	psy_ui_NumberEdit* sender);
static void samplesloopedit_oneditchangedsustainstart(SamplesLoopEdit*,
	psy_ui_NumberEdit* sender);
static void samplesloopedit_oneditchangedsustainend(SamplesLoopEdit*,
	psy_ui_NumberEdit* sender);
static void samplesloopedit_on_sample_loop_changed(SamplesLoopEdit*,
	psy_audio_Sample* sender);


/* implementation */
void samplesloopedit_init(SamplesLoopEdit* self, psy_ui_Component* parent)
{	
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	self->sample = NULL;
	self->prevent_update = FALSE;
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());
	/* continious loop */
	psy_ui_component_init(&self->cont, &self->component, NULL);
	psy_ui_component_set_default_align(&self->cont, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	psy_ui_label_init_text(&self->loopheaderlabel, &self->cont,
		"samplesview.cont-loop");	
	psy_ui_label_set_char_number(&self->loopheaderlabel, 18);	
	psy_ui_combobox_init(&self->loopdir, &self->cont);
	psy_ui_combobox_add_text(&self->loopdir,
		psy_ui_translate("samplesview.disabled"));
	psy_ui_combobox_add_text(&self->loopdir,
		psy_ui_translate("samplesview.forward"));
	psy_ui_combobox_add_text(&self->loopdir,
		psy_ui_translate("samplesview.bidirection"));
	psy_ui_combobox_select(&self->loopdir, 0);
	psy_ui_combobox_set_char_number(&self->loopdir, 14);	
	psy_ui_numberedit_init(&self->loopstartedit, &self->cont,
		"samplesview.start", 0.0, 0.0, UINT32_MAX);	
	psy_ui_numberedit_set_edit_char_number(&self->loopstartedit, 10);	
	psy_ui_numberedit_init(&self->loopendedit, &self->cont,
		"samplesview.end", 0.0, 0.0, UINT32_MAX);
	psy_ui_numberedit_set_edit_char_number(&self->loopendedit, 10);
	/* sustain loop */
	psy_ui_component_init(&self->sustain, &self->component, NULL);
	psy_ui_component_set_default_align(&self->sustain, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	psy_ui_label_init_text(&self->sustainloopheaderlabel, &self->sustain,
		"samplesview.sustain-loop");
	psy_ui_label_set_char_number(&self->sustainloopheaderlabel, 18);
	psy_ui_combobox_init(&self->sustainloopdir, &self->sustain);
	psy_ui_combobox_add_text(&self->sustainloopdir,
		psy_ui_translate("samplesview.disabled"));
	psy_ui_combobox_add_text(&self->sustainloopdir,
		psy_ui_translate("samplesview.forward"));
	psy_ui_combobox_add_text(&self->sustainloopdir,
		psy_ui_translate("samplesview.bidirection"));	
	psy_ui_combobox_select(&self->sustainloopdir, 0);
	psy_ui_combobox_set_char_number(&self->sustainloopdir, 14);	
	psy_ui_numberedit_init(&self->sustainloopstartedit, &self->sustain,
		"samplesview.start", 0.0, 0.0, UINT32_MAX);
	psy_ui_numberedit_set_edit_char_number(&self->sustainloopstartedit, 10);	
	psy_ui_numberedit_init(&self->sustainloopendedit, &self->sustain,
		"samplesview.end", 0.0, 0.0, UINT32_MAX);
	psy_ui_numberedit_set_edit_char_number(&self->sustainloopendedit, 10);
	psy_signal_connect(&self->loopdir.signal_selchanged, self,
		samplesloopedit_onlooptypechange);
	psy_signal_connect(&self->sustainloopdir.signal_selchanged, self,
		samplesloopedit_onsustainlooptypechange);
	psy_signal_connect(&self->loopstartedit.signal_changed, self,
		samplesloopedit_oneditchangedloopstart);
	psy_signal_connect(&self->loopendedit.signal_changed, self,
		samplesloopedit_oneditchangedloopend);
	psy_signal_connect(&self->sustainloopstartedit.signal_changed, self,
		samplesloopedit_oneditchangedsustainstart);
	psy_signal_connect(&self->sustainloopendedit.signal_changed, self,
		samplesloopedit_oneditchangedsustainend);	
	psy_audio_sampleloop_init(&self->currloop);
	psy_audio_sampleloop_init(&self->currsustainloop);
}

void samplesloopedit_set_sample(SamplesLoopEdit* self, psy_audio_Sample* sample)
{
	assert(self);

	self->sample = sample;
	if (self->sample) {	
		psy_signal_connect(&sample->signal_loop_changed, self,
			samplesloopedit_on_sample_loop_changed);	
		// psy_ui_component_enable_input(&self->component, 1);		
		psy_ui_numberedit_set_value(&self->loopstartedit, (double)sample->loop.start);
		psy_ui_numberedit_set_value(&self->loopendedit, (double)sample->loop.end);
		psy_ui_numberedit_set_value(&self->sustainloopstartedit,
			(double)sample->sustainloop.start);
		psy_ui_numberedit_set_value(&self->sustainloopendedit,
			(double)sample->sustainloop.end);
		psy_ui_combobox_select(&self->loopdir,
			LoopTypeToComboBox(self->sample->loop.type));		
		psy_ui_combobox_select(&self->sustainloopdir,
			LoopTypeToComboBox(self->sample->sustainloop.type));
		self->currloop = self->sample->loop;
		self->currsustainloop = self->sample->sustainloop;
	} else {
		// psy_ui_component_prevent_input(&self->component, 1);		
		psy_ui_numberedit_set_value(&self->loopstartedit, 0.0);
		psy_ui_numberedit_set_value(&self->loopendedit, 0.0);
		psy_ui_numberedit_set_value(&self->sustainloopstartedit, 0.0);
		psy_ui_numberedit_set_value(&self->sustainloopendedit, 0.0);
		psy_ui_combobox_select(&self->loopdir,
			LoopTypeToComboBox(psy_audio_SAMPLE_LOOP_DO_NOT));
		psy_ui_combobox_select(&self->sustainloopdir,
			LoopTypeToComboBox(psy_audio_SAMPLE_LOOP_DO_NOT));		
		psy_audio_sampleloop_init(&self->currloop);
		psy_audio_sampleloop_init(&self->currsustainloop);
	}
	samplesloopedit_looptypeenablepreventinput(self);
}

intptr_t LoopTypeToComboBox(psy_audio_SampleLoopType looptype)
{	
	switch (looptype) {
	case psy_audio_SAMPLE_LOOP_DO_NOT: return 0;
	case psy_audio_SAMPLE_LOOP_NORMAL: return 1;
	case psy_audio_SAMPLE_LOOP_BIDI: return 2;
	default: return 0;		
	}	
}

void samplesloopedit_on_sample_loop_changed(SamplesLoopEdit* self, 
	psy_audio_Sample* sample)
{
	if (!self->prevent_update) {
		samplesloopedit_set_sample(self, self->sample);
	}
}

void samplesloopedit_onlooptypechange(SamplesLoopEdit* self,
	psy_ui_ComboBox* sender)
{
	if (self->sample) {
		uintptr_t sel;

		sel = psy_ui_combobox_cur_sel(sender);
		if (ComboBoxToLoopType(sel) != self->sample->loop.type) {
			self->sample->loop.type = ComboBoxToLoopType(sel);
			samplesloopedit_looptypeenablepreventinput(self);
			self->prevent_update = TRUE;		
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);		
			self->prevent_update = FALSE;
		}
	}
}

void samplesloopedit_onsustainlooptypechange(SamplesLoopEdit* self,
	psy_ui_ComboBox* sender)
{
	if (self->sample) {
		uintptr_t sel;

		sel = psy_ui_combobox_cur_sel(sender);
		if (ComboBoxToLoopType(sel) != self->sample->sustainloop.type) {
			self->sample->sustainloop.type = ComboBoxToLoopType(sel);
			samplesloopedit_looptypeenablepreventinput(self);
			self->prevent_update = TRUE;
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);
			self->prevent_update = FALSE;
		}
	}
}

psy_audio_SampleLoopType ComboBoxToLoopType(intptr_t combobox_index)
{	
	switch (combobox_index) {			
	case 0: return psy_audio_SAMPLE_LOOP_DO_NOT;
	case 1: return psy_audio_SAMPLE_LOOP_NORMAL;
	case 2: return psy_audio_SAMPLE_LOOP_BIDI;
	default: return psy_audio_SAMPLE_LOOP_DO_NOT;
	}
}

void samplesloopedit_oneditchangedloopstart(SamplesLoopEdit* self,
	psy_ui_NumberEdit* sender)
{
	if (self->sample) {
		if (psy_ui_numberedit_value(sender) != self->sample->loop.start) {
			self->sample->loop.start = (uintptr_t)psy_ui_numberedit_value(sender);
			self->prevent_update = TRUE;
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);
			self->prevent_update = FALSE;
		}
	}
}

void samplesloopedit_oneditchangedloopend(SamplesLoopEdit* self,
	psy_ui_NumberEdit* sender)
{				
	if (self->sample) {
		if (psy_ui_numberedit_value(sender) != self->sample->loop.end) {
			self->sample->loop.end = (uintptr_t)psy_ui_numberedit_value(sender);
			self->prevent_update = TRUE;
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);
			self->prevent_update = FALSE;
		}
	}
}

void samplesloopedit_oneditchangedsustainstart(SamplesLoopEdit* self,
	psy_ui_NumberEdit* sender)
{
	if (self->sample) {
		if (psy_ui_numberedit_value(sender) != self->sample->sustainloop.start) {
			self->sample->sustainloop.start = (uintptr_t)psy_ui_numberedit_value(sender);
			self->prevent_update = TRUE;
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);
			self->prevent_update = FALSE;
		}
	}
}

void samplesloopedit_oneditchangedsustainend(SamplesLoopEdit* self,
	psy_ui_NumberEdit* sender)
{
	if (self->sample) {
		if (psy_ui_numberedit_value(sender) != self->sample->sustainloop.end) {
			self->sample->sustainloop.end = (uintptr_t)psy_ui_numberedit_value(sender);		
			psy_signal_emit(&self->sample->signal_loop_changed, self, 0);
		}
	}
}

void samplesloopedit_looptypeenablepreventinput(SamplesLoopEdit* self)
{
	return;
	if (self->sample) {
		if (self->sample->loop.type == psy_audio_SAMPLE_LOOP_DO_NOT) {
			psy_ui_component_prevent_input(&self->loopstartedit.component, 0);
			psy_ui_component_prevent_input(&self->loopendedit.component, 0);
		} else {
			psy_ui_component_enable_input(&self->loopstartedit.component, 0);
			psy_ui_component_enable_input(&self->loopendedit.component, 0);
		}
		if (self->sample->sustainloop.type == psy_audio_SAMPLE_LOOP_DO_NOT) {
			psy_ui_component_prevent_input(&self->sustainloopstartedit.component, 0);
			psy_ui_component_prevent_input(&self->sustainloopendedit.component, 0);
		} else {
			psy_ui_component_enable_input(&self->sustainloopstartedit.component, 0);
			psy_ui_component_enable_input(&self->sustainloopendedit.component, 0);
		}
	} else {
		psy_ui_component_prevent_input(&self->loopstartedit.component, 0);
		psy_ui_component_prevent_input(&self->loopendedit.component, 0);
		psy_ui_component_prevent_input(&self->sustainloopstartedit.component, 0);
		psy_ui_component_prevent_input(&self->sustainloopendedit.component, 0);	
	}
}
