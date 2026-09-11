/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "paramgear.h"

/* host */
#include "paramviews.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* ParamRackBox */

/* prototypes */
static void paramrackbox_on_mouse_double_click(ParamRackBox*,
	psy_ui_Component* sender, psy_ui_MouseEvent*);
static void paramrackbox_on_add_effect(ParamRackBox*,
	psy_ui_Button* sender);

/* implementation */
void paramrackbox_init(ParamRackBox* self, psy_ui_Component* parent,
	psy_audio_Machine* machine, Workspace* workspace)
{	
	assert(self);
	assert(workspace);
		
	psy_ui_component_init(&self->component, parent, NULL);
	self->machine = machine;	
	self->param_views = workspace_param_views(workspace);
	self->workspace = workspace;
	psy_ui_component_set_padding(&self->component, 
		psy_ui_margin_make_em(0.0, 0.5, 0.0, 0.0));	
	psy_ui_component_set_preferred_width(&self->component,
		psy_ui_value_make_ew(26.0));
	psy_ui_component_set_style_type(&self->component, STYLE_MACPARAM_TITLE);		
	psy_ui_component_init_align(&self->header, &self->component, NULL,
		psy_ui_ALIGN_TOP);	
	psy_ui_component_set_style_type(&self->header, STYLE_MACPARAM_TITLE);	
	/* title label */
	psy_ui_label_init(&self->title, &self->header);	
	psy_ui_component_set_preferred_height(&self->title.component,
		psy_ui_value_make_eh(1.5));
	psy_ui_label_prevent_translation(&self->title);	
	if (machine) {
		psy_ui_label_set_text(&self->title, psy_audio_machine_edit_name(
			machine));
	}
	psy_ui_component_set_align(&self->title.component, psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->title.component.signal_mouse_double_click, self,
		paramrackbox_on_mouse_double_click);	
	/* insert effect */
	if (machine && psy_audio_machine_type(machine) != psy_audio_MASTER) {
		psy_ui_button_init(&self->inserteffect, &self->header);
		psy_ui_button_prevent_translation(&self->inserteffect);
		psy_ui_button_set_text(&self->inserteffect, "+");
		psy_ui_component_set_align(&self->inserteffect.component,
			psy_ui_ALIGN_RIGHT);
		psy_signal_connect(&self->inserteffect.signal_clicked, self,
			paramrackbox_on_add_effect);
	}
	/* parameterlist */		
	paramview_init(&self->parameters, &self->component, machine,
		psycleconfig_macparam(workspace_cfg(workspace)), NULL,
		FALSE);
	psy_ui_component_set_wheel_scroll(&self->parameters.component, 4);
	psy_ui_component_set_overflow(&self->parameters.component,
		psy_ui_OVERFLOW_VSCROLL);
		psy_ui_component_set_scroll_step_width(&self->parameters.component,
		psy_ui_value_make_eh(4.0));
	psy_ui_component_set_scroll_step_height(&self->parameters.component,
		psy_ui_value_make_eh(2.0));	
	psy_ui_scroller_init(&self->scroller, &self->component, NULL, NULL);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);		
	psy_ui_scroller_set_client(&self->scroller, &self->parameters.component);	
	self->nextbox = NULL;	
}

ParamRackBox* paramrackbox_alloc(void)
{
	return(ParamRackBox*)malloc(sizeof(ParamRackBox));
}

ParamRackBox* paramrackbox_alloc_init(psy_ui_Component* parent,
	psy_audio_Machine* machine, Workspace* workspace)
{
	ParamRackBox* rv;
	
	rv = paramrackbox_alloc();
	if (rv) {
		paramrackbox_init(rv, parent, machine, workspace);
		psy_ui_component_deallocate_after_destroyed(&rv->component);
	}
	return rv;
}

void paramrackbox_select(ParamRackBox* self)
{
	assert(self);
	
	psy_ui_component_invalidate(&self->header);
}

void paramrackbox_deselect(ParamRackBox* self)
{
	assert(self);
	
	psy_ui_component_invalidate(&self->header);
}

void paramrackbox_on_mouse_double_click(ParamRackBox* self,
	psy_ui_Component* sender, psy_ui_MouseEvent* ev)
{	
	assert(self);
	
	if (self->machine && self->param_views) {
		paramviews_show(self->param_views, psy_audio_machine_slot(
			self->machine));
	}
}

