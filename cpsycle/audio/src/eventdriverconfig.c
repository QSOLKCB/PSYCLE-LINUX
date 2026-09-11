/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "eventdriverconfig.h"
/* host */
// #include "resources/resource.h"
/* ui */
// #include <uiopendialog.h>
// #include <uisavedialog.h>
/* audio */
#include "eventdriverplugin.h"
/* driver */
#include "../../driver/eventdriver.h"
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void eventdriverconfig_register_drivers(EventDriverConfig*);
static psy_Property* eventdriverconfig_make(EventDriverConfig*);
static void eventdriverconfig_make_operations(EventDriverConfig*);
static void eventdriverconfig_reset(EventDriverConfig*);
static void eventdriverconfig_load(EventDriverConfig*);
static void eventdriverconfig_save(EventDriverConfig*);
static intptr_t eventdriverconfig_selected_guid(const EventDriverConfig*);
static void eventdriverconfig_on_load(EventDriverConfig*, psy_Property* sender);
static void eventdriverconfig_on_save(EventDriverConfig*, psy_Property* sender);
static void eventdriverconfig_append_active(EventDriverConfig*, intptr_t guid);
static void eventdriverconfig_append_configuration(EventDriverConfig*,
	const char* key, intptr_t guid);
static void psy_audio_eventdriverconfig_update_edit_configure(EventDriverConfig*);
static bool eventdriverconfig_postload(EventDriverConfig*, uintptr_t count);

/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(EventDriverConfig* self)
{
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.postload =
			(psy_fp_configuration_postload)
			eventdriverconfig_postload;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}

/* implementation */
void eventdriverconfig_init(EventDriverConfig* self, psy_Property* parent,
	psy_Property* cmds)
{
	assert(self);

	psy_customconfiguration_init(&self->configuration);
	vtable_init(self);
	self->config = parent;
	self->drivers = NULL;
	self->activedrivers = NULL;	
	self->cmds = cmds;
	psy_customconfiguration_set_root(&self->configuration,
		eventdriverconfig_make(self));
}

void eventdriverconfig_dispose(EventDriverConfig* self)
{
	assert(self);
			
	self->config = NULL;
	self->drivers = NULL;
	self->activedrivers = NULL;
	psy_property_deallocate(self->cmds);
	self->cmds = NULL;	
	psy_audio_driverregister_dispose(&self->driver_register);	
}

void eventdriverconfig_register_drivers(EventDriverConfig* self)
{	
	assert(self);
	
	psy_audio_driverregister_register(&self->driver_register,
		PSY_EVENTDRIVER_KBD_GUID, "kbd", "Keyboard");
	psy_audio_driverregister_set_default_guid(&self->driver_register,
		PSY_EVENTDRIVER_KBD_GUID);
	psy_audio_driverregister_update_default(&self->driver_register);
#if defined(DIVERSALIS__OS__MICROSOFT)	
	psy_audio_driverregister_register(&self->driver_register,
		PSY_EVENTDRIVER_MMEMIDI_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "mmemidi.dll", "Mmme Midi");
	psy_audio_driverregister_register(&self->driver_register,
		PSY_EVENTDRIVER_DXJOYSTICK_GUID, PSYCLE_AUDIO_DRIVER_DIR psy_SLASHSTR "dxjoystick.dll", "DX Joystick");
#elif defined(DIVERSALIS__OS__LINUX)	
	psy_audio_driverregister_register(&self->driver_register,
		PSY_EVENTDRIVER_ALSAMIDI_GUID,
		PSYCLE_AUDIO_DRIVER_DIR"/alsamidi/libpsyalsamidi.so",
		"Alsa Midi");
	psy_audio_driverregister_register(&self->driver_register,
		PSY_EVENTDRIVER_EVJOYSTICK_GUID,
		PSYCLE_AUDIO_DRIVER_DIR"/build/libpsyevjoystick.so",
		"Evdev Joystick");
#endif	
}

