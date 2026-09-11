/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "eventdrivers.h"

/* driver */
#include "../../driver/eventdriver.h"
/* local */
#include "eventdriverplugin.h"
#include "library.h"
#include "kbddriver.h"
/* portable */
#include "../../detail/portable.h"


/* prototypes */
static void eventdrivers_on_driver_input(psy_audio_EventDrivers*,
	psy_EventDriver*);
static void psy_audio_eventdrivers_on_add_event_driver(psy_audio_EventDrivers*,
	psy_Property* sender);
static void psy_audio_eventdrivers_add_event_driver(psy_audio_EventDrivers*);
static void psy_audio_eventdrivers_on_active_event_driver(psy_audio_EventDrivers*,
	psy_Property* sender);
static void psy_audio_eventdrivers_on_edit_configuration(psy_audio_EventDrivers*,
	psy_Property* sender);
static intptr_t psy_audio_eventdrivers_selected_event_driver_guid(psy_audio_EventDrivers*);
static void psy_audio_eventdrivers_on_reset_event_driver(psy_audio_EventDrivers*,
	psy_Property* sender);
static void psy_audio_eventdrivers_reset_event_driver(psy_audio_EventDrivers*);
static void psy_audio_eventdrivers_on_remove_event_driver(psy_audio_EventDrivers*,
	psy_Property* sender);
static void psy_audio_eventdrivers_remove_active_event_driver(psy_audio_EventDrivers*);
static void psy_audio_eventdrivers_make_event_configurations(psy_audio_EventDrivers*);
static void psy_audio_eventdrivers_show_active_event_driver(psy_audio_EventDrivers*,
	intptr_t index);
static psy_EventDriver* psy_audio_eventdrivers_load(psy_audio_EventDrivers*,
	const char* path, const psy_Property* config);
static void psy_audio_eventdrivers_restart(psy_audio_EventDrivers*, intptr_t id,
	const psy_Property* configuration);
static void psy_audio_eventdrivers_remove(psy_audio_EventDrivers*, uintptr_t index);
static psy_EventDriver* psy_audio_eventdrivers_selected_event_driver(
	psy_audio_EventDrivers*);

/* implementation */
void psy_audio_eventdrivers_init(psy_audio_EventDrivers* self,
	void* systemhandle, psy_Configuration* config, psy_Property* cmddef)
{		
	assert(self);

	self->config_ = config;
	self->drivers = NULL;	
	self->kbddriver = NULL;
	self->logger = NULL;
	if (cmddef) {
		self->cmds = psy_property_clone(cmddef);
	} else {
		self->cmds = NULL;
	}
	self->systemhandle = systemhandle;	
	psy_signal_init(&self->signal_input);	
	if (self->config_) {
		psy_Property* edit_configure;
		
		edit_configure = psy_configuration_at(self->config_, "configure");
		if (edit_configure) {			
			psy_property_connect_children(edit_configure, TRUE,
				self, psy_audio_eventdrivers_on_edit_configuration);
		}
		psy_configuration_connect(self->config_, "addeventdriver",
			self, psy_audio_eventdrivers_on_add_event_driver);
		psy_configuration_connect(self->config_, "removeeventdriver",
			self, psy_audio_eventdrivers_on_remove_event_driver);
		psy_configuration_connect(self->config_, "activedrivers",
			self, psy_audio_eventdrivers_on_active_event_driver);
		psy_configuration_connect(self->config_, "diskop.defaults",
			self, psy_audio_eventdrivers_on_reset_event_driver);	
	}
}

void psy_audio_eventdrivers_dispose(psy_audio_EventDrivers* self)
{
	psy_list_deallocate(&self->drivers, (psy_fp_disposefunc)
		psy_audio_eventdriverplugin_dispose);	
	psy_property_deallocate(self->cmds);
	self->cmds = NULL;	
	psy_signal_dispose(&self->signal_input);	
}

