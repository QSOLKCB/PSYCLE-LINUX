/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "audiorecorder.h"
/* local */
#include "plugin_interface.h"
#include "songio.h"
/* dsp */
#include <convert.h>
#include <operations.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


const psy_audio_MachineInfo* psy_audio_audiorecorder_info(void)
{
	static psy_audio_MachineInfo const macinfo = {
		MI_VERSION,
		0x0250,
		0,
		psy_audio_MACHMODE_GENERATOR,
		"AudioRecorder"
			#ifndef NDEBUG
			" (debug build)"
			#endif
			,
		"AudioRecorder",
		"Psycledelics",
		"help",
		psy_audio_RECORDER,
		0,
		0,
		"",
		"",
		"record",
		psy_INDEX_INVALID,
		""
	};
	return &macinfo;
}

static const psy_audio_MachineInfo* info(psy_audio_AudioRecorder* self) {
	return psy_audio_audiorecorder_info();
}
static void dispose(psy_audio_AudioRecorder*);
static intptr_t mode(psy_audio_AudioRecorder* self) { return psy_audio_MACHMODE_GENERATOR; }
static void work(psy_audio_AudioRecorder*, psy_audio_BufferContext*);
static uintptr_t numinputs(psy_audio_AudioRecorder* self) { return 0; }
static uintptr_t numoutputs(psy_audio_AudioRecorder* self) { return 2; }
static psy_dsp_amp_range_t amprange(psy_audio_AudioRecorder* self)
{
	return PSY_DSP_AMP_RANGE_NATIVE;
}
static int loadspecific(psy_audio_AudioRecorder*, psy_audio_SongFile*,
	uintptr_t slot);
static int savespecific(psy_audio_AudioRecorder*, psy_audio_SongFile*,
	uintptr_t slot);
static void psy_audio_audiorecorder_change_port(psy_audio_AudioRecorder*, intptr_t newport);
// parameters
static unsigned int numparametercols(psy_audio_AudioRecorder*);
static uintptr_t numparameters(psy_audio_AudioRecorder*);
static void init_parameters(psy_audio_AudioRecorder*);
static psy_audio_MachineParam* parameter(psy_audio_AudioRecorder*, uintptr_t id);
static void psy_audio_audiorecorder_device_tweak(psy_audio_AudioRecorder*,
	psy_audio_ChoiceMachineParam* sender, double value);

static MachineVtable vtable;
static int vtable_initialized = 0;

static void vtable_init(psy_audio_AudioRecorder* self)
{
	if (!vtable_initialized) {
		vtable = *self->custommachine.machine.vtable;
		vtable.dispose =
			(psy_audio_fp_machine)
			dispose;
		vtable.mode =
			(fp_machine_mode)
			mode;
		vtable.work =
			(fp_machine_work)
			work;
		vtable.info =
			(fp_machine_info)
			info;
		vtable.numinputs =
			(fp_machine_numinputs)
			numinputs;
		vtable.numoutputs =
			(fp_machine_numoutputs)
			numoutputs;
		vtable.amprange =
			(fp_machine_amp_range)
			amprange;
		vtable.loadspecific =
			(fp_machine_loadspecific)
			loadspecific;
		vtable.savespecific =
			(fp_machine_savespecific)
			savespecific;
		vtable.numparametercols =
			(fp_machine_numparametercols)
			numparametercols;
		vtable.numparameters =
			(fp_machine_numparameters)
			numparameters;
		vtable.parameter =
			(fp_machine_parameter)
			parameter;
		vtable_initialized = 1;
	}
	self->custommachine.machine.vtable = &vtable;
}

void psy_audio_audiorecorder_init(psy_audio_AudioRecorder* self,
	psy_audio_MachineCallback* callback)
{
	assert(self);
	
	psy_audio_custommachine_init(&self->custommachine, callback);
	vtable_init(self);
	psy_audio_machine_set_edit_name(psy_audio_audiorecorder_base(self),
		"Recorder");
	self->_captureidx = -1;
	self->_gainvol = 1.0;
	init_parameters(self);
	if (psy_audio_machine_num_captures(
			psy_audio_audiorecorder_base(self)) > 0) {
		psy_audio_audiorecorder_change_port(self, 0);
	}	
}

void dispose(psy_audio_AudioRecorder* self)
{
	assert(self);

	psy_audio_choicemachineparam_dispose(&self->device);
	psy_audio_doublemachineparam_dispose(&self->gain);
	psy_audio_custommachine_dispose(&self->custommachine);
}