void paramrackbox_on_add_effect(ParamRackBox* self, psy_ui_Button* sender)
{
	assert(self);
	
	if (self->machine) {
		uintptr_t next_mac_id;
		
		if (self->nextbox && self->nextbox->machine) {
			next_mac_id = psy_audio_machine_slot(self->nextbox->machine);
		} else {
			next_mac_id = psy_INDEX_INVALID;
		}
		machineinsert_append(&self->workspace->insert, psy_audio_wire_make(
			psy_audio_machine_slot(self->machine),
			next_mac_id));
		workspace_select_view(self->workspace, viewindex_make_section(
			VIEW_ID_MACHINES, SECTION_ID_MACHINEVIEW_NEWMACHINE));
	}	
}


/* ParamRackPane */

/* implementation */
static void paramrackpane_on_destroyed(ParamRackPane*);
static void paramrackpane_build(ParamRackPane*);
static void paramrackpane_build_all(ParamRackPane*);
static void paramrackpane_build_inputs(ParamRackPane*);
static void paramrackpane_build_outputs(ParamRackPane*);
static void paramrackpane_build_in_chain(ParamRackPane*);
static void paramrackpane_build_out_chain(ParamRackPane*, uintptr_t slot);
static void paramrackpane_build_level(ParamRackPane*, uintptr_t level);
static void paramrackpane_insert_box(ParamRackPane*, uintptr_t slot);
static void paramrackpane_remove_box(ParamRackPane*, uintptr_t slot);
static void paramrackpane_clear(ParamRackPane*);
static void paramrackpane_connect_song(ParamRackPane*);
static void paramrackpane_on_machine_insert(ParamRackPane*,
	psy_audio_Machines* sender, uintptr_t slot);
static void paramrackpane_on_machine_removed(ParamRackPane*,
	psy_audio_Machines* sender, uintptr_t slot);
static void paramrackpane_on_wire_connected(ParamRackPane*,
	psy_audio_Connections* con, uintptr_t output, uintptr_t input);
static void paramrackpane_on_wire_disconnected(ParamRackPane*,
	psy_audio_Connections* con, uintptr_t output, uintptr_t input);
static void paramrackpane_on_song_changed(ParamRackPane*,
	psy_audio_Player* sender);
static void paramrackpane_on_machine_selected(ParamRackPane*,
	psy_audio_Machines* sender, uintptr_t slot);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(ParamRackPane* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *(self->component.vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			paramrackpane_on_destroyed;		
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &vtable);
}

/* implementation */
void paramrackpane_init(ParamRackPane* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	self->lastselected = psy_INDEX_INVALID;
	self->level = 2;
	self->mode = PARAMRACK_OUTCHAIN;
	self->workspace = workspace;
	self->lastinserted = NULL;
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 0.1, 0.0, 0.0));
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_set_scroll_step_width(&self->component,
		psy_ui_value_make_ew(35.0));	
	psy_ui_component_set_overflow(&self->component, psy_ui_OVERFLOW_HSCROLL);	
	psy_table_init(&self->boxes);	
	paramrackpane_connect_song(self);
	paramrackpane_build(self);
	psy_signal_connect(&workspace_player(workspace)->signal_song_changed,
		self, paramrackpane_on_song_changed);
}

void paramrackpane_on_destroyed(ParamRackPane* self)
{
	assert(self);
	
	psy_table_dispose(&self->boxes);
}

void paramrackpane_set_mode(ParamRackPane* self, ParamRackMode mode)
{
	assert(self);
	
	if (self->mode != mode) {
		self->mode = mode;
		paramrackpane_build(self);
	}
}