void psy_audio_eventdrivers_load_actives(psy_audio_EventDrivers* self)
{
	psy_Property* actives;	
	
	if (!self->config_) {
		psy_audio_eventdrivers_load(self, "kbd", NULL);
		return;
	}
	actives = psy_configuration_at(self->config_, "activedrivers");
	if (actives) {
		psy_List* p;
				
		for (p = psy_property_begin(actives); p != NULL; p = p->next) {
			psy_Property* active;
			intptr_t guid;
			char key[64];
			psy_Property* path;
			psy_Property* installeddrivers;
									
			active = (psy_Property*)p->entry;			
			guid = psy_property_item_int(active);
			installeddrivers = psy_configuration_at(self->config_,
				"installeddrivers");
			if (installeddrivers) {							
				psy_snprintf(key, 64, "%d", (int)guid);				
				path = psy_property_at(installeddrivers, key,
					PSY_PROPERTY_TYPE_NONE);
				if (path) {
					psy_Property* configuration;
					psy_Property* configurations;
	
					configuration = NULL;
					configurations = psy_configuration_at(self->config_, "configurations");
					if (configurations) {																	
						configuration = psy_property_at(configurations,
							psy_property_key(active), PSY_PROPERTY_TYPE_NONE);	
						if (configuration) {
							configuration = psy_property_first(configuration);
						}
					}					
					psy_audio_eventdrivers_load(self, psy_property_item_str(path),
						configuration);
				}
			}
		}		
	}	
}

psy_EventDriver* psy_audio_eventdrivers_load(psy_audio_EventDrivers* self,
	const char* path, const psy_Property* config)
{
	psy_EventDriver* rv;
	
	rv = NULL;
	if (path) {
		psy_audio_EventDriverPlugin* plugin;
				
		plugin = psy_audio_eventdriverplugin_alloc_init();
		if (!plugin) {
			return NULL;
		}
		plugin->driver.logger = self->logger;
		psy_audio_eventdriverplugin_load(plugin, path);
		if (strcmp(path, "kbd") == 0) {			
			self->kbddriver = psy_audio_eventdriverplugin_base(plugin);
		}
		psy_audio_eventdriverplugin_connect(plugin, self, (EVENTDRIVERWORKFN)
			eventdrivers_on_driver_input);
		psy_eventdriver_setcmddef(psy_audio_eventdriverplugin_base(plugin),
			self->cmds);
		if (config) {
			psy_eventdriver_configure(psy_audio_eventdriverplugin_base(plugin),
				config);
		}
		psy_eventdriver_open(psy_audio_eventdriverplugin_base(plugin));
		if (!plugin->client) {
			psy_eventdriver_dispose(psy_audio_eventdriverplugin_base(plugin));
			free(plugin);
		} else {
			rv = psy_audio_eventdriverplugin_base(plugin);
			psy_list_append(&self->drivers, plugin);			 
		}		
	}	
	return rv;
}

void psy_audio_eventdrivers_restart(psy_audio_EventDrivers* self, intptr_t id,
	const psy_Property* configuration)
{	
	psy_EventDriver* driver;

	driver = psy_audio_eventdrivers_driver(self, id);
	if (driver) {
		psy_eventdriver_close(driver);
		psy_eventdriver_configure(driver, configuration);		
		psy_eventdriver_open(driver);
	}
}

void psy_audio_eventdrivers_remove(psy_audio_EventDrivers* self,
	uintptr_t index)
{
	psy_List* p;
	psy_EventDriver* driver;
	
	driver = NULL;
	p = psy_list_at(self->drivers, index);
	if (p) {
		driver = (psy_EventDriver*)p->entry;		
		if (driver == self->kbddriver) {
			self->kbddriver = NULL;
		}
		psy_list_remove(&self->drivers, p);
	}	
	if (driver) {
		psy_eventdriver_deallocate(driver);		
	}
}

uintptr_t psy_audio_eventdrivers_size(psy_audio_EventDrivers* self)
{
	uintptr_t rv = 0;
	psy_List* p;
	
	for (p = self->drivers; p != NULL; psy_list_next(&p), ++rv);
	return rv;
}

psy_EventDriver* psy_audio_eventdrivers_driver(psy_audio_EventDrivers* self,
	uintptr_t index)
{
	psy_List* p;
	psy_EventDriver* driver;
	
	driver = NULL;
	p = psy_list_at(self->drivers, index);
	if (p) {
		return (psy_EventDriver*)(p->entry);
	}
	return NULL;
}

void eventdrivers_on_driver_input(psy_audio_EventDrivers* self,
	psy_EventDriver* sender)
{
	psy_signal_emit(&self->signal_input, sender, 0);
}

