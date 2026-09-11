/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineparamconfig.h"
/* file */
#include <fileselect.h>
/* container */
#include <configuration.h>
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* machineparamconfig_make(MachineParamConfig*, psy_Property*);
static void machineparamconfig_make_theme(MachineParamConfig*, psy_Property*);
static void machineparamconfig_on_load_control_skin(MachineParamConfig*,
	psy_Property* sender);
static void machineparamconfig_on_load_control_skin_change(MachineParamConfig*,
	psy_FileSelect* sender);
static void machineparamconfig_on_reset_control_skin(MachineParamConfig*,
	psy_Property* sender);
	
static void machineparamconfig_connect(MachineParamConfig*, const char* key,
	void* context, void* fp);
static psy_Property* machineparamconfig_at(MachineParamConfig*,
	const char* key);
static void machineparamconfig_reset(MachineParamConfig*);
	
/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MachineParamConfig* self)
{
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.reset =
			(psy_fp_configuration_reset)
			machineparamconfig_reset;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}	

/* implementation */
void machineparamconfig_init(MachineParamConfig* self, psy_Property* parent,
	psy_DiskOp* disk_op)
{
	assert(self);	

	psy_customconfiguration_init(&self->configuration);
	vtable_init(self);	
	self->disk_op = disk_op;
	psy_customconfiguration_set_root(&self->configuration,
		machineparamconfig_make(self, parent));	
}

void machineparamconfig_dispose(MachineParamConfig* self)
{
	assert(self);		

	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* machineparamconfig_make(MachineParamConfig* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "paramview"),
		"settings.param.native-machine-parameter-window");
	psy_property_set_text(
		psy_property_append_font(rv, "font",
		PSYCLE_DEFAULT_MACPARAM_FONT),
		"settings.param.font");		
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(rv, "loadcontrolskin"),
		"settings.param.load-dial-bitmap"),
		self, machineparamconfig_on_load_control_skin);
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(rv, "defaultskin"),
		"settings.param.default-skin"),
		self, machineparamconfig_on_reset_control_skin);
	machineparamconfig_make_theme(self, rv);
	return rv;
}

void machineparamconfig_make_theme(MachineParamConfig* self,
	psy_Property* parent)
{
	assert(self);

	self->theme = psy_property_enable_foldable(psy_property_set_text(
		psy_property_append_section(parent, "theme"),
		"settings.param.theme.theme"), TRUE);	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_str(self->theme,
			"machinedial_bmp", ""),
			PSY_PROPERTY_HINT_EDIT),
		"settings.param.theme.machinedialbmp");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUITitleColor", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.title-background");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUITitleFontColor", 0x00B4B4B4, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.title-font");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUITopColor", 0x00555555, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.param-background");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIFontTopColor", 0x00CDCDCD, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.param-font");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIBottomColor", 0x00444444, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.value-background");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIFontBottomColor", 0x00E7BD18, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.value-font");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIHTopColor", 0x00555555, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.selparam-background");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIHFontTopColor", 0x00CDCDCD, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.selparam-font");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIHBottomColor", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.selvalue-background");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"machineGUIHFontBottomColor", 0x00E7BD18, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.param.theme.selvalue-font");	
}

void machineparamconfig_reset(MachineParamConfig* self)
{	
	assert(self);
	
	psy_property_set_str(self->theme, "machinedial_bmp", ""),
	psy_property_set_int(self->theme, "machineGUITitleColor", 0x00292929);
	psy_property_set_int(self->theme, "machineGUITitleFontColor", 0x00B4B4B4);
	psy_property_set_int(self->theme, "machineGUITopColor", 0x00555555);
	psy_property_set_int(self->theme, "machineGUIFontTopColor", 0x00CDCDCD);
	psy_property_set_int(self->theme, "machineGUIBottomColor", 0x00444444);
	psy_property_set_int(self->theme, "machineGUIFontBottomColor", 0x00E7BD18);
	psy_property_set_int(self->theme, "machineGUIHTopColor", 0x00555555);
	psy_property_set_int(self->theme, "machineGUIHFontTopColor", 0x00CDCDCD);
	psy_property_set_int(self->theme, "machineGUIHBottomColor", 0x00292929);
	psy_property_set_int(self->theme, "machineGUIHFontBottomColor", 0x00E7BD18);
}

void machineparamconfig_on_load_control_skin(MachineParamConfig* self,
	psy_Property* sender)
{
	assert(self);
			
	if (self->disk_op) {
		psy_FileSelect load;				
				
		psy_fileselect_init(&load);
		psy_fileselect_connect_change(&load, self, (psy_fp_fileselect_change)
			machineparamconfig_on_load_control_skin_change);
		psy_fileselect_set_title(&load, "Load Dial Bitmap");
		psy_fileselect_set_default_extension(&load, "psc");	
		psy_fileselect_add_filter(&load, "Control Skins", "*.psc");
		psy_fileselect_add_filter(&load, "Bitmaps", "*.bmp");
		psy_diskop_execute(self->disk_op, &load, NULL, NULL);
		psy_fileselect_dispose(&load);
	}
}

void machineparamconfig_on_load_control_skin_change(MachineParamConfig* self,
	psy_FileSelect* sender)
{
	assert(self);
	
	psy_property_set_str(self->theme, "machinedial_bmp",  psy_fileselect_value(
		sender));
}

void machineparamconfig_on_reset_control_skin(MachineParamConfig* self,
	psy_Property* sender)
{
	assert(self);
	
	machineparamconfig_reset(self);
}