void paramrackpane_build(ParamRackPane* self)
{	
	assert(self);
		
	psy_ui_component_hide(&self->component);
	paramrackpane_clear(self);
	if (!self->machines) {	
		psy_ui_component_show(&self->component);
		psy_ui_component_align_full(psy_ui_component_parent(
			&self->component));		
		return;
	}	
	self->lastselected = psy_audio_machines_selected(self->machines);
	self->lastinserted = NULL;
	switch (self->mode) {
	case PARAMRACK_INPUTS:		
		paramrackpane_build_inputs(self);			
	break;
	case PARAMRACK_OUTPUTS:					
		paramrackpane_build_outputs(self);			
		break;
	case PARAMRACK_INCHAIN:		
		paramrackpane_build_in_chain(self);			
		break;
	case PARAMRACK_OUTCHAIN: {		
		paramrackpane_build_out_chain(self, self->lastselected);			
		break; }
	case PARAMRACK_LEVEL: {		
		paramrackpane_build_level(self, self->level);			
		break; }		
	case PARAMRACK_ALL:
	default:			
		paramrackpane_build_all(self);
		break;
	}	
	psy_ui_component_show(&self->component);
	psy_ui_component_align_full(psy_ui_component_parent(&self->component));	
	psy_ui_component_invalidate(psy_ui_component_parent(&self->component));
	psy_ui_component_prevent_app_focus_out_recursive(&self->component);
}

void paramrackpane_on_song_changed(ParamRackPane* self,
	psy_audio_Player* sender)
{	
	assert(self);
	
	self->lastselected = psy_INDEX_INVALID;
	paramrackpane_connect_song(self);
	paramrackpane_build(self);
}

void paramrackpane_clear(ParamRackPane* self)
{
	assert(self);
	
	psy_ui_component_clear(&self->component);
	psy_table_clear(&self->boxes);
}

void paramrackpane_build_all(ParamRackPane* self)
{
	psy_TableIterator it;	
	
	assert(self);
	
	for (it = psy_audio_machines_begin(self->machines);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		if (!psy_audio_machines_isvirtualgenerator(self->machines,
				psy_tableiterator_key(&it))) {
			paramrackpane_insert_box(self, psy_tableiterator_key(&it));
		}
	}
}

void paramrackpane_build_inputs(ParamRackPane* self)
{
	psy_audio_MachineSockets* sockets;

	assert(self);

	paramrackpane_insert_box(self, self->lastselected);
	sockets = psy_audio_connections_at(&self->machines->connections,
		self->lastselected);
	if (sockets) {
		psy_TableIterator it;

		for (it = psy_audio_wiresockets_begin(&sockets->inputs);
				!psy_tableiterator_equal(&it, psy_table_end());
				psy_tableiterator_inc(&it)) {
			psy_audio_WireSocket* socket;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&it);
			if (socket->slot != psy_INDEX_INVALID) {
				if (!psy_table_exists(&self->boxes, socket->slot)) {
					paramrackpane_insert_box(self, socket->slot);
				}
			}
		}
	}
}

void paramrackpane_build_outputs(ParamRackPane* self)
{	
	psy_audio_MachineSockets* sockets;

	assert(self);
	
	paramrackpane_insert_box(self, self->lastselected);
	sockets = psy_audio_connections_at(&self->machines->connections,
		self->lastselected);
	if (sockets) {
		psy_TableIterator it;

		for (it = psy_audio_wiresockets_begin(&sockets->outputs);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
			psy_audio_WireSocket* socket;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&it);
			if (socket->slot != psy_INDEX_INVALID) {
				if (!psy_table_exists(&self->boxes, socket->slot)) {
					paramrackpane_insert_box(self, socket->slot);
				}
			}
		}
	}
}

void paramrackpane_build_in_chain(ParamRackPane* self)
{
	assert(self);
	
	if (self->machines) {
		MachineList* path;
			
		path = psy_audio_compute_path(self->machines, self->lastselected,
			FALSE);
		for (; path != 0; path = path->next) {
			uintptr_t slot;

			slot = (size_t)path->entry;
			if (slot == psy_INDEX_INVALID) {
				/* skip parallel process machine delimiter */
				continue;
			}
			paramrackpane_insert_box(self, slot);
		}
		psy_list_free(path);
	}
}

void paramrackpane_build_out_chain(ParamRackPane* self, uintptr_t slot)
{
	psy_audio_MachineSockets* sockets;
	
	assert(self);
	
	paramrackpane_insert_box(self, slot);
	sockets = psy_audio_connections_at(&self->machines->connections, slot);
	if (sockets) {
		psy_TableIterator it;

		for (it = psy_audio_wiresockets_begin(&sockets->outputs);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
			psy_audio_WireSocket* socket;

			socket = (psy_audio_WireSocket*)psy_tableiterator_value(&it);
			if (socket->slot != psy_INDEX_INVALID) {
				if (!psy_table_exists(&self->boxes, socket->slot)) {
					paramrackpane_build_out_chain(self, socket->slot);
				}
			}
		}
	}
}

