/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "gear.h"

/* host */
#include "cmdsgeneral.h"
#include "paramviews.h"
#include "styles.h"

/* GearButtons */

/* implementation */
void gearbuttons_init(GearButtons* self, psy_ui_Component* parent,
	ParamViews* paramviews)
{
	assert(self);

	psy_ui_component_init(gearbuttons_base(self), parent, NULL);	
	psy_ui_component_set_default_align(gearbuttons_base(self),
		psy_ui_ALIGN_TOP, psy_ui_defaults_vmargin(psy_ui_defaults()));	
	psy_ui_button_init_text(&self->createreplace, gearbuttons_base(self),
		"gear.create-replace");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->createreplace),
		0);
	psy_ui_button_init_text(&self->del, gearbuttons_base(self),
		"gear.delete");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->del), 0);
	psy_ui_button_init_text(&self->parameters, gearbuttons_base(self),
		"gear.parameters");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->parameters), 0);
	psy_ui_button_init_text(&self->properties, gearbuttons_base(self),
		"gear.properties");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->properties), 0);
	psy_ui_button_init_text(&self->exchange, gearbuttons_base(self),
		"gear.exchange");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->exchange), 0);
	psy_ui_button_init_text(&self->clone, gearbuttons_base(self),
		"gear.clone");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->clone), 0);
	psy_ui_button_init_text(&self->showmaster, gearbuttons_base(self),
		"gear.show-master");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->showmaster), 0);
}

/* Gear */

/* prototypes */
static void gear_init_header(Gear*);
static void gear_init_title(Gear*);
static void gear_init_view_buttons(Gear*);
static void gear_init_boxes(Gear*, ParamViews*);
static void gear_init_tab_bar(Gear*);
static void gear_on_create(Gear*, psy_ui_Component* sender);
static void gear_on_delete(Gear*, psy_ui_Component* sender);
static void gear_on_song_changed(Gear*, psy_audio_Player* sender);
static void gear_connect_song(Gear*);
static void gear_on_clone(Gear*, psy_ui_Component* sender);
static void gear_on_exchange(Gear* self, psy_ui_Component* sender);
static void gear_on_parameters(Gear*, psy_ui_Component* sender);
static void gear_on_properties(Gear*, psy_ui_Component* sender);
static void gear_on_master(Gear*, psy_ui_Component* sender);
static void gear_on_machine_selected(Gear*, psy_audio_Machines* sender,
	uintptr_t mac_id);
static void gear_show_generators(Gear*);
static void gear_show_effects(Gear*);
static void gear_on_tabbar_changed(Gear*, psy_ui_TabBar* sender,
	uintptr_t tabindex);
static void gear_select_section(Gear*, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options);
static void gear_on_float(Gear*, psy_ui_Button* sender);
static void gear_on_focus(Gear*);

/* vtable */
static psy_ui_ComponentVtable gear_vtable;
static bool gear_vtable_initialized = FALSE;

