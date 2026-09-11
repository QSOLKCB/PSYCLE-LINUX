/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "keyboardmiscconfig.h"
/* host */
#include "resources/resource.h"
/* container */
#include <properties.h>


/* prototypes */
static psy_Property* keyboardmiscconfig_make(KeyboardMiscConfig*, psy_Property* parent);
static void keyboardmiscconfig_make_predefs(KeyboardMiscConfig*);
static void keyboardmiscconfig_make_metronome(KeyboardMiscConfig*);
static void keyboardmiscconfig_make_seqedit(KeyboardMiscConfig*);

/* implementation */
void keyboardmiscconfig_init(KeyboardMiscConfig* self, psy_Property* parent)
{
	assert(self);
	assert(parent);

	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,		
		keyboardmiscconfig_make(self, parent));	
	keyboardmiscconfig_make_predefs(self);
	keyboardmiscconfig_make_metronome(self);
	keyboardmiscconfig_make_seqedit(self);	
}

void keyboardmiscconfig_dispose(KeyboardMiscConfig* self)
{
	assert(self);

	metronomeconfig_dispose(&self->metronome_);
	seqeditconfig_dispose(&self->seqedit_);
	predefsconfig_dispose(&self->predefs_);
	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* keyboardmiscconfig_make(KeyboardMiscConfig* self, psy_Property* parent)
{
	psy_Property* rv;
	psy_Property* choice;

	assert(self);

	rv = psy_property_set_text(
		psy_property_append_section(parent, "inputhandling"),
		"settings.kbd.kbd-misc");	
	psy_property_set_svg(rv, "img.keyboard");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"playstartwithrctrl", TRUE),
		"settings.kbd.ctrl-play");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"ft2home", TRUE),
		"settings.kbd.ft2-home");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"ft2delete", TRUE),
		"settings.kbd.ft2-delete");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"effcursoralwayssdown", FALSE),
		"settings.kbd.cursoralwayssdown");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"recordtweaksastws", 0),
		"settings.kbd.record-tws");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"advancelineonrecordtweak", 0),
		"settings.kbd.advance-line-on-record");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"movecursoronestep", 0),
		"settings.kbd.force-patstep1");
	choice = psy_property_set_text(
		psy_property_append_choice(rv,
			"pgupdowntype", 0),
		"settings.kbd.pgupdowntype");
	psy_property_set_text(
		psy_property_append_int(choice, "beat",
			0, 0, 0),
		"settings.kbd.pgupdowntype-one-beat");
	psy_property_set_text(
		psy_property_append_int(choice, "bar",
			0, 0, 0),
		"settings.kbd.pgupdowntype-one-bar");
	psy_property_set_text(
		psy_property_append_int(choice, "lines",
			0, 0, 0),
		"settings.kbd.pgupdowntype-lines");
	psy_property_set_text(
		psy_property_append_int(rv, "pgupdownstep",
			4, 0, 32),
		"settings.kbd.pgupdowntype-step-lines");	
	psy_property_set_text(
		psy_property_append_int(rv, "cursorstep",
			1, 0, 32),
		"settings.kbd.cursorstep");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"savereminder", TRUE),
		"settings.kbd.savereminder");
	psy_property_set_text(
		psy_property_append_int(rv,
			"numdefaultlines", 64, 1, 1024),
		"settings.kbd.numdefaultlines");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"allowmultiinstances", FALSE),
		"settings.kbd.allowmultiinstances");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"followsong", FALSE),
		"settings.kbd.followsong");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"record-tweak", FALSE),
		"seqview.record-tweak");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"recordnoteoff", FALSE),
		"seqview.record-noteoff");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"multikey", FALSE),
		"seqview.multichannel-audition");
	psy_property_set_text(
		psy_property_append_bool(rv,
			"notestoeffects", FALSE),
		"seqview.allow-notes-to_effect");
	psy_property_set_text(psy_property_append_bool(rv,
		"ft2fileexplorer", TRUE), "settings.kbd.ft2-explorer");
	return rv;
}

void keyboardmiscconfig_make_predefs(KeyboardMiscConfig* self)
{
	assert(self);

	predefsconfig_init(&self->predefs_, psy_customconfiguration_root(
		&self->configuration));
}

void keyboardmiscconfig_make_metronome(KeyboardMiscConfig* self)
{
	assert(self);

	metronomeconfig_init(&self->metronome_, psy_customconfiguration_root(
		&self->configuration));	
}

void keyboardmiscconfig_make_seqedit(KeyboardMiscConfig* self)
{
	assert(self);

	seqeditconfig_init(&self->seqedit_, psy_customconfiguration_root(
		&self->configuration));
}
