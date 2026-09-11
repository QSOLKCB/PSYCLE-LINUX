/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "custommachine.h"
/* dsp */
#include <operations.h>
#include <rand.h>
/* platform */
#include "../../detail/os.h"
#include "../../detail/portable.h"


/* prototypes */
static void psy_audio_custommachine_init_memory(psy_audio_CustomMachine*,
	uintptr_t numframes);
static void psy_audio_custommachine_dispose_memory(psy_audio_CustomMachine*);
static const char* psy_audio_custommachine_edit_name(psy_audio_CustomMachine*);
static void psy_audio_custommachine_set_edit_name(psy_audio_CustomMachine*,
	const char* name);
static void psy_audio_custommachine_set_panning(psy_audio_CustomMachine*,
	double);
static double psy_audio_custommachine_panning(const psy_audio_CustomMachine*);
static void psy_audio_custommachine_mute(psy_audio_CustomMachine*);
static void psy_audio_custommachine_unmute(psy_audio_CustomMachine*);
static int psy_audio_custommachine_muted(psy_audio_CustomMachine*);
static void psy_audio_custommachine_bypass(psy_audio_CustomMachine*);
static void psy_audio_custommachine_unbypass(psy_audio_CustomMachine*);
static int psy_audio_custommachine_bypassed(psy_audio_CustomMachine*);
static void psy_audio_custommachine_standby(psy_audio_CustomMachine*);
static void psy_audio_custommachine_deactivate_standby(psy_audio_CustomMachine*);
static int psy_audio_custommachine_has_standby(psy_audio_CustomMachine*);
static void psy_audio_custommachine_set_bus(psy_audio_CustomMachine*);
static void psy_audio_custommachine_unset_bus(psy_audio_CustomMachine*);
static int psy_audio_custommachine_is_bus(psy_audio_CustomMachine*);
static psy_audio_Buffer* psy_audio_custommachine_memory(
	psy_audio_CustomMachine*);
static uintptr_t psy_audio_custommachine_memory_size(psy_audio_CustomMachine*);
static void psy_audio_custommachine_set_memory_size(psy_audio_CustomMachine*,
	uintptr_t size);
static uintptr_t psy_audio_custommachine_slot(psy_audio_CustomMachine*);
static void psy_audio_custommachine_set_slot(psy_audio_CustomMachine*,
	uintptr_t slot_);
void psy_audio_custommachine_set_position(psy_audio_CustomMachine*,
	double x_, double y_);
static void psy_audio_custommachine_position(psy_audio_CustomMachine*,
	double* x_, double* y_);
/* parameters */
static void psy_audio_custommachine_select_param(psy_audio_CustomMachine*,
	uintptr_t index);
static uintptr_t psy_audio_custommachine_selected_param(
	psy_audio_CustomMachine*);
/* auxcolumn */
static const char* psy_audio_custommachine_aux_col_name(
	psy_audio_CustomMachine*, uintptr_t index);
static uintptr_t psy_audio_custommachine_num_aux_cols(psy_audio_CustomMachine*);
static uintptr_t psy_audio_custommachine_selected_aux_col(
	psy_audio_CustomMachine*);
static void psy_audio_custommachine_select_aux_col(psy_audio_CustomMachine*,
	uintptr_t index);
static psy_audio_ParamTranslator* psy_audio_custommachine_param_translator(
	psy_audio_CustomMachine*);
static psy_dsp_amp_range_t psy_audio_custommachine_amp_range(
	psy_audio_CustomMachine*);

