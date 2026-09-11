/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "stepbox.h"
/* platform */
#include "../../detail/portable.h"

/* prototypes */
static void patterncursorstepbox_build(PatternCursorStepBox*);
static void patterncursorstepbox_onselectionchanged(PatternCursorStepBox*,
	psy_ui_ComboBox* sender);

/* implementation */
void patterncursorstepbox_init(PatternCursorStepBox* self, psy_ui_Component*
	parent, Workspace* workspace)
{	
	psy_ui_component_init(patterncursorstepbox_base(self), parent, NULL);
	self->workspace = workspace;
	psy_ui_component_set_default_align(patterncursorstepbox_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_component_set_align_expand(patterncursorstepbox_base(self),
		psy_ui_HEXPAND);
	psy_ui_label_init(&self->header, patterncursorstepbox_base(self));
	psy_ui_label_set_text(&self->header, "pv.step");
	psy_ui_combobox_init(&self->combobox,
		patterncursorstepbox_base(self));
	psy_signal_connect(&self->combobox.signal_selchanged, self,
		patterncursorstepbox_onselectionchanged);
	psy_ui_combobox_set_char_number(&self->combobox, 3);
	patterncursorstepbox_build(self);
	patterncursorstepbox_update(self);	
}

void patterncursorstepbox_build(PatternCursorStepBox* self)
{
	int step;

	for (step = 0; step <= 16; ++step) {
		char text[20];

		psy_snprintf(text, 20, "%d", step);
		psy_ui_combobox_add_text(&self->combobox, text);
	}	
}

void patterncursorstepbox_onselectionchanged(PatternCursorStepBox* self,
	psy_ui_ComboBox* sender)
{			
	if (psy_ui_combobox_cur_sel(sender) != psy_INDEX_INVALID) {
		psy_Property* p;

		p = psy_configuration_at(psycleconfig_misc(workspace_cfg(self->workspace)),
			"cursorstep");
		if (p) {
			psy_property_set_item_int(p, psy_ui_combobox_cur_sel(sender));
		}		
	}
}

void patterncursorstepbox_update(PatternCursorStepBox* self)
{
	psy_ui_combobox_select(&self->combobox,
		psy_configuration_value_int(psycleconfig_misc(workspace_cfg(self->workspace)),
			"cursorstep", 1));
}
