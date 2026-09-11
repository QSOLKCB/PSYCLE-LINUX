/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "instrumentpitchview.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void instrumentpitchview_on_tweaked(InstrumentPitchView*,
	psy_ui_Component*, uintptr_t pointindex);
static void instrumentpitchview_on_envelope_view_tweaked(InstrumentPitchView*,
	psy_ui_Component* sender, uintptr_t pointindex);

/* implementation */
void instrumentpitchview_init(InstrumentPitchView* self,
	psy_ui_Component* parent, psy_audio_Instruments* instruments)
{
	assert(self);

	self->instruments = instruments;
	self->instrument = 0;
	psy_ui_component_init(&self->component, parent, NULL);		
	envelopeview_init(&self->envelopeview, &self->component);
	envelopeview_settext(&self->envelopeview, "instview.pitch-envelope");
	psy_ui_component_set_margin(&self->envelopeview.component,
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));
	psy_ui_component_set_align(&self->envelopeview.component,
		psy_ui_ALIGN_CLIENT);
	adsrsliders_init(&self->adsrsliders, &self->component);
	psy_ui_component_set_align(&self->adsrsliders.component,
		psy_ui_ALIGN_BOTTOM);
	psy_signal_connect(&self->adsrsliders.signal_tweaked, self,
		instrumentpitchview_on_tweaked);
	psy_signal_connect(&self->envelopeview.envelopebox.signal_tweaked, self,
		instrumentpitchview_on_tweaked);
	psy_signal_connect(&self->envelopeview.signal_tweaked, self,
		instrumentpitchview_on_envelope_view_tweaked);
}

void instrumentpitchview_set_instrument(InstrumentPitchView* self,
	psy_audio_Instrument* instrument)
{
	self->instrument = instrument;
	if (self->instrument) {
		adsrsliders_set_envelope(&self->adsrsliders,			
			&self->instrument->pitchenvelope);
		envelopeview_setenvelope(&self->envelopeview,
			&self->instrument->pitchenvelope);
	} else {
		adsrsliders_set_envelope(&self->adsrsliders, NULL);
		envelopeview_setenvelope(&self->envelopeview, NULL);
	}	
}

void instrumentpitchview_on_tweaked(InstrumentPitchView* self,
	psy_ui_Component* sender, uintptr_t pointindex)
{
	if (self->instrument) {
		envelopeview_update(&self->envelopeview);		
	}
}

void instrumentpitchview_on_envelope_view_tweaked(InstrumentPitchView* self,
	psy_ui_Component* sender, uintptr_t pointindex)
{
	adsrsliders_update(&self->adsrsliders);
}
