/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "songbar.h"


/* implementation */
void songbar_init(SongBar* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);

	psy_ui_component_init(songbar_base(self), parent, NULL);	
	psy_ui_component_set_default_align(songbar_base(self), psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	songtrackbar_init(&self->song_track_bar_, songbar_base(self), workspace);
	timebar_init(&self->time_bar_, songbar_base(self),
		workspace_player(workspace));
	linesperbeatbar_init(&self->lines_per_beat_bar_, songbar_base(self),
		workspace_player(workspace));
	octavebar_init(&self->octave_bar_, songbar_base(self),
		workspace_player(workspace));
}