static void gear_vtable_init(Gear* self)
{
	assert(self);

	if (!gear_vtable_initialized) {
		gear_vtable = *(self->component.vtable);
		gear_vtable.on_focus =
			(psy_ui_fp_component)
			gear_on_focus;
		gear_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(gear_base(self), &gear_vtable);
}

/* implementation */
void gear_init(Gear* self, psy_ui_Component* parent, ParamViews* param_views,
	Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(gear_base(self), parent, NULL);
	gear_vtable_init(self);
	psy_ui_component_set_tab_index(gear_base(self), 0);
	self->workspace = workspace;
	self->param_views = param_views;
	self->splitter_ = NULL;
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		gear_on_song_changed);	
	psy_ui_component_init_align(&self->client, gear_base(self), NULL,
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_style_type(&self->client, STYLE_SIDE_VIEW);		
	gear_init_header(self);
	gear_init_boxes(self, param_views);	
	gear_init_tab_bar(self);
	gearbuttons_init(&self->buttons, &self->client, param_views);
	psy_ui_component_set_align(gearbuttons_base(&self->buttons),
		psy_ui_ALIGN_RIGHT);
	psy_signal_connect(&self->buttons.createreplace.signal_clicked, self,
		gear_on_create);
	psy_signal_connect(&self->buttons.del.signal_clicked, self, gear_on_delete);
	psy_signal_connect(&self->buttons.clone.signal_clicked, self,
		gear_on_clone);
	psy_signal_connect(&self->buttons.parameters.signal_clicked, self,
		gear_on_parameters);
	psy_signal_connect(&self->buttons.properties.signal_clicked, self,
		gear_on_properties);
	psy_signal_connect(&self->buttons.showmaster.signal_clicked, self,
		gear_on_master);	
	psy_signal_connect(&self->buttons.exchange.signal_clicked, self,
		gear_on_exchange);	
	psy_signal_connect(&self->tabbar.signal_change, self,
		gear_on_tabbar_changed);	
	psy_signal_connect(&self->component.signal_select_section, self,
		gear_select_section);
	gear_connect_song(self);
	psy_ui_component_prevent_app_focus_out_recursive(&self->component);
}

void gear_init_header(Gear* self)
{	
	assert(self);

	gear_init_title(self);
	psy_ui_label_init_text(&self->label, &self->component, "Machines:Generator");
	psy_ui_component_set_align(psy_ui_label_base(&self->label), psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(psy_ui_label_base(&self->label),
		psy_ui_margin_make_em(0.0, 0.0, 0.25, 0.0));
}

void gear_init_title(Gear* self)
{	
	assert(self);
	
	psy_ui_component_set_title(gear_base(self), "machinebar.gear");
	titlebar_init(&self->title_bar_, gear_base(self), "machinebar.gear");
	closebar_set_property(&self->title_bar_.close_bar_,
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showgear"));
	gear_init_view_buttons(self);
}

void gear_init_boxes(Gear* self, ParamViews* param_views)
{
	psy_ui_notebook_init(&self->notebook, &self->client);
	psy_ui_component_set_margin(psy_ui_notebook_base(&self->notebook),
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
	machinesbox_init(&self->machinesboxgen, psy_ui_notebook_base(
		&self->notebook),
		(workspace_song(self->workspace))
		? psy_audio_song_machines(workspace_song(self->workspace))
		: NULL,
		MACHINEBOX_GENERATOR, param_views);
	machinesbox_init(&self->machinesboxfx,
		psy_ui_notebook_base(&self->notebook),
		(workspace_song(self->workspace))
		? psy_audio_song_machines(workspace_song(self->workspace))
		: NULL, MACHINEBOX_FX, param_views);
	instrumentsbox_init(&self->instrumentsbox,
		psy_ui_notebook_base(&self->notebook),
		(workspace_song(self->workspace))
		? psy_audio_song_instruments(workspace_song(self->workspace))
		: NULL);
	psy_ui_component_set_margin(&self->instrumentsbox.groupheader,
		psy_ui_margin_zero());
	samplesbox_init(&self->samplesbox, psy_ui_notebook_base(&self->notebook),
		workspace_song(self->workspace)
		? psy_audio_song_samples(workspace_song(self->workspace))
		: NULL);
}

void gear_init_tab_bar(Gear* self)
{
	assert(self);

	psy_ui_component_init_align(&self->bottom, &self->client, NULL,
		psy_ui_ALIGN_BOTTOM);
	psy_ui_tabbar_init(&self->tabbar, &self->bottom);
	psy_ui_tabbar_append_tabs(&self->tabbar, "gear.generators", "gear.effects",
		"gear.instruments", "gear.waves", NULL);
	psy_ui_notebook_connect_controller(&self->notebook,
		&self->tabbar.signal_change);
	psy_ui_tabbar_select(&self->tabbar, 0);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tabbar),
		psy_ui_ALIGN_CLIENT);
	psy_ui_sizer_init(&self->sizer_, &self->bottom);
	psy_ui_component_set_align(psy_ui_sizer_base(&self->sizer_),
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_hide(psy_ui_sizer_base(&self->sizer_));
	psy_ui_tabbar_select(&self->tabbar, 0);

}

void gear_init_view_buttons(Gear* self)
{
	assert(self);

	psy_ui_component_init_align(&self->view_buttons, &self->title_bar_.component, NULL,
		psy_ui_ALIGN_RIGHT);
	psy_ui_component_init_align(&self->view_buttons_client, &self->view_buttons, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->view_buttons_client, psy_ui_ALIGN_LEFT,
		psy_ui_margin_zero());	
	psy_ui_button_init_resource_connect(&self->view_float, &self->view_buttons_client,
		"img.float", self, gear_on_float);	
}

void gear_on_create(Gear* self, psy_ui_Component* sender)
{	
	psy_audio_Song* song;
	
	assert(self);
	
	if (!workspace_song(self->workspace)) {
		return;
	}
	song = workspace_song(self->workspace);
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
	case 0:
	case 1: {
		psy_audio_Machines* machines;

		machines = &song->machines_;
		if (psy_audio_machines_selected(machines) != psy_INDEX_INVALID) {
			self->workspace->insert.replace_mac =
			psy_audio_machines_selected(machines);
		} else {
			self->workspace->insert.replace_mac = psy_INDEX_INVALID;
			self->workspace->insert.random_position = TRUE;
		}
		workspace_select_view(self->workspace, viewindex_make_section(
			VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_NEWMACHINE));
		break; }
	case 3: {
		psy_audio_Instruments* instruments;
		psy_audio_InstrumentIndex inst;
		psy_audio_SampleIndex smpl;
		
		instruments = psy_audio_song_instruments(song);
		inst = psy_audio_instruments_selected(instruments);
		smpl = samplesbox_selected(&self->samplesbox);
		psy_audio_instruments_select(psy_audio_song_instruments(song),
			psy_audio_instrumentindex_make(inst.group, smpl.slot));
		workspace_load_sample(self->workspace, smpl);
		break; }
	default:
		break;		
	}
}

void gear_on_delete(Gear* self, psy_ui_Component* sender)
{
	assert(self);
	
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
	case 0: machinesbox_remove(&self->machinesboxgen);
		break;
	case 1: machinesbox_remove(&self->machinesboxfx);
		break;
	default:
		break;
	}
}

void gear_on_song_changed(Gear* self, psy_audio_Player* sender)
{	
	psy_audio_Song* song;
	
	assert(self);
	
	song = psy_audio_player_song(sender);
	if (song) {		
		machinesbox_set_machines(&self->machinesboxgen,
			psy_audio_song_machines(song));
		machinesbox_set_machines(&self->machinesboxfx,
			psy_audio_song_machines(song));
		instrumentsbox_set_instruments(&self->instrumentsbox,
			psy_audio_song_instruments(song));
		samplesbox_set_samples(&self->samplesbox,
			psy_audio_song_samples(song));
		gear_connect_song(self);
	} else {		
		machinesbox_set_machines(&self->machinesboxgen, NULL);
		machinesbox_set_machines(&self->machinesboxfx, NULL);
		instrumentsbox_set_instruments(&self->instrumentsbox, NULL);
		samplesbox_set_samples(&self->samplesbox, NULL);
	}
	psy_ui_component_invalidate(gear_base(self));
}

void gear_connect_song(Gear* self)
{
	psy_audio_Song* song;
	
	assert(self);
	
	song = workspace_song(self->workspace);
	if (song) {	
		psy_signal_connect(&psy_audio_song_machines(song)->signal_slotchange,
			self, gear_on_machine_selected);
	}
}

void gear_on_machine_selected(Gear* self, psy_audio_Machines* sender,
	uintptr_t mac_id)
{
	psy_audio_Machine* machine;
	
	assert(self);

	machine = psy_audio_machines_at(sender, mac_id);
	if (machine) {
		if (psy_audio_machine_mode(machine) == psy_audio_MACHMODE_GENERATOR) {
			gear_show_generators(self);
		} else if (psy_audio_machine_mode(machine) == psy_audio_MACHMODE_FX) {
			gear_show_effects(self);
		}
	} else if (mac_id < 0x40) {
		gear_show_generators(self);
	} else if (mac_id < 0x80) {
		gear_show_effects(self);
	}	
}

void gear_show_generators(Gear* self)
{
	assert(self);
	
	if (psy_ui_tabbar_selected(&self->tabbar) != 0) {
		psy_ui_tabbar_select(&self->tabbar, 0);
	}
}

void gear_show_effects(Gear* self)
{
	assert(self);
	
	if (psy_ui_tabbar_selected(&self->tabbar) != 1) {
		psy_ui_tabbar_select(&self->tabbar, 1);
	}
}

void gear_on_clone(Gear* self, psy_ui_Component* sender)
{
	assert(self);
	
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
	case 0: machinesbox_clone(&self->machinesboxgen);
		break;
	case 1: machinesbox_clone(&self->machinesboxfx);
		break;
	default:
		break;
	}
}

