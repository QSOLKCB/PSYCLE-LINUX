/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "metronomeconfig.h"
/* container */
#include <properties.h>


/* prototypes */
static psy_Property* metronomeconfig_make(MetronomeConfig*, psy_Property*);

/* implementation */
void metronomeconfig_init(MetronomeConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);	

	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,
		metronomeconfig_make(self, parent));
}

void metronomeconfig_dispose(MetronomeConfig* self)
{
	assert(self);
	
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* metronomeconfig_make(MetronomeConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "metronome"),
		"settings.metronome.metronome");	
	psy_property_set_text(
		psy_property_append_bool(rv, "show", FALSE),
		"settings.metronome.show");
	psy_property_set_text(
		psy_property_append_int(rv, "machine", 0x3F, 0, 0x40),
		"settings.metronome.machine");
	psy_property_set_text(
		psy_property_append_int(rv, "note", 48, 0, 119),
		"settings.metronome.note");
	return rv;
}
