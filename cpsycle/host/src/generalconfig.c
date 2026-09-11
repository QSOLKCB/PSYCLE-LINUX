/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "generalconfig.h"
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* generalconfig_make(GeneralConfig*, psy_Property*);
static void generalconfig_make_workbench(GeneralConfig*, psy_Property* parent);

/* implementation */
void generalconfig_init(GeneralConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration, generalconfig_make(
		self, parent));
}

void generalconfig_dispose(GeneralConfig* self)
{
	assert(self);
	
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* generalconfig_make(GeneralConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "general"),
		"settings.general.general");
	psy_property_enable_foldable(rv, TRUE);
	psy_property_hide(psy_property_set_text(
		psy_property_append_str(rv, "version", "alpha"),
		"settings.general.version"));	
	psy_property_set_text(
		psy_property_append_bool(rv, "showsonginfoonload", TRUE),
		"settings.general.show-song-info-on-load");		
	psy_property_set_text(
		psy_property_append_bool(rv, "playsongafterload", TRUE),
		"settings.general.play-song-after-load");
	psy_property_set_text(
		psy_property_append_bool(rv, "showpatternnames", FALSE),
		"settings.general.show-pattern-names");
	psy_property_set_text(
		psy_property_append_bool(rv, "saverecentsongs", TRUE),
		"settings.general.save-recent-songs");
	generalconfig_make_workbench(self, rv);
	return rv;
}

void generalconfig_make_workbench(GeneralConfig* self, psy_Property* parent)
{		
	psy_Property* bench;
	
	bench = psy_property_set_text(psy_property_append_section(parent,
		"bench"), "settings.general.bench");
	psy_property_enable_foldable(bench, TRUE);
	psy_property_set_text(
		psy_property_append_bool(bench, "showaboutatstart", TRUE),
		"settings.general.show-about-at-startup");		
	psy_property_set_text(
		psy_property_append_bool(bench, "showmaximizedatstart", TRUE),
		"settings.general.show-maximized-at-startup");
	psy_property_set_text(
		psy_property_append_bool(bench, "showsequenceedit", FALSE),
		"settings.general.show-sequenceedit");
	psy_property_set_text(
		psy_property_append_bool(bench, "showstepsequencer", FALSE),
		"settings.general.show-sequencestepbar");
	psy_property_set_text(
		psy_property_append_bool(bench, "showpianokbd", FALSE),
		"settings.general.show-pianokbd");	
	psy_property_set_text(
		psy_property_append_bool(bench, "showplugineditor", FALSE),
		"settings.general.show-plugineditor");		
	psy_property_set_text(
			psy_property_append_bool(bench, "showparamrack", FALSE),
		"settings.general.show-paramrack");		
	psy_property_set_text(
		psy_property_append_bool(bench, "showgear", FALSE),
		"settings.general.show-gear");
	psy_property_set_text(
		psy_property_append_bool(bench, "showmidi", FALSE),
		"settings.general.show-midi");
	psy_property_set_text(
		psy_property_append_bool(bench, "showcpu", FALSE),
		"settings.general.show-cpu");
}
