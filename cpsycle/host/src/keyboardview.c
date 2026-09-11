/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "keyboardview.h"


/* implementation */
void keyboardview_init(KeyboardView* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_style_type(keyboardview_base(self), STYLE_SIDE_VIEW);
	psy_ui_component_prevent_app_focus_out(&self->component);
	closebar_init(&self->close_bar, keyboardview_base(self),
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showpianokbd"));	
	keyboardstate_init(&self->state, psy_ui_HORIZONTAL, TRUE);
	pianokeyboard_init(&self->keyboard, keyboardview_base(self), &self->state,
		workspace_player(workspace));		
	psy_ui_component_set_align(&self->keyboard.component, psy_ui_ALIGN_CENTER);
	psy_ui_component_set_preferred_width(&self->keyboard.component,
		psy_ui_value_make_ew((double)psy_audio_NOTECOMMANDS_RELEASE));	
}
