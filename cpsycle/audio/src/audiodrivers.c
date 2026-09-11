/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "audiodrivers.h"

/* local */
#include "sequencer.h"
/* dsp */
#include <operations.h>
#include <rms.h>
/* container */
#include <configuration.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void psy_audio_audiodrivers_on_edit_configuration(
	psy_audio_AudioDrivers*, psy_Property* sender);
static void psy_audio_audiodrivers_on_driver_select(psy_audio_AudioDrivers*,
	psy_Property* sender);
static void psy_audio_audiodrivers_load_driver(psy_audio_AudioDrivers*,
	const char* path, const psy_Property* config, bool open);
static const psy_Property* audiodrivers_read_configuration_const(
	const psy_audio_AudioDrivers*, intptr_t guid);
void psy_audio_audiodrivers_store_configuration(psy_audio_AudioDrivers*,
	const psy_Property* configuration);
void psy_audio_audiodrivers_remove_configuration(psy_audio_AudioDrivers*,
	intptr_t guid);
static intptr_t psy_audio_audiodrivers_selected_guid(const psy_audio_AudioDrivers*);
static void psy_audio_audiodrivers_select_driver(psy_audio_AudioDrivers*,
	intptr_t guid, bool open);

/* implementation */
void psy_audio_audiodrivers_init(psy_audio_AudioDrivers* self, void* handle,
	psy_audio_Sequencer* sequencer, psy_Configuration* config, void* context,
	AUDIODRIVERWORKFN work, struct psy_Logger* logger)
{
	assert(self);
		
	self->systemhandle = handle;
	self->sequencer = sequencer;
	self->config_ = config;
	self->context = context;
	self->fp = work;
	self->prevent_open = FALSE;
	psy_audio_audiodriverplugin_init(&self->driver_plugin, logger);
	if (self->config_) {
		psy_audio_audiodrivers_select_driver(self, 
			psy_audio_audiodrivers_selected_guid(self),
		FALSE /* do not open yet */);
		psy_configuration_connect(self->config_,
			"audiodrivers",  self, psy_audio_audiodrivers_on_driver_select);
	}
}

void psy_audio_audiodrivers_dispose(psy_audio_AudioDrivers* self)
{
	assert(self);
	
	psy_audio_audiodriverplugin_dispose(&self->driver_plugin);
}

void psy_audio_audiodrivers_load_driver(psy_audio_AudioDrivers* self,
	const char* path, const psy_Property* config, bool open)
{
	psy_AudioDriver* driver;
	
	assert(self);
		
	psy_audio_audiodriverplugin_load(&self->driver_plugin, path);
	psy_audio_audiodriverplugin_connect(&self->driver_plugin, 
		self->systemhandle, self->context,
		(AUDIODRIVERWORKFN)self->fp);
	driver = psy_audio_audiodriverplugin_base(&self->driver_plugin);
	if (driver) {
		psy_audio_sequencer_setsamplerate(self->sequencer,
			psy_audiodriver_samplerate(driver));
		psy_dsp_rmsvol_setsamplerate((uint32_t)
			psy_audiodriver_samplerate(driver));
		psy_audiodriver_refresh_ports(driver);
		if (config) {			
			psy_audiodriver_configure(driver, config);
		}		
		if (open) {		
			printf("open\n");
			psy_audiodriver_open(driver);
		}	
	}
}

void psy_audio_audiodrivers_enable_render(psy_audio_AudioDrivers* self)
{
	assert(self);
	
	psy_audio_audiodrivers_load_driver(self, "fileout", NULL, FALSE);
}

void psy_audio_audiodrivers_restore(psy_audio_AudioDrivers* self)
{
	assert(self);
	
	psy_audio_audiodrivers_select_driver(self,
		psy_audio_audiodrivers_selected_guid(self), TRUE);
}

