/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"
#include "../../detail/os.h"

#include "ladspaparam.h"

#ifdef PSYCLE_USE_LADSPA

/* local */
#include "plugin_interface.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

/* prototypes */
static int ladspapluginparam_name(psy_audio_LadspaParam*, char* text);
static int ladspapluginparam_label(psy_audio_LadspaParam*, char* text);
static void ladspapluginparam_tweak(psy_audio_LadspaParam*, double val);
static int ladspapluginparam_describe(psy_audio_LadspaParam*, char* text);
static double ladspapluginparam_norm_value(psy_audio_LadspaParam*);
static void ladspapluginparam_range(psy_audio_LadspaParam*,
	intptr_t* minval, intptr_t* maxval);
static void ladspapluginparam_set_default(psy_audio_LadspaParam*);

/* vtable */
static MachineParamVtable ladspapluginparam_vtable;
static bool ladspapluginparam_vtable_initialized = FALSE;

static void ladspapluginparam_vtable_init(psy_audio_LadspaParam* self)
{
	if (!ladspapluginparam_vtable_initialized) {
		ladspapluginparam_vtable = *(self->custommachineparam.machineparam.vtable);
		ladspapluginparam_vtable.name = (fp_machineparam_name)ladspapluginparam_name;
		ladspapluginparam_vtable.label = (fp_machineparam_label)
			ladspapluginparam_label;
		ladspapluginparam_vtable.tweak = (fp_machineparam_tweak)
			ladspapluginparam_tweak;
		ladspapluginparam_vtable.normvalue = (fp_machineparam_norm_value)
			ladspapluginparam_norm_value;
		ladspapluginparam_vtable.range = (fp_machineparam_range)
			ladspapluginparam_range;
		ladspapluginparam_vtable.describe = (fp_machineparam_describe)
			ladspapluginparam_describe;
	}
	self->custommachineparam.machineparam.vtable =
		&ladspapluginparam_vtable;
}	

void psy_audio_ladspaparam_init(
	psy_audio_LadspaParam* self,
	LADSPA_PortDescriptor descriptor,
	LADSPA_PortRangeHint hint,
	const char *newname,
	uintptr_t index,
	uintptr_t port_index)
{
	psy_audio_custommachineparam_init(&self->custommachineparam,
		"",
		"",
		MPF_STATE,
		0,
		0xFFFF);
	ladspapluginparam_vtable_init(self);	
	self->index = index;
	self->port_index = port_index;
	self->descriptor = descriptor;
	self->hint = hint;
	self->port_name = newname;		
	if (LADSPA_IS_HINT_BOUNDED_BELOW(self->hint.HintDescriptor)) {
		self->minval_ = self->hint.LowerBound;		
	} else {
		self->minval_ = 0.0;
	}
	if (LADSPA_IS_HINT_BOUNDED_ABOVE(self->hint.HintDescriptor)) {
		self->maxval_ = self->hint.UpperBound;		
	} else {
		self->maxval_ = 1.0;
	}	
	if (LADSPA_IS_HINT_SAMPLE_RATE(self->hint.HintDescriptor)) {
		self->minval_ *= 44100.0;
		self->maxval_ *= 44100.0;
	}
	ladspapluginparam_set_default(self);
}

void psy_audio_ladspaparam_dispose(psy_audio_LadspaParam* self)
{
	psy_audio_custommachineparam_dispose(&self->custommachineparam);
}

psy_audio_LadspaParam* psy_audio_ladspaparam_alloc(void)
{
	return (psy_audio_LadspaParam*)malloc(sizeof(psy_audio_LadspaParam));
}

psy_audio_LadspaParam* psy_audio_ladspaparam_allocinit(
	LADSPA_PortDescriptor descriptor,
	LADSPA_PortRangeHint hint,
	const char *newname,	
	uintptr_t index,
	uintptr_t port_index)
{
	psy_audio_LadspaParam* rv;

	rv = psy_audio_ladspaparam_alloc();
	if (rv) {
		psy_audio_ladspaparam_init(rv, descriptor, hint, newname,
			index, port_index);
	}
	return rv;
}

int ladspapluginparam_label(psy_audio_LadspaParam* self, char* text)
{
	text[0] = '\0';
	/* self->effect->dispatcher(self->effect, effGetParamLabel, (VstInt32)self->index, 0, text, 0); */
	return *text != '\0';
}

