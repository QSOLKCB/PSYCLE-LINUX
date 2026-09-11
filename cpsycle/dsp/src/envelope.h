/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_dsp_ENVELOPE_H
#define psy_dsp_ENVELOPE_H

#include "dsptypes.h"
#include <list.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_dsp_EnvelopePoint {
	/* time at which to set the value. Unit can be different things depending on the context. */
	double time;
	/* 0 .. 1.0f . (or -1.0 1.0 or whatever else) Use it as a multiplier. */
	double value;
	double minvalue;
	double maxvalue;
	double mintime;
	double maxtime;
} psy_dsp_EnvelopePoint;

void psy_dsp_envelopepoint_init(psy_dsp_EnvelopePoint*, 
	psy_dsp_seconds_t time,
	double value,
	psy_dsp_seconds_t mintime,
	psy_dsp_seconds_t maxtime,
	double minvalue,
	double maxvalue);

psy_dsp_EnvelopePoint* psy_dsp_envelopepoint_alloc(void);

psy_dsp_EnvelopePoint psy_dsp_envelopepoint_make_all(
	psy_dsp_seconds_t time,
	double value,
	psy_dsp_seconds_t mintime,
	psy_dsp_seconds_t maxtime,
	double minvalue,
	double maxvalue);

psy_dsp_EnvelopePoint psy_dsp_envelopepoint_make(
	psy_dsp_seconds_t time,
	double value);

psy_dsp_EnvelopePoint psy_dsp_envelopepoint_make_start(void);

/*
** Mode defines what the first value of a PointValue means
** TICK = one tracker tick ( speed depends on the BPM )
** MILIS = a millisecond. (independant of BPM).
*/
typedef enum {
	psy_dsp_ENVELOPETIME_TICK,
	psy_dsp_ENVELOPETIME_SECONDS
} psy_dsp_EnvelopeTimeMode;

/* The envelope is made of a list of pointvalues. */
typedef psy_List* psy_dsp_EnvelopePoints;

/*!
** @struct psy_dsp_Envelope
** @brief Defines an envelope and is used by internal machines like
**        sampler, sampulse.
*/
typedef struct psy_dsp_Envelope {
	/* Envelope is enabled or disabled */
	bool enabled;
	/*
	** if m_Carryand a new note enters, the envelope position is set to that
	** of the previous note *on the same channel*
	*/
	bool carry;
	/* Array of Points of the envelope. */
	psy_dsp_EnvelopePoints points;
	/* Loop Start Point */
	uintptr_t loopstart;
	/* Loop End Point */
	uintptr_t loopend;	
	/* Sustain Start Point */
	uintptr_t sustainbegin;
	/* Sustain End Point */
	uintptr_t sustainend;
	/* Envelope mode (meaning of the time value) */
	psy_dsp_EnvelopeTimeMode timemode;
	/* to_string return string */
	char* str;
} psy_dsp_Envelope;

void psy_dsp_envelope_init(psy_dsp_Envelope*);
void psy_dsp_envelope_init_adsr(psy_dsp_Envelope*);
void psy_dsp_envelope_init_dispose(psy_dsp_Envelope*);
void psy_dsp_envelope_init_copy(psy_dsp_Envelope* self,
	const psy_dsp_Envelope* source);
/*
** Appends a new point at the end of the list.
** note: be sure that the pointtime is the highest of the points
*/
void psy_dsp_envelope_append(psy_dsp_Envelope*,
	psy_dsp_EnvelopePoint);
/* Removes a point from the points Array. */
void psy_dsp_envelope_delete(psy_dsp_Envelope*, 
	uintptr_t pointindex);
/* Clears the points Array */
void psy_dsp_envelope_clear(psy_dsp_Envelope* self);
psy_dsp_EnvelopePoint psy_dsp_envelope_at(const psy_dsp_Envelope*,
	uintptr_t pointindex);
void psy_dsp_envelope_set_time_and_value(psy_dsp_Envelope*,
	uintptr_t pointindex, psy_dsp_seconds_t pointtime, double pointval);
void psy_dsp_envelope_settime(psy_dsp_Envelope*,
	uintptr_t pointindex, psy_dsp_seconds_t pointtime);

INLINE uintptr_t psy_dsp_envelope_num_of_points(
	const psy_dsp_Envelope* self)
{
	return psy_list_size(self->points);
}

INLINE psy_dsp_seconds_t psy_dsp_envelope_time(
	const psy_dsp_Envelope* self, uintptr_t pointindex)
{
	psy_dsp_EnvelopePoint rv;

	rv = psy_dsp_envelope_at(self, pointindex);
	return rv.time;
}