void paramrackpane_build_level(ParamRackPane* self, uintptr_t level)
{
	assert(self);
	
	if (self->machines) {
		MachineList* path;

		path = psy_audio_machines_level(self->machines, psy_audio_MASTER_INDEX,
			level);
		for (; path != 0; path = path->next) {
			uintptr_t slot;

			slot = (size_t)path->entry;
			if (slot == psy_INDEX_INVALID) {
				continue;
			}
			paramrackpane_insert_box(self, slot);
		}
		psy_list_free(path);
	}
}

 void paramrackpane_insert_box(ParamRackPane* self, uintptr_t slot)
{
	assert(self);
	
	if (psy_audio_machines_at(self->machines, slot)) {
		ParamRackBox* box;

		if (psy_table_exists(&self->boxes, slot)) {
			paramrackpane_remove_box(self, slot);
		}
		box = paramrackbox_alloc_init(&self->component,
			psy_audio_machines_at(self->machines, slot),
			self->workspace);
		if (box) {												
			if (self->lastinserted) {
				self->lastinserted->nextbox = box;
			}
			self->lastinserted = box;
			psy_ui_component_set_align(&box->component,
				psy_ui_ALIGN_LEFT);
			psy_table_insert(&self->boxes, slot, box);			
		}		
	}
}

void paramrackpane_remove_box(ParamRackPane* self, uintptr_t slot)
{
	assert(self);
	
	if (psy_table_exists(&self->boxes, slot)) {
		ParamRackBox* box;

		box = (ParamRackBox*)psy_table_at(&self->boxes, slot);
		psy_ui_component_deallocate(&box->component);		
		psy_table_remove(&self->boxes, slot);		
	}
}

void paramrackpane_connect_song(ParamRackPane* self)
{
	assert(self);
	
	if (workspace_song(self->workspace)) {
		self->machines = &self->workspace->song->machines_;
		psy_signal_connect(&self->machines->signal_slotchange, self,
			paramrackpane_on_machine_selected);
		psy_signal_connect(&self->machines->signal_insert, self,
			paramrackpane_on_machine_insert);
		psy_signal_connect(&self->machines->signal_removed, self,
			paramrackpane_on_machine_removed);
		psy_signal_connect(&self->machines->connections.signal_connected,
			self, paramrackpane_on_wire_connected);
		psy_signal_connect(&self->machines->connections.signal_disconnected,
			self, paramrackpane_on_wire_disconnected);
	} else {
		self->machines = NULL;
	}
}

void paramrackpane_on_machine_insert(ParamRackPane* self,
	psy_audio_Machines* sender, uintptr_t slot)
{
	assert(self);
	
	/* handled in select */	
}

void paramrackpane_on_machine_removed(ParamRackPane* self,
	psy_audio_Machines* sender, uintptr_t slot)
{
	assert(self);
	
	paramrackpane_build(self);
}

void paramrackpane_on_wire_connected(ParamRackPane* self,
	psy_audio_Connections* con, uintptr_t output, uintptr_t input)
{
	assert(self);
	
	if (self->mode != PARAMRACK_ALL) {
		paramrackpane_build(self);
	}
}

void paramrackpane_on_wire_disconnected(ParamRackPane* self,
	psy_audio_Connections* con, uintptr_t output, uintptr_t input)
{
	assert(self);
	
	if (self->mode != PARAMRACK_ALL) {
		paramrackpane_build(self);
	}
}

void paramrackpane_on_machine_selected(ParamRackPane* self,
	psy_audio_Machines* sender, uintptr_t slot)
{
	assert(self);
	
	if (self->lastselected == slot) {
		return;
	}
	if (self->mode == PARAMRACK_LEVEL) {
		uintptr_t level;

		level = psy_audio_machines_levelofmachine(self->machines, slot);
		if (level != psy_INDEX_INVALID) {
			paramrackpane_build(self);			
		}
	} else if (self->mode == PARAMRACK_ALL) {
		if (psy_table_exists(&self->boxes, self->lastselected)) {
			paramrackbox_deselect((ParamRackBox*)
				psy_table_at(&self->boxes, self->lastselected));
		}
		if (psy_table_exists(&self->boxes, slot)) {
			paramrackbox_select((ParamRackBox*)
				psy_table_at(&self->boxes, slot));
			self->lastselected = slot;
		} else {
			self->lastselected = psy_INDEX_INVALID;
		}
	} else {
		self->lastselected = slot;
		paramrackpane_build(self);
	}
}