void gear_on_exchange(Gear* self, psy_ui_Component* sender)
{
	assert(self);
	
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
	case 0: machinesbox_exchange(&self->machinesboxgen);
		break;
	case 1: machinesbox_exchange(&self->machinesboxfx);
		break;
	default:
		break;
	}
}

void gear_on_parameters(Gear* self, psy_ui_Component* sender)
{
	assert(self);
	
	switch (psy_ui_tabbar_selected(&self->tabbar)) {
	case 0:
		machinesbox_show_parameters(&self->machinesboxgen);
		break;
	case 1:
		machinesbox_show_parameters(&self->machinesboxfx);
		break;
	default:
		break;
	}
}

void gear_on_properties(Gear* self, psy_ui_Component* sender)
{
	psy_audio_Machines* machines;

	assert(self);

	if (!workspace_song(self->workspace)) {
		return;
	}
	machines = &self->workspace->song->machines_;
	if (psy_audio_machines_selected(machines) != psy_INDEX_INVALID) {
		workspace_select_view(self->workspace, viewindex_make_all(
			VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_PROPERTIES,
			psy_audio_machines_selected(machines),
			psy_INDEX_INVALID));
	}
}

void gear_on_master(Gear* self, psy_ui_Component* sender)
{
	assert(self);
	
	if (self->param_views) {
		paramviews_show(self->param_views, psy_audio_MASTER_INDEX);
	}
}