INLINE uintptr_t psy_dsp_envelope_time_ms(
	const psy_dsp_Envelope* self, uintptr_t pointindex)
{
	psy_dsp_EnvelopePoint pt;

	if (pointindex < psy_dsp_envelope_num_of_points(self)) {
		pt = psy_dsp_envelope_at(self, pointindex);
		if (self->timemode == psy_dsp_ENVELOPETIME_SECONDS) {
			return (intptr_t)(pt.time * 1000.f);
		}
	}
	return psy_INDEX_INVALID;
}

void psy_dsp_envelope_set_value(psy_dsp_Envelope*,
	uintptr_t pointindex, double pointval);

INLINE double psy_dsp_envelope_value(
	const psy_dsp_Envelope* self, uintptr_t pointindex)
{
	psy_dsp_EnvelopePoint rv;

	rv = psy_dsp_envelope_at(self, pointindex);
	return rv.value;
}

const char* psy_dsp_envelope_to_string(const psy_dsp_Envelope*);

INLINE bool psy_dsp_envelope_empty(const psy_dsp_Envelope* self)
{
	return self->points == NULL;
}

/* Properties */

/* Set or Get the point Index for Sustainand Loop. */
INLINE uintptr_t psy_dsp_envelope_sustain_begin(
	const psy_dsp_Envelope* self)
{
	return self->sustainbegin;
}
/* value has to be an existing point! */
INLINE void psy_dsp_envelope_set_sustain_begin(
	psy_dsp_Envelope* self, const uintptr_t value)
{
	self->sustainbegin = value;
}

INLINE uintptr_t psy_dsp_envelope_sustain_end(
	const psy_dsp_Envelope* self)
{
	return self->sustainend;
}
/* value has to be an existing point! */
INLINE void psy_dsp_envelope_set_sustain_end(
	psy_dsp_Envelope* self, const uintptr_t value)
{
	self->sustainend = value;
}
INLINE uintptr_t psy_dsp_envelope_loop_start(
	const psy_dsp_Envelope* self)
{
	return self->loopstart;
}
/* value has to be an existing point! */
INLINE void psy_dsp_envelope_set_loop_start(
	psy_dsp_Envelope* self, const uintptr_t value)
{
	self->loopstart = value;
}

INLINE uintptr_t psy_dsp_envelope_loop_end(
	const psy_dsp_Envelope* self)
{
	return self->loopend;
}
/* value has to be an existing point! */
INLINE void psy_dsp_envelope_set_loop_end(
	psy_dsp_Envelope* self, const uintptr_t value)
{
	self->loopend = value;
}

/* If the envelope IsEnabled, it is used and triggered. Else, it is not. */
INLINE bool psy_dsp_envelope_is_enabled(
	const psy_dsp_Envelope* self)
{
	return self->enabled;
}

INLINE void psy_dsp_envelope_set_enabled(
	psy_dsp_Envelope* self, const bool value)
{
	self->enabled = value;
}

/*
** if IsCarry() and a new note enters, the envelope position is set to that
** of the previous note *on the same channel*
*/
INLINE bool psy_dsp_envelope_is_carry(
	const psy_dsp_Envelope* self)
{
	return self->carry;
}

INLINE void psy_dsp_envelope_set_carry(
	psy_dsp_Envelope* self, const bool value)
{
	self->carry = value;
}

INLINE void psy_dsp_envelope_set_mode(psy_dsp_Envelope* self,
	psy_dsp_EnvelopeTimeMode mode)
{
	self->timemode = mode;
}

INLINE psy_dsp_EnvelopeTimeMode psy_dsp_envelope_mode(
	const psy_dsp_Envelope* self) 
{
	return self->timemode;
}

INLINE psy_dsp_EnvelopeTimeMode psy_dsp_envelope_in_ticks(
	const psy_dsp_Envelope* self) 
{
	return (self->timemode == psy_dsp_ENVELOPETIME_TICK);
}
/*
** The extra parameters are used to convert the existing points from one unit
**  to the other.
**  void psy_dsp_envelope_mode(psy_dsp_Envelope* self,
** 	const Mode::Type _mode, const int bpm = 125, const int tpb = 24, const int onemilli = 1);
*/

/* Only meaningful when used as adsr */
/* INLINE void psy_dsp_envelope_AttackTime(int time, bool rezoom = true); */
INLINE psy_dsp_seconds_t psy_dsp_envelope_attacktime(const
	psy_dsp_Envelope* self)
{
	assert(self);

	return psy_dsp_envelope_time(self, 1);	
}