/* ParamRackBatchBar */
void paramrackbatchbar_init(ParamRackBatchBar* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));	
	psy_ui_button_init_text(&self->select, &self->component, "Select in Gear");
}

/* ParamRackModeBar */
static void paramrackmodebar_on_destroyed(ParamRackModeBar*);
static void paramrackmodebar_onmodeselect(ParamRackModeBar*, psy_ui_Button* sender);

/* vtable */
static psy_ui_ComponentVtable paramrackmodebar_vtable;
static bool paramrackmodebar_vtable_initialized = FALSE;

static void paramrackmodebar_vtable_init(ParamRackModeBar* self)
{
	if (!paramrackmodebar_vtable_initialized) {
		paramrackmodebar_vtable = *(self->component.vtable);
		paramrackmodebar_vtable.on_destroyed =
			(psy_ui_fp_component)
			paramrackmodebar_on_destroyed;
		paramrackmodebar_vtable_initialized = TRUE;
	}
	self->component.vtable = &paramrackmodebar_vtable;
}

/* implementation */
void paramrackmodebar_init(ParamRackModeBar* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	paramrackmodebar_vtable_init(self);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_defaults_hmargin(psy_ui_defaults()));
	psy_signal_init(&self->signal_select);
	psy_ui_button_init_text_connect(&self->all, &self->component,
		"All", self, paramrackmodebar_onmodeselect);
	psy_ui_button_init_text_connect(&self->inputs, &self->component,
		"Inputs", self, paramrackmodebar_onmodeselect);
	psy_ui_button_init_text_connect(&self->outputs, &self->component,
		"Outputs", self, paramrackmodebar_onmodeselect);
	psy_ui_button_init_text_connect(&self->inchain, &self->component,
		"Inchain", self, paramrackmodebar_onmodeselect);
	psy_ui_button_init_text_connect(&self->outchain, &self->component,
		"Outchain", self, paramrackmodebar_onmodeselect);
	psy_ui_button_init_text_connect(&self->level, &self->component,
		"Level", self, paramrackmodebar_onmodeselect);
}

void paramrackmodebar_on_destroyed(ParamRackModeBar* self)
{
	assert(self);
	
	psy_signal_dispose(&self->signal_select);
}

void paramrackmodebar_onmodeselect(ParamRackModeBar* self,
	psy_ui_Button* sender)
{
	ParamRackMode mode;
	
	assert(self);
		
	mode = PARAMRACK_NONE;
	if (sender == &self->all) {		
		mode = PARAMRACK_ALL;
	} else if (sender == &self->inputs) {
		mode = PARAMRACK_INPUTS;
	} else if (sender == &self->outputs) {
		mode = PARAMRACK_OUTPUTS;
	} else if (sender == &self->inchain) {
		mode = PARAMRACK_INCHAIN;
	} else if (sender == &self->outchain) {
		mode = PARAMRACK_OUTCHAIN;
	} else if (sender == &self->level) {
		mode = PARAMRACK_LEVEL;
	}
	paramrackmodebar_set_mode(self, mode);
}

void paramrackmodebar_set_mode(ParamRackModeBar* self, ParamRackMode mode)
{
	assert(self);
	
	psy_ui_button_disable_highlight(&self->all);
	psy_ui_button_disable_highlight(&self->inputs);
	psy_ui_button_disable_highlight(&self->outputs);
	psy_ui_button_disable_highlight(&self->inchain);
	psy_ui_button_disable_highlight(&self->outchain);
	psy_ui_button_disable_highlight(&self->level);
	switch (mode) {
	case PARAMRACK_ALL:
		psy_ui_button_highlight(&self->all);
		break;
	case PARAMRACK_INPUTS:
		psy_ui_button_highlight(&self->inputs);
		break;
	case PARAMRACK_OUTPUTS:
		psy_ui_button_highlight(&self->outputs);
		break;
	case PARAMRACK_INCHAIN:
		psy_ui_button_highlight(&self->inchain);
		break;
	case PARAMRACK_OUTCHAIN:
		psy_ui_button_highlight(&self->outchain);
		break;
	case PARAMRACK_LEVEL:
		psy_ui_button_highlight(&self->level);
		break;
	default:
		break;
	}
	if (mode != PARAMRACK_NONE) {
		psy_signal_emit(&self->signal_select, self, 1, mode);
	}
}


