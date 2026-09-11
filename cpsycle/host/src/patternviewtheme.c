/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewtheme.h"
/* host */
#include "patternviewconfig.h"
#include "skinio.h"
/* ui*/
#include <uicolour.h>
#include <uifont.h>
/* file */
#include <dir.h>
/* platform */
#include "../../detail/portable.h"

#define PSYCLE__PATH__DEFAULT_PATTERN_HEADER_SKIN "Psycle Default (internal)"


/* PatternViewTheme */

/* prototypes */
static psy_Property* patternviewtheme_make(PatternViewTheme*, psy_Property* parent);
static psy_Property* patternviewtheme_at(PatternViewTheme*, const char* key);
static void patternviewtheme_reset(PatternViewTheme*);

/* vtable */
static psy_ConfigurationVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(PatternViewTheme* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *self->configuration.configuration.vtable;		
		vtable.reset =
			(psy_fp_configuration_reset)
			patternviewtheme_reset;
		vtable_initialized = TRUE;
	}
	self->configuration.configuration.vtable = &vtable;
}	


/* implementation */
void patternviewtheme_init(PatternViewTheme* self, psy_Property* parent)
{
	assert(self);
	
	psy_customconfiguration_init(&self->configuration);
	psy_customconfiguration_set_root(&self->configuration,
		patternviewtheme_make(self, parent));
}

void patternviewtheme_dispose(PatternViewTheme* self)
{
	assert(self);

	psy_customconfiguration_dispose(&self->configuration);
}

psy_Property* patternviewtheme_make(PatternViewTheme* self, psy_Property* parent)
{
	psy_Property* rv;

	assert(self);

	rv = self->theme = psy_property_enable_foldable(psy_property_set_text(
		psy_property_append_section(parent, "theme"),
		"settings.pv.theme.theme"), TRUE);
	psy_property_set_text(
		psy_property_append_str(self->theme,
			"pattern_fontface", "Tahoma"),
		"settings.pv.theme.fontface");
	psy_property_set_text(
		psy_property_append_int(self->theme,
			"pattern_font_point", 0x00000050, 0, 0),
		"settings.pv.theme.fontpoint");
	psy_property_set_text(
		psy_property_append_int(self->theme,
			"pattern_font_flags", 0x00000001, 0, 0),
		"settings.pv.theme.fontflags");
	psy_property_set_text(
		psy_property_append_int(self->theme,
			"pattern_font_x", 0x00000009, 0, 0),
		"settings.pv.theme.font_x");
	psy_property_set_text(
		psy_property_append_int(self->theme,
			"pattern_font_y", 0x0000000B, 0, 0),
		"settings.pv.theme.font_y");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_separator", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.separator");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_separator2", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.separator2");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_background", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.background");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_background2", 0x00292929, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.background2");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_font", 0x00CACACA, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.font");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_font2", 0x00CACACA, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.font2");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontCur", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontcur");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontCur2", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontcur2");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontSel", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontsel");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontSel2", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontsel2");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontPlay", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontplay");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_fontPlay2", 0x00FFFFFF, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.fontplay2");
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_row", 0x003E3E3E, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.row");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_row2", 0x003E3E3E, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.row2");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_rowbeat", 0x00363636, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.rowbeat");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_rowbeat2", 0x00363636, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.rowbeat2");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_row4beat", 0x00595959, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.row4beat");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_row4beat2", 0x00595959, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.row4beat2");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_selection", 0x009B7800, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.selection");	
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_selection2", 0x009B7800, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.selection2");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_playbar", 0x009F7B00, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.playbar");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_playbar2", 0x009F7B00, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.playbar2");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_cursor", 0x009F7B00, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.cursor");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_cursor2", 0x009F7B00, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.cursor2");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_midline", 0x007D6100, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.midline");		
	psy_property_set_text(
		psy_property_set_hint(psy_property_append_int(self->theme,
			"pvc_midline2", 0x007D6100, 0, 0),
			PSY_PROPERTY_HINT_EDITCOLOR),
		"settings.pv.theme.midline2");		
	self->header_skin = psy_property_append_str(self->theme,
		"pattern_header_skin", "default-em");
	psy_property_append_bool(self->theme, "pattern_header_classic", TRUE);	
	return rv;
}

void patternviewtheme_reset(PatternViewTheme* self)
{	
	assert(self);
	
	/* reset pattern */	
	psy_property_set_str(self->theme, "pattern_fontface", "Tahoma");
	psy_property_set_int(self->theme, "pattern_font_point", 0x00000050);
	psy_property_set_int(self->theme, "pattern_font_flags", 0x00000001);
	psy_property_set_int(self->theme, "pattern_font_x", 0x00000009);
	psy_property_set_int(self->theme, "pattern_font_y", 0x0000000B);
	psy_property_set_int(self->theme, "pvc_separator", 0x00292929);
	psy_property_set_int(self->theme, "pvc_separator2", 0x00292929);
	psy_property_set_int(self->theme, "pvc_background", 0x00292929);					
	psy_property_set_int(self->theme, "pvc_background2", 0x00292929);
	psy_property_set_int(self->theme, "pvc_font", 0x00CACACA);
	psy_property_set_int(self->theme, "pvc_font2", 0x00CACACA);
	psy_property_set_int(self->theme, "pvc_fontCur", 0x00FFFFFF);			
	psy_property_set_int(self->theme, "pvc_fontCur2", 0x00FFFFFF);
	psy_property_set_int(self->theme, "pvc_fontSel", 0x00FFFFFF);	
	psy_property_set_int(self->theme, "pvc_fontSel2", 0x00FFFFFF);
	psy_property_set_int(self->theme, "pvc_fontPlay", 0x00FFFFFF);			
	psy_property_set_int(self->theme, "pvc_fontPlay2", 0x00FFFFFF);
	psy_property_set_int(self->theme, "pvc_row", 0x003E3E3E);			
	psy_property_set_int(self->theme, "pvc_row2", 0x003E3E3E);			
	psy_property_set_int(self->theme, "pvc_rowbeat", 0x00363636);
	psy_property_set_int(self->theme, "pvc_rowbeat2", 0x00363636);
	psy_property_set_int(self->theme, "pvc_row4beat", 0x00595959);
	psy_property_set_int(self->theme, "pvc_row4beat2", 0x00595959);
	psy_property_set_int(self->theme, "pvc_selection", 0x009B7800);
	psy_property_set_int(self->theme, "pvc_selection2", 0x009B7800);
	psy_property_set_int(self->theme, "pvc_playbar", 0x009F7B00);
	psy_property_set_int(self->theme, "pvc_playbar2", 0x009F7B00);
	psy_property_set_int(self->theme, "pvc_cursor", 0x009F7B00);
	psy_property_set_int(self->theme, "pvc_cursor2", 0x009F7B00);
	psy_property_set_int(self->theme, "pvc_midline", 0x007D6100);
	psy_property_set_int(self->theme, "pvc_midline2", 0x007D6100);
	/* reset header */
	psy_property_notify(psy_property_set_str(self->theme,
		"pattern_header_skin", ""));	
}
