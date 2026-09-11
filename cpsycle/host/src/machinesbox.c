/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machinesbox.h"
/* host */
#include "paramviews.h"
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void machinesbox_clear(MachinesBox*);
static void machinesbox_build(MachinesBox*);
static void machinesbox_append(MachinesBox*, uintptr_t slot);
static uintptr_t machinesbox_set(MachinesBox*, uintptr_t slot);
static bool machinesbox_check_slot(const MachinesBox*, uintptr_t slot);
static void machinesbox_on_machine_selected(MachinesBox*,
	psy_audio_Machines* sender, uintptr_t slot);
static void machinesbox_on_machines_insert(MachinesBox*,
	psy_audio_Machines* sender, uintptr_t slot);
static void machinesbox_on_machines_removed(MachinesBox*,
	psy_audio_Machines*, uintptr_t slot);
static void machinesbox_on_listbox_selected(MachinesBox*,
	psy_ui_ListBox* sender);
static psy_audio_Machine* machinesbox_machine_at(MachinesBox*,
	uintptr_t listbox_index);
static uintptr_t machinesbox_slot(const MachinesBox*, uintptr_t listbox_index);
static uintptr_t machinesbox_index(const MachinesBox*, uintptr_t machine_slot);
static void machinesbox_mac_range(const MachinesBox*, uintptr_t* rv_start_slot,
	uintptr_t* rv_end_slot);
static char* machinesbox_item_text(MachinesBox*, uintptr_t slot, char* rv);

/* implementation */
void machinesbox_init(MachinesBox* self, psy_ui_Component* parent,
	psy_audio_Machines* machines, MachineBoxMode mode,
	ParamViews* param_views)
{	
	assert(self);
	
	psy_ui_listbox_init_multi_select(&self->listbox, parent);
	psy_ui_component_set_tab_index(psy_ui_listbox_base(
		&self->listbox), 0);
	psy_ui_component_set_tab_index(&self->listbox.pane.component, 0);
	self->param_views = param_views;
	self->mode = mode;	
	psy_ui_component_set_style_type(machinesbox_base(self), STYLE_BOX);		
	machinesbox_set_machines(self, machines);
	psy_signal_connect(&self->listbox.signal_selchanged, self,
		machinesbox_on_listbox_selected);	
}

void machinesbox_build(MachinesBox* self)
{
	uintptr_t slot;
	uintptr_t start;
	uintptr_t end;

	assert(self);

	machinesbox_clear(self);
	machinesbox_mac_range(self, &start, &end);
	for (slot = start; slot <= end; ++slot) {
		machinesbox_append(self, slot);
	}	
}

void machinesbox_mac_range(const MachinesBox* self, uintptr_t* rv_start_slot,
	uintptr_t* rv_end_slot)
{
	assert(self);
	assert(rv_start_slot);
	assert(rv_end_slot);
	
	switch (self->mode) {
	case MACHINEBOX_FX:
		*rv_start_slot = 0x40;
		*rv_end_slot = 0x40 + 0x3F;
		break;
	case MACHINEBOX_ALL:
		*rv_start_slot = 0;
		*rv_end_slot = 0xFF;
		break;
	default:
		*rv_start_slot = 0;
		*rv_end_slot = 0x3F;		
		break;
	}	
}

void machinesbox_append(MachinesBox* self, uintptr_t slot)
{
		uintptr_t list_index;
		psy_ui_ListItem* item;
		char buffer[128];
			
		list_index = psy_ui_listbox_add_text(&self->listbox, 
			machinesbox_item_text(self, slot, buffer));
		item = psy_ui_listbox_at(&self->listbox, list_index);
		if (item) {
			psy_ui_listitem_set_id(item, slot);
		}		
}

uintptr_t machinesbox_set(MachinesBox* self, uintptr_t slot)
{
	uintptr_t list_index;
	
	list_index = machinesbox_index(self, slot);
	if (list_index != psy_INDEX_INVALID) {
		char buffer[128];
		
		psy_ui_listbox_set_text(&self->listbox, machinesbox_item_text(
			self, slot, buffer), list_index);
	}
	return list_index;
}

char* machinesbox_item_text(MachinesBox* self, uintptr_t slot, char* rv)
{	
	assert(self);
		
	if (self->machines) {
		psy_audio_Machine* machine;

		machine = psy_audio_machines_at(self->machines, slot);
		if (machine) {
			psy_snprintf(rv, 128, "%02X:%s", slot, psy_audio_machine_edit_name(
				machine));
			return rv;
		}
	}
	psy_snprintf(rv, 128, "%02X:", slot);
	return rv;
}

bool machinesbox_check_slot(const MachinesBox* self, uintptr_t slot)
{
	uintptr_t start_slot;
	uintptr_t end_slot;

	assert(self);
	machinesbox_mac_range(self, &start_slot, &end_slot);
	return (slot >= start_slot && slot <= end_slot);
}

void machinesbox_clear(MachinesBox* self)
{
	assert(self);

	psy_ui_listbox_clear(&self->listbox);	
}

