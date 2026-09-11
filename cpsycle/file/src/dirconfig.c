/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "dirconfig.h"

/* local */
#include "dir.h"
/* container.h */
#include <properties.h>
/* platform */
#include "../../detail/cpu.h"
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* psy_dirconfig_make(psy_DirConfig*, psy_Property* parent);
static void psy_dirconfig_make_plugin_dirs(psy_DirConfig* self, psy_Property* parent);
static void psy_dirconfig_append_dir_edit(psy_DirConfig*, psy_Property* parent,
	const char* key, const char* label, const char* defaultdir);
	
/* implementation */
void psy_dirconfig_init(psy_DirConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);
	
	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,	
		psy_dirconfig_make(self, parent));	
}

void psy_dirconfig_dispose(psy_DirConfig* self)
{
	assert(self);

	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* psy_dirconfig_make(psy_DirConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);
	
	rv = psy_property_set_text(
		psy_property_append_section(parent, "directories"),
		"settings.dirs.dirs");	
	psy_property_set_svg(rv, "img.folder");
#if (DIVERSALIS__CPU__SIZEOF_POINTER == 4)	
	psy_property_hide(
		psy_property_set_text(
			psy_property_append_str(rv,
				"app", PSYCLE_APP_DIR), "App directory"));
#else
	psy_property_hide(
		psy_property_set_text(
			psy_property_append_str(rv,
				"app", PSYCLE_APP64_DIR), "App directory"));
#endif		
	psy_dirconfig_append_dir_edit(self, rv,
		"songs", "settings.dirs.song",
		PSYCLE_SONGS_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, rv,
		"samples", "settings.dirs.samples",
		PSYCLE_SAMPLES_DEFAULT_DIR);	
	psy_dirconfig_append_dir_edit(self, rv,
		"doc", "settings.dirs.doc",
		PSYCLE_DOC_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, rv,
		"presets", "User Presets directory",
		PSYCLE_USERPRESETS_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, rv,
		"skins", "settings.dirs.skin",
		PSYCLE_SKINS_DEFAULT_DIR);			
	psy_dirconfig_make_plugin_dirs(self, rv);
	return rv;
}

void psy_dirconfig_make_plugin_dirs(psy_DirConfig* self, psy_Property* parent)
{	
	psy_Property* plugins;
	
	assert(self);
	
	plugins = psy_property_set_text(
		psy_property_append_section(parent, "plugins"),
		"settings.dirs.plugins");
	psy_dirconfig_append_dir_edit(self, plugins,
		"plugins32", "settings.dirs.plugin32",
		PSYCLE_PLUGINS32_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, plugins,
		"plugins64", "settings.dirs.plugin64",
		PSYCLE_PLUGINS64_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, plugins,
		"luascripts", "settings.dirs.lua",
		PSYCLE_LUASCRIPTS_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, plugins,
		"vsts32", "settings.dirs.vst32",
		PSYCLE_VSTS32_DEFAULT_DIR);
	psy_dirconfig_append_dir_edit(self, plugins,
		"vsts64", "settings.dirs.vst64",
		PSYCLE_VSTS64_DEFAULT_DIR);
	psy_property_set_text(
		psy_property_append_bool(plugins, "usejbridge", FALSE),
		"32<->64 bit bridge support");
	psy_dirconfig_append_dir_edit(self, plugins,
		"ladspas", "settings.dirs.ladspa",
		PSYCLE_LADSPAS_DEFAULT_DIR);
#ifdef PSYCLE_USE_LV2		
	psy_dirconfig_append_dir_edit(self, plugins,
		"lv2s", "settings.dirs.lv2",
		PSYCLE_LV2_DEFAULT_DIR);
#endif		
}

void psy_dirconfig_append_dir_edit(psy_DirConfig* self, psy_Property* parent,
	const char* key, const char* label, const char* defaultdir)
{
	assert(self);

	psy_property_set_hint(psy_property_set_text(
		psy_property_append_str(parent, key, defaultdir), label),
		PSY_PROPERTY_HINT_EDITDIR);
}