int ladspapluginparam_name(psy_audio_LadspaParam* self, char* text)
{
	text[0] = '\0';
	strcpy(text, self->port_name);
	/* self->effect->dispatcher(self->effect, effGetParamName, (VstInt32)self->index, 0, text, 0); */
	return *text != '\0';
}

void ladspapluginparam_tweak(psy_audio_LadspaParam* self, double value)
{	
	double range;
	
	range = (self->maxval_ - self->minval_);	
	self->value_ = (LADSPA_Data)(value * range + self->minval_);
}

int ladspapluginparam_describe(psy_audio_LadspaParam* self, char* text)
{
	text[0] = '\0';
	
	if (LADSPA_IS_HINT_TOGGLED(self->hint.HintDescriptor)) {
		strcpy(text, (self->value_ > 0.0) ? "on" : "off");	
	} else if (LADSPA_IS_HINT_INTEGER(self->hint.HintDescriptor)) {
		sprintf(text, "%.0f", self->value_);		
	} else {
		sprintf(text, "%.4f", self->value_);
	}
	return *text != '\0';
}

double ladspapluginparam_norm_value(psy_audio_LadspaParam* self)
{
	double range;
	
	range = (self->maxval_ - self->minval_);
	if (range != 0.0) {	
		return (self->value_ - self->minval_) / (range);
	}
	return 0.0;
}

void ladspapluginparam_range(psy_audio_LadspaParam* self, intptr_t* minval,
	intptr_t* maxval)
{
	*minval = 0;
	*maxval = 0xFFFF;
}

void ladspapluginparam_set_default(psy_audio_LadspaParam* self)
{
	LADSPA_Data fDefault=0.0f;
	
	switch (self->hint.HintDescriptor & LADSPA_HINT_DEFAULT_MASK) {
	case LADSPA_HINT_DEFAULT_NONE:
		fDefault = 0.f;
		break;
	case LADSPA_HINT_DEFAULT_MINIMUM:
		fDefault = self->hint.LowerBound;			
		break;
	case LADSPA_HINT_DEFAULT_LOW:
		if (LADSPA_IS_HINT_LOGARITHMIC(self->hint.HintDescriptor)) {
			fDefault = (LADSPA_Data)
				exp(log(self->hint.LowerBound) * 0.75 +
				log(self->hint.UpperBound) * 0.25);
		} else {
			fDefault  = (LADSPA_Data)
				(self->hint.LowerBound * 0.75 +
				self->hint.UpperBound * 0.25);
		}
		break;
	case LADSPA_HINT_DEFAULT_MIDDLE:
		if (LADSPA_IS_HINT_LOGARITHMIC(self->hint.HintDescriptor)) {
			fDefault  = (LADSPA_Data)
				sqrt(self->hint.LowerBound * self->hint.UpperBound);
		} else {
			fDefault = (LADSPA_Data)
				(0.5 * (self->hint.LowerBound + self->hint.UpperBound));
		}
		break;
	case LADSPA_HINT_DEFAULT_HIGH:
		if (LADSPA_IS_HINT_LOGARITHMIC(self->hint.HintDescriptor)) {
			fDefault = (LADSPA_Data)
				exp(log(self->hint.LowerBound) * 0.25 +
				log(self->hint.UpperBound) * 0.75);
		} else {
			fDefault = (LADSPA_Data)
				(self->hint.LowerBound * 0.25 +
				self->hint.UpperBound * 0.75);
		}
		break;
	case LADSPA_HINT_DEFAULT_MAXIMUM:
		fDefault = (LADSPA_Data)self->hint.UpperBound;
		break;
	case LADSPA_HINT_DEFAULT_0:
		fDefault = 0.0f;		
		break;
	case LADSPA_HINT_DEFAULT_1:
		fDefault = 1.0f;
		break;
	case LADSPA_HINT_DEFAULT_100:
		fDefault = 100.0f;
		break;
	case LADSPA_HINT_DEFAULT_440:
		fDefault = 440.0f;
		break;
	default:
		break;
	}	
	self->value_ = fDefault;	
}

#endif /* PSYCLE_USE_LADSPA */
