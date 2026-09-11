/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_MACHINEPARAM_H
#define psy_audio_MACHINEPARAM_H

/* local */
#include "wire.h"
/* container */
#include <signal.h>
#include <hashtbl.h>
/* dsp */
#include <dsptypes.h>

#include "../../detail/psydef.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_audio_MachineParam
** @brief Base class for various parameter classes (e.g. int, choice, gain).
** 
** @detail
** Enables reuse of repeating parameter classes. Instead tweaking with
** a parameter index, the machine is asked with a index for a parameter
** object. On the parameter object operations can be done, either direct
** or using the machine interface having proxy exception handling
**
** Example
**
** This assumes a valid param
**
** void handle_range(psy_audio_MachineParam* param)
** {
**    intptr_t minval;
**    intptr_t maxval;
**    intptr_t steps;
**    
**    psy_audio_machineparam_range(param, &minval, &maxval);
**    steps = maxval - minval;
** }
**
** The access above is not proxy protected. Todo so use the machine method.
**
** void handle_range(psy_audio_MachineParam* param, psy_audio_Machine* machine)
** {
**    intptr_t minval;
**    intptr_t maxval;
**    intptr_t steps;
**
**    psy_audio_machine_parameter_range(machine, param, &minval, &maxval);
**    steps = maxval - minval;
** }
*/

struct psy_audio_MachineParam;

typedef	void (*fp_machineparam_tweak)(struct psy_audio_MachineParam*,
	double val);
typedef double (*fp_machineparam_norm_value)(const struct
	psy_audio_MachineParam*);
typedef double (*fp_machineparam_reset)(struct psy_audio_MachineParam*);
typedef void (*fp_machineparam_range)(struct psy_audio_MachineParam*,
	intptr_t* minval, intptr_t* maxval);
typedef	int (*fp_machineparam_type)(struct psy_audio_MachineParam*);
typedef	int (*fp_machineparam_label)(struct psy_audio_MachineParam*,
	char* text);
typedef	int (*fp_machineparam_name)(struct psy_audio_MachineParam*,
	char* text);
typedef	int (*fp_machineparam_describe)(struct psy_audio_MachineParam*,
	char* text);

typedef struct MachineParamVtable {
	fp_machineparam_norm_value normvalue;
	fp_machineparam_range range;
	fp_machineparam_type type;
	fp_machineparam_label label;
	fp_machineparam_name name;
	fp_machineparam_describe describe;
	/* events */
	fp_machineparam_tweak tweak;	
	fp_machineparam_reset reset;
} MachineParamVtable;

typedef struct psy_audio_MachineParam {
	MachineParamVtable* vtable;
	psy_Signal signal_normvalue;
	psy_Signal signal_tweak;
	psy_Signal signal_describe;
	psy_Signal signal_name;
	psy_Signal signal_label;
	bool crashed;
	bool isslidergroup;
	uintptr_t param0;
	uintptr_t param1;
} psy_audio_MachineParam;

void psy_audio_machineparam_init(psy_audio_MachineParam*);
void psy_audio_machineparam_dispose(psy_audio_MachineParam*);

INLINE void psy_audio_machineparam_tweak(psy_audio_MachineParam* self,
	double value)
{
	self->vtable->tweak(self, value);
}

/* reset to default */
INLINE void psy_audio_machineparam_reset(psy_audio_MachineParam* self)
{
	self->vtable->reset(self);
}

/* [0.0..1.0] */
INLINE double psy_audio_machineparam_norm_value(const psy_audio_MachineParam*
	self)
{
	return self->vtable->normvalue(self);
}

/* scale in integer */
INLINE void psy_audio_machineparam_range(psy_audio_MachineParam* self,
	intptr_t* minval, intptr_t* maxval)
{
	self->vtable->range(self, minval, maxval);
}

INLINE int psy_audio_machineparam_name(psy_audio_MachineParam* self,
	char* text)
{
	return self->vtable->name(self, text);
}

INLINE int psy_audio_machineparam_label(psy_audio_MachineParam* self,
	char* text)
{
	return self->vtable->label(self, text);
}

INLINE int psy_audio_machineparam_type(psy_audio_MachineParam* self)
{
	return self->vtable->type(self);
}

INLINE int psy_audio_machineparam_describe(psy_audio_MachineParam* self,
	char* text)
{
	return self->vtable->describe(self, text);
}

/* converts normvalue(0.f .. 1.f) -> scaled integer value */
INLINE intptr_t psy_audio_machineparam_scaled_value(psy_audio_MachineParam* self)
{
	intptr_t rv;
	intptr_t minval;
	intptr_t maxval;
	intptr_t range;

	self->vtable->range(self, & minval, & maxval);
	range = maxval - minval;
	rv = (int)(self->vtable->normvalue(self) * range) + minval;
	return rv;
}