void psy_audio_eventdrivers_idle(psy_audio_EventDrivers* self)
{
	psy_List* p;

	for (p = self->drivers; p != NULL; psy_list_next(&p)) {		
		psy_EventDriver* driver;

		driver = (psy_EventDriver*)psy_list_entry(p);		
		if (driver) {			
			psy_eventdriver_idle(driver);
		}
	}
}

void psy_audio_eventdrivers_connect(psy_audio_EventDrivers* self, void* context,
	fp_eventdriver_input fp)
{
	psy_signal_connect(&self->signal_input, context, fp);
}

void psy_audio_eventdrivers_on_add_event_driver(psy_audio_EventDrivers* self,
	psy_Property* sender)
{
	assert(self);

	psy_audio_eventdrivers_add_event_driver(self);
}

void psy_audio_eventdrivers_add_event_driver(psy_audio_EventDrivers* self)
{
	intptr_t guid;

	assert(self);

	guid = psy_audio_eventdrivers_selected_event_driver_guid(self);
	if (guid != 0) {
		psy_EventDriver* driver;
		psy_Property* activedrivers;
		psy_Property* driver_path;
		char key[64];
			
		psy_snprintf(key, 64, "installeddrivers.%d", (int)guid);
		driver_path = psy_configuration_at(self->config_, key);
		if (!driver_path) {
			return;
		}
		driver = psy_audio_eventdrivers_load(self,
			psy_property_item_str(driver_path), self->cmds);

		if (driver) {
			psy_eventdriver_setcmddef(driver, self->cmds);
		}
		activedrivers = psy_configuration_at(self->config_, "activedrivers");
		if (!activedrivers) {
			return;
		}
		psy_audio_eventdrivers_update_active(self);
		psy_property_set_item_int(activedrivers,
			psy_audio_eventdrivers_size(self) - 1);
		psy_audio_eventdrivers_make_event_configurations(self);
		psy_audio_eventdrivers_show_active_event_driver(self,
			psy_property_item_int(activedrivers));
	}
}

intptr_t psy_audio_eventdrivers_selected_event_driver_guid(
	psy_audio_EventDrivers* self)
{
	psy_Property* p;
	psy_Property* drivers;

	assert(self);
	
	drivers = psy_configuration_at(self->config_, "installeddrivers");
	p = psy_property_at_choice(drivers);
	if (p) {
		return atoi(psy_property_key(p));
	}
	return 0;
}

void psy_audio_eventdrivers_on_active_event_driver(psy_audio_EventDrivers* self,
	psy_Property* sender)
{
	assert(self);

	psy_audio_eventdrivers_show_active_event_driver(self, psy_property_item_int(
		sender));
}

void psy_audio_eventdrivers_show_active_event_driver(psy_audio_EventDrivers* self,
	intptr_t index)
{
	psy_EventDriver* driver;
	psy_Property* edit_configure;

	assert(self);

	edit_configure = psy_configuration_at(self->config_, "configure");
	if (!edit_configure) {
		return;
	}
	psy_property_clear(edit_configure);
	driver = psy_audio_eventdrivers_driver(self, index);
	if (driver && psy_eventdriver_configuration(driver)) {
		psy_property_append_property(edit_configure,
			psy_property_clone(psy_eventdriver_configuration(driver)));		
		psy_property_connect_children(edit_configure, TRUE,
			self, psy_audio_eventdrivers_on_edit_configuration);
	}
	psy_property_rebuild(edit_configure);
}

void psy_audio_eventdrivers_on_edit_configuration(psy_audio_EventDrivers* self,
	psy_Property* sender)
{	
	psy_Property* edit_configure;
	psy_Property* edit_configuration;	

	assert(self);
	
	edit_configure = psy_configuration_at(self->config_, "configure");
	if (!edit_configure) {
		return;
	}
	edit_configuration = psy_property_first(edit_configure);
	if (edit_configuration) {
		psy_Property* activedrivers;

		activedrivers = psy_configuration_at(self->config_, "activedrivers");
		if (!activedrivers) {
			return;
		}		
		psy_audio_eventdrivers_restart(self, psy_property_item_int(
			activedrivers), edit_configuration);
		psy_audio_eventdrivers_make_event_configurations(self);
	}
}

