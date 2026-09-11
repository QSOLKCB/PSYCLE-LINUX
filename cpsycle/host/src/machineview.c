/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machineview.h"
/* host */
#include "resources/resource.h"
#include "styles.h"
#include "skinio.h"
#include "wireview.h"
/* platform */
#include "../../detail/os.h"
#include "../../detail/portable.h"


#if defined DIVERSALIS__OS__UNIX
#define _MAX_PATH 4096
#endif

/* prototypes */
static void machineview_init_component(MachineView*, psy_ui_Component* parent);
static void machineview_init_notebook(MachineView*);
static void machineview_init_bar(MachineView* self);
static void machineview_init_wire_view(MachineView*,
	psy_ui_Component* tabbarparent);
static void machineview_init_stack_view(MachineView*,
	psy_ui_Component* tabbarparent);
static void machineview_init_new_machine(MachineView*,
	psy_ui_Component* tabbarparent);
static void machineview_init_tabbar(MachineView*,
	psy_ui_Component* tabbarparent);
static void machineview_init_menu(MachineView*);
static void machineview_on_tabbar_changed(MachineView*, psy_ui_TabBar* sender,
	uintptr_t index);
static void machineview_connect_signals(MachineView*);
static void machineview_connect_configuration(MachineView*, psy_Configuration*);
static void machineview_configure(MachineView*, psy_Configuration* config);
static uintptr_t machineview_section(const MachineView*);
static void machineview_on_song_changed(MachineView*,
	psy_audio_Player* sender);
static void machineview_set_song(MachineView*, psy_audio_Song*);	
static void machineview_on_mouse_down(MachineView*, psy_ui_MouseEvent*);
static void machineview_on_mouse_up(MachineView*, psy_ui_MouseEvent*);
static void machineview_on_mouse_double_click(MachineView*, psy_ui_MouseEvent*);
static void machineview_on_key_down(MachineView*, psy_ui_KeyboardEvent*);
static void machineview_on_focus(MachineView*);
static void machineview_select_section(MachineView*, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options);
/* config */
static void machineview_on_vu_meters(MachineView*, psy_Property* sender);
static void machineview_on_machine_index(MachineView*, psy_Property* sender);
static void machineview_on_select_section(MachineView*,
	psy_ui_Component* sender, uintptr_t param1, uintptr_t param2);
static void machineview_on_invalidate(MachineView*, psy_Property*);
static void machineview_on_update_styles(MachineView*, psy_Property* sender);
static void machineview_on_draw_vu_meters(MachineView*, psy_Property* sender);
	
/* vtable */
static psy_ui_ComponentVtable machineview_vtable;
static bool machineview_vtable_initialized = FALSE;