INLINE void psy_audio_machineparam_tweak_scaled(psy_audio_MachineParam* self,
	intptr_t scaledvalue)
{
	intptr_t minval;
	intptr_t maxval;
	intptr_t range;
	double value;

	psy_audio_machineparam_range(self, &minval, &maxval);
	range = maxval - minval;
	if (range == 0) {
		value = 0.0;
	} else {
		value = (scaledvalue - minval) / (double)range;
	}
	if (value > 1.0) {
		value = 1.0;
	}
	if (value < 0.0) {
		value = 0.0;
	}
	psy_audio_machineparam_tweak(self, value);
}

struct psy_audio_CustomMachineParam;

typedef struct psy_audio_CustomMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	int type;	
	uintptr_t index;
	intptr_t row;
} psy_audio_CustomMachineParam;

void psy_audio_custommachineparam_init(psy_audio_CustomMachineParam*,
	const char* name, const char* label, int type, int minval, int maxval);
void psy_audio_custommachineparam_dispose(psy_audio_CustomMachineParam*);

psy_audio_CustomMachineParam* psy_audio_custommachineparam_alloc(void);
psy_audio_CustomMachineParam* psy_audio_custommachineparam_alloc_init(
	const char* name, const char* label, int type,
	int minval, int maxval);

INLINE uintptr_t psy_audio_custommachineparam_index(
	const psy_audio_CustomMachineParam* self)
{
	return (self->index);
}

INLINE psy_audio_MachineParam* psy_audio_custommachineparam_base(
	psy_audio_CustomMachineParam* self)
{
	return &(self->machineparam);
}

/* psy_audio_InfoMachineParam */
typedef struct psy_audio_InfoMachineParam {
	psy_audio_MachineParam machineparam;
	char* name;
	char* label;
	int style;
} psy_audio_InfoMachineParam;

void psy_audio_infomachineparam_init(psy_audio_InfoMachineParam*,
	const char* name, const char* label, int style);
void psy_audio_infomachineparam_dispose(psy_audio_InfoMachineParam*);

INLINE psy_audio_MachineParam* psy_audio_infomachineparam_base(
	psy_audio_InfoMachineParam* self)
{
	return &(self->machineparam);
}

psy_audio_InfoMachineParam* psy_audio_infomachineparam_alloc(void);
psy_audio_InfoMachineParam* psy_audio_infomachineparam_allocinit(
	const char* name, const char* label, int style);

/* psy_audio_IntMachineParam */
typedef struct psy_audio_IntMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	char* mask;
	int type;
	int32_t* data;
} psy_audio_IntMachineParam;

void psy_audio_intmachineparam_init(psy_audio_IntMachineParam*,
	const char* name, const char* label, int type, int32_t* data,
	intptr_t minval, intptr_t maxval);
void psy_audio_intmachineparam_dispose(psy_audio_IntMachineParam*);
void psy_audio_intmachineparam_setmask(psy_audio_IntMachineParam*,
	const char* mask);

INLINE psy_audio_MachineParam* psy_audio_intmachineparam_base(
	psy_audio_IntMachineParam* self)
{
	return &(self->machineparam);
}

psy_audio_IntMachineParam* psy_audio_intmachineparam_alloc(void);
psy_audio_IntMachineParam* psy_audio_intmachineparam_allocinit(
	const char* name, const char* label, int type, int32_t* data,
	intptr_t minval, intptr_t maxval);

/* psy_audio_UIntPtrMachineParam */
typedef struct psy_audio_UIntPtrMachineParam {
	psy_audio_MachineParam machineparam;
	uintptr_t minval;
	uintptr_t maxval;
	char* name;
	char* label;
	char* mask;
	int type;
	uintptr_t* data;
} psy_audio_UIntPtrMachineParam;

void psy_audio_uintptrmachineparam_init(psy_audio_UIntPtrMachineParam*,
	const char* name, const char* label, int type, uintptr_t* data,
	uintptr_t minval, uintptr_t maxval);
void psy_audio_uintptrmachineparam_dispose(psy_audio_UIntPtrMachineParam*);
void psy_audio_uintptrmachineparam_setmask(psy_audio_UIntPtrMachineParam*,
	const char* mask);

INLINE psy_audio_MachineParam* psy_audio_uintptrmachineparam_base(
	psy_audio_UIntPtrMachineParam* self)
{
	return &(self->machineparam);
}

psy_audio_UIntPtrMachineParam* psy_audio_uintptrmachineparam_alloc(void);
psy_audio_UIntPtrMachineParam* psy_audio_uintptrmachineparam_allocinit(
	const char* name, const char* label, int type, uintptr_t* data,
	uintptr_t minval, uintptr_t maxval);

/* psy_audio_DoubleMachineParam [0 .. 1] */
typedef struct psy_audio_DoubleMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	char* mask;
	int type;
	double* data;
} psy_audio_DoubleMachineParam;

void psy_audio_doublemachineparam_init(psy_audio_DoubleMachineParam*,
	const char* name, const char* label, int type, double* data,
	intptr_t minval, intptr_t maxval);
void psy_audio_doublemachineparam_dispose(psy_audio_DoubleMachineParam*);

void psy_audio_doublemachineparam_setmask(psy_audio_DoubleMachineParam*,
	const char* mask);

