/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"

#include "machineparam.h"
/* local */
#include "machines.h"
#include "plugin_interface.h"
/* dsp */
#include <convert.h>
#include <valuemapper.h>
/* std */
#include <math.h>
/* platform */
#include "../../detail/portable.h"


/* MachineParameter */
static void machineparam_tweak(psy_audio_MachineParam* self, double value)
{
	psy_signal_emit_double(&self->signal_tweak, self, 1, value);
}

static void machineparam_reset(psy_audio_MachineParam* self)
{
}

static double machineparam_norm_value(const psy_audio_MachineParam* self)
{
	double rv = 0.0;

	psy_signal_emit(&((psy_audio_MachineParam*)self)->signal_normvalue,
		(psy_audio_MachineParam*)self, 1, (void*)(&rv));
	return rv;
}

static void machineparam_range(psy_audio_MachineParam* self, intptr_t* minval,
	intptr_t* maxval)
{
	*minval = 0;
	*maxval = 0;
}

static int machineparam_type(psy_audio_MachineParam* self)
{
	return MPF_STATE;
}

static int machineparam_label(psy_audio_MachineParam* self, char* text)
{
	text[0] = '\0';
	return 0;
}

static int machineparam_name(psy_audio_MachineParam* self, char* text)
{
	text[0] = '\0';
	return 0;
}

static int machineparam_describe(psy_audio_MachineParam* self, char* text)
{
	text[0] = '\0';
	return 0;
}

static MachineParamVtable machineparam_vtable;
static int machineparam_vtable_initialized = 0;

static void machineparam_vtable_init(psy_audio_MachineParam* self)
{
	if (!machineparam_vtable_initialized) {
		machineparam_vtable.tweak = machineparam_tweak;
		machineparam_vtable.normvalue = machineparam_norm_value;
		machineparam_vtable.range = machineparam_range;
		machineparam_vtable.name = machineparam_name;
		machineparam_vtable.label = machineparam_label;
		machineparam_vtable.type = machineparam_type;
		machineparam_vtable.describe = machineparam_describe;
		machineparam_vtable_initialized = 1;
	}
}

void psy_audio_machineparam_init(psy_audio_MachineParam* self)
{
	machineparam_vtable_init(self);
	self->vtable = &machineparam_vtable;
	psy_signal_init(&self->signal_normvalue);
	psy_signal_init(&self->signal_tweak);
	psy_signal_init(&self->signal_describe);
	psy_signal_init(&self->signal_name);
	psy_signal_init(&self->signal_label);
	self->isslidergroup = FALSE;
	self->param0 = psy_INDEX_INVALID;
	self->param1 = psy_INDEX_INVALID;
}

void psy_audio_machineparam_dispose(psy_audio_MachineParam* self)
{
	psy_signal_dispose(&self->signal_normvalue);
	psy_signal_dispose(&self->signal_tweak);
	psy_signal_dispose(&self->signal_describe);
	psy_signal_dispose(&self->signal_name);
	psy_signal_dispose(&self->signal_label);
}


/* CustomParameter */

