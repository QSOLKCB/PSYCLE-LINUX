/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "audioconfig.h"

/* local */
#include "audiodriverplugin.h"
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/psyconf.h"


/* prototypes */
static psy_Property* audioconfig_make(psy_audio_AudioConfig*, psy_Property* parent);
static void audioconfig_register_drivers(psy_audio_AudioConfig*);
static void audioconfig_register(psy_audio_AudioConfig*, intptr_t guid, const char* path,
	const char* label);
static void audioconfig_register_configuration(psy_audio_AudioConfig*,
	const char* path);
static void audioconfig_make_threads(psy_audio_AudioConfig*);
static psy_Property* audioconfig_read_configuration(psy_audio_AudioConfig*,
	intptr_t guid);
static void audioconfig_remove_configuration(psy_audio_AudioConfig*, intptr_t guid);
static void audioconfig_store_configuration(psy_audio_AudioConfig*,
	const psy_Property* configuration);


/* implementation */
void psy_audio_audioconfig_init(psy_audio_AudioConfig* self, psy_Property* parent)
{
	assert(self);
		
	psy_customconfiguration_init(&self->configuration);	
	self->audio_enabled = TRUE;
	psy_customconfiguration_set_root(&self->configuration,
		audioconfig_make(self, parent));
}

void psy_audio_audioconfig_dispose(psy_audio_AudioConfig* self)
{
	assert(self);	
		
}

psy_Property* audioconfig_make(psy_audio_AudioConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = self->inputoutput = psy_property_set_text(
		psy_property_append_section(parent, "inputoutput"),
		"settings.io.input-output");
	psy_property_set_svg(rv, "img.pulse");
	self->driver_choice = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(rv, "audiodrivers", 0),
		"settings.audio-drivers"), PSY_PROPERTY_HINT_COMBO);
	self->configurations = psy_property_hide(psy_property_append_section(
		rv, "configurations"));
	audioconfig_register_drivers(self);	
	audioconfig_make_threads(self);
	self->edit_configure = psy_property_prevent_save(psy_property_set_text(
		psy_property_append_section(rv, "configure"), "settings.configure"));
	return rv;
}

void audioconfig_register_drivers(psy_audio_AudioConfig* self)
{
	intptr_t default_index;
	
	assert(self);

	audioconfig_register(self, PSY_AUDIODRIVER_SILENTDRIVER_GUID,
		"silent", "Silent");	
#if defined(DIVERSALIS__OS__MICROSOFT)
	/*
	** output target for the audio driver dlls is {solutiondir}/Debug or 
	** {solutiondir}/Release
	** if they aren't found, check if direcories fit and if dlls are compiled
	*/
	default_index = 2; /* PSY_AUDIODRIVER_DIRECTX_GUID */	
	audioconfig_register(self,
		PSY_AUDIODRIVER_MME_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "mme.dll", "Mme");
	audioconfig_register(self,
		PSY_AUDIODRIVER_DIRECTX_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "directx.dll", "DirectX");
	audioconfig_register(self,
		PSY_AUDIODRIVER_WASAPI_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "wasapi.dll", "Wasapi");
	audioconfig_register(self,
		PSY_AUDIODRIVER_ASIO_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "asiodriver.dll", "Asio");
#elif defined(DIVERSALIS__OS__LINUX)
	default_index = 1; /* PSY_AUDIODRIVER_ALSA_GUID */
	audioconfig_register(self,
		PSY_AUDIODRIVER_ALSA_GUID,
		PSYCLE_AUDIO_DRIVER_DIR"/build/libpsyalsa.so", "Alsa");
	audioconfig_register(self,
		PSY_AUDIODRIVER_JACK_GUID,
		PSYCLE_AUDIO_DRIVER_DIR"/build/libpsyjack.so", "Jack");
#else
	default_index = 0;
#endif
#ifdef PSYCLE_USE_SDL2_AUDIO_DRIVER
	audioconfig_register(self,
		PSY_AUDIODRIVER_SDL2_GUID,
		PSYCLE_AUDIO_DRIVER_DIR"/build/libpsysdl2.so", "Sdl2");
#endif

	psy_property_set_item_int(self->driver_choice, default_index);		
}

void audioconfig_register(psy_audio_AudioConfig* self, intptr_t guid, const char* path,
	const char* label)
{
	char key[64];
	
	assert(self);
	assert(psy_strlen(path) != 0);
	assert(guid != 0);
			
	psy_snprintf(key, 64, "%d", (int)guid);
	psy_property_prevent_save(psy_property_set_text(psy_property_append_str(
		self->driver_choice, key, path), label));
	audioconfig_register_configuration(self, path);		
}

void audioconfig_register_configuration(psy_audio_AudioConfig* self, const char* path)
{
	psy_audio_AudioDriverPlugin plugin;	
	const psy_Property* configuration;

	assert(self);
		
	psy_audio_audiodriverplugin_init(&plugin, NULL);
	psy_audio_audiodriverplugin_load(&plugin, path);	
	configuration = psy_audiodriver_configuration(
		psy_audio_audiodriverplugin_base(&plugin));
	if (configuration) {
		audioconfig_store_configuration(self, configuration);		
	}
	psy_audio_audiodriverplugin_dispose(&plugin);
}

psy_Property* audioconfig_read_configuration(psy_audio_AudioConfig* self, intptr_t guid)
{
	psy_Property* rv;
	psy_List* p;
	
	assert(self);
	
	rv = NULL;
	p = psy_property_begin(self->configurations);
	for (; p != NULL; psy_list_next(&p)) {
		psy_Property* curr;		
		
		curr = (psy_Property*)psy_list_entry_const(p);		
		if (psy_property_at_int(curr, "guid", 0) == guid) {
			rv = curr;
			break;
		}
	}
	return rv;
}

void audioconfig_store_configuration(psy_audio_AudioConfig* self, const psy_Property*
	configuration)
{	
	intptr_t guid;
	
	assert(self);
	
	guid = psy_property_at_int(configuration, "guid", 0);
	if (guid == 0) {
		return;
	}
	audioconfig_remove_configuration(self, guid);	
	psy_property_hide(psy_property_append_property(self->configurations,
		psy_property_clone(configuration)));
}

void audioconfig_remove_configuration(psy_audio_AudioConfig* self, intptr_t guid)
{
	psy_Property* stored_configuration;

	assert(self);
	
	stored_configuration = audioconfig_read_configuration(self, guid);
	if (stored_configuration) {
		psy_property_remove(self->configurations, stored_configuration);
	}
}

void audioconfig_make_threads(psy_audio_AudioConfig* self)
{	
	psy_Property* threads;

	assert(self);
	
	threads = psy_property_set_text(
		psy_property_append_section(self->inputoutput, "threads"),
		"settings.io.audio-threads");	
	psy_property_set_text(
		psy_property_append_int(threads, "num", 0, 0, 99),
		"settings.io.audio-threads-num");		
}
