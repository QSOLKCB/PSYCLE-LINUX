/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_dsp_INTERPOLATE_H
#define psy_dsp_INTERPOLATE_H

#include "dsptypes.h"

#include "../../detail/psydef.h"

#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

double psy_dsp_hermite_curve_interpolate(intptr_t kf0, intptr_t kf1, intptr_t kf2,
	intptr_t kf3, intptr_t curposition, intptr_t maxposition, double tangmult,
	bool interpolation);

#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_INTERPOLATE_H */
