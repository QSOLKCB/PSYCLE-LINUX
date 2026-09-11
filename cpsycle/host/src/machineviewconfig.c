/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineviewconfig.h"
/* host */
#include "skinio.h"
/* container */
#include <properties.h>
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static psy_Property* machineviewconfig_make(MachineViewConfig*, psy_Property* parent);
static void machineviewconfig_make_stack_view(MachineViewConfig*,
	psy_Property* parent);
static void machineviewconfig_make_theme(MachineViewConfig*,
	psy_Property* parent);
static void machineviewconfig_reset(MachineViewConfig*);
	
/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MachineViewConfig* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.reset =
			(psy_fp_configuration_reset)
			machineviewconfig_reset;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}	

/* implementation */
void machineviewconfig_init(MachineViewConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);
	vtable_init(self);	
	psy_customconfiguration_set_root(&self->configuration,
		machineviewconfig_make(self, parent));
}

void machineviewconfig_dispose(MachineViewConfig* self)
{
	assert(self);	

	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* machineviewconfig_make(MachineViewConfig* self, psy_Property* parent)
{	
	psy_Property* rv;

	assert(self);

	rv = psy_property_set_text(psy_property_append_section(
		parent, "machineview"), "settings.mv.machineview");
	psy_property_set_hint(psy_property_set_text(psy_property_append_double(
		rv, "zoom", 1.0, 0.1, 4.0), "settings.visual.zoom"),
		PSY_PROPERTY_HINT_ZOOM);
	psy_property_set_text(psy_property_append_bool(rv,
		"drawmachineindexes", TRUE), "settings.mv.draw-machine-indexes");
	psy_property_set_text(psy_property_append_bool(rv,
		"drawvumeters", TRUE), "settings.mv.draw-vumeters");
	psy_property_set_text(psy_property_append_bool(rv,
		"drawvirtualgenerators", FALSE), "settings.mv.draw-virtualgenerators");	
	machineviewconfig_make_stack_view(self, rv);
	machineviewconfig_make_theme(self, rv);
	machineviewconfig_reset(self);
	return rv;
}

void machineviewconfig_make_stack_view(MachineViewConfig* self,
	psy_Property* parent)
{
	psy_Property* stackview;

	assert(self);
	
	stackview = psy_property_set_text(psy_property_append_section(parent,
		"stackview"), "settings.mv.stackview");
	psy_property_set_text(psy_property_append_bool(stackview,
		"drawsmalleffects", FALSE), "settings.mv.stackview-draw-smalleffects");
}

void machineviewconfig_make_theme(MachineViewConfig* self, psy_Property* parent)
{	
	intptr_t style_value = 0;
	const char* style_str = "";	

	assert(self);
		
	self->theme = psy_property_enable_foldable(psy_property_set_text(
		psy_property_append_section(parent, "theme"),
		"settings.mv.theme.theme"), TRUE);	
	/* generator */
	psy_property_set_text(psy_property_append_font(
		self->theme, "generator_font", PSYCLE_DEFAULT_MACHINE_FONT),
		"settings.mv.theme.generators-font-face");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_generator_fontcolour", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.generators-font");		
	/* effect */
	psy_property_set_text(psy_property_append_font(
		self->theme, "effect_font", PSYCLE_DEFAULT_MACHINE_FONT),
		"settings.mv.theme.effect-fontface");	
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_effect_fontcolour", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.effects-font");		
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_colour", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.background");
	/* wire */
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_wirecolour", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.wirecolour");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_wirecolour2", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.wirecolour2");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_polycolour", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.polygons");		
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_wirewidth", 0x00000001, 0, 0),
		PSY_PROPERTY_HINT_EDITCOLOR), "settings.mv.theme.wire-width");
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_wireaa", 0x01, 0, 0), PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.mv.theme.antialias-halo");
	psy_property_set_text(psy_property_append_str(self->theme,
		"machine_background", style_str),
		"settings.mv.theme.machine-background");
	self->machine_skin = psy_property_set_text(psy_property_append_str(
		self->theme, "machine_skin", "default-text"), "Machine Skin");		
	psy_property_set_text(psy_property_set_hint(psy_property_append_int(
		self->theme, "mv_triangle_size", style_value, 0, 0),
		PSY_PROPERTY_HINT_EDIT), "settings.mv.theme.polygon-size");	
}

void machineviewconfig_reset(MachineViewConfig* self)
{
	assert(self);

	assert(self);
	
	psy_property_set_str(self->theme, "machine_background", "");
	psy_property_set_str(self->theme, "machine_skin", "default-text");
	psy_property_set_int(self->theme, "mv_colour", 0x00232323);
	psy_property_set_int(self->theme, "mv_wirecolour", 0x005F5F5F);	
	psy_property_set_int(self->theme, "mv_polycolour", 0x005F5F5F);
	psy_property_set_int(self->theme, "mv_triangle_size", 5);
	psy_property_set_int(self->theme, "mv_generator_fontcolour", 0x00B1C8B0);
	psy_property_set_int(self->theme, "mv_effect_fontcolour", 0x00D1C5B6);	
}
