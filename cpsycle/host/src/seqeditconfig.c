/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "seqeditconfig.h"
/* container */
#include <properties.h>


/* prototypes */
static psy_Property* seqeditconfig_make(SeqEditConfig*, psy_Property*);

/* implementation */
void seqeditconfig_init(SeqEditConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);	
	
	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,	
		seqeditconfig_make(self, parent));
}

void seqeditconfig_dispose(SeqEditConfig* self)
{
	assert(self);
	
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* seqeditconfig_make(SeqEditConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "seqedit"),
		"settings.seqeditor.seqeditor");	
	psy_property_set_hint(psy_property_set_text(
		psy_property_append_int(rv, "machine", 0x3E, 0, 0x40),
		"settings.seqeditor.machine"), PSY_PROPERTY_HINT_EDITHEX);
	return rv;
}
