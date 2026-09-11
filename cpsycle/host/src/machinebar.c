/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "machinebar.h"
/* audio */
#include <songio.h>
#include <containerconvert.h>
#include <plugin_interface.h>
/* platform */ 
#include "../../detail/portable.h"


/* prototypes */
static void machinebar_on_destroyed(MachineBar*);
static void machinebar_init_machine_combobox(MachineBar*);
static void machinebar_init_tool_buttons(MachineBar*);
static void machinebar_init_select_inst_param_combobox(MachineBar*);
static void machinebar_init_inst_param_combobox(MachineBar*);
static void machinebar_init_sample_buttons(MachineBar*);
static MachineBarInstParamMode machinebar_mode(MachineBar*);
static void machinebar_update_mode(MachineBar*);
static void machinebar_on_machine_box_sel_change(MachineBar*,
	psy_ui_ComboBox* sender);
static void machinebar_update_combo_gen(MachineBar*);
static void machinebar_on_machine_insert(MachineBar*,
	psy_audio_Machines* sender, uintptr_t slot);
static bool machinebar_insert_machine(MachineBar*, size_t slot,
	psy_audio_Machine*);
static void machinebar_on_machine_removed(MachineBar*,
	psy_audio_Machines* sender, uintptr_t slot);
static void machinebar_on_machine_select(MachineBar*,
	psy_audio_Machines* sender, uintptr_t slot);
static void machinebar_on_param_selected(MachineBar*,
	psy_audio_Machines* sender, uintptr_t slot);
static void machinebar_on_select_inst_param_sel_change(MachineBar*,
	psy_ui_ComboBox* sender);
static void machinebar_on_inst_param_box_sel_change(MachineBar*,
	psy_ui_ComboBox* sender);
static void machinebar_update_combo_ins(MachineBar*);
static void machinebar_build_param_list(MachineBar*);
static void machinebar_build_aux_combo(MachineBar*);
static void machinebar_on_instrument_name_changed(MachineBar*,
	psy_ui_Component* sender);
static void machinebar_on_instrument_insert(MachineBar*,
	psy_audio_Instruments* sender, uintptr_t slot);
static void machinebar_on_instrument_slot_changed(MachineBar*,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex*);
static void machinebar_on_song_changed(MachineBar*, psy_audio_Player* sender);
static void machinebar_set_song(MachineBar*, psy_audio_Song*);
static void machinebar_connect_song(MachineBar*);
static void machinebar_connect_instrument_signals(MachineBar*);
static void machinebar_clear_combo_gen(MachineBar*);
static void machinebar_on_prev_machine(MachineBar*, psy_ui_Component* sender);
static void machinebar_on_next_machine(MachineBar*, psy_ui_Component* sender);
static uintptr_t machinebar_combo_box_index(MachineBar*, uintptr_t slot);
static void machinebar_on_sample_load(MachineBar*, psy_ui_Component* sender);
static void machinebar_on_sample_edit(MachineBar*, psy_ui_Component* sender);
static void machinebar_update_aux(MachineBar*);
static void machinebar_on_aux_changed(MachineBar*, psy_audio_Machines* sender);

/* vtable */
static psy_ui_ComponentVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(MachineBar* self)
{
	assert(self);

	if (!vtable_initialized) {
		vtable = *(machinebar_base(self)->vtable);
		vtable.on_destroyed =
			(psy_ui_fp_component)
			machinebar_on_destroyed;
		vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(machinebar_base(self), &vtable);
}

/* implementation */
void machinebar_init(MachineBar* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);
	assert(workspace);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	self->workspace = workspace;
	self->curr_instr_group = 0;
	psy_table_init(&self->comboboxslots);
	psy_table_init(&self->slotscombobox);	
	psy_ui_component_set_default_align(machinebar_base(self),
		psy_ui_ALIGN_LEFT, psy_ui_margin_make_em(0.0, 2.0, 0.0, 0.0));
	machinebar_init_machine_combobox(self);	
	machinebar_init_tool_buttons(self);
	machinebar_init_select_inst_param_combobox(self);
	machinebar_init_inst_param_combobox(self);	
	machinebar_init_sample_buttons(self);
	psy_signal_connect(&workspace->player_.signal_song_changed, self,
		machinebar_on_song_changed);
	machinebar_set_song(self, workspace_song(workspace));
}