void gear_on_tabbar_changed(Gear* self, psy_ui_TabBar* sender,
	uintptr_t tabindex)
{
	assert(self);
	
	switch (tabindex) {
	case 0:
	psy_ui_label_set_text(&self->label, "gear.labelgenerator");
		break;
	case 1:
	psy_ui_label_set_text(&self->label, "gear.labeleffect");
		break;
	case 2:
	psy_ui_label_set_text(&self->label, "gear.labelinstruments");
		break;
	case 3:
	psy_ui_label_set_text(&self->label, "gear.labelsamples");
		break;
	default:
		break;		
	}
}

void gear_select_section(Gear* self, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options)
{
	assert(self);

	psy_ui_tabbar_select(&self->tabbar, section);
}

void gear_on_float(Gear* self, psy_ui_Button* sender)
{		
	assert(self);	
	
	gear_base(self)->splitter_ = self->splitter_;
	gear_base(self)->frame_icons_ = titlebar_base(&self->title_bar_);
	gear_base(self)->sizer_ = psy_ui_sizer_base(&self->sizer_);
	gear_base(self)->kbd_ = workspace_kbd_driver(self->workspace);
	psy_ui_component_float(gear_base(self));	
}

void gear_on_focus(Gear* self)
{
	assert(self);

	psy_ui_component_set_focus(gearbuttons_base(&self->buttons));
}