/* ParamRack */

/* prototypes */
static void paramrack_on_mode_selected(ParamRack*, ParamRackModeBar* sender,
	intptr_t index);
static void paramrack_on_level_changed(ParamRack*, psy_ui_NumberEdit* sender);
static void paramrack_on_machine_selected(ParamRack*,
	psy_audio_Machines*, uintptr_t slot);
static void paramrack_on_song_changed(ParamRack*, psy_audio_Player* sender);
static void paramrack_connect_song(ParamRack*, psy_audio_Song*);

/* implementation */
void paramrack_init(ParamRack* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);
	
	psy_ui_component_init(&self->component, parent, NULL);	
	psy_ui_component_set_style_type(&self->component, STYLE_SIDE_VIEW);
	psy_ui_component_set_preferred_height(&self->component,
		psy_ui_value_make_eh(10.0));	
	/* left */
	closebar_init(&self->close, &self->component,
		psy_configuration_at(psycleconfig_general(workspace_cfg(workspace)),
			"bench.showparamrack"));	
	/* bottom */
	psy_ui_component_init_align(&self->bottom, &self->component, NULL,
		psy_ui_ALIGN_BOTTOM);	
	/* level edit */
	psy_ui_numberedit_init(&self->leveledit, &self->bottom, "", 2, 0,
		INT32_MAX);
	psy_ui_component_set_align(&self->leveledit.component, psy_ui_ALIGN_RIGHT);
	psy_signal_connect(&self->leveledit.signal_changed, self,
		paramrack_on_level_changed);
	/* chain mode */
	paramrackmodebar_init(&self->modebar, &self->bottom);		
	psy_ui_component_set_align(&self->modebar.component,
		psy_ui_ALIGN_RIGHT);
	psy_signal_connect(&self->modebar.signal_select, self,
		paramrack_on_mode_selected);	
	/* pane */
	paramrackpane_init(&self->pane, &self->component, workspace);		
	/* scroller */
	psy_ui_scroller_init(&self->scroller, &self->component, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->pane.component);
	psy_ui_component_set_align(&self->pane.component, psy_ui_ALIGN_VCLIENT);
	psy_ui_component_set_align(&self->scroller.component, psy_ui_ALIGN_CLIENT);	
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		paramrack_on_song_changed);
	paramrack_connect_song(self, workspace_song(workspace));
	paramrackmodebar_set_mode(&self->modebar, self->pane.mode);	
	psy_ui_component_prevent_app_focus_out_recursive(&self->component);
}

void paramrack_on_mode_selected(ParamRack* self, ParamRackModeBar* sender,
	intptr_t index)
{	
	assert(self);
			
	paramrackpane_set_mode(&self->pane, (ParamRackMode)index);
	psy_ui_component_show_align(&self->pane.component);	
}

void paramrack_on_level_changed(ParamRack* self, psy_ui_NumberEdit* sender)
{
	assert(self);
	
	if (self->pane.mode == PARAMRACK_LEVEL) {
		self->pane.level = (uintptr_t)psy_ui_numberedit_value(&self->leveledit);
		paramrackpane_build(&self->pane);
	}
}

void paramrack_on_song_changed(ParamRack* self, psy_audio_Player* sender)
{	
	assert(self);
		
	paramrack_connect_song(self, psy_audio_player_song(sender));		
}

void paramrack_connect_song(ParamRack* self, psy_audio_Song* song)
{
	assert(self);
	
	if (song) {
		psy_audio_Machines* machines;

		machines = psy_audio_song_machines(song);
		psy_signal_connect(&machines->signal_slotchange, self,
			paramrack_on_machine_selected);
	}
}

void paramrack_on_machine_selected(ParamRack* self, psy_audio_Machines* sender,
	uintptr_t slot)
{	
	assert(self);
	
	if (self->pane.mode == PARAMRACK_LEVEL && self->pane.lastselected != slot) {
		uintptr_t level;

		level = psy_audio_machines_levelofmachine(sender, slot);
		if (level != psy_INDEX_INVALID) {			
			psy_ui_numberedit_set_value(&self->leveledit, (int32_t)level);
		}
	}
}
