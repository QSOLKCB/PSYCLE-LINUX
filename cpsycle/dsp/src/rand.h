/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_dsp_RAND_H
#define psy_dsp_RAND_H

#include "../../detail/stdint.h"
#include "../../detail/os.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static int16_t psy_rand16(void)
{
#ifdef DIVERSALIS__OS__MICROSOFT
	return rand();
#else
	return ((int16_t)(32767.0 * ((double)rand() / (double)RAND_MAX)));
#endif
}

#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_RAND_H */