psy_Property* eventdriverconfig_make(EventDriverConfig* self)
{
	psy_Property* rv;

	assert(self);

	rv = self->eventinputs = psy_property_set_text(psy_property_append_section(
		self->config, "eventinput"), "Event Input");
	psy_property_set_text(self->eventinputs, "settings.event-input");	
	psy_property_set_svg(self->eventinputs, "img.patterns");
	psy_audio_driverregister_init(&self->driver_register, self->eventinputs,
		"installeddrivers", "Input Drivers");
	self->drivers = self->driver_register.drivers_;
	eventdriverconfig_register_drivers(self);	
	eventdriverconfig_make_operations(self);
	self->configurations = psy_property_hide(
		psy_property_append_section(self->eventinputs, "configurations"));
	eventdriverconfig_append_active(self, PSY_EVENTDRIVER_KBD_GUID);
	psy_audio_eventdriverconfig_update_edit_configure(self);
	return rv;
}

void eventdriverconfig_make_operations(EventDriverConfig* self)
{
	psy_Property* diskop;

	assert(self);
		
	psy_property_set_text(
		psy_property_append_action(self->eventinputs, "addeventdriver"),
		"Add to active drivers");
	self->activedrivers = psy_property_enableappend(
		psy_property_set_text(psy_property_append_choice(
			self->eventinputs, "activedrivers", 0), "Active Drivers"));	
	psy_property_set_text(
		psy_property_append_action(self->eventinputs, "removeeventdriver"),
		"Remove active driver");
	diskop = psy_property_set_text(psy_property_append_section(
		self->eventinputs, "diskop"), "Diskop");	
	psy_property_prevent_save(diskop);
	psy_property_set_text(
		psy_property_append_action(diskop, "defaults"),
		"Defaults");
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(diskop, "load"),
		"Load"), self, eventdriverconfig_on_load);
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(diskop, "save"),
		"Save"), self, eventdriverconfig_on_save);	
	self->edit_configure = psy_property_enable_foldable(psy_property_set_text(
		psy_property_append_section(self->eventinputs, "configure"),
		"Configure"), TRUE);	
	psy_property_prevent_save(self->edit_configure);	
}


void eventdriverconfig_load(EventDriverConfig* self)
{
	// psy_EventDriver* eventdriver;

	assert(self);

	//eventdriver = eventdriverconfig_selected_driver(self);
	// if (eventdriver && psy_eventdriver_configuration(eventdriver)) {
		// psy_ui_OpenDialog opendialog;

		//psy_ui_opendialog_init_all(&opendialog, 0,
		//	"Load Event Driver Configuration",
		//	"Psycle Event Driver Configuration|*.psk", "PSK",
		//	psy_dir_config());
		//if (psy_ui_opendialog_execute(&opendialog)) {			
		//	psy_Property* local;
		//	psy_PropertyReader propertyreader;

		//	local = psy_property_clone(psy_eventdriver_configuration(eventdriver));			
		//	psy_propertyreader_init(&propertyreader, local,
		//		psy_path_full(psy_ui_opendialog_path(
		//			&opendialog)));
		//	psy_propertyreader_load(&propertyreader);
		//	psy_propertyreader_dispose(&propertyreader);
		//	psy_eventdriver_configure(eventdriver, local);
		//	if (self->activedrivers) {
		//		eventdriverconfig_show_active(self,
		//			psy_property_item_int(self->activedrivers));
		//	}
			/* eventdriverconfig_update_driverconfigure_section(self); */
		//	psy_property_deallocate(local);
		//}
		//psy_ui_opendialog_dispose(&opendialog);
	// }
}

void eventdriverconfig_save(EventDriverConfig* self)
{
	// psy_EventDriver* eventdriver;

	assert(self);

	// eventdriver = eventdriverconfig_selected_driver(self);
	// if (eventdriver && psy_eventdriver_configuration(eventdriver)) {
		/*psy_ui_SaveDialog dialog;
		int success;

		psy_ui_savedialog_init_all(&dialog, 0,
			"Save Event Driver Configuration",
			"Psycle Event Driver Configuration|*.psk", "PSK",
			psy_dir_config());
		success = psy_ui_savedialog_execute(&dialog);
		if (success) {			
			psy_PropertyWriter propertywriter;

			psy_propertywriter_init(&propertywriter,
				psy_eventdriver_configuration(eventdriver),
				psy_path_full(psy_ui_savedialog_path(&dialog)));
			psy_propertywriter_save(&propertywriter);
			psy_propertywriter_dispose(&propertywriter);
		}
		psy_ui_savedialog_dispose(&dialog);*/
	// }
}