/* vtable */
static MachineVtable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_audio_CustomMachine* self)
{
	assert(self);
	
	if (!vtable_initialized) {
		vtable = *self->machine.vtable;
		vtable.setpanning =
			(fp_machine_set_panning)
			psy_audio_custommachine_set_panning;
		vtable.panning =
			(fp_machine_panning)
			psy_audio_custommachine_panning;
		vtable.mute =
			(psy_audio_fp_machine)
			psy_audio_custommachine_mute;
		vtable.unmute =
			(psy_audio_fp_machine)
			psy_audio_custommachine_unmute;
		vtable.muted =
			(fp_machine_muted)
			psy_audio_custommachine_muted;
		vtable.bypass =
			(psy_audio_fp_machine)
			psy_audio_custommachine_bypass;
		vtable.unbypass =
			(psy_audio_fp_machine)
			psy_audio_custommachine_unbypass;
		vtable.bypassed =
			(fp_machine_bypassed)
			psy_audio_custommachine_bypassed;
		vtable.standby =
			(psy_audio_fp_machine)
			psy_audio_custommachine_standby;
		vtable.deactivatestandby =
			(psy_audio_fp_machine)
			psy_audio_custommachine_deactivate_standby;
		vtable.hasstandby =
			(fp_machine_has_standby)
			psy_audio_custommachine_has_standby;
		vtable.setbus =
			(psy_audio_fp_machine)
			psy_audio_custommachine_set_bus;
		vtable.unsetbus =
			(psy_audio_fp_machine)
			psy_audio_custommachine_unset_bus;
		vtable.isbus =
			(fp_machine_isbus)
			psy_audio_custommachine_is_bus;
		vtable.editname =
			(fp_machine_editname)
			psy_audio_custommachine_edit_name;
		vtable.seteditname =
			(fp_machine_set_editname)
			psy_audio_custommachine_set_edit_name;
		vtable.buffermemory =
			(fp_machine_buffer_memory)
			psy_audio_custommachine_memory;
		vtable.buffermemorysize =
			(fp_machine_buffer_memory_size)
			psy_audio_custommachine_memory_size;
		vtable.setbuffermemorysize =
			(fp_machine_set_buffer_memory_size)
			psy_audio_custommachine_set_memory_size;
		vtable.setslot =
			(fp_machine_set_slot)
			psy_audio_custommachine_set_slot;
		vtable.slot =
			(fp_machine_slot)
			psy_audio_custommachine_slot;
		vtable.setposition =
			(fp_machine_set_position)
			psy_audio_custommachine_set_position;
		vtable.position =
			(fp_machine_position)
			psy_audio_custommachine_position;
		vtable.numauxcolumns =
			(fp_machine_num_aux_columns)
			psy_audio_custommachine_num_aux_cols;
		vtable.selectauxcolumn =	
			(fp_machine_select_aux_column)
			psy_audio_custommachine_select_aux_col;
		vtable.auxcolumnselected =
			(fp_machine_aux_column_selected)
			psy_audio_custommachine_selected_aux_col;
		vtable.paramselected =
			(fp_machine_param_selected)
			psy_audio_custommachine_selected_param;
		vtable.selectparam =
			(fp_machine_select_param)
			psy_audio_custommachine_select_param;
		vtable.instparamtranslator =
			(fp_machine_inst_param_translator)
			psy_audio_custommachine_param_translator;
		vtable.amprange =
			(fp_machine_amp_range)
			psy_audio_custommachine_amp_range;
		vtable_initialized = TRUE;
	}
	self->machine.vtable = &vtable;
}

/* implementation */
void psy_audio_custommachine_init(psy_audio_CustomMachine* self,
	psy_audio_MachineCallback* callback)
{	
	psy_audio_machine_init(&self->machine, callback);
	vtable_init(self);	
	self->edit_name_ = NULL;
	self->muted_ = 0;
	self->bypassed_ = 0;
	self->is_bus_ = 0;
	self->pan = 0.5;
	self->slot_ = psy_INDEX_INVALID;
	self->x_ = 0;
	self->y_ = 0;
	self->selected_aux_col_ = 0;
	self->selected_param_ = 0;
	self->amp_range_ = PSY_DSP_AMP_RANGE_NATIVE;
	psy_audio_custommachine_init_memory(self, psy_audio_MAX_STREAM_SIZE);
	psy_audio_paramtranslator_init(&self->param_translator_);
}

void psy_audio_custommachine_init_memory(psy_audio_CustomMachine* self,
	uintptr_t numframes)
{
	psy_audio_buffer_init(&self->memorybuffer, 2);
	self->memorybuffersize = numframes;
	psy_audio_buffer_allocsamples(&self->memorybuffer, self->memorybuffersize);
	psy_audio_buffer_clearsamples(&self->memorybuffer, self->memorybuffersize);
	psy_audio_buffer_enablerms(&self->memorybuffer);
}

void psy_audio_custommachine_dispose(psy_audio_CustomMachine* self)
{	
	free(self->edit_name_);
	self->edit_name_ = NULL;
	psy_audio_custommachine_dispose_memory(self);
	machine_base_dispose(&self->machine);
	psy_audio_paramtranslator_dispose(&self->param_translator_);
}

void psy_audio_custommachine_dispose_memory(psy_audio_CustomMachine* self)
{	
	psy_audio_buffer_dispose(&self->memorybuffer);
}

void psy_audio_custommachine_set_amp_range(psy_audio_CustomMachine* self,
	psy_dsp_amp_range_t amp_range_)
{
	self->amp_range_ = amp_range_;
}

void psy_audio_custommachine_set_panning(psy_audio_CustomMachine* self,
	double val)
{
	self->pan = val < 0.0 ? 0.0 : val > 1.0 ? 1.0 : val;
}

double psy_audio_custommachine_panning(const psy_audio_CustomMachine* self)
{
	return self->pan;
}

const char* psy_audio_custommachine_edit_name(
	psy_audio_CustomMachine* self)
{
	return self->edit_name_;
}

void psy_audio_custommachine_set_edit_name(psy_audio_CustomMachine* self,
	const char* name)
{
	psy_strreset(&self->edit_name_, name);
}

void psy_audio_custommachine_mute(psy_audio_CustomMachine* self)
{
	self->muted_ = 1;
}

