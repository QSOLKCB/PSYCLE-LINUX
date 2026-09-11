/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "presetsbar.h"

/* platform */
#include "../../detail/portable.h"


/* prototypes */
static void programbar_fill_bank_combobox(ProgramBar*);
static void programbar_fill_program_combobox(ProgramBar*);
static void programbar_on_sel_change_program(ProgramBar*,
	psy_ui_ComboBox* sender);
static void programbar_on_sel_change_bank(ProgramBar*,
	psy_ui_ComboBox* sender);

/* implementation */
void programbar_init(ProgramBar* self, psy_ui_Component* parent)
{
	assert(self);
	
	psy_ui_component_init(&self->component, parent, NULL);
	self->machine = NULL;
	psy_ui_component_set_align_expand(&self->component, psy_ui_HEXPAND);
	psy_ui_component_set_default_align(&self->component, psy_ui_ALIGN_LEFT,
		psy_ui_margin_make_em(0.0, 1.0, 0.0, 0.0));
	psy_ui_combobox_init(&self->combo_bank, &self->component);
	psy_ui_combobox_set_char_number(&self->combo_bank, 12);
	psy_signal_connect(&self->combo_bank.signal_selchanged, self,
		programbar_on_sel_change_bank);
	psy_ui_combobox_init(&self->combo_program, &self->component);
	psy_ui_combobox_set_char_number(&self->combo_program, 20);		
	psy_signal_connect(&self->combo_program.signal_selchanged, self,
		programbar_on_sel_change_program);		
}

void programbar_set_machine(ProgramBar* self, psy_audio_Machine* machine)
{	
	assert(self);
	
	self->machine = machine;
	programbar_fill_bank_combobox(self);
	programbar_fill_program_combobox(self);
	psy_ui_combobox_select(&self->combo_bank,
		psy_audio_machine_curr_bank(self->machine));
	psy_ui_combobox_select(&self->combo_program,
		psy_audio_machine_curr_program(self->machine));
}

void programbar_on_sel_change_bank(ProgramBar* self, psy_ui_ComboBox* sender)
{
	assert(self);
	
	if (self->machine) {
		psy_audio_machine_set_curr_bank(self->machine, psy_ui_combobox_cur_sel(
			sender));
		programbar_fill_program_combobox(self);
		psy_ui_combobox_select(&self->combo_program,
			psy_audio_machine_curr_program(self->machine));
	}
}

void programbar_on_sel_change_program(ProgramBar* self,
	psy_ui_ComboBox* sender)
{
	assert(self);
	
	if (self->machine) {
		psy_audio_machine_set_curr_program(self->machine,
			psy_ui_combobox_cur_sel(sender));
	}
}

void programbar_fill_bank_combobox(ProgramBar* self)
{
	assert(self);

	psy_ui_combobox_clear(&self->combo_bank);	
	if (self->machine) {
		uintptr_t numb;
		uintptr_t j;
		
		numb = psy_audio_machine_num_banks(self->machine);
		for (j = 0; j < numb; ++j) {
			char s1[38];
			char s2[256];
			
			psy_audio_machine_bank_name(self->machine, j, s2);
			psy_snprintf(s1, 38, "%d: %s", (int)j, s2);
			psy_ui_combobox_add_text(&self->combo_bank, s1);
		}		
	}
}

void programbar_fill_program_combobox(ProgramBar* self)
{
	assert(self);

	psy_ui_combobox_clear(&self->combo_program);	
	if (self->machine) {
		uintptr_t nump;
		uintptr_t i;
		
		nump = psy_audio_machine_num_programs(self->machine);
		for (i = 0; i < nump; ++i) {							
			char s1[38];
			char s2[256];
			
			psy_audio_machine_program_name(self->machine, 0, i, s2);					
			psy_snprintf(s1, 38, "%d: %s", (int)i, s2);
			psy_ui_combobox_add_text(&self->combo_program, s1);
		}		
	}
}