/* prototypes */
static void customparam_tweak(psy_audio_CustomMachineParam*, double val);
static double customparam_norm_value(const psy_audio_CustomMachineParam*);
static void customparam_range(psy_audio_CustomMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int customparam_type(psy_audio_CustomMachineParam*);
static int customparam_label(psy_audio_CustomMachineParam*, char* text);
static int customparam_name(psy_audio_CustomMachineParam*, char* text);
static int customparam_describe(psy_audio_CustomMachineParam*, char* text);

/* vtable */
static MachineParamVtable customparam_vtable;
static int customparam_vtable_initialized = 0;

static void customparam_vtable_init(psy_audio_CustomMachineParam* self)
{
	if (!customparam_vtable_initialized) {
		customparam_vtable = *(self->machineparam.vtable);
		customparam_vtable.tweak =
			(fp_machineparam_tweak)
			customparam_tweak;
		customparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			customparam_norm_value;
		customparam_vtable.range =
			(fp_machineparam_range)
			customparam_range;
		customparam_vtable.name =
			(fp_machineparam_name)
			customparam_name;
		customparam_vtable.label =
			(fp_machineparam_label)
			customparam_label;
		customparam_vtable.type =
			(fp_machineparam_type)
			customparam_type;
		customparam_vtable.describe =
			(fp_machineparam_describe)
			customparam_describe;
		customparam_vtable_initialized = 1;
	}
	self->machineparam.vtable = &customparam_vtable;
}

/* implementation */
void psy_audio_custommachineparam_init(psy_audio_CustomMachineParam* self, 
	const char* name, const char* label, int type,
	int minval, int maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	customparam_vtable_init(self);	
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->row = 0;
}

void psy_audio_custommachineparam_dispose(psy_audio_CustomMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
}

psy_audio_CustomMachineParam* psy_audio_custommachineparam_alloc(void)
{
	return (psy_audio_CustomMachineParam*)malloc(sizeof(psy_audio_CustomMachineParam));
}

psy_audio_CustomMachineParam* psy_audio_custommachineparam_alloc_init(
	const char* name, const char* label, int type,
	int minval, int maxval)
{
	psy_audio_CustomMachineParam* rv;

	rv = psy_audio_custommachineparam_alloc();
	if (rv) {
		psy_audio_custommachineparam_init(rv, name, label, type, minval, maxval);
	}
	return rv;
}

void customparam_tweak(psy_audio_CustomMachineParam* self, double value)
{
	psy_signal_emit_double(&self->machineparam.signal_tweak, self, 1, value);
}

int customparam_describe(psy_audio_CustomMachineParam* self, char* text)
{
	int rv = 0;

	if (self->machineparam.signal_describe.slots != NULL) {
		psy_signal_emit(&self->machineparam.signal_describe, self, 2,
			(void*)(&rv), (void*)(text));
	}
	return rv;
}

double customparam_norm_value(const psy_audio_CustomMachineParam* self)
{
	double rv = 0.0;

	psy_signal_emit(
		&((psy_audio_CustomMachineParam*)self)->machineparam.signal_normvalue,
		(psy_audio_CustomMachineParam*)self, 1, (void*)(&rv));
	return rv;
}

void customparam_range(psy_audio_CustomMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}
int customparam_type(psy_audio_CustomMachineParam* self)
{ 
	return self->type;
}

int customparam_label(psy_audio_CustomMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int customparam_name(psy_audio_CustomMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

// InfoMachineParam
static int infomachineparam_type(psy_audio_InfoMachineParam*);
static int infomachineparam_label(psy_audio_InfoMachineParam*, char* text);
static int infomachineparam_name(psy_audio_InfoMachineParam*, char* text);
static int infomachineparam_describe(psy_audio_InfoMachineParam*, char* text);

static MachineParamVtable infomachineparam_vtable;
static int infomachineparam_vtable_initialized = 0;

static void infomachineparam_vtable_init(psy_audio_InfoMachineParam* self)
{
	if (!infomachineparam_vtable_initialized) {
		infomachineparam_vtable = *(self->machineparam.vtable);
		infomachineparam_vtable.name = (fp_machineparam_name)infomachineparam_name;
		infomachineparam_vtable.label = (fp_machineparam_label)infomachineparam_label;
		infomachineparam_vtable.type = (fp_machineparam_type)infomachineparam_type;
		infomachineparam_vtable.describe = (fp_machineparam_describe)infomachineparam_describe;
		infomachineparam_vtable_initialized = 1;
	}
}

void psy_audio_infomachineparam_init(psy_audio_InfoMachineParam* self,
	const char* name, const char* label, int style)
{
	psy_audio_machineparam_init(&self->machineparam);
	infomachineparam_vtable_init(self);
	self->machineparam.vtable = &infomachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->style = style;
}

void psy_audio_infomachineparam_dispose(psy_audio_InfoMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
}

psy_audio_InfoMachineParam* psy_audio_infomachineparam_alloc(void)
{
	return (psy_audio_InfoMachineParam*)malloc(sizeof(psy_audio_InfoMachineParam));
}

psy_audio_InfoMachineParam* psy_audio_infomachineparam_allocinit(
	const char* name, const char* label, int style)
{
	psy_audio_InfoMachineParam* rv;

	rv = psy_audio_infomachineparam_alloc();
	if (rv) {
		psy_audio_infomachineparam_init(rv, name, label, style);
	}
	return rv;
}

int infomachineparam_describe(psy_audio_InfoMachineParam* self, char* text)
{	
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		// psy_signal_emit(&self->machineparam.signal_describe, self, 1, text);
		return 1;
	}
	return 0;
}

int infomachineparam_label(psy_audio_InfoMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int infomachineparam_name(psy_audio_InfoMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int infomachineparam_type(psy_audio_InfoMachineParam* self)
{
	return MPF_INFOLABEL | self->style;
}

// IntMachineParam
static void intmachineparam_tweak(psy_audio_IntMachineParam*, double val);
static double intmachineparam_norm_value(const psy_audio_IntMachineParam*);
static void intmachineparam_range(psy_audio_IntMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int intmachineparam_type(psy_audio_IntMachineParam*);
static int intmachineparam_label(psy_audio_IntMachineParam*, char* text);
static int intmachineparam_name(psy_audio_IntMachineParam*, char* text);
static int intmachineparam_describe(psy_audio_IntMachineParam*, char* text);

static MachineParamVtable intmachineparam_vtable;
static int intmachineparam_vtable_initialized = 0;

static void intmachineparam_vtable_init(psy_audio_IntMachineParam* self)
{
	if (!intmachineparam_vtable_initialized) {
		intmachineparam_vtable = *(self->machineparam.vtable);
		intmachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			intmachineparam_tweak;
		intmachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			intmachineparam_norm_value;
		intmachineparam_vtable.range =
			(fp_machineparam_range)
			intmachineparam_range;
		intmachineparam_vtable.name =
			(fp_machineparam_name)
			intmachineparam_name;
		intmachineparam_vtable.label =
			(fp_machineparam_label)
			intmachineparam_label;
		intmachineparam_vtable.type =
			(fp_machineparam_type)
			intmachineparam_type;
		intmachineparam_vtable.describe =
			(fp_machineparam_describe)
			intmachineparam_describe;
		intmachineparam_vtable_initialized = 1;
	}
}

void psy_audio_intmachineparam_init(psy_audio_IntMachineParam* self,
	const char* name, const char* label, int type, int32_t* data, intptr_t minval,
	intptr_t maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	intmachineparam_vtable_init(self);
	self->machineparam.vtable = &intmachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->mask = 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->data = data;
}

void psy_audio_intmachineparam_dispose(psy_audio_IntMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	free(self->mask);
}

psy_audio_IntMachineParam* psy_audio_intmachineparam_alloc(void)
{
	return (psy_audio_IntMachineParam*)malloc(sizeof(psy_audio_IntMachineParam));
}

psy_audio_IntMachineParam* psy_audio_intmachineparam_allocinit(
	const char* name, const char* label, int type, int32_t* data,
	intptr_t minval, intptr_t maxval)
{
	psy_audio_IntMachineParam* rv;

	rv = psy_audio_intmachineparam_alloc();
	if (rv) {
		psy_audio_intmachineparam_init(rv, name, label, type, data, minval, maxval);
	}
	return rv;
}

void intmachineparam_range(psy_audio_IntMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void intmachineparam_tweak(psy_audio_IntMachineParam* self, double value)
{
	if (self->data) {
		intptr_t scaled;

		scaled = (intptr_t)(value * (self->maxval - self->minval) + 0.5f) +
			self->minval;
		*self->data = (int32_t)scaled;
		psy_signal_emit_double(&self->machineparam.signal_tweak, self, 1, value);
	} else {
		psy_signal_emit_double(&self->machineparam.signal_tweak, self, 1, value);
	}
}

double intmachineparam_norm_value(const psy_audio_IntMachineParam* self)
{
	double rv = 0.0;

	if (self->data) {
		rv = ((self->maxval - self->minval) != 0)
			? (*self->data - self->minval) /
			(double)(self->maxval - self->minval)
			: 0.0;
	} else {
		psy_signal_emit(
			&((psy_audio_IntMachineParam*)self)->machineparam.signal_normvalue,
			(psy_audio_IntMachineParam*)self, 1, (void*)(&rv));
	}
	return rv;
}

int intmachineparam_describe(psy_audio_IntMachineParam* self, char* text)
{
	int rv = 1;

	psy_snprintf(text, 128, (self->mask) ? self->mask : "%d", 
		(int) psy_audio_machineparam_scaled_value(
			psy_audio_intmachineparam_base(self)));
	return rv;
}

int intmachineparam_label(psy_audio_IntMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int intmachineparam_name(psy_audio_IntMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int intmachineparam_type(psy_audio_IntMachineParam* self)
{
	return self->type;
}

void psy_audio_intmachineparam_setmask(psy_audio_IntMachineParam* self, const char* mask)
{
	free(self->mask);
	self->mask = (mask) ? strdup(mask) : 0; 
}


// UIntPtrMachineParam
static void uintptrmachineparam_tweak(psy_audio_UIntPtrMachineParam*, double val);
static double uintptrmachineparam_norm_value(const psy_audio_UIntPtrMachineParam*);
static void uintptrmachineparam_range(psy_audio_UIntPtrMachineParam*,
	uintptr_t* minval, uintptr_t* maxval);
static int uintptrmachineparam_type(psy_audio_UIntPtrMachineParam*);
static int uintptrmachineparam_label(psy_audio_UIntPtrMachineParam*, char* text);
static int uintptrmachineparam_name(psy_audio_UIntPtrMachineParam*, char* text);
static int uintptrmachineparam_describe(psy_audio_UIntPtrMachineParam*, char* text);

static MachineParamVtable uintptrmachineparam_vtable;
static bool uintptrmachineparam_vtable_initialized = FALSE;

static void uintptrmachineparam_vtable_init(psy_audio_UIntPtrMachineParam* self)
{
	if (!uintptrmachineparam_vtable_initialized) {
		uintptrmachineparam_vtable = *(self->machineparam.vtable);
		uintptrmachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			uintptrmachineparam_tweak;
		uintptrmachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			uintptrmachineparam_norm_value;
		uintptrmachineparam_vtable.range =
			(fp_machineparam_range)
			uintptrmachineparam_range;
		uintptrmachineparam_vtable.name =
			(fp_machineparam_name)
			uintptrmachineparam_name;
		uintptrmachineparam_vtable.label =
			(fp_machineparam_label)
			uintptrmachineparam_label;
		uintptrmachineparam_vtable.type =
			(fp_machineparam_type)uintptrmachineparam_type;
		uintptrmachineparam_vtable.describe =
			(fp_machineparam_describe)
			uintptrmachineparam_describe;
		uintptrmachineparam_vtable_initialized = TRUE;
	}
}

void psy_audio_uintptrmachineparam_init(psy_audio_UIntPtrMachineParam* self,
	const char* name, const char* label, int type, uintptr_t* data,
	uintptr_t minval, uintptr_t maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	uintptrmachineparam_vtable_init(self);
	self->machineparam.vtable = &uintptrmachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->mask = 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->data = data;
}

void psy_audio_uintptrmachineparam_dispose(psy_audio_UIntPtrMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	free(self->mask);
}

psy_audio_UIntPtrMachineParam* psy_audio_uintptrmachineparam_alloc(void)
{
	return (psy_audio_UIntPtrMachineParam*)malloc(sizeof(psy_audio_UIntPtrMachineParam));
}

psy_audio_UIntPtrMachineParam* psy_audio_uintptrmachineparam_allocinit(
	const char* name, const char* label, int type, uintptr_t* data,
	uintptr_t minval, uintptr_t maxval)
{
	psy_audio_UIntPtrMachineParam* rv;

	rv = psy_audio_uintptrmachineparam_alloc();
	if (rv) {
		psy_audio_uintptrmachineparam_init(rv, name, label, type, data, minval, maxval);
	}
	return rv;
}

void uintptrmachineparam_range(psy_audio_UIntPtrMachineParam* self,
	uintptr_t* minval, uintptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void uintptrmachineparam_tweak(psy_audio_UIntPtrMachineParam* self, double value)
{
	if (self->data) {
		uintptr_t scaled;

		scaled = (uintptr_t)(value * (self->maxval - self->minval) + 0.5f) +
			self->minval;
		*self->data = scaled;
	}
}

double uintptrmachineparam_norm_value(const psy_audio_UIntPtrMachineParam* self)
{
	double rv = 0.0;

	if (self->data) {
		rv = ((self->maxval - self->minval) != 0)
			? (*self->data - self->minval) /
			(double)(self->maxval - self->minval)
			: 0.0;
	} else {
		psy_signal_emit(
			&((psy_audio_UIntPtrMachineParam*)self)->machineparam.signal_normvalue,
			(psy_audio_UIntPtrMachineParam*)self, 1, (void*)(&rv));
	}
	return rv;
}

int uintptrmachineparam_describe(psy_audio_UIntPtrMachineParam* self, char* text)
{
	int rv = 1;

	psy_snprintf(text, 128, (self->mask) ? self->mask : "%u",
		(unsigned int)psy_audio_machineparam_scaled_value(
			psy_audio_uintptrmachineparam_base(self)));
	return rv;
}

int uintptrmachineparam_label(psy_audio_UIntPtrMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int uintptrmachineparam_name(psy_audio_UIntPtrMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int uintptrmachineparam_type(psy_audio_UIntPtrMachineParam* self)
{
	return self->type;
}

void psy_audio_uintptrmachineparam_setmask(psy_audio_UIntPtrMachineParam* self, const char* mask)
{
	free(self->mask);
	self->mask = (mask) ? strdup(mask) : 0;
}


// DoubleMachineParam
static void doublemachineparam_tweak(psy_audio_DoubleMachineParam*, double val);
static double doublemachineparam_norm_value(const psy_audio_DoubleMachineParam*);
static void doublemachineparam_range(psy_audio_DoubleMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int doublemachineparam_type(psy_audio_DoubleMachineParam*);
static int doublemachineparam_label(psy_audio_DoubleMachineParam*, char* text);
static int doublemachineparam_name(psy_audio_DoubleMachineParam*, char* text);
static int doublemachineparam_describe(psy_audio_DoubleMachineParam*, char* text);

static MachineParamVtable doublemachineparam_vtable;
static int doublemachineparam_vtable_initialized = 0;

static void doublemachineparam_vtable_init(psy_audio_DoubleMachineParam* self)
{
	if (!doublemachineparam_vtable_initialized) {
		doublemachineparam_vtable = *(self->machineparam.vtable);
		doublemachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			doublemachineparam_tweak;
		doublemachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			doublemachineparam_norm_value;
		doublemachineparam_vtable.range =
			(fp_machineparam_range)
			doublemachineparam_range;
		doublemachineparam_vtable.name =
			(fp_machineparam_name)
			doublemachineparam_name;
		doublemachineparam_vtable.label =
			(fp_machineparam_label)
			doublemachineparam_label;
		doublemachineparam_vtable.type =
			(fp_machineparam_type)
			doublemachineparam_type;
		doublemachineparam_vtable.describe =
			(fp_machineparam_describe)
			doublemachineparam_describe;
		doublemachineparam_vtable_initialized = 1;
	}
}

void psy_audio_doublemachineparam_init(psy_audio_DoubleMachineParam* self,
	const char* name, const char* label, int type, double* data, intptr_t minval,
	intptr_t maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	doublemachineparam_vtable_init(self);
	self->machineparam.vtable = &doublemachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->mask = 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->data = data;
}

void psy_audio_doublemachineparam_dispose(psy_audio_DoubleMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	free(self->mask);
}

psy_audio_DoubleMachineParam* psy_audio_doublemachineparam_alloc(void)
{
	return (psy_audio_DoubleMachineParam*)malloc(sizeof(psy_audio_DoubleMachineParam));
}

psy_audio_DoubleMachineParam* psy_audio_doublemachineparam_alloc_init(
	const char* name, const char* label, int type, double* data,
	intptr_t minval, intptr_t maxval)
{
	psy_audio_DoubleMachineParam* rv;

	rv = psy_audio_doublemachineparam_alloc();
	if (rv) {
		psy_audio_doublemachineparam_init(rv, name, label, type, data, minval, maxval);
	}
	return rv;
}

void doublemachineparam_range(psy_audio_DoubleMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void doublemachineparam_tweak(psy_audio_DoubleMachineParam* self, double value)
{
	if (self->data) {		
		*self->data = value;
	}
}

double doublemachineparam_norm_value(const psy_audio_DoubleMachineParam* self)
{
	if (self->data) {
		return (*self->data);
	}
	return 0.0;
}

int doublemachineparam_describe(psy_audio_DoubleMachineParam* self, char* text)
{	
	int rv = 0;

	if (self->data) {
		if (self->machineparam.signal_describe.slots != NULL) {
			psy_signal_emit(&self->machineparam.signal_describe, self, 2,
				(void*)(&rv), (void*)(text));
		} else {
			psy_snprintf(text, 128, (self->mask) ? self->mask : "%f", (*self->data));
			rv = 1;
		}
	}
	return rv;
}

int doublemachineparam_label(psy_audio_DoubleMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int doublemachineparam_name(psy_audio_DoubleMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int doublemachineparam_type(psy_audio_DoubleMachineParam* self)
{
	return self->type;
}

void psy_audio_doublemachineparam_setmask(psy_audio_DoubleMachineParam* self, const char* mask)
{
	free(self->mask);
	self->mask = (mask) ? strdup(mask) : 0;
}

// Choice Machine Parameter
static void choicemachineparam_tweak(psy_audio_ChoiceMachineParam*, double val);
static double choicemachineparam_norm_value(const psy_audio_ChoiceMachineParam*);
static void choicemachineparam_range(psy_audio_ChoiceMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int choicemachineparam_type(psy_audio_ChoiceMachineParam*);
static int choicemachineparam_label(psy_audio_ChoiceMachineParam*, char* text);
static int choicemachineparam_name(psy_audio_ChoiceMachineParam*, char* text);
static int choicemachineparam_describe(psy_audio_ChoiceMachineParam*, char* text);

static MachineParamVtable choicemachineparam_vtable;
static int choicemachineparam_vtable_initialized = 0;

static void choicemachineparam_vtable_init(psy_audio_ChoiceMachineParam* self)
{
	if (!choicemachineparam_vtable_initialized) {
		choicemachineparam_vtable = *(self->machineparam.vtable);
		choicemachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			choicemachineparam_tweak;
		choicemachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			choicemachineparam_norm_value;
		choicemachineparam_vtable.range =
			(fp_machineparam_range)
			choicemachineparam_range;
		choicemachineparam_vtable.name =
			(fp_machineparam_name)
			choicemachineparam_name;
		choicemachineparam_vtable.label =
			(fp_machineparam_label)
			choicemachineparam_label;
		choicemachineparam_vtable.type =
			(fp_machineparam_type)
			choicemachineparam_type;
		choicemachineparam_vtable.describe =
			(fp_machineparam_describe)
			choicemachineparam_describe;
		choicemachineparam_vtable_initialized = 1;
	}
}

void psy_audio_choicemachineparam_init(psy_audio_ChoiceMachineParam* self,
	const char* name, const char* label, int type, int32_t* data, intptr_t minval,
	intptr_t maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	choicemachineparam_vtable_init(self);
	self->machineparam.vtable = &choicemachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->data = data;
	psy_table_init(&self->descriptions);
}

void psy_audio_choicemachineparam_dispose(psy_audio_ChoiceMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	psy_table_dispose_all(&self->descriptions, (psy_fp_disposefunc)NULL);
}

void choicemachineparam_range(psy_audio_ChoiceMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void choicemachineparam_tweak(psy_audio_ChoiceMachineParam* self, double value)
{
	if (self->data) {
		intptr_t scaled;

		scaled = (intptr_t)(value * (self->maxval - self->minval) + 0.5f) +
			self->minval;
		*self->data = (int32_t)scaled;
		psy_signal_emit_double(&self->machineparam.signal_tweak, self, 1, value);
	} else {
		psy_signal_emit_double(&self->machineparam.signal_tweak, self, 1, value);
	}
}

double choicemachineparam_norm_value(const psy_audio_ChoiceMachineParam* self)
{
	double rv = 0.0;

	if (self->data) {
		rv = ((self->maxval - self->minval) != 0)
			? (*self->data - self->minval) /
			(double)(self->maxval - self->minval)
			: 0.0;
	} else {
		psy_signal_emit(
			&((psy_audio_ChoiceMachineParam*)self)->machineparam.signal_normvalue,
			(psy_audio_ChoiceMachineParam*)self, 1, (void*)(&rv));
	}
	return rv;
}

int choicemachineparam_describe(psy_audio_ChoiceMachineParam* self, char* text)
{
	char* desc;
	uintptr_t selection;
	
	selection = (uintptr_t)(choicemachineparam_norm_value(self) *
		(self->maxval - self->minval));
	desc = (char*)psy_table_at(&self->descriptions, selection);
	if (desc) {
		psy_snprintf(text, 128, "%s", desc);
	} else {
		psy_snprintf(text, 128, "%d",
			(int)psy_audio_machineparam_scaled_value(
				psy_audio_choicemachineparam_base(self)));
	}	
	return TRUE;
}

int choicemachineparam_label(psy_audio_ChoiceMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int choicemachineparam_name(psy_audio_ChoiceMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int choicemachineparam_type(psy_audio_ChoiceMachineParam* self)
{
	return self->type;
}

void psy_audio_choicemachineparam_set_description(psy_audio_ChoiceMachineParam*
	self, uintptr_t index, const char* desc)
{
	char* olddesc;

	olddesc = (char*)psy_table_at(&self->descriptions, index);
	if (olddesc) {		
		psy_table_remove(&self->descriptions, index);
		free(olddesc);
	}
	if (desc) {
		psy_table_insert(&self->descriptions, index, strdup(desc));
	}
}


/* VolumeMachineParam */

/* prototypes */
static void volumemachineparam_tweak(psy_audio_VolumeMachineParam*, double val);
static double volumemachineparam_norm_value(const psy_audio_VolumeMachineParam*);
static void volumemachineparam_range(psy_audio_VolumeMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int volumemachineparam_type(psy_audio_VolumeMachineParam*);
static int volumemachineparam_label(psy_audio_VolumeMachineParam*, char* text);
static int volumemachineparam_name(psy_audio_VolumeMachineParam*, char* text);
static int volumemachineparam_describe(psy_audio_VolumeMachineParam*, char* text);

/* vtable */
static MachineParamVtable volumemachineparam_vtable;
static int volumemachineparam_vtable_initialized = 0;

static void volumemachineparam_vtable_init(psy_audio_VolumeMachineParam* self)
{
	if (!volumemachineparam_vtable_initialized) {
		volumemachineparam_vtable = *(self->machineparam.vtable);
		volumemachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			volumemachineparam_tweak;
		volumemachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			volumemachineparam_norm_value;
		volumemachineparam_vtable.range =
			(fp_machineparam_range)
			volumemachineparam_range;
		volumemachineparam_vtable.name =
			(fp_machineparam_name)
			volumemachineparam_name;
		volumemachineparam_vtable.label =
			(fp_machineparam_label)
			volumemachineparam_label;
		volumemachineparam_vtable.type =
			(fp_machineparam_type)
			volumemachineparam_type;
		volumemachineparam_vtable.describe =
			(fp_machineparam_describe)
			volumemachineparam_describe;
		volumemachineparam_vtable_initialized = 1;
	}
}

/* implementation */
void psy_audio_volumemachineparam_init(psy_audio_VolumeMachineParam* self,
	const char* name, const char* label, int type, double* data)
{
	psy_audio_machineparam_init(&self->machineparam);
	volumemachineparam_vtable_init(self);
	self->machineparam.vtable = &volumemachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->mask = 0;
	self->minval = 0;
	self->maxval = 0x1000;
	self->type = type;
	self->data = data;
	self->mode = psy_audio_VOLUME_MIXER;
}

void psy_audio_volumemachineparam_dispose(psy_audio_VolumeMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	free(self->mask);
}

psy_audio_VolumeMachineParam* psy_audio_volumemachineparam_alloc(void)
{
	return (psy_audio_VolumeMachineParam*)malloc(sizeof(psy_audio_VolumeMachineParam));
}

psy_audio_VolumeMachineParam* psy_audio_volumemachineparam_alloc_init(
	const char* name, const char* label, int type, double* data)
{
	psy_audio_VolumeMachineParam* rv;

	rv = psy_audio_volumemachineparam_alloc();
	if (rv) {
		psy_audio_volumemachineparam_init(rv, name, label, type, data);
	}
	return rv;
}

void volumemachineparam_range(psy_audio_VolumeMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void volumemachineparam_tweak(psy_audio_VolumeMachineParam* self, double value)
{
	if (self->data) {				
		if (self->mode == psy_audio_VOLUME_MIXER) {
			double dbs;
			intptr_t scaled;

			scaled = (intptr_t)(value * self->maxval);
			dbs = (scaled / 42.67) - 96.0;
			*self->data = psy_dsp_convert_db_to_amp(dbs);
		} else {
			/* psy_audio_VOLUME_LINEAR */
			*self->data = (intptr_t)(value * self->maxval) / (double)self->maxval;
		}
	}
}

double volumemachineparam_norm_value(const psy_audio_VolumeMachineParam* self)
{
	double rv = 0.0;

	if (self->data) {
		if (self->mode == psy_audio_VOLUME_MIXER) {
			double dbs;
			int value;
			int scaled;

			value = (int)(*self->data * 0x1000);

			dbs = psy_dsp_convert_amp_to_db(*self->data);
			scaled = (int)((dbs + 96.0f) * 42.67); /* *(0x1000 / 96.0f)	*/
			rv = scaled / (double)0x1000;
		} else {
			/* psy_audio_VOLUME_LINEAR */
			rv = (double)(*self->data);
		}
	}
	return rv;
}

int volumemachineparam_describe(psy_audio_VolumeMachineParam* self, char* text)
{
	if (self->data) {
		if (*self->data < 0.00002f) {
			psy_snprintf(text, 20, "%s", "-inf");
		} else {
			if (self->mode == psy_audio_VOLUME_MIXER) {				
				psy_snprintf(text, 20, "%.01fdB", psy_dsp_convert_amp_to_db(
					*self->data));
			} else {
				psy_snprintf(text, 20, "%d", (int)(*self->data * self->maxval));
			}
		}
	}
	return self->data != NULL;
}

int volumemachineparam_label(psy_audio_VolumeMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int volumemachineparam_name(psy_audio_VolumeMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int volumemachineparam_type(psy_audio_VolumeMachineParam* self)
{
	return self->type;
}

void psy_audio_volumemachineparam_setmode(psy_audio_VolumeMachineParam* self,
	psy_audio_VolumeMode mode)
{
	self->mode = mode;
}

void psy_audio_volumemachineparam_setrange(psy_audio_VolumeMachineParam* self, intptr_t minval,
	intptr_t maxval)
{
	self->minval = minval;
	self->maxval = maxval;
}

void psy_audio_volumemachineparam_setmask(psy_audio_VolumeMachineParam* self, const char* mask)
{
	free(self->mask);
	self->mask = (mask) ? strdup(mask) : 0;
}

// GainMachineParam
static void gainmachineparam_tweak(psy_audio_GainMachineParam*, double val);
static double gainmachineparam_norm_value(const psy_audio_GainMachineParam*);
static void gainmachineparam_range(psy_audio_GainMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int gainmachineparam_type(psy_audio_GainMachineParam*);
static int gainmachineparam_label(psy_audio_GainMachineParam*, char* text);
static int gainmachineparam_name(psy_audio_GainMachineParam*, char* text);
static int gainmachineparam_describe(psy_audio_GainMachineParam*, char* text);

static MachineParamVtable gainmachineparam_vtable;
static int gainmachineparam_vtable_initialized = 0;

static void gainmachineparam_vtable_init(psy_audio_GainMachineParam* self)
{
	if (!gainmachineparam_vtable_initialized) {
		gainmachineparam_vtable = *(self->machineparam.vtable);
		gainmachineparam_vtable.tweak = (fp_machineparam_tweak)gainmachineparam_tweak;
		gainmachineparam_vtable.normvalue = (fp_machineparam_norm_value)gainmachineparam_norm_value;
		gainmachineparam_vtable.range = (fp_machineparam_range)gainmachineparam_range;
		gainmachineparam_vtable.name = (fp_machineparam_name)gainmachineparam_name;
		gainmachineparam_vtable.label = (fp_machineparam_label)gainmachineparam_label;
		gainmachineparam_vtable.type = (fp_machineparam_type)gainmachineparam_type;
		gainmachineparam_vtable.describe = (fp_machineparam_describe)gainmachineparam_describe;
		gainmachineparam_vtable_initialized = 1;
	}
}

void psy_audio_gainmachineparam_init(psy_audio_GainMachineParam* self,
	const char* name, const char* label, int type, double* data, intptr_t minval,
	intptr_t maxval)
{
	psy_audio_machineparam_init(&self->machineparam);
	gainmachineparam_vtable_init(self);
	self->machineparam.vtable = &gainmachineparam_vtable;
	self->name = (name) ? strdup(name) : 0;
	self->label = (label) ? strdup(label) : 0;
	self->mask = 0;
	self->minval = minval;
	self->maxval = maxval;
	self->type = type;
	self->data = data;
}

void psy_audio_gainmachineparam_dispose(psy_audio_GainMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	free(self->label);
	free(self->mask);
}

psy_audio_GainMachineParam* psy_audio_gainmachineparam_alloc(void)
{
	return (psy_audio_GainMachineParam*)malloc(sizeof(psy_audio_GainMachineParam));
}

psy_audio_GainMachineParam* psy_audio_gainmachineparam_alloc_init(
	const char* name, const char* label, int type, double* data,
	intptr_t minval, intptr_t maxval)
{
	psy_audio_GainMachineParam* rv;

	rv = psy_audio_gainmachineparam_alloc();
	if (rv) {
		psy_audio_gainmachineparam_init(rv, name, label, type, data, minval, maxval);
	}
	return rv;
}

void gainmachineparam_range(psy_audio_GainMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = self->minval;
	*maxval = self->maxval;
}

void gainmachineparam_tweak(psy_audio_GainMachineParam* self, double value)
{
	if (self->data) {
		*self->data = value * 4;
	}
}

double gainmachineparam_norm_value(const psy_audio_GainMachineParam* self)
{	
	if (self->data) {
		return (*self->data) / 4.0;
	}
	return 0.0;
}

int gainmachineparam_describe(psy_audio_GainMachineParam* self, char* text)
{
	if (self->data) {		
		double dbs;
		
		dbs = ((*self->data) > 0.0)
			? psy_dsp_convert_amp_to_db(*self->data)
			: -100.0;
		psy_snprintf(text, 128, "%.01fdB", dbs);
	}
	return (self->data != NULL);
}

int gainmachineparam_label(psy_audio_GainMachineParam* self, char* text)
{
	if (self->label) {
		psy_snprintf(text, 128, "%s", self->label);
		psy_signal_emit(&self->machineparam.signal_label, self, 1, text);
		return 1;
	}
	return 0;
}

int gainmachineparam_name(psy_audio_GainMachineParam* self, char* text)
{
	if (self->name) {
		psy_snprintf(text, 128, "%s", self->name);
		psy_signal_emit(&self->machineparam.signal_name, self, 1, text);
		return 1;
	}
	return 0;
}

int gainmachineparam_type(psy_audio_GainMachineParam* self)
{
	return self->type;
}

void psy_audio_gainmachineparam_setmask(psy_audio_GainMachineParam* self, const char* mask)
{
	free(self->mask);
	self->mask = (mask) ? strdup(mask) : 0;
}

// WireParam
// WireParam
static void psy_audio_wiremachineparam_tweak(psy_audio_WireMachineParam*, double val);
static double psy_audio_wiremachineparam_norm_value(const psy_audio_WireMachineParam*);
static void psy_audio_wiremachineparam_range(psy_audio_WireMachineParam*,
	intptr_t* minval, intptr_t* maxval);
static int psy_audio_wiremachineparam_describe(psy_audio_WireMachineParam*, char* text);
static int psy_audio_wiremachineparam_name(psy_audio_WireMachineParam*, char* text);

static MachineParamVtable psy_audio_wiremachineparam_vtable;
static bool psy_audio_wiremachineparam_vtable_initialized = FALSE;

static void psy_audio_wiremachineparam_vtable_init(psy_audio_WireMachineParam* self)
{
	if (!psy_audio_wiremachineparam_vtable_initialized) {
		psy_audio_wiremachineparam_vtable = *(self->machineparam.vtable);
		psy_audio_wiremachineparam_vtable.tweak =
			(fp_machineparam_tweak)
			psy_audio_wiremachineparam_tweak;
		psy_audio_wiremachineparam_vtable.normvalue =
			(fp_machineparam_norm_value)
			psy_audio_wiremachineparam_norm_value;
		psy_audio_wiremachineparam_vtable.describe =
			(fp_machineparam_describe)
			psy_audio_wiremachineparam_describe;
		psy_audio_wiremachineparam_vtable.range =
			(fp_machineparam_range)
			psy_audio_wiremachineparam_range;
		psy_audio_wiremachineparam_vtable.name =
			(fp_machineparam_name)
			psy_audio_wiremachineparam_name;
		psy_audio_wiremachineparam_vtable_initialized = TRUE;
	}
}

void psy_audio_wiremachineparam_init(psy_audio_WireMachineParam* self,
	const char* name, psy_audio_Wire wire, psy_audio_Machines* machines)
{
	psy_audio_machineparam_init(&self->machineparam);
	psy_audio_wiremachineparam_vtable_init(self);
	self->machineparam.vtable = &psy_audio_wiremachineparam_vtable;
	self->wire = wire;
	self->machines = machines;
	self->name = psy_strdup(name);
}

void psy_audio_wiremachineparam_dispose(psy_audio_WireMachineParam* self)
{
	psy_audio_machineparam_dispose(&self->machineparam);
	free(self->name);
	self->name = NULL;
}

psy_audio_WireMachineParam* psy_audio_wiremachineparam_alloc(void)
{
	return (psy_audio_WireMachineParam*)malloc(sizeof(psy_audio_WireMachineParam));
}

psy_audio_WireMachineParam* psy_audio_wiremachineparam_allocinit(
	const char* name, psy_audio_Wire wire, psy_audio_Machines* machines)
{
	psy_audio_WireMachineParam* rv;

	rv = psy_audio_wiremachineparam_alloc();
	if (rv) {
		psy_audio_wiremachineparam_init(rv, name, wire, machines);
	}
	return rv;
}

void psy_audio_wiremachineparam_deallocate(psy_audio_WireMachineParam* self)
{
	psy_audio_wiremachineparam_dispose(self);
	free(self);
}

void psy_audio_wiremachineparam_tweak(psy_audio_WireMachineParam* self,
	double val)
{
	if (!self->machines) {
		return;
	}
	psy_audio_connections_set_wire_volume(&self->machines->connections,
		self->wire, val * 4.0);
}

double psy_audio_wiremachineparam_norm_value(const psy_audio_WireMachineParam*
	self)
{
	if (!self->machines) {
		return 0.0;
	}
	return psy_audio_connections_wire_volume(&self->machines->connections,
		self->wire) / 4.0;
}

void psy_audio_wiremachineparam_range(psy_audio_WireMachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	*minval = 0;
	*maxval = 127;
}

int psy_audio_wiremachineparam_describe(psy_audio_WireMachineParam* self, char* text)
{
	double volume;

	if (!self->machines) {
		return 0;
	}
	volume = psy_audio_connections_wire_volume(&self->machines->connections,
		self->wire);
	if (volume < 0.00002) {
		psy_snprintf(text, 20, "%s", "-inf");
	} else {				
		psy_snprintf(text, 20, "%.01fdB", psy_dsp_convert_amp_to_db(volume));
		return 1;
	}
	return 0;
}

int psy_audio_wiremachineparam_name(psy_audio_WireMachineParam* self, char* text)
{
	psy_snprintf(text, 128, "%s", self->name);
	return 1;
}