static void machineview_vtable_init(MachineView* self)
{
	assert(self);
	
	if (!machineview_vtable_initialized) {
		machineview_vtable = *(self->component.vtable);		
		machineview_vtable.on_mouse_down =
			(psy_ui_fp_component_on_mouse_event)
			machineview_on_mouse_down;
		machineview_vtable.on_mouse_up =
			(psy_ui_fp_component_on_mouse_event)
			machineview_on_mouse_up;
		machineview_vtable.on_mouse_double_click =
			(psy_ui_fp_component_on_mouse_event)
			machineview_on_mouse_double_click;
		machineview_vtable.on_key_down =
			(psy_ui_fp_component_on_key_event)
			machineview_on_key_down;
		machineview_vtable.section =
			(psy_ui_fp_component_section)
			machineview_section;
		machineview_vtable.on_focus =
			(psy_ui_fp_component)
			machineview_on_focus;		
		machineview_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(machineview_base(self),
		&machineview_vtable);	
}

/* implementation */
void machineview_init(MachineView* self, psy_ui_Component* parent,
	psy_ui_Component* tabbarparent, Workspace* workspace)
{
	assert(self);
	
	machineview_init_component(self, parent);		
	self->shownewmachine = FALSE;
	self->workspace = workspace;
	self->macview_config = psycleconfig_macview(workspace_cfg(self->workspace));
	psy_ui_component_set_tab_index(machineview_base(self), 0);
	machineviewstyles_init(&self->machine_styles,
		psycleconfig_directories(workspace_cfg(self->workspace)));
	machinestyleconfigurator_init(&self->machine_style_configurator,
		&self->machine_styles,
		psycleconfig_macview(workspace_cfg(self->workspace)));
	paramviewstyles_init(&self->param_styles, psycleconfig_directories(
		workspace_cfg(self->workspace)));
	paramstyleconfigurator_init(&self->param_style_configurator,
		&self->param_styles,
		psycleconfig_macparam(workspace_cfg(self->workspace)));
	machineview_init_bar(self);
	machineview_init_notebook(self);
	machineview_connect_configuration(self, self->macview_config);
	machineview_init_wire_view(self, tabbarparent);
	machineview_init_stack_view(self, tabbarparent);	
	machineview_init_new_machine(self, tabbarparent);	
	machineview_init_tabbar(self, tabbarparent);
	machineview_init_menu(self);
	machineview_set_song(self, workspace_song(workspace));
	machineview_connect_signals(self);	
	machineview_configure(self, self->macview_config);		
	machineview_select_section(self, &self->component, SECTION_ID_MACHINEVIEW_WIRES, 0);	
	psy_signal_connect(&self->component.signal_select_section, self,
		machineview_on_select_section);
}

void machineview_init_component(MachineView* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(machineview_base(self), parent, NULL);
	machineview_vtable_init(self);	
	psy_ui_component_set_title(machineview_base(self), "main.machines");
	psy_ui_component_set_id(machineview_base(self), VIEW_ID_MACHINES);	
}

void machineview_init_notebook(MachineView* self)
{
	assert(self);
	
	psy_ui_notebook_init(&self->notebook, machineview_base(self));
	psy_ui_component_set_align(psy_ui_notebook_base(&self->notebook),
		psy_ui_ALIGN_CLIENT);
}

void machineview_init_bar(MachineView* self)
{
	assert(self);

	machineviewbar_init(&self->machineviewbar, &self->component,
		psycleconfig_macview(workspace_cfg(self->workspace)),
		workspace_player(self->workspace));
	psy_ui_component_set_align(&self->machineviewbar.component,
		psy_ui_ALIGN_BOTTOM);
	self->component.sizer_ = psy_ui_sizer_base(&self->machineviewbar.sizer_);
}

void machineview_init_wire_view(MachineView* self,
	psy_ui_Component* tabbarparent)
{
	assert(self);
	
	machinewireview_init(&self->wireview,
		psy_ui_notebook_base(&self->notebook), tabbarparent,
		workspace_param_views(self->workspace), &self->machine_menu,
		self->workspace);	
	psy_ui_component_set_align(machinewireview_base(&self->wireview),
		psy_ui_ALIGN_CLIENT);	
}

void machineview_init_stack_view(MachineView* self,
	psy_ui_Component* tabbarparent)
{
	assert(self);
	
	machinestackview_init(&self->stackview,
		psy_ui_notebook_base(&self->notebook), tabbarparent,
		workspace_param_views(self->workspace), &self->machine_menu,
		self->workspace);
}

void machineview_init_new_machine(MachineView* self,
	psy_ui_Component* tabbarparent)
{
	assert(self);
	
	newmachine_init(&self->newmachine, psy_ui_notebook_base(&self->notebook),
		self->workspace);
	psy_ui_component_set_align(newmachine_base(&self->newmachine),
		psy_ui_ALIGN_CLIENT);
}

void machineview_init_tabbar(MachineView* self, psy_ui_Component* tabbarparent)
{
	psy_ui_Tab* tab;

	assert(self);
	
	psy_ui_tabbar_init(&self->tab_bar_, tabbarparent);
	psy_ui_component_set_id(psy_ui_tabbar_base(&self->tab_bar_),
		VIEW_ID_MACHINES);
	psy_ui_component_set_align(psy_ui_tabbar_base(&self->tab_bar_),
		psy_ui_ALIGN_LEFT);	
	tab = psy_ui_tabbar_append(&self->tab_bar_, "mv.wires",
		SECTION_ID_MACHINEVIEW_WIRES);
	psy_ui_tab_set_icon(tab, psy_ui_app_svg(psy_ui_app(), "img.machines"));
	tab = psy_ui_tabbar_append(&self->tab_bar_, "mv.stack",
		SECTION_ID_MACHINEVIEW_STACK);
	psy_ui_tab_set_icon(tab, psy_ui_app_svg(psy_ui_app(), "img.stack"));
	tab = psy_ui_tabbar_append(&self->tab_bar_, "mv.new-machine",
		SECTION_ID_MACHINEVIEW_NEWMACHINE);
	psy_ui_tab_set_icon(tab, psy_ui_app_svg(psy_ui_app(), "img.new-machine"));
}

void machineview_init_menu(MachineView* self)
{
	assert(self);
	
	machinemenu_init(&self->machine_menu, machineview_base(self),
		&self->wireview.pane.wire_frames_);
	psy_ui_component_set_align(machinemenu_base(&self->machine_menu),
		psy_ui_ALIGN_RIGHT);	
	psy_ui_component_hide(machinemenu_base(&self->machine_menu));
}

void machineview_connect_signals(MachineView* self)
{
	assert(self);
	
	psy_signal_connect(&self->component.signal_select_section, self,
		machineview_select_section);	
	psy_signal_connect(&self->tab_bar_.signal_change, self,
		machineview_on_tabbar_changed);
	psy_signal_connect(&self->workspace->player_.signal_song_changed, self,
		machineview_on_song_changed);	
}

void machineview_connect_configuration(MachineView* self,
	psy_Configuration* config)
{
	assert(self);
		
	psy_configuration_connect(config, "drawvumeters",
		self, machineview_on_vu_meters);
	psy_configuration_connect(config, "drawmachineindexes",
		self, machineview_on_machine_index);	
	machinestyleconfigurator_connect(&self->machine_style_configurator);
	psy_configuration_connect(config, "theme.machine_skin",
		self, machineview_on_update_styles);
	psy_signal_connect(&psy_ui_app_zoom(psy_ui_app())->signal_zoom,
		self, machineview_on_invalidate);
	psy_configuration_connect(config, "zoom",
		self, machineview_on_invalidate);
	paramstyleconfigurator_connect(&self->param_style_configurator);
}

void machineview_configure(MachineView* self, psy_Configuration* config)
{	
	assert(self);
	
	psy_configuration_configure(config, "drawvumeters");
	psy_configuration_configure(config, "drawmachineindexes");	
	machinestyleconfigurator_configure(&self->machine_style_configurator);	
	paramstyleconfigurator_configure(&self->param_style_configurator);	
}

void machineview_on_mouse_double_click(MachineView* self, psy_ui_MouseEvent* ev)
{		
	assert(self);
		
	if (psy_ui_mouseevent_button(ev) == 1) {
		if (psy_ui_component_section(&self->component) !=
				SECTION_ID_MACHINEVIEW_NEWMACHINE) {		
			self->newmachine.restore_view.section = psy_ui_component_section(
				&self->component);		
			self->shownewmachine = TRUE;
			return;
		} else {
			self->shownewmachine = FALSE;
		}
	}
	psy_ui_mouseevent_stop_propagation(ev);
}

void machineview_on_mouse_down(MachineView* self, psy_ui_MouseEvent* ev)
{	
	assert(self);
	
	psy_ui_mouseevent_stop_propagation(ev);
}

void machineview_on_mouse_up(MachineView* self, psy_ui_MouseEvent* ev)
{
	assert(self);
	
	if (self->shownewmachine) {		
		psy_ui_component_select_section(machineview_base(self),
			SECTION_ID_MACHINEVIEW_NEWMACHINE,
			psy_INDEX_INVALID);		
		self->shownewmachine = FALSE;
		psy_ui_mouseevent_stop_propagation(ev);
	} else if (psy_ui_mouseevent_button(ev) == 2) {
		if (psy_ui_tabbar_selected(&self->tab_bar_) ==
				SECTION_ID_MACHINEVIEW_NEWMACHINE) {
			psy_ui_tabbar_select(&self->tab_bar_,
				self->newmachine.restore_view.section);
		}
	}
}

void machineview_on_key_down(MachineView* self, psy_ui_KeyboardEvent* ev)
{
	assert(self);
	
	if (psy_ui_keyboardevent_keycode(ev) == psy_ui_KEY_ESCAPE) {
		if (psy_ui_component_section(&self->component) ==
				SECTION_ID_MACHINEVIEW_NEWMACHINE) {
			psy_ui_tabbar_select(&self->tab_bar_,
				self->newmachine.restore_view.section);
			if (psy_ui_notebook_active_page(&self->notebook)) {
				psy_ui_component_set_focus(
					psy_ui_notebook_active_page(&self->notebook));
			}
		}
		psy_ui_keyboardevent_stop_propagation(ev);
	} 
}

void machineview_on_focus(MachineView* self)
{
	assert(self);
	
	if (psy_ui_notebook_active_page(&self->notebook)) {
		psy_ui_component_set_focus(
			psy_ui_notebook_active_page(&self->notebook));
	}
}

void machineview_on_tabbar_changed(MachineView* self, psy_ui_TabBar* sender,
	uintptr_t index)
{
	psy_ui_Tab* tab;

	assert(self);
		
	tab = psy_ui_tabbar_tab(sender, index);
	if (machineview_section(self) != psy_ui_component_id(&tab->component)) {
		machineview_select_section(self, machineview_base(self), 
			psy_ui_component_id(&tab->component), 0);
	}
}

void machineview_select_section(MachineView* self, psy_ui_Component* sender,
	uintptr_t section, uintptr_t options)
{
	assert(self);
				
	if (section != psy_INDEX_INVALID) {
		psy_ui_tabbar_mark_id(&self->tab_bar_, section);
		psy_ui_notebook_select_by_component_id(&self->notebook,
			section);
		if (section == SECTION_ID_MACHINEVIEW_NEWMACHINE) {
			psy_ui_component_hide_align(machineviewbar_base(
				&self->machineviewbar));
		} else {
			psy_ui_component_show_align(machineviewbar_base(
				&self->machineviewbar));
		}
	}
}

uintptr_t machineview_section(const MachineView* self)
{
	const psy_ui_Component* curr;
	
	assert(self);
	
	curr = psy_ui_notebook_active_page_const(&self->notebook);
	if (curr) {		
		return psy_ui_component_id(curr);
	}	
	return SECTION_ID_MACHINEVIEW_WIRES; 
}

void machineview_on_song_changed(MachineView* self, psy_audio_Player* sender)
{	
	assert(self);
			
	machineview_set_song(self, psy_audio_player_song(sender));
	psy_ui_tabbar_select_id(&self->tab_bar_, SECTION_ID_MACHINEVIEW_WIRES);	
}

void machineview_set_song(MachineView* self, psy_audio_Song* song)
{
	assert(self);
	
	if (song) {		
		machinemenu_set_machines(&self->machine_menu,
			psy_audio_song_machines(song));
	} else {
		machinemenu_set_machines(&self->machine_menu, NULL);
	}
}

void machineview_idle(MachineView* self)
{		
	machinewireview_idle(&self->wireview);	
	machinestackview_idle(&self->stackview);	
}

/* configuration */
void machineview_on_vu_meters(MachineView* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		machineui_enable_vumeter();
	} else {
		machineui_prevent_vumeter();
	}
}