void machinebar_on_destroyed(MachineBar* self)
{
	assert(self);

	psy_table_dispose(&self->comboboxslots);
	psy_table_dispose(&self->slotscombobox);
}

void machinebar_init_machine_combobox(MachineBar* self)
{
	assert(self);

	psy_ui_combobox_init(&self->machinebox, machinebar_base(self));
	psy_ui_combobox_set_char_number(&self->machinebox, 30);
	psy_ui_component_set_tab_index(psy_ui_combobox_base(&self->machinebox), 0);
	psy_signal_connect(&self->machinebox.signal_selchanged, self,
		machinebar_on_machine_box_sel_change);	
}

void machinebar_init_tool_buttons(MachineBar* self)
{
	psy_Configuration* general;

	assert(self);

	general = psycleconfig_general(workspace_cfg(self->workspace));
	psy_ui_button_init(&self->gear, machinebar_base(self));
	psy_ui_button_exchange(&self->gear, psy_configuration_at(general,
		"bench.showgear"));
	psy_ui_button_set_text(&self->gear, "machinebar.gear");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->gear), 0);
	psy_ui_button_init(&self->dock, machinebar_base(self));
	psy_ui_button_exchange(&self->dock,
		psy_configuration_at(general, "bench.showparamrack"));
	psy_ui_button_set_text(&self->dock, "machinebar.dock");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->dock), 0);
#ifdef PSYCLE_USE_PLUGIN_EDITOR	
	psy_ui_button_init(&self->editor, machinebar_base(self));
	psy_ui_button_exchange(&self->editor, psy_configuration_at(general,
		"bench.showplugineditor"));
	psy_ui_button_set_text(&self->editor, "machinebar.editor");
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->editor), 0);
#endif	
}

void machinebar_init_select_inst_param_combobox(MachineBar* self)
{
	psy_ui_combobox_init(&self->select_inst_param_, &self->component);
	psy_ui_combobox_set_char_number(&self->select_inst_param_, 14);
	psy_ui_combobox_add_text(&self->select_inst_param_, "Params");
	psy_ui_combobox_add_text(&self->select_inst_param_, psy_ui_translate(
		"Instrument"));
	psy_ui_component_set_tab_index(psy_ui_combobox_base(&self->select_inst_param_), 0);
	psy_signal_connect(&self->select_inst_param_.signal_selchanged, self,
		machinebar_on_select_inst_param_sel_change);
}

void machinebar_init_inst_param_combobox(MachineBar* self)
{
	assert(self);

	psy_ui_combobox_init(&self->inst_param_box_, machinebar_base(self));
	psy_ui_combobox_set_char_number(&self->inst_param_box_, 30.0);	
	psy_ui_component_set_tab_index(psy_ui_combobox_base(&self->inst_param_box_), 0);
	psy_signal_connect(&self->inst_param_box_.signal_selchanged, self,
		machinebar_on_inst_param_box_sel_change);
}

void machinebar_init_sample_buttons(MachineBar* self)
{
	assert(self);

	psy_ui_button_init_text_connect(&self->load, &self->component,
		"machinebar.load", self, machinebar_on_sample_load);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->load), 0);
	psy_ui_button_init_text_connect(&self->edit, &self->component,
		"machinebar.waveeditor", self, machinebar_on_sample_edit);
	psy_ui_component_set_tab_index(psy_ui_button_base(&self->edit), 0);
}

