/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "compatconfig.h"


/* prototypes */
static psy_Property* compatconfig_make(CompatConfig*, psy_Property*);

/* implementation */
void compatconfig_init(CompatConfig* self, psy_Property* parent)
{
	assert(self && parent);

	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration, compatconfig_make(
		self, parent));
}

void compatconfig_dispose(CompatConfig* self)
{
	assert(self);
	
}

psy_Property* compatconfig_make(CompatConfig* self, psy_Property* parent)
{
	psy_Property* rv;
	
	assert(self);	

	rv = psy_property_set_text(psy_property_append_section(parent,
		"compatibility"), "settings.compatibility");	
	psy_property_enable_foldable(rv, TRUE);
	psy_property_set_text(psy_property_append_bool(rv, "loadnewgamefxblitz",
		0), "newmachine.jme-version-unknown");
	return rv;
}