void machineview_on_machine_index(MachineView* self, psy_Property* sender)
{
	assert(self);
	
	if (psy_property_item_bool(sender)) {
		machineui_enable_macindex();
	} else {
		machineui_prevent_macindex();
	}
}

void machineview_on_select_section(MachineView* self, psy_ui_Component* sender,
	uintptr_t param1, uintptr_t param2)
{
	assert(self);	
	
	if (param1 == SECTION_ID_MACHINEVIEW_PROPERTIES) {
		machinemenu_select(&self->machine_menu, param2);		
	} else if (param1 == SECTION_ID_MACHINEVIEW_BANK_MANGER) {
		machinemenu_select(&self->machine_menu, param2);
		machinemenu_show_bank_manager(&self->machine_menu);		
	}
}

void machineview_on_invalidate(MachineView* self, psy_Property* sender)
{
	psy_ui_component_align(&self->component);
	psy_ui_component_invalidate(&self->component);
}

void machineview_on_update_styles(MachineView* self, psy_Property* sender)
{
	assert(self);
		
	machinewireview_update_styles(&self->wireview);
}

void machineview_on_draw_vu_meters(MachineView* self, psy_Property* sender)
{
	/*	if (psy_property_item_bool(sender)) {
			psy_audio_player_set_vu_meter_mode(&self->workspace->player,
				VUMETER_RMS);
		}
		else {
			psy_audio_player_set_vu_meter_mode(&self->workspace->player,
				VUMETER_NONE);
		}*/
}