INLINE void psy_dsp_envelope_set_attack_time(
	psy_dsp_Envelope* self, psy_dsp_seconds_t time)
{
	assert(self);

	psy_dsp_envelope_settime(self, 1, time);
}

INLINE psy_dsp_seconds_t psy_dsp_envelope_decay_time(const
	psy_dsp_Envelope* self)
{
	assert(self);

	return psy_dsp_envelope_time(self, 2) -
		psy_dsp_envelope_time(self, 1);	
}

INLINE void psy_dsp_envelope_set_decay_time(
	psy_dsp_Envelope* self, psy_dsp_seconds_t time)
{
	assert(self);

	psy_dsp_envelope_settime(self, 2,
		psy_dsp_envelope_attacktime(self) + time);
}

INLINE void psy_dsp_envelope_set_sustain_value(
	psy_dsp_Envelope* self, double sustain)
{
	assert(self);

	psy_dsp_envelope_set_value(self, 2, sustain);
}

INLINE double psy_dsp_envelope_sustain_value(const
	psy_dsp_Envelope* self)
{
	assert(self);

	return psy_dsp_envelope_value(self, 2);
}

INLINE psy_dsp_seconds_t psy_dsp_envelope_release_time(const
	psy_dsp_Envelope* self)
{
	assert(self);

	return psy_dsp_envelope_time(self, 3) -
		psy_dsp_envelope_time(self, 2);
}

INLINE void psy_dsp_envelope_set_release_time(
	psy_dsp_Envelope* self, psy_dsp_seconds_t time)
{
	assert(self);

	psy_dsp_envelope_settime(self, 3,
		psy_dsp_envelope_attacktime(self) +
		psy_dsp_envelope_decay_time(self) +
		time);
}

void psy_dsp_envelope_set_adsr(psy_dsp_Envelope*,
	psy_dsp_seconds_t attack,
	psy_dsp_seconds_t decay,
	double sustain,
	psy_dsp_seconds_t release);

/* 
** psy_dsp_EnvelopeController
**
** used by lua plugins and sampler ps1
** (sampulse has an own controller: xmenvelope_controller)
*/
typedef struct psy_dsp_EnvelopeController {
	int rsvd;
	psy_dsp_Envelope settings;	
	double samplerate;
	double bpm;
	int tpb;
	psy_List* currstage;	
	psy_List* susbeginstage;
	psy_List* susendstage;
	double startpeak;
	double value;
	double step;	
	uintptr_t samplecount;
	uintptr_t nexttime;
	bool susdone;
	bool fastrelease;		
} psy_dsp_EnvelopeController;

void psy_dsp_envelopecontroller_init(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_init_envelope(psy_dsp_EnvelopeController*,
	const psy_dsp_Envelope*);
void psy_dsp_envelopecontroller_init_adsr(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_dispose(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_reset(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_set_settings(psy_dsp_EnvelopeController*,
	const psy_dsp_Envelope*);
void psy_dsp_envelopecontroller_set_time_and_value(psy_dsp_EnvelopeController*,
	uintptr_t pointindex, psy_dsp_seconds_t pointtime, double pointval);
void psy_dsp_envelopecontroller_set_value(psy_dsp_EnvelopeController*,
	uintptr_t pointindex, double pointval);
psy_dsp_EnvelopePoint psy_dsp_envelopecontroller_at(const
	psy_dsp_EnvelopeController*, uintptr_t pointindex);
psy_List*  psy_dsp_envelopecontroller_begin(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_set_sample_rate(psy_dsp_EnvelopeController*,
	double samplerate);
void psy_dsp_envelopecontroller_updatespeed(psy_dsp_EnvelopeController*,
	uintptr_t tpb, double bpm);
double psy_dsp_envelopecontroller_tick(psy_dsp_EnvelopeController*);
double psy_dsp_envelopecontroller_tick_ps1(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_start(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_stop(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_release(psy_dsp_EnvelopeController*);
void psy_dsp_envelopecontroller_fast_release(psy_dsp_EnvelopeController*);

INLINE bool psy_dsp_envelopecontroller_releasing(psy_dsp_EnvelopeController* self)
{
	return (self->settings.points && self->currstage == self->settings.points->tail);
}

INLINE bool psy_dsp_envelopecontroller_playing(psy_dsp_EnvelopeController* self)
{		
	return self->currstage != NULL;	
}

#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_ENVELOPE_H */