void psy_audio_custommachine_unmute(psy_audio_CustomMachine* self)
{
	self->muted_ = 0; 
}

int psy_audio_custommachine_muted(psy_audio_CustomMachine* self)
{
		return self->muted_;
}

void psy_audio_custommachine_bypass(psy_audio_CustomMachine* self)
{
	self->bypassed_ = 1;
}

void psy_audio_custommachine_unbypass(psy_audio_CustomMachine* self)
{
	self->bypassed_ = 0;
}

int psy_audio_custommachine_bypassed(psy_audio_CustomMachine* self)
{
	return self->bypassed_;
}

void psy_audio_custommachine_standby(psy_audio_CustomMachine* self)
{
	if (psy_audio_machine_mode(&self->machine) == psy_audio_MACHMODE_GENERATOR)
	{
		psy_audio_machine_mute(&self->machine);
	} else {
		psy_audio_machine_bypass(&self->machine);
	}	
}

void psy_audio_custommachine_deactivate_standby(psy_audio_CustomMachine* self) {
	if (psy_audio_machine_mode(&self->machine) == psy_audio_MACHMODE_GENERATOR)
	{
		psy_audio_machine_unmute(&self->machine);
	} else {
		psy_audio_machine_unbypass(&self->machine);
	}
}

int psy_audio_custommachine_has_standby(psy_audio_CustomMachine* self)
{
	if (psy_audio_machine_mode(&self->machine) == psy_audio_MACHMODE_GENERATOR) {
		return psy_audio_machine_muted(&self->machine);
	} else {
		return psy_audio_machine_bypassed(&self->machine);
	}
}

void psy_audio_custommachine_set_bus(psy_audio_CustomMachine* self)
{
	self->is_bus_ = 1;
	psy_audio_machine_bus_changed(&self->machine);
}

void psy_audio_custommachine_unset_bus(psy_audio_CustomMachine* self)
{
	self->is_bus_ = 0;
	psy_audio_machine_bus_changed(&self->machine);
}

int psy_audio_custommachine_is_bus(psy_audio_CustomMachine* self)
{
	return self->is_bus_;
}

psy_audio_Buffer* psy_audio_custommachine_memory(
	psy_audio_CustomMachine* self)
{
	return &self->memorybuffer;	
}

uintptr_t psy_audio_custommachine_memory_size(
	psy_audio_CustomMachine* self)
{
	return self->memorybuffersize;	
}

void psy_audio_custommachine_set_memory_size(psy_audio_CustomMachine* self,
	uintptr_t size)
{
	psy_audio_custommachine_dispose_memory(self);
	psy_audio_custommachine_init_memory(self, size);
}

uintptr_t psy_audio_custommachine_slot(psy_audio_CustomMachine* self)
{
	return self->slot_;
}

void psy_audio_custommachine_set_slot(psy_audio_CustomMachine* self,
	uintptr_t slot_)
{
	self->slot_ = slot_;
}

void psy_audio_custommachine_set_position(psy_audio_CustomMachine* self,
	double x_, double y_)
{
	assert(self);
	
	if (x_ == psy_audio_MACH_AUTO_POSITION) {
		self->x_ = psy_rand16() / 64.0;
	} else {
		self->x_ = x_;
	}
	if (y_ == psy_audio_MACH_AUTO_POSITION) {		
		self->y_ = psy_rand16() / 80.0;
	} else {
		self->y_ = y_;
	}
}

void psy_audio_custommachine_position(psy_audio_CustomMachine* self,
	double* x_, double* y_)
{
	*x_ = self->x_;
	*y_ = self->y_;
}

/* parameters */
void psy_audio_custommachine_select_param(psy_audio_CustomMachine* self,
	uintptr_t index)
{
	self->selected_param_ = index;
}

uintptr_t psy_audio_custommachine_selected_param(psy_audio_CustomMachine* self)
{
	return self->selected_param_;
}

/* auxcolumn */
const char* psy_audio_custommachine_aux_col_name(psy_audio_CustomMachine* self,
	uintptr_t index)
{
	return "";
}

uintptr_t psy_audio_custommachine_num_aux_cols(psy_audio_CustomMachine* self)
{ 
	return 0;
}

uintptr_t psy_audio_custommachine_selected_aux_col(
	psy_audio_CustomMachine* self)
{
	return self->selected_aux_col_;
}

void psy_audio_custommachine_select_aux_col(psy_audio_CustomMachine* self,
	uintptr_t index)
{
	self->selected_aux_col_ = index;
}

psy_audio_ParamTranslator* psy_audio_custommachine_param_translator(
	psy_audio_CustomMachine* self)
{
	return &self->param_translator_;
}

psy_dsp_amp_range_t psy_audio_custommachine_amp_range(
	psy_audio_CustomMachine* self)
{
	return self->amp_range_;	
}
