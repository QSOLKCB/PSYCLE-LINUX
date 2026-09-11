/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "globalconfig.h"


/* host */
#include "resources/resource.h"
/* ui */
#include <uicomponent.h> /* Translator */
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* globalconfig_make(GlobalConfig*, psy_Property*);
static void globalconfig_make_sections(GlobalConfig*, psy_Property* parent);
static void globalconfig_make_lang(GlobalConfig*, psy_Property* parent);
static void globalconfig_make_general(GlobalConfig*, psy_Property* parent);
static void globalconfig_make_compat(GlobalConfig*, psy_Property* parent);

/* implementation */
void globalconfig_init(GlobalConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,
		globalconfig_make(self, parent));	
}

void globalconfig_dispose(GlobalConfig* self)
{
	assert(self);

	languageconfig_dispose(&self->lang_);
	generalconfig_dispose(&self->general_);
	compatconfig_dispose(&self->compat_);
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* globalconfig_make(GlobalConfig* self, psy_Property* parent)
{	
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "global"),
		"settings.global.configuration");	
	psy_property_set_svg(rv, "img.settings");
	psy_property_set_text(
		psy_property_append_bool(rv, "enableaudio", TRUE),
		"settings.global.enable-audio");
	psy_property_set_text(
		psy_property_append_action(rv, "regenerateplugincache"),
		"settings.global.regenerate-plugincache");	
	globalconfig_make_sections(self, rv);
	return rv;
}

void globalconfig_make_sections(GlobalConfig* self, psy_Property* parent)
{
	assert(self);

	
	globalconfig_make_lang(self, parent);
	globalconfig_make_general(self, parent);
	globalconfig_make_compat(self, parent);
}

void globalconfig_make_lang(GlobalConfig* self, psy_Property* parent)
{
	assert(self);

	languageconfig_init(&self->lang_, parent, psy_ui_translator());
}

void globalconfig_make_general(GlobalConfig* self, psy_Property* parent)
{
	assert(self);

	generalconfig_init(&self->general_, parent);
}

void globalconfig_make_compat(GlobalConfig* self, psy_Property* parent)
{
	assert(self);

	compatconfig_init(&self->compat_, parent);
}