intptr_t eventdriverconfig_current(EventDriverConfig* self)
{
	assert(self && self->activedrivers);
	
	return psy_property_item_int(self->activedrivers);	
}

intptr_t eventdriverconfig_selected_guid(const EventDriverConfig* self)
{
	psy_Property* p;	
	
	assert(self);
	assert(self->drivers);
		
	p = psy_property_at_choice(((EventDriverConfig*)self)->drivers);
	if (p && psy_property_type(p) == PSY_PROPERTY_TYPE_INTEGER) {		
		return psy_property_item_int(p);
	}
	return 0;
}

void eventdriverconfig_append_active(EventDriverConfig* self, intptr_t guid)
{	
	uintptr_t num_actives;
	char key[256];
	
	assert(self);
	
	num_actives = psy_property_size(self->activedrivers);
	psy_snprintf(key, 256, "configuration-%i", num_actives);
	psy_property_set_text(
		psy_property_append_int(self->activedrivers, key,
		guid, 0, 0),
		psy_audio_driverregister_label(&self->driver_register, guid));	
	eventdriverconfig_append_configuration(self, key, guid);
}

void eventdriverconfig_append_configuration(EventDriverConfig* self,
	const char* key, intptr_t guid)
{		
	assert(self);
	
	psy_audio_EventDriverPlugin plugin;	
	psy_Property* section;
	const psy_Property* configuration;
	const char* path;

	assert(self);
			
	section = psy_property_append_section(self->configurations, key);
	path = psy_audio_driverregister_path(&self->driver_register, guid);
	if (psy_strlen(path) == 0) {
		return;
	}
	psy_audio_eventdriverplugin_init(&plugin);
	psy_audio_eventdriverplugin_load(&plugin, path);
	psy_eventdriver_setcmddef(psy_audio_eventdriverplugin_base(&plugin),
		self->cmds);
	configuration = psy_eventdriver_configuration(
		psy_audio_eventdriverplugin_base(&plugin));	
	if (configuration) {
		const psy_EventDriverInfo* driverinfo;

		driverinfo = psy_eventdriver_info(
			psy_audio_eventdriverplugin_base(&plugin));
		if (driverinfo) {					
			psy_property_append_property(section, psy_property_clone(
				psy_eventdriver_configuration(
				psy_audio_eventdriverplugin_base(&plugin))));			
		}		
	}
	psy_audio_eventdriverplugin_dispose(&plugin);		
}

void psy_audio_eventdriverconfig_update_edit_configure(EventDriverConfig* self)
{	
	psy_Property* configuration;
	psy_Property* active;
	uintptr_t index;
	
	assert(self);
		
	psy_property_clear(self->edit_configure);
	index = psy_property_item_int(self->activedrivers);
	active = psy_property_at_index(self->activedrivers, index);
	if (!active) {
		return;
	}	
	configuration = psy_property_at(self->configurations,
		psy_property_key(active), PSY_PROPERTY_TYPE_NONE);
	if (!configuration) {
		return;
	}
	configuration = psy_property_first(configuration);
	if (!configuration) {
		return;
	}
	psy_property_append_property(self->edit_configure,
		psy_property_clone(configuration));
}

/* events */

void eventdriverconfig_on_load(EventDriverConfig* self, psy_Property* sender)
{
	assert(self);

	eventdriverconfig_load(self);
}

void eventdriverconfig_on_save(EventDriverConfig* self, psy_Property* sender)
{
	assert(self);

	eventdriverconfig_save(self);
}

bool eventdriverconfig_postload(EventDriverConfig* self, uintptr_t count)
{	
	if (count == 0) {
		psy_List* p;

		assert(self);

		psy_property_clear(self->configurations);
		for (p = psy_property_begin(self->activedrivers); p != NULL; p = p->next) {
			psy_Property* active;
			intptr_t guid;

			active = (psy_Property*)p->entry;
			guid = psy_property_item_int(active);
			eventdriverconfig_append_configuration(self, psy_property_key(active),
				guid);
		}		
		return TRUE;
	}
	psy_audio_eventdriverconfig_update_edit_configure(self);
	return FALSE;
}
