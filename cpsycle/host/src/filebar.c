/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "filebar.h"
/* host */
#include "resources/resource.h"


/* prototypes */
static void filebar_init_buttons(FileBar*, psy_ui_Component* parent);
static void filebar_connect_configure(FileBar*);
static void filebar_on_new_song(FileBar*, psy_ui_Component* sender);
static void filebar_on_disk_op(FileBar*, psy_ui_Component* sender);
static void filebar_on_load_song(FileBar*, psy_ui_Component* sender);
static void filebar_on_save_song_as(FileBar*, psy_ui_Component* sender);
static void filebar_on_ft2_explorer(FileBar*, psy_Property* sender);
static void filebar_on_render(FileBar*, psy_ui_Component* sender);

/* implementation */
void filebar_init(FileBar* self, psy_ui_Component* parent, Workspace* workspace)
{	
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(filebar_base(self), parent, NULL);
	self->workspace_ = workspace;
	psy_ui_component_init_align(&self->client, filebar_base(self), NULL,
		psy_ui_ALIGN_TOP);		
	psy_ui_component_set_default_align(&self->client, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_ui_label_init_text(&self->desc_, &self->client, "file.song");
	self->desc_.component.flags_ |= psy_ui_COMPONENTFLAGS_PREVENT_MOUSE_INPUT;
	filebar_init_buttons(self, &self->client);
	filebar_connect_configure(self);	
}

void filebar_init_buttons(FileBar* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_button_init_text_connect(&self->song_new_, parent,
		"file.new", self, filebar_on_new_song);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->song_new_), 0);
	psy_ui_button_set_svg(&self->song_new_, psy_ui_app_svg(psy_ui_app(),
		"img.new"));		
	psy_ui_button_init_text_connect(&self->song_disk_op_, parent,
		"file.disk_op", self, filebar_on_disk_op);	
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->song_disk_op_), 0);
	psy_ui_button_init_text_connect(&self->song_load_, parent,
		"file.load", self, filebar_on_load_song);	
	psy_ui_button_set_svg(&self->song_load_,
		psy_ui_app_svg(psy_ui_app(), "img.open"));
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->song_load_), 0);
	psy_ui_button_init_text_connect(&self->song_save_, parent,
		"file.save", self, filebar_on_save_song_as);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->song_save_), 0);
	psy_ui_button_set_svg(&self->song_save_,
		psy_ui_app_svg(psy_ui_app(), "img.save"));		
	psy_ui_button_init_text_connect(&self->song_render_, parent,
		"file.render", self, filebar_on_render);	
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->song_render_), 0);
	psy_ui_button_set_svg(&self->song_render_,
		psy_ui_app_svg(psy_ui_app(), "img.pulse"));		
}

void filebar_connect_configure(FileBar* self)
{
	assert(self);
	
	psy_configuration_connect(psycleconfig_misc(workspace_cfg(
		self->workspace_)), "ft2fileexplorer", self, filebar_on_ft2_explorer);
	psy_configuration_configure(psycleconfig_misc(workspace_cfg(
		self->workspace_)), "ft2fileexplorer");
}

void filebar_on_new_song(FileBar* self, psy_ui_Component* sender)
{
	assert(self);
	
	workspace_new_song(self->workspace_);
}

void filebar_on_disk_op(FileBar* self, psy_ui_Component* sender)
{
	assert(self);
	
	workspace_disk_op_song(self->workspace_);
}

void filebar_on_load_song(FileBar* self, psy_ui_Component* sender)
{	
	assert(self);
			
	workspace_load_song(self->workspace_);
}

void filebar_on_save_song_as(FileBar* self, psy_ui_Component* sender)
{	
	assert(self);
				
	workspace_save_song_as(self->workspace_);
}

void filebar_on_ft2_explorer(FileBar* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {				
		psy_ui_component_show(psy_ui_button_base(&self->song_disk_op_));
		psy_ui_component_hide(psy_ui_button_base(&self->song_load_));
		psy_ui_component_hide(psy_ui_button_base(&self->song_save_));
	} else {
		psy_ui_component_hide(psy_ui_button_base(&self->song_disk_op_));
		psy_ui_component_show(psy_ui_button_base(&self->song_load_));
		psy_ui_component_show(psy_ui_button_base(&self->song_save_));
	}
	if (psy_ui_component_draw_visible(psy_ui_app_main(psy_ui_app()))) {
		psy_ui_component_align_invalidate(psy_ui_app_main(psy_ui_app()));		
	}
}

void filebar_on_render(FileBar* self, psy_ui_Component* sender)
{
	assert(self);
	
	workspace_select_view(self->workspace_,
		viewindex_make(VIEW_ID_RENDERVIEW));
}