MachineBarInstParamMode machinebar_mode(MachineBar* self)
{
	assert(self);

	if (psy_ui_combobox_cur_sel(&self->select_inst_param_) == 0) {
		return MACHINEBAR_PARAM;
	}
	if (psy_audio_machines_selected_machine(self->machines) &&
			psy_audio_machine_num_aux_columns(
				psy_audio_machines_selected_machine(self->machines)) > 0) {
		return MACHINEBAR_AUX;
	}
	return MACHINEBAR_PARAM;
}

void machinebar_update_mode(MachineBar* self)
{
	psy_audio_Machine* machine;

	assert(self);

	machine = psy_audio_machines_selected_machine(self->machines);	
	if (machine && (machine_supports(machine, psy_audio_SUPPORTS_INSTRUMENTS) ||
			psy_audio_machine_num_aux_columns(machine))) {
		psy_ui_combobox_select(&self->select_inst_param_, 1);
	} else {
		psy_ui_combobox_select(&self->select_inst_param_, 0);
	}
}

void machinebar_on_instrument_insert(MachineBar* self,
	psy_audio_Instruments* sender, uintptr_t slot)
{	

	assert(self);

	machinebar_update_aux(self);
}

void machinebar_on_instrument_slot_changed(MachineBar* self,
	psy_audio_Instrument* sender, const psy_audio_InstrumentIndex* slot)
{
	assert(self);

	if (self->curr_instr_group != slot->group) {
		machinebar_update_aux(self);
	}
	psy_ui_combobox_select(&self->inst_param_box_, slot->subslot);	
}

void machinebar_on_machine_insert(MachineBar* self, psy_audio_Machines* sender,
	uintptr_t slot)
{	
	assert(self);

	machinebar_update_combo_gen(self);
	psy_ui_combobox_select(&self->machinebox,
		machinebar_combo_box_index(self,
		psy_audio_machines_selected(sender)));
}

void machinebar_on_machine_removed(MachineBar* self, psy_audio_Machines* sender,
	uintptr_t slot)
{
	assert(self);

	machinebar_update_combo_gen(self);
	psy_ui_combobox_select(&self->machinebox,
		machinebar_combo_box_index(self,
		psy_audio_machines_selected(sender)));
}

void machinebar_update_combo_gen(MachineBar* self)
{
	assert(self);

	machinebar_clear_combo_gen(self);
	if (psy_audio_machines_size(self->machines) == 1) {
		psy_ui_combobox_add_text(&self->machinebox, psy_ui_translate(
			"mv.no-machines-loaded"));
		psy_ui_combobox_select(&self->machinebox, 0);
	} else if (self->machines &&
			(psy_audio_machines_size(self->machines) > 0)) {
		uintptr_t i;

		for (i = 0; i <= psy_audio_machines_maxindex(self->machines); ++i) {
			psy_audio_Machine* machine;

			machine = psy_audio_machines_at(self->machines, i);
			if (machine) {
				machinebar_insert_machine(self, i, machine);
			}
		}
		i = psy_ui_combobox_add_text(&self->machinebox,
			"------------------------------");
		psy_table_insert(&self->comboboxslots, i, (void*)psy_INDEX_INVALID);
		psy_table_insert(&self->slotscombobox, psy_INDEX_INVALID, (void*)i);
	}
}

void machinebar_clear_combo_gen(MachineBar* self)
{
	assert(self);

	psy_ui_combobox_clear(&self->machinebox);
	psy_table_dispose(&self->comboboxslots);
	psy_table_init(&self->comboboxslots);
	psy_table_dispose(&self->slotscombobox);
	psy_table_init(&self->slotscombobox);
}

bool machinebar_insert_machine(MachineBar* self, size_t slot, psy_audio_Machine* machine)
{			
	assert(self);

	if (slot != psy_audio_MASTER_INDEX && psy_audio_machine_info(machine) &&
			psy_audio_machine_info(machine)->shortname) {
		uintptr_t comboboxindex;

		char buffer[128];
		psy_snprintf(buffer, 128, "%02X: %s", slot, 
			machine->vtable->info(machine)->shortname); 
		comboboxindex = psy_ui_combobox_add_text(&self->machinebox, buffer);
		psy_table_insert(&self->comboboxslots, comboboxindex, (void*)slot);
		psy_table_insert(&self->slotscombobox, slot, (void*)comboboxindex);
	}
	return 1;
}