void init_parameters(psy_audio_AudioRecorder* self)
{
	assert(self);

	int i;
	int num;

	num = (int)psy_audio_machine_num_captures(psy_audio_audiorecorder_base(self));
	psy_audio_choicemachineparam_init(&self->device,
		"Capture Device", "Capture Device", MPF_STATE,
		(int32_t*)&self->_captureidx,
		0, num > 0 ? num - 1 : 0);
	if (num == 0) {
		psy_audio_choicemachineparam_set_description(&self->device, 0, "None");
	}
	for (i = 0; i < num; ++i) {
		psy_audio_choicemachineparam_set_description(&self->device, i,
			psy_audio_machine_capture_name(psy_audio_audiorecorder_base(self),
				i));
	}
	if (num > 0) {
		self->_captureidx = 0;
	}
	psy_signal_connect(&self->device.machineparam.signal_tweak, self,
		psy_audio_audiorecorder_device_tweak);
	psy_audio_doublemachineparam_init(&self->gain, "gain", "Gain Control",
		MPF_STATE | MPF_SMALL, &self->_gainvol, 0, 0x1000);
}

int loadspecific(psy_audio_AudioRecorder* self, psy_audio_SongFile* songfile,
	uintptr_t slot)
{
	uint32_t size;
	int32_t readcaptureidx;
	float gainvol;
	int status;

	assert(self);

	/* size of this part params to load */
	if ((status = psyfile_read(songfile->file, &size, sizeof(size)))) {
		return status;
	}
	if ((status = psyfile_read(songfile->file, &readcaptureidx,
			sizeof(readcaptureidx)))) {
		return status;
	}
	if ((status = psyfile_read(songfile->file, &gainvol, sizeof(gainvol)))) {
		return status;
	}
	self->_gainvol = gainvol;
	self->_captureidx = readcaptureidx;
	/* ChangePort(readcaptureidx); */
	return PSY_OK;
}

int savespecific(psy_audio_AudioRecorder* self, psy_audio_SongFile* songfile,
	uintptr_t slot)
{
	uint32_t size;
	int status;
	
	size = sizeof(self->_captureidx) + sizeof(self->_gainvol);
	/* size of this part params to save */
	if ((status = psyfile_write_uint32(songfile->file, size))) {
		return status;
	}
	if ((status = psyfile_write_int32(songfile->file, (int32_t)self->_captureidx))) {
		return status;
	}
	if ((status = psyfile_write_float(songfile->file, (float)self->_gainvol))) {
		return status;
	}
	return PSY_OK;
}

unsigned int numparametercols(psy_audio_AudioRecorder* self)
{
	return 1;
}

uintptr_t numparameters(psy_audio_AudioRecorder* self)
{	
	return 2;
}

psy_audio_MachineParam* parameter(psy_audio_AudioRecorder* self, uintptr_t id)
{
	if (id == 0) {
		return psy_audio_choicemachineparam_base(&self->device);
	}
	if (id == 1) {
		return psy_audio_doublemachineparam_base(&self->gain);
	}
	return NULL;
}

void work(psy_audio_AudioRecorder* self, psy_audio_BufferContext* bc)
{
	if (!psy_audio_machine_bypassed(psy_audio_audiorecorder_base(self)) &&
		 !psy_audio_machine_muted(psy_audio_audiorecorder_base(self))) {
	
		float* left = 0;
		float* right = 0;		

		psy_audio_machine_read_buffers(
			psy_audio_audiorecorder_base(self),
			self->_captureidx,
			&left, &right,
			psy_audio_buffercontext_num_samples(bc));
		if (left == NULL) {
			dsp.clear(bc->output_->samples[0], psy_audio_buffercontext_num_samples(bc));
		} else {
			dsp.movmul(left, bc->output_->samples[0], psy_audio_buffercontext_num_samples(bc), (float)self->_gainvol);
		}
		if (right == NULL) {
			dsp.clear(bc->output_->samples[1], psy_audio_buffercontext_num_samples(bc));
		} else {
			dsp.movmul(right, bc->output_->samples[1], psy_audio_buffercontext_num_samples(bc), (float)self->_gainvol);
		}		
	}
}

void psy_audio_audiorecorder_device_tweak(psy_audio_AudioRecorder* self,
	psy_audio_ChoiceMachineParam* sender, double value)
{
	assert(self);

	psy_audio_audiorecorder_change_port(self, psy_audio_choicemachineparam_choice(sender));
}

void psy_audio_audiorecorder_change_port(psy_audio_AudioRecorder* self, intptr_t newport)
{
//	if (self->_captureidx != newport) {
		psy_audio_machine_remove_capture(psy_audio_audiorecorder_base(self),
			self->_captureidx);
		psy_audio_machine_add_capture(psy_audio_audiorecorder_base(self),
			newport);
		self->_captureidx = newport;
//	}
}
