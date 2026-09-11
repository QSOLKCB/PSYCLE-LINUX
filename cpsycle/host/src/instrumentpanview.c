/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "instrumentpanview.h"
/* dsp */
#include <valuemapper.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void instrumentpanview_on_inst_pan_enabled(InstrumentPanView*,
	psy_ui_CheckBox* sender);
static void instrumentpanview_update_sliders(InstrumentPanView*);
static void instrumentpanview_on_describe(InstrumentPanView*,
	psy_ui_Slider*, char* text);
static void instrumentpanview_on_tweak(InstrumentPanView*,
	psy_ui_Slider*, double value);
static void instrumentpanview_on_value(InstrumentPanView*,
	psy_ui_Slider*, double* value);
static void instrumentpanview_on_tweaked(InstrumentPanView*,
	psy_ui_Component*, uintptr_t pointindex);
static void instrumentpanview_on_envelope_view_tweaked(InstrumentPanView*,
	psy_ui_Component* sender, uintptr_t pointindex);

/* implementation */
void instrumentpanview_init(InstrumentPanView* self, psy_ui_Component* parent,
	psy_audio_Instruments* instruments, Workspace* workspace)
{
	int i;
	psy_ui_Slider* sliders[] = {	
		&self->randompanning,
		&self->notemodcenternote,
		&self->notemodamount
	};
	
	self->workspace = workspace;
	self->instrument = 0;
	self->instruments = instruments;	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_component_init(&self->top, &self->component, NULL);
	psy_ui_component_set_default_align(&self->top, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_component_init(&self->instpan, &self->top, NULL);	
	psy_ui_component_set_default_align(&self->instpan, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_checkbox_init(&self->instpanenabled, &self->instpan);
	psy_signal_connect(&self->instpanenabled.signal_clicked, self,
		instrumentpanview_on_inst_pan_enabled);
	psy_ui_checkbox_set_text(&self->instpanenabled, "Instrument Pan");
	psy_ui_component_set_minimum_size(&self->instpanenabled.component,
		psy_ui_size_make_em(22, 1.0));
	psy_ui_slider_init(&self->instpanning, &self->instpan);
	psy_ui_slider_connect(&self->instpanning, self,
		(ui_slider_fpdescribe)instrumentpanview_on_describe,
		(ui_slider_fptweak)instrumentpanview_on_tweak,
		(ui_slider_fpvalue)instrumentpanview_on_value);
	psy_ui_component_set_minimum_size(&self->instpanning.component,
		psy_ui_size_make_em(50.0, 1.0));
	psy_ui_slider_set_value_char_number(&self->instpanning, 8.0);
	psy_ui_slider_show_horizontal(&self->instpanning);
	psy_ui_slider_init(&self->randompanning, &self->top);
	psy_ui_slider_set_text(&self->randompanning,
		"Swing (Randomize)");
	psy_ui_slider_init(&self->notemodcenternote, &self->top);
	psy_ui_slider_set_text(&self->notemodcenternote,
		"Note Mod Center Note");
	psy_ui_slider_init(&self->notemodamount, &self->top);
	psy_ui_slider_set_text(&self->notemodamount,
		"Note Mod Amount");
	psy_ui_component_set_default_align(&self->top, psy_ui_ALIGN_TOP,
		psy_ui_margin_zero());
	envelopeview_init(&self->envelopeview, &self->component);
	envelopeview_settext(&self->envelopeview,
		psy_ui_translate("instview.pan-envelope"));
	psy_ui_component_set_align(&self->envelopeview.component,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_margin(&self->envelopeview.component,
		psy_ui_margin_make_em(1.0, 0.0, 1.0, 0.0));
	adsrsliders_init(&self->adsrsliders, &self->component);	
	psy_ui_component_set_align(&self->adsrsliders.component,
		psy_ui_ALIGN_BOTTOM);
	psy_ui_component_set_margin(&self->adsrsliders.component,
		psy_ui_margin_zero());
	psy_signal_connect(&self->adsrsliders.signal_tweaked, self,
		instrumentpanview_on_tweaked);
	psy_signal_connect(&self->envelopeview.envelopebox.signal_tweaked, self,
		instrumentpanview_on_tweaked);
	for (i = 0; i < 3; ++i) {
		psy_ui_slider_set_char_number(sliders[i], 25);
		psy_ui_slider_set_value_char_number(sliders[i], 15);
		psy_ui_slider_connect(sliders[i], self,
			(ui_slider_fpdescribe)instrumentpanview_on_describe,
			(ui_slider_fptweak)instrumentpanview_on_tweak,
			(ui_slider_fpvalue)instrumentpanview_on_value);
	}
	psy_signal_connect(&self->envelopeview.signal_tweaked, self,
		instrumentpanview_on_envelope_view_tweaked);
	instrumentpanview_update_sliders(self);
}

void instrumentpanview_set_instrument(InstrumentPanView* self,
	psy_audio_Instrument* instrument)
{	
	self->instrument = instrument;
	if (self->instrument) {
		adsrsliders_set_envelope(&self->adsrsliders, &instrument->panenvelope);
		envelopeview_setenvelope(&self->envelopeview,
			&instrument->panenvelope);
		if (instrument->panenabled) {
			psy_ui_checkbox_check(&self->instpanenabled);
		} else {
			psy_ui_checkbox_disable_check(&self->instpanenabled);
		}
	} else {
		adsrsliders_set_envelope(&self->adsrsliders, NULL);
		envelopeview_setenvelope(&self->envelopeview, NULL);
		psy_ui_checkbox_disable_check(&self->instpanenabled);
	}
	instrumentpanview_update_sliders(self);	
}

void instrumentpanview_update_sliders(InstrumentPanView* self)
{
	psy_ui_slider_update(&self->instpanning);
	psy_ui_slider_update(&self->randompanning);
	psy_ui_slider_update(&self->notemodcenternote);
	psy_ui_slider_update(&self->notemodamount);
}

void instrumentpanview_on_inst_pan_enabled(InstrumentPanView* self,
	psy_ui_CheckBox* sender)
{
	if (!self->instrument) {
		return;
	}
	if (psy_ui_checkbox_checked(sender)) {
		self->instrument->panenabled = TRUE;
	} else {
		self->instrument->panenabled = FALSE;
	}
}

void instrumentpanview_on_describe(InstrumentPanView* self,
	psy_ui_Slider* slider, char* txt)
{	
	if (slider == &self->randompanning) {
		if (!self->instrument || self->instrument->randompanning == 0.f) {
			psy_snprintf(txt, 10, "off");
		} else {
			psy_snprintf(txt, 20, "%d%%",
				(int)(self->instrument->randompanning * 100));			
		}
	} else if (slider == &self->instpanning) {
		int pos;
		
		if (self->instrument) {
			pos = (int)(self->instrument->initpan * 128);
		} else {
			pos = 0;
		}
		if (pos == 0) psy_snprintf(txt, 64, "||%02d  ", pos);
		else if (pos < 32) psy_snprintf(txt, 64, "<<%02d  ", pos);
		else if (pos < 64) psy_snprintf(txt, 64, " <%02d< ", pos);
		else if (pos == 64) psy_snprintf(txt, 64, " |%02d| ", pos);
		else if (pos <= 96) psy_snprintf(txt, 64, " >%02d> ", pos);
		else if (pos < 128) psy_snprintf(txt, 64, "  %02d>>", pos);
		else psy_snprintf(txt, 64, "  %02d||", pos);
	} else if (slider == &self->notemodcenternote) {
		char tmp[40];
		int pos;
		char notes[12][3] = { "C-","C#","D-","D#","E-","F-","F#","G-","G#","A-","A#","B-" };
		int offset;
		psy_dsp_NotesTabMode notetabmode;
		psy_Configuration* cfg;

		if (self->instrument) {
			pos = self->instrument->notemodpancenter;
		} else {
			pos = psy_audio_NOTECOMMANDS_MIDDLEC;
		}
		cfg = psycleconfig_patview(workspace_cfg(self->workspace));
		if (psy_configuration_value_int(cfg, "notetab", 0) == 0) {
			notetabmode = psy_dsp_NOTESTAB_A440;
		} else {
			notetabmode = psy_dsp_NOTESTAB_A220;
		}		
		offset = (notetabmode == psy_dsp_NOTESTAB_A440)  ? -1 : 0;
		psy_snprintf(tmp, 40, "%s", notes[pos % 12]);
		psy_snprintf(txt, 64, "%s%d", tmp, offset + (pos / 12));
	} else if (slider == &self->notemodamount) {
		int pos;

		if (self->instrument) {
			pos = self->instrument->notemodpansep;
		} else {
			pos = 0;
		}		
		psy_snprintf(txt, 64, "%.02f%%", (pos / 2.56f));		
	}
}

void instrumentpanview_on_tweak(InstrumentPanView* self, psy_ui_Slider* slider,
	double value)
{
	if (!self->instrument) {
		return;
	}
	if (slider == &self->randompanning) {
		self->instrument->randompanning = value;
	} else if (slider == &self->instpanning) {
		self->instrument->initpan = value;
	} else if (slider == &self->notemodcenternote) {
		self->instrument->notemodpancenter = (uint8_t)(value * 119);
	} else if (slider == &self->notemodamount) {
		self->instrument->notemodpansep = (int8_t)(value * 64) - 32;
	}
}

void instrumentpanview_on_value(InstrumentPanView* self,
	psy_ui_Slider* slider, double* value)
{	
	if (slider == &self->randompanning) {
		*value = (self->instrument)
			? self->instrument->randompanning
			: 0.f;
	} else if (slider == &self->instpanning) {
		*value = (self->instrument)
			? self->instrument->initpan
			: 0.5f;	
	} else if (slider == &self->notemodcenternote) {
		*value = (self->instrument)
			? self->instrument->notemodpancenter / 119.f
			: psy_audio_NOTECOMMANDS_MIDDLEC / 119.f;
	} else if (slider == &self->notemodamount) {
		*value = (self->instrument)
			? ((self->instrument->notemodpansep) / 64.f) + 0.5f
			: 0.5f;
	}
}

void instrumentpanview_on_tweaked(InstrumentPanView* self,
	psy_ui_Component* sender, uintptr_t pointindex)
{
	if (self->instrument) {
		envelopeview_update(&self->envelopeview);		
	}
}

void instrumentpanview_on_envelope_view_tweaked(InstrumentPanView* self,
	psy_ui_Component* sender, uintptr_t pointindex)
{
	adsrsliders_update(&self->adsrsliders);
}
