/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "patternviewstate.h"
/* host */
#include "styles.h"
/* audio */
#include <pattern.h>
/* std */
#include <math.h>
#include <assert.h>
/* platform */
#include "../../detail/trace.h"
#include "../../detail/portable.h"


/* PatternViewState */

/* prototypes */
static void patternviewstate_on_single_mode(PatternViewState*,
	psy_Property* sender);

/* implementation */
void patternviewstate_init(PatternViewState* self,
	psy_Configuration* pat_cfg, psy_Configuration* misc_cfg,
	psy_audio_Player* player, psy_audio_Sequence* sequence,
	psy_UndoRedo* undo_redo)
{
	assert(self);
	
	psy_audio_sequencecursor_init(&self->cursor);
	patternselection_init(&self->selection);
	beatline_init(&self->beat_line, self->cursor.lpb);
	self->player = player;
	self->sequence = sequence;	
	self->patconfig = pat_cfg;
	self->misc_cfg_ = misc_cfg;
	self->chord = FALSE;
	self->undo_redo = undo_redo;
	self->chord_begin = 0;	
	self->insert_duration = psy_dsp_beatpos_zero();
	if (self->patconfig) {
		self->singlemode = psy_configuration_value_bool(self->patconfig,
			"displaysinglepattern", TRUE);
		psy_configuration_connect(self->patconfig, "displaysinglepattern",
			self, patternviewstate_on_single_mode);
	}  else {
		self->singlemode = FALSE;
	}
	psy_audio_pattern_init(&self->patternpaste);	
}

void patternviewstate_dispose(PatternViewState* self)
{	
	assert(self);
	
	patternselection_dispose(&self->selection);
	psy_audio_pattern_dispose(&self->patternpaste);
}

void patternviewstate_sync_cursor_to_sequence(PatternViewState* self)
{
	if (patternviewstate_sequence(self)) {
		patternviewstate_set_cursor(self,
			patternviewstate_sequence(self)->cursor);
	}
}

bool patternviewstate_hasmovecursorwhenpaste(const PatternViewState* self)
{	
	if (self->patconfig) {
		psy_Property* p;
		
		p = psy_configuration_at(self->patconfig, "movecursorwhenpaste");
		if (p) {
			return psy_property_item_bool(p);
		}
	}
	return TRUE;
}

void patternviewstate_on_single_mode(PatternViewState* self,
	psy_Property* sender)
{
	assert(self);

	self->singlemode = psy_property_item_bool(sender);
}

double patternviewstate_linenumber_num_digits(const PatternViewState* self)
{
	double rv;

	assert(self);

	rv = 0.0;
	if (psy_configuration_value_bool(self->patconfig, "linenumbers", FALSE)) {
		rv += 5.0;
		if (psy_configuration_value_bool(self->patconfig,
			"linenumbersinhex", FALSE)) {

		}
		if (psy_configuration_value_bool(self->patconfig, "beatoffset",
				FALSE)) {
			rv += 5.0;
		}
		if (!self->singlemode) {
			rv += 1.0;
		}
	}
	return rv;
}
