/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewconfig.h"
/* container */
#include <configuration.h>
#include <properties.h>


/* prototypes*/
static psy_Property* patternviewconfig_make(PatternViewConfig*, psy_Property* parent);
static void patternviewconfig_on_single_mode(PatternViewConfig*,
	psy_Property* sender);
static void patternviewconfig_reset(PatternViewConfig*);
static bool patternviewconfig_single_mode(const PatternViewConfig*);
	
/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PatternViewConfig* self)
{
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.reset =
			(psy_fp_configuration_reset)
			patternviewconfig_reset;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}		

/* implementation */
void patternviewconfig_init(PatternViewConfig* self, psy_Property* parent)
{		
	assert(self);
	assert(parent);
	
	psy_customconfiguration_init(&self->configuration);
	vtable_init(self);
	self->singlemode = TRUE;	
	psy_customconfiguration_set_root(&self->configuration,
		patternviewconfig_make(self, parent));
	psy_configuration_connect(patternviewconfig_base(self),
		"displaysinglepattern", self, patternviewconfig_on_single_mode);
	patternviewtheme_init(&self->theme, psy_customconfiguration_root(
		&self->configuration));
}

void patternviewconfig_dispose(PatternViewConfig* self)
{
	assert(self);
				
	patternviewtheme_dispose(&self->theme);	
}

psy_Property* patternviewconfig_make(PatternViewConfig* self,
	psy_Property* parent)
{
	psy_Property* rv;	
	psy_Property* choice;

	assert(self);

	rv = psy_property_append_section(parent, "patternview");
	psy_property_set_text(rv,
		"settings.pv.patternview");	
	psy_property_set_text(
		psy_property_append_font(rv, "font", PSYCLE_DEFAULT_PATTERN_FONT),
		"settings.pv.font");
	psy_property_set_hint(psy_property_set_text(psy_property_append_double(
		rv, "zoom", 1.0, 0.1, 4.0),
		"settings.visual.zoom"),
		PSY_PROPERTY_HINT_ZOOM);
	psy_property_set_text(
		psy_property_append_bool(rv, "smoothscroll", FALSE),
		"settings.pv.smoothscroll");
	psy_property_set_text(
		psy_property_append_bool(rv, "drawemptydata", FALSE),
		"settings.pv.draw-empty-data");
	psy_property_set_text(
		psy_property_append_bool(rv, "griddefaults", TRUE),
		"settings.pv.default-entries");
	psy_property_set_text(
		psy_property_append_bool(rv, "linenumbers", TRUE),
		"settings.pv.line-numbers");
	psy_property_set_text(
		psy_property_append_bool(rv, "beatoffset", FALSE),
		"settings.pv.beat-offset");
	psy_property_set_text(
		psy_property_append_bool(rv, "linenumberscursor", TRUE),
		"settings.pv.line-numbers-cursor");
	psy_property_set_text(
		psy_property_append_bool(rv, "linenumbersinhex", FALSE),
		"settings.pv.line-numbers-in-hex");
	psy_property_set_text(
		psy_property_append_bool(rv, "wideinstcolumn", FALSE),
		"settings.pv.wide-instrument-column");
	psy_property_set_text(
		psy_property_append_bool(rv, "trackscopes", TRUE),
		"settings.pv.pattern-track-scopes");
	psy_property_set_text(
		psy_property_append_bool(rv, "wraparound", TRUE),
		"settings.pv.wrap-around");
	psy_property_set_text(
		psy_property_append_bool(rv, "centercursoronscreen", FALSE),
		"settings.pv.center-cursor-on-screen");
	psy_property_set_text(
		psy_property_append_int(rv, "beatsperbar", 4, 1, 16),
		"settings.pv.bar-highlighting");
	psy_property_set_text(
		psy_property_append_bool(rv, "notetab", TRUE),
		"settings.pv.a4-440hz");
	psy_property_set_text(
		psy_property_append_bool(rv, "movecursorwhenpaste", TRUE),
		"settings.pv.move-cursor-when-paste");
	psy_property_set_text(
		psy_property_append_bool(rv, "displaysinglepattern", TRUE),
		"settings.pv.displaysinglepattern");	
	/* pattern display choice */
	choice = psy_property_set_text(
		psy_property_append_choice(rv, "patterndisplay", 0),
		"settings.pv.patterndisplay");
	psy_property_set_text(
		psy_property_append_int(choice, "tracker",
			0, 0, 0),
		"settings.pv.tracker");
	psy_property_set_text(
		psy_property_append_int(choice, "piano",
			0, 0, 0),
		"settings.pv.piano");
	psy_property_set_text(
		psy_property_append_int(choice, "splitvertical",
			0, 0, 0),
		"settings.pv.splitvertical");
	psy_property_set_text(
		psy_property_append_int(choice, "splithorizontal",
			0, 0, 0),
		"settings.pv.splithorizontal");		
	return rv;
}

bool patternviewconfig_single_mode(const PatternViewConfig* self)
{
	return self->singlemode;	
}

void patternviewconfig_on_single_mode(PatternViewConfig* self,
	psy_Property* sender)
{
	self->singlemode = psy_property_item_bool(sender);	
}

void patternviewconfig_reset(PatternViewConfig* self)
{
	assert(self);
	
	psy_configuration_reset(&self->theme.configuration.configuration);
}
