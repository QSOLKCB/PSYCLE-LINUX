/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

/*! \file dsptypes.h 
**  \brief Dsp related data types used in psycle
*/

#ifndef psy_dsp_BEATPOS_H
#define psy_dsp_BEATPOS_H

#include "../../detail/psydef.h"
/* std */
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define psy_dsp_DEFAULT_PPQ 480

typedef int32_t psy_dsp_ppq_t;

/*! 
** \struct psy_dsp_beatpos_t
** \brief PPQ rastered tick position to time sequencer events
*/
typedef struct psy_dsp_beatpos_t
{
	psy_dsp_ppq_t ticks;
	uint32_t res;
} psy_dsp_beatpos_t;

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_zero(void)
{	
	psy_dsp_beatpos_t rv;

	rv.ticks = 0;
	rv.res = psy_dsp_DEFAULT_PPQ;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_one(void)
{	
	psy_dsp_beatpos_t rv;

	rv.ticks = psy_dsp_DEFAULT_PPQ;
	rv.res = psy_dsp_DEFAULT_PPQ;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_make_real(double v,
	psy_dsp_ppq_t res)
{
	psy_dsp_beatpos_t rv;

	rv.ticks = (int32_t)(v * res);	
	rv.res = res;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_make_lines(uintptr_t lines,
	uintptr_t lpb, psy_dsp_ppq_t res)
{
	psy_dsp_beatpos_t rv;

	rv.ticks = (int32_t)((lines * psy_dsp_DEFAULT_PPQ) / lpb);
	rv.res = res;
	return rv;
}

INLINE double psy_dsp_beatpos_real(psy_dsp_beatpos_t pos)
{
	return pos.ticks / (double)pos.res;
}

INLINE intptr_t psy_dsp_beatpos_line(psy_dsp_beatpos_t pos, uintptr_t lpb)
{
	intptr_t rv;

	rv = (intptr_t)(psy_dsp_beatpos_real(pos) * lpb);	
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_quantize(psy_dsp_beatpos_t v,
	uintptr_t lpb)
{	
	return psy_dsp_beatpos_make_real(
		floor(psy_dsp_beatpos_real(v) * lpb) / (double)lpb,
		v.res);
}

INLINE bool psy_dsp_beatpos_less(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{	
	return (p1.ticks < p2.ticks);
}

INLINE bool psy_dsp_beatpos_less_equal(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{	
	return (p1.ticks <= p2.ticks);
}

INLINE bool psy_dsp_beatpos_equal(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{	
	return (p1.ticks == p2.ticks);
}

INLINE bool psy_dsp_beatpos_is_zero(psy_dsp_beatpos_t v)	
{	
	return (v.ticks == 0);
}

INLINE bool psy_dsp_beatpos_not_equal(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{	
	return (p1.ticks != p2.ticks);
}

INLINE bool psy_dsp_beatpos_not_zero(psy_dsp_beatpos_t v)	
{	
	return (v.ticks != 0);
}

INLINE bool psy_dsp_beatpos_greater_equal(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	return (p1.ticks >= p2.ticks);
}

INLINE bool psy_dsp_beatpos_greater(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{	
	return (p1.ticks > p2.ticks);
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_add(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;
		
	rv.ticks = p1.ticks + p2.ticks;
	rv.res = p1.res;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_mod(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;

	rv.ticks = p1.ticks % p2.ticks;
	rv.res = p1.res;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_sub(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;
		
	rv.ticks = p1.ticks - p2.ticks;
	rv.res = p1.res;
	return rv;
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_mul(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;
		
	rv.ticks = p1.ticks * p2.ticks;
	rv.res = p1.res;
	return rv;	
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_min(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;
		
	if (p1.ticks < p2.ticks) {
		rv = p1;
	} else {
		rv = p2;
	}
	rv.res = p1.res;
	return rv;	
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_max(psy_dsp_beatpos_t p1,
	psy_dsp_beatpos_t p2)
{
	psy_dsp_beatpos_t rv;
		
	if (p1.ticks > p2.ticks) {
		rv = p1;
	} else {
		rv = p2;
	}
	rv.res = p1.res;
	return rv;	
}

INLINE int psy_dsp_beatpos_testrange(psy_dsp_beatpos_t position,
	psy_dsp_beatpos_t interval_start, psy_dsp_beatpos_t interval_width)
{
	return psy_dsp_beatpos_greater_equal(position, interval_start) &&
		   psy_dsp_beatpos_less(position, 
			  psy_dsp_beatpos_add(interval_start, interval_width));
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_floor(psy_dsp_beatpos_t v)
{	
	return psy_dsp_beatpos_make_real(floor(psy_dsp_beatpos_real(v)),
		v.res);
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_ceil(psy_dsp_beatpos_t v)
{	
	return psy_dsp_beatpos_make_real(ceil(psy_dsp_beatpos_real(v)),
		v.res);
}

INLINE psy_dsp_beatpos_t psy_dsp_beatpos_neg(psy_dsp_beatpos_t v)
{	
	psy_dsp_beatpos_t rv;
	
	rv.ticks = -v.ticks;
	return rv;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_BEATPOS_H */