void machinesbox_on_listbox_selected(MachinesBox* self, psy_ui_ListBox* sender)
{			
	uintptr_t slot;

	assert(self);

	if (!self->machines) {
		return;
	}
	slot = machinesbox_slot(self, psy_ui_listbox_cur_sel(sender));
	if (slot != psy_INDEX_INVALID) {		
		/* prevent self notify to keep multi selection */
		psy_signal_disconnect(&self->machines->signal_slotchange, self,
				machinesbox_on_machine_selected);
		psy_audio_machines_select(self->machines, slot);
		psy_signal_connect(&self->machines->signal_slotchange, self,
			machinesbox_on_machine_selected);		
	}
}

void machinesbox_on_machines_insert(MachinesBox* self,
	psy_audio_Machines* sender, uintptr_t slot)
{	
	if (!machinesbox_check_slot(self, slot)) {
		return;
	}
	psy_ui_listbox_set_cur_sel(&self->listbox, machinesbox_set(self, slot));	
}

void machinesbox_on_machine_selected(MachinesBox* self,
	psy_audio_Machines* sender, uintptr_t slot)
{	
	psy_ui_listbox_set_cur_sel(&self->listbox, machinesbox_index(self, slot));
}

void machinesbox_on_machines_removed(MachinesBox* self,
	psy_audio_Machines* sender, uintptr_t slot)
{					
	psy_ui_listbox_set_cur_sel(&self->listbox, machinesbox_set(self, slot));
}

void machinesbox_clone(MachinesBox* self)
{
	uintptr_t selection[256];
	uintptr_t count;

	assert(self);

	if (!self->machines) {
		return;
	}
	machinesbox_selection(self, selection, &count);
	psy_audio_machines_clone_selection(self->machines, selection, count);		
}

void machinesbox_remove(MachinesBox* self)
{	
	uintptr_t selection[256];
	uintptr_t count;

	assert(self);

	if (!self->machines) {
		return;
	}
	machinesbox_selection(self, selection, &count);	
	psy_audio_machines_remove_selection(self->machines, selection, count,
		TRUE);
}

void machinesbox_exchange(MachinesBox* self)
{
	uintptr_t selection[256];
	uintptr_t count;

	assert(self);

	if (!self->machines) {
		return;
	}
	machinesbox_selection(self, selection, &count);
	psy_audio_machines_exchange_selection(self->machines, selection, count,
		TRUE);
}

void machinesbox_connect_to_master(MachinesBox* self)
{
	uintptr_t selection[256];
	uintptr_t count;

	assert(self);

	if (!self->machines) {
		return;
	}
	machinesbox_selection(self, selection, &count);
	psy_audio_machines_connect_selection_to(self->machines, selection, count,
		psy_audio_MASTER_INDEX);
}

void machinesbox_show_parameters(MachinesBox* self)
{	
	uintptr_t selection[256];
	uintptr_t i;
	uintptr_t count;

	assert(self);

	if (!self->param_views) {
		return;
	}
	machinesbox_selection(self, selection, &count);	
	for (i = 0; i < count; ++i) {			
		paramviews_show(self->param_views, selection[i]);
	}	
}

psy_audio_Machine* machinesbox_machine_at(MachinesBox* self, uintptr_t listbox_index)
{
	assert(self);
	
	return psy_audio_machines_at(self->machines, machinesbox_slot(self,
		listbox_index));
}

uintptr_t machinesbox_slot(const MachinesBox* self, uintptr_t listbox_index)
{
	const psy_ui_ListItem* item;

	assert(self);

	item = psy_ui_listbox_at_const(&self->listbox, listbox_index);
	if (item) {				
		return psy_ui_listitem_id(item);
	}
	return psy_INDEX_INVALID;
}

uintptr_t machinesbox_index(const MachinesBox* self, uintptr_t machine_slot)
{	
	uintptr_t rv;
	uintptr_t i;
	uintptr_t num;

	assert(self);

	rv = psy_INDEX_INVALID;
	num = psy_ui_listbox_count(&self->listbox);
	for (i = 0; i < num; ++i) {
		const psy_ui_ListItem* item;

		item = psy_ui_listbox_at_const(&self->listbox, i);
		if (item && (psy_ui_listitem_id(item) == machine_slot)) {
			rv = i;
			break;
		}		
	}
	return rv;
}

void machinesbox_selection(const MachinesBox* self, uintptr_t* rv_selection,
	uintptr_t* rv_count)
{
	assert(self);
	assert(rv_selection);
	assert(rv_count);

	*rv_count = psy_ui_listbox_sel_count(&self->listbox);
	if (*rv_count > 0) {
		uintptr_t i;

		psy_ui_listbox_sel_items(&self->listbox, rv_selection, *rv_count);
		for (i = 0; i < *rv_count; ++i) {
			rv_selection[i] = machinesbox_slot(self, rv_selection[i]);
		}
	}
}

void machinesbox_set_machines(MachinesBox* self, psy_audio_Machines* machines)
{
	assert(self);

	self->machines = machines;
	machinesbox_build(self);
	if (!self->machines) {
		return;
	}
	psy_audio_machines_connect_insert(self->machines,
		self, machinesbox_on_machines_insert);
	psy_audio_machines_connect_removed(self->machines,
		self, machinesbox_on_machines_removed);
	psy_audio_machines_connect_slot_change(self->machines,
		self, machinesbox_on_machine_selected);	
}