void machinebar_on_machine_box_sel_change(MachineBar* self,
	psy_ui_ComboBox* sender)
{	
	uintptr_t slot;
	
	assert(self);

	slot = (uintptr_t)psy_table_at(&self->comboboxslots, 
		psy_ui_combobox_cur_sel(sender));
	psy_audio_machines_select(self->machines, slot);	
}

void machinebar_on_machine_select(MachineBar* self, psy_audio_Machines* machines,
	uintptr_t slot)
{	
	assert(self);

	psy_ui_combobox_select(&self->machinebox,
		machinebar_combo_box_index(self, slot));
	if (workspace_song(self->workspace)) {
		machinebar_update_mode(self);
		machinebar_update_combo_ins(self);
	}
}

void machinebar_on_param_selected(MachineBar* self, psy_audio_Machines* sender,
	uintptr_t slot)
{	
	assert(self);

	psy_ui_combobox_select(&self->inst_param_box_, slot);
}

void machinebar_on_aux_changed(MachineBar* self, psy_audio_Machines* sender)
{
	assert(self);

	machinebar_update_aux(self);
}

void machinebar_update_aux(MachineBar* self)
{
	assert(self);

	switch (machinebar_mode(self)) {
	case MACHINEBAR_AUX:
		machinebar_build_aux_combo(self);
		if (psy_audio_machines_selected_machine(self->machines)) {
			psy_ui_combobox_select(&self->inst_param_box_,
				psy_audio_machine_aux_column_selected(
					psy_audio_machines_selected_machine(self->machines)));
		}
		else {
			psy_ui_combobox_select(&self->select_inst_param_, -1);
		}
		break;
	default:
		break;
	}
}

void machinebar_update_combo_ins(MachineBar* self)
{
	assert(self);

	switch (machinebar_mode(self)) {		
		case MACHINEBAR_AUX:
			psy_ui_combobox_select(&self->select_inst_param_, 1);
			machinebar_build_aux_combo(self);
			if (psy_audio_machines_selected_machine(self->machines)) {
				psy_ui_combobox_select(&self->inst_param_box_,
					psy_audio_machine_aux_column_selected(
						psy_audio_machines_selected_machine(self->machines)));
			} else {
				psy_ui_combobox_select(&self->select_inst_param_, -1);
			}
			break;
		case MACHINEBAR_PARAM:
		default:
			psy_ui_combobox_select(&self->select_inst_param_, 0);
			machinebar_build_param_list(self);
			if (psy_audio_machines_selected_machine(self->machines)) {
				psy_ui_combobox_select(&self->inst_param_box_,
					psy_audio_machine_param_selected(
						psy_audio_machines_selected_machine(self->machines)));
			} else {
				psy_ui_combobox_select(&self->inst_param_box_,
					psy_audio_machines_paramselected(self->machines));
			}
			break;		
	}	
}

void machinebar_build_param_list(MachineBar* self)
{
	psy_audio_Machine* machine;

	assert(self);

	psy_ui_combobox_clear(&self->inst_param_box_);
	machine = (self->machines)
		? psy_audio_machines_at(self->machines, psy_audio_machines_selected(self->machines))
		: NULL;
	if (machine) {
		uintptr_t i;

		for (i = 0; i != psy_audio_machine_num_tweak_parameters(machine); ++i) {
			psy_audio_MachineParam* param;			

			param = psy_audio_machine_tweak_parameter(machine, i);
			if (param) {
				char text[128];
				char label[128];

				if (!psy_audio_machineparam_label(param, label)) {
					if (!psy_audio_machineparam_name(param, label)) {
						psy_snprintf(text, 128, "Parameter", label);
					}
				}
				if ((psy_audio_machineparam_type(param) & MPF_HEADER) ==
						MPF_HEADER) {
					psy_snprintf(text, 128, "-----%s-----", label);
				} else {
					psy_snprintf(text, 128, "%s", label);
				}
				psy_ui_combobox_add_text(&self->inst_param_box_, text);
			}
		}
	} else {
		psy_ui_combobox_add_text(&self->inst_param_box_, psy_ui_translate(
			"mv.no-machine"));
	}
}