void psy_audio_eventdrivers_on_reset_event_driver(psy_audio_EventDrivers* self,
	psy_Property* sender)
{
	assert(self);

	psy_audio_eventdrivers_reset_event_driver(self);
}

void psy_audio_eventdrivers_reset_event_driver(psy_audio_EventDrivers* self)
{
	psy_Property* activedrivers;

	assert(self);

	activedrivers = psy_configuration_at(self->config_, "activedrivers");
	if (!activedrivers) {
		return;
	}
	if (psy_audio_eventdrivers_selected_event_driver(self)) {
		psy_eventdriver_configure(psy_audio_eventdrivers_selected_event_driver(self),
			NULL);
		psy_audio_eventdrivers_show_active_event_driver(self,
			psy_property_item_int(activedrivers));
	}
}

psy_EventDriver* psy_audio_eventdrivers_selected_event_driver(psy_audio_EventDrivers* self)
{
	psy_Property* activedrivers;

	assert(self);

	activedrivers = psy_configuration_at(self->config_, "activedrivers");
	if (!activedrivers) {
		return NULL;
	}
	return psy_audio_eventdrivers_driver(self,
		psy_property_item_int(activedrivers));
}

void psy_audio_eventdrivers_on_remove_event_driver(psy_audio_EventDrivers* self,
	psy_Property* sender)
{
	assert(self);

	psy_audio_eventdrivers_remove_active_event_driver(self);
}

void psy_audio_eventdrivers_remove_active_event_driver(psy_audio_EventDrivers* self)
{
	psy_Property* activedrivers;

	assert(self);

	activedrivers = psy_configuration_at(self->config_, "activedrivers");
	if (!activedrivers) {
		return;
	}
	psy_audio_eventdrivers_remove(self,	psy_property_item_int(activedrivers));
	psy_audio_eventdrivers_update_active(self);
	if (psy_property_item_int(activedrivers) > 0) {
		psy_property_set_item_int(activedrivers,
			psy_property_item_int(activedrivers) - 1);
	}
	psy_audio_eventdrivers_show_active_event_driver(self,
		psy_property_item_int(activedrivers));
}

void psy_audio_eventdrivers_update_active(psy_audio_EventDrivers* self)
{
	psy_Property* activedrivers;
	uintptr_t numdrivers;
	uintptr_t i;

	assert(self);
	
	if (!self->config_) {
		return;
	}
	activedrivers = psy_configuration_at(self->config_, "activedrivers");
	if (!activedrivers) {
		return;
	}
	psy_property_clear(activedrivers);
	numdrivers = psy_audio_eventdrivers_size(self);
	for (i = 0; i < numdrivers; ++i) {
		psy_EventDriver* driver;

		driver = psy_audio_eventdrivers_driver(self, i);
		if (driver) {
			const psy_EventDriverInfo* driverinfo;

			driverinfo = psy_eventdriver_info(driver);
			if (driverinfo) {
				char key[40];

				psy_snprintf(key, 40, "configuration-%d", (int)i);
				psy_property_set_text(
					psy_property_append_int(activedrivers, key,
						driverinfo->guid, 0, 0),
					driverinfo->Name);
			}
		}
	}
	psy_property_rebuild(activedrivers);
}

void psy_audio_eventdrivers_make_event_configurations(psy_audio_EventDrivers* self)
{
	psy_Property* configurations;
	uintptr_t numdrivers;
	uintptr_t i;
	
	assert(self);

	configurations = psy_configuration_at(self->config_, "configurations");
	if (!configurations) {
		return;
	}
	psy_property_clear(configurations);
	numdrivers = psy_audio_eventdrivers_size(self);
	for (i = 0; i < numdrivers; ++i) {
		psy_EventDriver* driver;

		driver = psy_audio_eventdrivers_driver(self, i);
		if (driver) {						
			if (psy_eventdriver_configuration(driver)) {
				char key[256];
				psy_Property* section;

				psy_snprintf(key, 256, "configuration-%d", i);
				section = psy_property_append_section(configurations, key);
				psy_property_append_property(section, psy_property_clone(
					psy_eventdriver_configuration(driver)));
			}
		}
	}
}
