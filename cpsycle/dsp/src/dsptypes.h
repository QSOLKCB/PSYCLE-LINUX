/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

/*!
** @file dsptypes.h 
** @brief Dsp related data types used in psycle
*/

#ifndef psy_dsp_TYPES_H
#define psy_dsp_TYPES_H

/* local */
#include "beatpos.h"
/* std */
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

#define psy_dsp_PI 3.14159265358979323846
#define psy_dsp_PI_F 3.14159265358979323846f
#define psy_dsp_e 2.71828182845904523536028747135266249775724709369995

#define psy_dsp_BPM_UNSET 0.0

typedef double psy_dsp_percent_t;
typedef double psy_dsp_seconds_t;
typedef double psy_dsp_nanoseconds_t;
typedef uintptr_t psy_dsp_frame_t;

typedef enum {
	PSY_DSP_AMP_RANGE_VST,
	PSY_DSP_AMP_RANGE_NATIVE,
	PSY_DSP_AMP_RANGE_IGNORE
} psy_dsp_amp_range_t;

#define psy_dsp_epsilon 0.001

INLINE int psy_dsp_testrange(double position,
	double interval_start, double interval_width)
{
	return (position >= interval_start && position < (interval_start + 
		interval_width));
}

/*
** C1999 *round* - converts a floating point number to an integer by rounding
** to the nearest integer. This function has the same semantic as C1999's
** *round* series of functions.
** note: it is unspecified whether rounding x.5 rounds up, down or towards the
** even integer.
*/

/* float */
INLINE int psy_dsp_roundf(float x)
{
	return (int)((x > 0) ? floorf(x + 0.5f) : ceilf(x - 0.5f));
}

/* double */
INLINE intptr_t psy_dsp_round(double x)
{
	return (intptr_t)((x > 0) ? floor(x + 0.5) : ceil(x - 0.5));
}

#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_TYPES_H */