void psy_audio_audiodrivers_select_driver(psy_audio_AudioDrivers* self,
	intptr_t guid, bool open)
{	
	const psy_Property* drivers;
	const psy_Property* configuration;
	psy_Property* edit_configure;	
	const char* module_path;	
	char key[64];
	
	assert(self);
	
	if (guid == 0) {
		return;
	}				
	drivers = psy_configuration_at(self->config_, "audiodrivers");	
	if (!drivers) {
		return;
	}
	psy_snprintf(key, 64, "%d", (int)guid);	
	module_path = psy_property_at_str(drivers, key, NULL);		
	if (psy_strlen(module_path) == 0) {
		return;
	}		
	configuration = audiodrivers_read_configuration_const(self, guid);
	psy_audio_audiodrivers_load_driver(self, module_path,
		configuration, psy_audio_audiodrivers_enabled(self) && open &&
		!self->prevent_open);			
	edit_configure = psy_configuration_at(self->config_, "configure");
	if (!edit_configure) {
		return;
	}
	psy_property_clear(edit_configure);	
	configuration = psy_audiodriver_configuration(
		psy_audio_audiodriverplugin_base(&self->driver_plugin));
	if (configuration) {
		psy_property_append_property(edit_configure,
			psy_property_clone(configuration));
		psy_property_connect_children(edit_configure, TRUE,
			self, psy_audio_audiodrivers_on_edit_configuration);
		psy_audio_audiodrivers_store_configuration(self, configuration);
	}
	psy_property_rebuild(edit_configure);
}

const psy_Property* audiodrivers_read_configuration_const(
	const psy_audio_AudioDrivers* self, intptr_t guid)
{
	const psy_Property* rv;
	const psy_Property* configurations;
	const psy_List* p;
	
	assert(self);
	
	rv = NULL;
	if (self->config_) {
		configurations = psy_configuration_at(self->config_, "configurations");
		p = psy_property_begin_const(configurations);
		for (; p != NULL; psy_list_next_const(&p)) {
			psy_Property* curr;		
			
			curr = (psy_Property*)psy_list_entry_const(p);		
			if (psy_property_at_int(curr, "guid", 0) == guid) {
				rv = curr;
				break;
			}
		}
	}
	return rv;
}

void psy_audio_audiodrivers_on_edit_configuration(psy_audio_AudioDrivers* self,
	psy_Property* sender)
{
	const psy_Property* edit_configure;
	const psy_Property* edit_configuration;

	assert(self);

	edit_configure = psy_configuration_at(self->config_, "configure");
	if (!edit_configure) {
		return;
	}
	edit_configuration = psy_property_first_const(edit_configure);
	if (edit_configuration) {
		if (psy_audio_audiodrivers_enabled(self)) {
			psy_audiodriver_restart(psy_audio_audiodriverplugin_base(&
				self->driver_plugin), edit_configuration);
		}		
		psy_audio_audiodrivers_store_configuration(self, edit_configuration);
	}
}

void psy_audio_audiodrivers_store_configuration(psy_audio_AudioDrivers* self,
	const psy_Property* configuration)
{	
	intptr_t guid;
	psy_Property* configurations;
	
	assert(self);
	
	guid = psy_property_at_int(configuration, "guid", 0);
	if (guid == 0) {
		return;
	}
	configurations = psy_configuration_at(self->config_, "configurations");
	psy_audio_audiodrivers_remove_configuration(self, guid);	
	psy_property_hide(psy_property_append_property(configurations,
		psy_property_clone(configuration)));
}

void psy_audio_audiodrivers_remove_configuration(psy_audio_AudioDrivers* self,
	intptr_t guid)
{
	psy_Property* stored_configuration;

	assert(self);
	
	stored_configuration = (psy_Property*)audiodrivers_read_configuration_const(
		self, guid);
	if (stored_configuration) {
		psy_Property* configurations;
		
		configurations = psy_configuration_at(self->config_, "configurations");
		psy_property_remove(configurations, stored_configuration);
	}
}

void psy_audio_audiodrivers_on_driver_select(psy_audio_AudioDrivers* self,
	psy_Property* sender)
{	
	assert(self);
	
	psy_audio_audiodrivers_select_driver(self,
		psy_audio_audiodrivers_selected_guid(self),
		psy_audio_audiodrivers_enabled(self));
}

bool psy_audio_audiodrivers_enabled(const psy_audio_AudioDrivers* self)
{
	assert(self);
	
	return psy_audiodriver_opened(psy_audio_audiodriverplugin_base_const(
		&self->driver_plugin));	
}

intptr_t psy_audio_audiodrivers_selected_guid(const psy_audio_AudioDrivers*
	self)
{
	const psy_Property* drivers;
	
	assert(self);
	
	drivers = psy_configuration_at(self->config_, "audiodrivers");
	if (drivers) {
		psy_Property* p;
		
		p = psy_property_at_choice((psy_Property*)drivers);
		if (p) {
			return atoi(psy_property_key(p));
		}
	}
	return 0;
}