void machinebar_build_aux_combo(MachineBar* self)
{	
	psy_audio_Machine* machine;

	assert(self);

	psy_ui_combobox_clear(&self->inst_param_box_);
	machine = psy_audio_machines_selected_machine(self->machines);
	if (machine) {
		uintptr_t index;

		for (index = 0 ; index < psy_audio_machine_num_aux_columns(machine);
				++index) {
			psy_ui_combobox_add_text(&self->inst_param_box_,
				psy_audio_machine_aux_column_name(machine, index));
		}
	}
}

void machinebar_on_select_inst_param_sel_change(MachineBar* self, psy_ui_ComboBox* sender)
{
	assert(self);

	machinebar_update_combo_ins(self);
}

void machinebar_on_inst_param_box_sel_change(MachineBar* self, psy_ui_ComboBox* sender)
{
	uintptr_t sel;

	assert(self);

	if (!self->machines) {
		return;
	}
	sel = psy_ui_combobox_cur_sel(sender);
	switch (machinebar_mode(self)) {
	case MACHINEBAR_PARAM:
		psy_audio_machines_selectparam(self->machines, sel);
		break;
	case MACHINEBAR_AUX: {
		const psy_audio_Machine* machine;

		psy_audio_machines_selectauxcolumn(self->machines, sel);

		machine = psy_audio_machines_selectedmachine_const(self->machines);
		if (machine && psy_audio_machine_type(machine) == psy_audio_SAMPLER) {
			psy_audio_InstrumentIndex inst;

			if (self->instruments) {
				inst = psy_audio_instruments_selected(self->instruments);
			}
			else {
				inst.group = 0;
			}
			inst.subslot = sel;
			psy_audio_instruments_select(self->instruments, inst);
		}
		break; }	
	default:
		break;
	}	
}

void machinebar_on_song_changed(MachineBar* self, psy_audio_Player* sender)
{	
	assert(self);

	machinebar_set_song(self, psy_audio_player_song(sender));	
}

void machinebar_set_song(MachineBar* self, psy_audio_Song* song)
{
	assert(self);

	if (song) {
		self->machines = &song->machines_;
		self->instruments = &song->instruments_;
	} else {
		self->machines = NULL;
		self->instruments = NULL;
	}
	machinebar_connect_song(self);
	machinebar_update_combo_gen(self);
	machinebar_update_aux(self);
	if (self->machines) {
		psy_ui_combobox_select(&self->machinebox,
			machinebar_combo_box_index(self,
				psy_audio_machines_selected(self->machines)));
	}
	if (self->instruments) {
		psy_ui_combobox_select(&self->inst_param_box_,
			psy_audio_instruments_selected(self->instruments).subslot);
	} else {
		psy_ui_combobox_select(&self->inst_param_box_, -1);
	}
}

void machinebar_connect_song(MachineBar* self)
{
	assert(self);

	if (self->machines) {
		psy_signal_connect(&self->machines->signal_insert, self,
			machinebar_on_machine_insert);		
		psy_signal_connect(&self->machines->signal_removed, self,
			machinebar_on_machine_removed);
		psy_signal_connect(&self->machines->signal_slotchange, self,
			machinebar_on_machine_select);
		psy_signal_connect(&self->instruments->signal_insert, self,
			machinebar_on_instrument_insert);
		psy_signal_connect(&self->instruments->signal_slotchange, self,
			machinebar_on_instrument_slot_changed);
		psy_signal_connect(&self->machines->signal_paramselected, self,
			machinebar_on_param_selected);
		psy_signal_connect(&self->machines->signal_aux_changed, self,
			machinebar_on_aux_changed);
		machinebar_connect_instrument_signals(self);
	}
}

