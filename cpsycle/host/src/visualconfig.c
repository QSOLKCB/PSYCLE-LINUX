/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "visualconfig.h"
/* host */
#include "skinio.h"
#include "styles.h"
#include "workspace.h"
/* ui */
#include <uicomponent.h> /* Translator */
#include <uiapp.h> /* Styles */
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"

#if defined DIVERSALIS__COMPILER__GNU || defined DIVERSALIS__OS__POSIX
#define _MAX_PATH 4096
#endif


/* prototypes */
static void visualconfig_make(VisualConfig*, psy_Property*);
static void visualconfig_make_vu(VisualConfig*, psy_Property* parent);
static void visualconfig_reset_vu(VisualConfig*);
static void visualconfig_on_reset_skin(VisualConfig*, psy_Property* sender);
static void visualconfig_on_load_skin(VisualConfig*, psy_Property* sender);
static void visualconfig_on_load_file_skin_change(VisualConfig*,
	psy_FileSelect* sender);
static bool visualconfig_postload(VisualConfig*, uintptr_t count);
static void visualconfig_set_skin(VisualConfig*, psy_Configuration*,
	psy_Property* theme);
static void visualconfig_load_skin(VisualConfig*, const char* path);
static void visualconfig_reset_skin(VisualConfig*);
static void visualconfig_reset_control_skin(VisualConfig*);

/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(VisualConfig* self)
{
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.postload =
			(psy_fp_configuration_postload)
			visualconfig_postload;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}

/* implementation */
void visualconfig_init(VisualConfig* self, psy_Property* parent,
	psy_DiskOp* disk_op)
{
	assert(self);
	assert(parent);	

	psy_customconfiguration_init(&self->configuration);
	vtable_init(self);
	self->disk_op = disk_op;
	self->parent = parent;
	self->dirconfig = NULL;	
	visualconfig_make(self, parent);
	psy_customconfiguration_set_root(&self->configuration,
		self->visual);
}

void visualconfig_dispose(VisualConfig* self)
{
	assert(self);

	patternviewconfig_dispose(&self->patview);
	machineviewconfig_dispose(&self->macview);
	machineparamconfig_dispose(&self->macparam);	
}

void visualconfig_make(VisualConfig* self, psy_Property* parent)
{
	assert(self);

	self->visual = psy_property_set_text(
		psy_property_append_section(parent, "visual"),
		"settings.visual.visual");
	psy_property_set_svg(self->visual, "img.visual");
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(self->visual, "loadskin"),
			"settings.visual.load-skin"),
			self, visualconfig_on_load_skin);
	psy_property_connect(psy_property_set_text(
		psy_property_append_action(self->visual, "defaultskin"),
		"settings.visual.default-skin"), self, visualconfig_on_reset_skin);
	self->defaultfont = psy_property_set_text(
		psy_property_append_font(self->visual, "defaultfont",
			PSYCLE_DEFAULT_FONT), "settings.visual.default-font");
	psy_property_set_hint(psy_property_set_text(
		psy_property_append_double(self->visual, "zoom", 1.0, 0.1, 4.0),
		"settings.visual.zoom"), PSY_PROPERTY_HINT_ZOOM);
	self->apptheme = psy_property_set_text(
		psy_property_append_choice(self->visual, "apptheme", 1),
		"settings.visual.apptheme");			
	psy_property_set_text(psy_property_append_int(
		self->apptheme, "light", psy_ui_LIGHTTHEME, 0, 2),
		"settings.visual.light");
	psy_property_set_text(psy_property_append_int(
		self->apptheme, "dark", psy_ui_DARKTHEME, 0, 2),
		"settings.visual.dark");
	/* psy_property_set_text(psy_property_append_int(
		self->apptheme, "win98", psy_ui_WIN98THEME, 0, 2),
		"Windows 98"); */
	patternviewconfig_init(&self->patview, self->visual);
	machineviewconfig_init(&self->macview, self->visual);
	visualconfig_make_vu(self, self->visual);
	machineparamconfig_init(&self->macparam, self->visual, self->disk_op);		
}

void visualconfig_make_vu(VisualConfig* self, psy_Property* parent)
{		
	self->vu = psy_property_set_text(psy_property_append_section(parent,
		"vucolors"), "VU Colors");	
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->vu, "vu2", 0x00403731, 0, 0), PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.mv.theme.vu-background");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->vu, "vu1", 0x0080FF80, 0, 0), PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.mv.theme.vu-bar");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->vu, "vu3", 0x00262bd7, 0, 0), PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.mv.theme.onclip");		
}


