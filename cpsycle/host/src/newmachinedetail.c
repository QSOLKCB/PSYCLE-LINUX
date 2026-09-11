/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "newmachinedetail.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void newmachinedetail_on_destroyed(NewMachineDetail*);
static void newmachinedetail_on_category_edit_accept(NewMachineDetail*,
	psy_ui_Component* sender);
static void newmachinedetail_on_category_edit_reject(NewMachineDetail*,
	psy_ui_Component* sender);

/* vtable */
static psy_ui_ComponentVtable newmachinedetail_vtable;
static bool newmachinedetail_vtable_initialized = FALSE;

static void newmachinedetail_vtable_init(NewMachineDetail* self)
{
	assert(self);

	if (!newmachinedetail_vtable_initialized) {
		newmachinedetail_vtable = *(self->component.vtable);
		newmachinedetail_vtable.on_destroyed =
			(psy_ui_fp_component)
			newmachinedetail_on_destroyed;
		newmachinedetail_vtable_initialized = TRUE;
	}
	self->component.vtable = &newmachinedetail_vtable;
}

/* implementation */
void newmachinedetail_init(NewMachineDetail* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	double numcol0;

	assert(self);
	assert(workspace);

	/* component */
	psy_ui_component_init(&self->component, parent, NULL);
	newmachinedetail_vtable_init(self);
	self->workspace = workspace;
	machineinfo_init(&self->plugin);	
	numcol0 = 12.0;	
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_perc(0.25, 0.0));	
	psy_ui_component_set_padding(&self->component,
		psy_ui_margin_make_em(0.5, 0.5, 0.5, 2.0));
	psy_signal_init(&self->signal_categorychanged);		
	/* details description */
	psy_ui_label_init_text(&self->detaildesc, &self->component,
		"Machine Properties");	
	psy_ui_component_set_align(psy_ui_label_base(&self->detaildesc),
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(psy_ui_label_base(&self->detaildesc),
		psy_ui_margin_make_em(0.0, 0.0, 0.5, 0.0));
	/* details */
	psy_ui_component_init(&self->details, &self->component, NULL);
	psy_ui_component_set_align(&self->details, psy_ui_ALIGN_TOP);	
	psy_ui_component_set_style_type(&self->details, STYLE_BOX_BRIGHT);		
	psy_ui_component_set_default_align(&self->details, psy_ui_ALIGN_TOP,
		psy_ui_defaults_vmargin(psy_ui_defaults())); 
	/* plugin name */
	labelpair_init_right(&self->plugname, &self->details,
		"newmachine.name", numcol0);	
	psy_ui_component_set_align(&self->plugname.second.component,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_align(&self->plugname.component,
		psy_ui_ALIGN_TOP);	
	psy_ui_component_set_margin(&self->plugname.component,
		psy_ui_margin_make_em(0.5, 0.0, 0.5, 0.0));
	/* description */
	labelpair_init_right(&self->desc, &self->details,
		"Description", numcol0);	
	psy_ui_component_set_align(&self->desc.second.component,
		psy_ui_ALIGN_CLIENT);	
	psy_ui_label_enable_wrap(&self->desc.second);
	psy_ui_component_set_align(&self->desc.component,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_preferred_height(&self->desc.component,
		psy_ui_value_make_eh(4.0));	
	/* category */
	psy_ui_component_init(&self->category, &self->details, NULL);	
	psy_ui_label_init_text(&self->categorydesc, &self->category,
		"newmachine.category");
	psy_ui_component_set_margin(&self->categorydesc.component,
		psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));
	psy_ui_label_set_text_alignment(&self->categorydesc,
		psy_ui_ALIGNMENT_RIGHT);
	psy_ui_label_set_char_number(&self->categorydesc, numcol0);
	psy_ui_component_set_align(psy_ui_label_base(&self->categorydesc),
		psy_ui_ALIGN_LEFT);
	psy_ui_edit_init(& self->categoryedit, &self->category);
	psy_ui_component_set_style_type(&self->categoryedit.pane.component,
		psy_INDEX_INVALID);
	psy_ui_edit_enable_input_field(&self->categoryedit);	
	psy_ui_component_set_align(psy_ui_edit_base(
		&self->categoryedit), psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->categoryedit.signal_accept,
		self, newmachinedetail_on_category_edit_accept);
	psy_signal_connect(&self->categoryedit.signal_reject,
		self, newmachinedetail_on_category_edit_reject);
	labelpair_init_right(&self->apiversion, &self->details,
		"newmachine.apiversion", numcol0);	
	labelpair_init_right(&self->version, &self->details,
		"newmachine.version", numcol0);	
	labelpair_init_right(&self->dllname, &self->details,
		"newmachine.module", numcol0);	
	newmachinedetail_set_plugin(self, NULL);
	/* bottom */
	psy_ui_component_init(&self->bottom, &self->component, NULL);
	psy_ui_component_set_align(&self->bottom, psy_ui_ALIGN_BOTTOM);	
	psy_ui_component_set_default_align(&self->bottom,
		psy_ui_ALIGN_TOP, psy_ui_defaults_vmargin(psy_ui_defaults()));
	psy_ui_label_init_text(&self->compatlabel, &self->bottom,
		"newmachine.song-loading-compatibility");	
	psy_ui_checkbox_init_text(&self->compatblitzgamefx, &self->bottom,	
		"newmachine.jme-version-unknown");
	psy_ui_checkbox_enable_wrap(&self->compatblitzgamefx);	
	psy_ui_checkbox_exchange(&self->compatblitzgamefx,
		psy_configuration_at(psycleconfig_compat(
			workspace_cfg(self->workspace)), "loadnewgamefxblitz"));
}

void newmachinedetail_on_destroyed(NewMachineDetail* self)
{
	assert(self);

	machineinfo_dispose(&self->plugin);
	psy_signal_dispose(&self->signal_categorychanged);
}

void newmachinedetail_set_plugin(NewMachineDetail* self,
	const psy_audio_MachineInfo* plugin)
{	
	psy_Path path;
	
	assert(self);

	if (plugin) {
		machineinfo_copy(&self->plugin, plugin);
	} else {
		machineinfo_dispose(&self->plugin);
		machineinfo_init(&self->plugin);
	}
	newmachinedetail_setdescription(self, self->plugin.desc);
	newmachinedetail_setplugname(self, self->plugin.name);
	psy_path_init(&path, self->plugin.modulepath);
	newmachinedetail_setdllname(self, psy_path_filename(&path));
	psy_path_dispose(&path);
	newmachinedetail_setapiversion(self, self->plugin.apiversion);
	newmachinedetail_setplugversion(self, self->plugin.plugversion);
	newmachinedetail_setcategoryname(self, self->plugin.category);						
}

void newmachinedetail_setdescription(NewMachineDetail* self,
	const char* text)
{
	assert(self);

	psy_ui_label_prevent_translation(&self->desc.second);
	psy_ui_label_set_text(&self->desc.second, text);
}

void newmachinedetail_setplugname(NewMachineDetail* self,
	const char* text)
{
	assert(self);

	psy_ui_label_set_text(&self->plugname.second, text);
}

void newmachinedetail_setdllname(NewMachineDetail* self,
	const char* text)
{	
	assert(self);

	psy_ui_label_set_text(&self->dllname.second, text);
}

void newmachinedetail_setcategoryname(NewMachineDetail* self,
	const char* text)
{	
	assert(self);

	psy_ui_edit_set_text(&self->categoryedit, text);	
}

const char* newmachinedetail_category(const NewMachineDetail* self)
{
	assert(self);

	return psy_ui_edit_text(&self->categoryedit);
}

void newmachinedetail_setplugversion(NewMachineDetail* self,
	intptr_t version)
{
	char valstr[64];

	assert(self);

	psy_snprintf(valstr, 64, "%d", (int)version);	
	psy_ui_label_set_text(&self->version.second, valstr);
}

void newmachinedetail_setapiversion(NewMachineDetail* self,
	intptr_t apiversion)
{
	char valstr[64];

	assert(self);

	psy_snprintf(valstr, 64, "%d", (int)apiversion);
	psy_ui_label_set_text(&self->apiversion.second, valstr);
}

void newmachinedetail_on_category_edit_accept(NewMachineDetail* self,
	psy_ui_Component* sender)
{	
	assert(self);

	psy_signal_emit(&self->signal_categorychanged, self, 0);
	psy_ui_component_set_focus(psy_ui_component_parent(
		&self->component));
}

void newmachinedetail_on_category_edit_reject(NewMachineDetail* self,
	psy_ui_Component* sender)
{
	assert(self);

	psy_ui_component_set_focus(psy_ui_component_parent(
		&self->component));
}

const psy_audio_MachineInfo* newmachinedetail_plugin(const
	NewMachineDetail* self)
{
	assert(self);

	return &self->plugin;
}