void machinebar_connect_instrument_signals(MachineBar* self)
{
	assert(self);

	if (self->instruments) {
		psy_TableIterator it;

		for (it = psy_audio_instruments_groupbegin(self->instruments, 0);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
			psy_audio_Instrument* instrument;

			instrument = (psy_audio_Instrument*)psy_tableiterator_value(&it);
			psy_signal_connect(&instrument->signal_name_changed, self,
				machinebar_on_instrument_name_changed);
		}
	}
}

void machinebar_on_next_machine(MachineBar* self, psy_ui_Component* sender)
{
	assert(self);

	if (self->machines && psy_audio_machines_selected(self->machines) > 0) {
		psy_audio_machines_select(self->machines,
			psy_audio_machines_selected(self->machines) - 1);
	}
}

void machinebar_on_prev_machine(MachineBar* self, psy_ui_Component* sender)
{
	assert(self);

	if (self->machines) {
		psy_audio_machines_select(self->machines,
			psy_audio_machines_selected(self->machines) + 1);
	}
}

void machinebar_on_instrument_name_changed(MachineBar* self, psy_ui_Component* sender)
{
	assert(self);

	if (self->instruments) {
		machinebar_update_aux(self);
		psy_ui_combobox_select(&self->inst_param_box_,
			psy_audio_instruments_selected(self->instruments).subslot);
	}
}

uintptr_t machinebar_combo_box_index(MachineBar* self, uintptr_t slot)
{	
	assert(self);

	if (psy_table_exists(&self->slotscombobox, slot)) {
		return (uintptr_t)psy_table_at(&self->slotscombobox, slot);
	}
	return 0;
}

void machinebar_on_sample_load(MachineBar* self, psy_ui_Component* sender)
{
	uintptr_t sampler_mac_id;
	uintptr_t xmsampler_mac_id;
	bool has_sampler;

	assert(self);

	if (!self->machines) {
		return;
	}
	sampler_mac_id = psy_audio_machines_first_instance(self->machines, 
		psy_audio_SAMPLER);
	xmsampler_mac_id = psy_audio_machines_first_instance(self->machines, 
		psy_audio_XMSAMPLER);
	has_sampler = (sampler_mac_id != psy_INDEX_INVALID) ||
		(xmsampler_mac_id != psy_INDEX_INVALID);
	if (!has_sampler) {
		psy_audio_Machine* sampler;		
		
		sampler = psy_audio_machinefactory_make_machine(
			&self->workspace->player_.machinefactory,
			psy_audio_SAMPLER, "",
			psy_INDEX_INVALID);
		if (sampler) {
			uintptr_t mac_id;

			psy_audio_machine_set_position(sampler, 16, 16);
			mac_id = psy_audio_machines_append(self->machines, sampler);
			psy_audio_machines_select(self->machines, mac_id);
		}
	} else {
		if (sampler_mac_id != psy_INDEX_INVALID) {
			psy_audio_machines_select(self->machines, sampler_mac_id);
		} else {
			psy_audio_machines_select(self->machines, xmsampler_mac_id);
		}
	}
	if (workspace_song(self->workspace)) {
		psy_audio_SampleIndex smpl;	
			
		smpl = psy_audio_sampleindex_make(
			psy_ui_combobox_cur_sel(&self->inst_param_box_),
			0);	
		workspace_load_sample(self->workspace, smpl);
	}	
}

void machinebar_on_sample_edit(MachineBar* self, psy_ui_Component* sender)
{
	assert(self);

	workspace_select_view(self->workspace, viewindex_make_section(
		VIEW_ID_SAMPLES, SECTION_ID_SAMPLES_EDIT));
}