void visualconfig_set_directories(VisualConfig* self, psy_Configuration*
	dirconfig)
{
	assert(self);

	self->dirconfig = dirconfig;	
}

void visualconfig_load_skin(VisualConfig* self, const char* path)
{
	psy_Property skin;
	const char* machine_gui_bitmap;

	assert(self);
	
	if (!self->dirconfig) {
		return;
	}
	psy_property_init(&skin);
	skin_load(&skin, path);
	machine_gui_bitmap = psy_property_at_str(&skin, "machine_GUI_bitmap", 0);
	if (psy_strlen(machine_gui_bitmap) == 0) {
		machine_gui_bitmap = "psycle17_knobs.bmp";
	}
	if (machine_gui_bitmap) {
		char psc[_MAX_PATH];

		psy_dir_find_file(
			psy_configuration_value_str(self->dirconfig,
				"skins", PSYCLE_SKINS_DEFAULT_DIR),
			machine_gui_bitmap, psc);
		if (psc[0] != '\0') {
			psy_Path path;

			psy_path_init(&path, psc);
			if (strcmp(psy_path_ext(&path), "bmp") == 0) {
				psy_property_set_str(&skin, "machinedial_bmp",
					psy_path_full(&path));
			}
			else if (skin_load_psc(&skin, psc) == PSY_OK) {
				const char* bpm;

				bpm = psy_property_at_str(&skin, "machinedial_bmp", NULL);
				if (bpm) {
					psy_Path full;

					psy_path_init(&full, psc);
					psy_path_set_name(&full, "");
					psy_path_set_ext(&full, "");
					psy_path_set_name(&full, bpm);
					psy_property_set_str(&skin, "machinedial_bmp",
						psy_path_full(&full));
					psy_path_dispose(&full);
				}
			}
		}
	}	
	visualconfig_set_skin(self, machineparamconfig_base(&self->macparam), &skin);
	visualconfig_set_skin(self, machineviewconfig_base(&self->macview), &skin);
	visualconfig_set_skin(self, patternviewconfig_base(&self->patview), &skin);
	psy_property_dispose(&skin);
}

void visualconfig_set_skin(VisualConfig* self, psy_Configuration* cfg,
	psy_Property* theme)
{
	psy_Property* theme_cfg;
		
	theme_cfg = psy_configuration_at(cfg, "theme");
	if (!theme_cfg) {
		return;
	}			
	psy_property_sync(theme_cfg, theme);	
}

void visualconfig_reset_skin(VisualConfig* self)
{
	assert(self);
	
	psy_configuration_reset(patternviewconfig_base(&self->patview));
	psy_configuration_reset(machineviewconfig_base(&self->macview));
	psy_configuration_reset(machineparamconfig_base(&self->macparam));
	visualconfig_reset_vu(self);	
}

void visualconfig_reset_vu(VisualConfig* self)
{
	assert(self);
	
	psy_property_set_int(self->vu, "vu1", 0x0080FF80); 	
	psy_property_set_int(self->vu, "vu2", 0x00403731); 
	psy_property_set_int(self->vu, "vu3", 0x00262bd7);	
}

void visualconfig_on_load_skin(VisualConfig* self, psy_Property* sender)
{
	if (self->disk_op) {
		psy_FileSelect load;

		assert(self);
				
		psy_fileselect_init(&load);
		psy_fileselect_connect_change(&load, self,
			(psy_fp_fileselect_change)visualconfig_on_load_file_skin_change);
		psy_fileselect_set_title(&load, psy_ui_translate("Load"));
		if (self->dirconfig) {		
			psy_fileselect_set_directory(&load, 
				psy_configuration_value_str(self->dirconfig,
					"skins", PSYCLE_SKINS_DEFAULT_DIR));		
		}
		psy_fileselect_set_default_extension(&load, "PSV");	
		psy_fileselect_add_filter(&load, "Psycle Display Presets", "*.psv");	
		psy_diskop_execute(self->disk_op, &load, NULL, NULL);
		psy_fileselect_dispose(&load);	
	}
}

static void visualconfig_on_load_file_skin_change(VisualConfig* self,
	psy_FileSelect* sender)
{
	assert(self);
	
	visualconfig_load_skin(self, psy_fileselect_value(sender));	
}

void visualconfig_on_reset_skin(VisualConfig* self, psy_Property* sender)
{
	visualconfig_reset_skin(self);
}

bool visualconfig_postload(VisualConfig* self, uintptr_t count)
{
	assert(self);

	psy_configuration_configure(psy_customconfiguration_base(&self->configuration),
		"apptheme");
	return FALSE;
}