INLINE psy_audio_MachineParam* psy_audio_doublemachineparam_base(
	psy_audio_DoubleMachineParam* self)
{
	return &(self->machineparam);
}

psy_audio_DoubleMachineParam* psy_audio_doublemachineparam_alloc(void);
psy_audio_DoubleMachineParam* psy_audio_doublemachineparam_allocinit(
	const char* name, const char* label, int type, double* data, intptr_t minval,
	intptr_t maxval);

/* psy_audio_ChoiceMachineParam */
typedef struct psy_audio_ChoiceMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	int type;
	int32_t* data;
	psy_Table descriptions;
} psy_audio_ChoiceMachineParam;

void psy_audio_choicemachineparam_init(psy_audio_ChoiceMachineParam*,
	const char* name, const char* label, int type, int32_t* data,
	intptr_t minval, intptr_t maxval);
void psy_audio_choicemachineparam_dispose(psy_audio_ChoiceMachineParam*);
void psy_audio_choicemachineparam_set_description(psy_audio_ChoiceMachineParam*,
	uintptr_t index, const char* desc);

INLINE psy_audio_MachineParam* psy_audio_choicemachineparam_base(
	psy_audio_ChoiceMachineParam* self)
{
	return &(self->machineparam);
}

INLINE intptr_t psy_audio_choicemachineparam_choice(
	psy_audio_ChoiceMachineParam* self)
{
	return psy_audio_machineparam_scaled_value(&self->machineparam);	
}

typedef enum {
	psy_audio_VOLUME_LINEAR,
	psy_audio_VOLUME_MIXER
} psy_audio_VolumeMode;

/* psy_audio_VolumeMachineParam [0 .. 1] */
typedef struct psy_audio_VolumeMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	char* mask;
	int type;
	double* data;
	int scale;
	psy_audio_VolumeMode mode;
} psy_audio_VolumeMachineParam;

void psy_audio_volumemachineparam_init(psy_audio_VolumeMachineParam*,
	const char* name, const char* label, int type, double* data);
void psy_audio_volumemachineparam_dispose(psy_audio_VolumeMachineParam*);
void psy_audio_volumemachineparam_setmode(psy_audio_VolumeMachineParam*,
	psy_audio_VolumeMode);
void psy_audio_volumemachineparam_setrange(psy_audio_VolumeMachineParam*,
	intptr_t minval, intptr_t maxval);
void psy_audio_volumemachineparam_setmask(psy_audio_VolumeMachineParam*,
	const char* mask);

psy_audio_VolumeMachineParam* psy_audio_volumemachineparam_alloc(void);
psy_audio_VolumeMachineParam* psy_audio_volumemachineparam_alloc_init(
	const char* name, const char* label, int type, double* data);

INLINE psy_audio_MachineParam* psy_audio_volumemachineparam_base(
	psy_audio_VolumeMachineParam* self)
{
	return &(self->machineparam);
}

/* psy_audio_GainMachineParam [0 .. 4 : -12db .. 12db] */
typedef struct psy_audio_GainMachineParam {
	psy_audio_MachineParam machineparam;
	intptr_t minval;
	intptr_t maxval;
	char* name;
	char* label;
	char* mask;
	int type;
	double* data;
} psy_audio_GainMachineParam;

void psy_audio_gainmachineparam_init(psy_audio_GainMachineParam*,
	const char* name, const char* label, int type, double* data, intptr_t minval,
	intptr_t maxval);
void psy_audio_gainmachineparam_dispose(psy_audio_GainMachineParam*);
void psy_audio_gainmachineparam_setmask(psy_audio_GainMachineParam*,
	const char* mask);

psy_audio_GainMachineParam* psy_audio_gainmachineparam_alloc(void);
psy_audio_GainMachineParam* psy_audio_gainmachineparam_alloc_init(
	const char* name, const char* label, int type, double* data, intptr_t minval,
	intptr_t maxval);

INLINE psy_audio_MachineParam* psy_audio_gainmachineparam_base(
	psy_audio_GainMachineParam* self)
{
	return &(self->machineparam);
}

/* WireParam */
struct psy_audio_Machines;

typedef struct psy_audio_WireMachineParam {
	psy_audio_MachineParam machineparam;
	psy_audio_Wire wire;
	char* name;
	/* references */
	struct psy_audio_Machines* machines;
} psy_audio_WireMachineParam;

void psy_audio_wiremachineparam_init(psy_audio_WireMachineParam*,
	const char* name, psy_audio_Wire wire, struct psy_audio_Machines*);
void psy_audio_wiremachineparam_dispose(psy_audio_WireMachineParam*);

psy_audio_WireMachineParam* psy_audio_wiremachineparam_alloc(void);
psy_audio_WireMachineParam* psy_audio_wiremachineparam_allocinit(
	const char* name, psy_audio_Wire, struct psy_audio_Machines*);
void psy_audio_wiremachineparam_deallocate(psy_audio_WireMachineParam*);

INLINE psy_audio_MachineParam* psy_audio_wiremachineparam_base(
	psy_audio_WireMachineParam* self)
{
	return &(self->machineparam);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_MACHINEPARAM_H */
