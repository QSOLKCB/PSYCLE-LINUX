// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net

#ifndef psy_dsp_OPERATIONS_H
#define psy_dsp_OPERATIONS_H

#include "../../detail/psydef.h"
#include "../../detail/os.h"

#include "dsptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	void* (*memory_alloc)(size_t count, size_t size);
	void (*memory_dealloc)(void* address);
	void (*add)(float *src, float *dst, uintptr_t num,
		float vol);
	void (*mul)(float *dst, uintptr_t numSamples, float mul);
	void (*movmul)(float *src, float *dst, intptr_t num,
		float mul);
	void (*clear)(float *dst, uintptr_t num);
	float* (*crop)(float* src, uintptr_t offset, uintptr_t num);	
	void (*interleave)(float* dst, float* left,
		float* right, uintptr_t num);
	void (*erase_all_nans_infinities_and_denormals)(float* dst,
		uintptr_t num);
	float (*maxvol)(const float* src, uintptr_t num);
	void (*accumulate)(double* accumleft, 
		double* accumright, 
		const float* __restrict pSamplesL,
		const float* __restrict pSamplesR,
		int count);
	void (*reverse)(float* dst, uintptr_t num);	
} psy_dsp_Operations;

extern psy_dsp_Operations dsp;

void psy_dsp_noopt_init(psy_dsp_Operations*);
void psy_dsp_sse2_init(psy_dsp_Operations*);

void psy_dsp_init(void);


// (Fideloop's)
INLINE void psy_dsp_normalize(psy_dsp_Operations* self, float* dst,
	uintptr_t numframes)
{
	if (numframes > 0) {
		float maxL = 0;
		double ratio = 0;

		maxL = self->maxvol(dst, numframes);
		if (maxL > 0.0) {
			ratio = (double)32767 / maxL;
		}
		if (ratio != 1) {
			self->mul(dst, numframes, (float)ratio);
		}
	}
}

INLINE void psy_dsp_fade(psy_dsp_Operations* self, float* dst,
	uintptr_t numframes, float startvol, float endvol)
{
	uintptr_t j;
	double slope;

	slope = ((double)endvol - (double)startvol) / (double)(numframes);
	for (j = 0; j < numframes; ++j) {
		dst[j] *= (float)(startvol + j * slope);
	}
}

INLINE void dsp_erase_all_nans_infinities_and_denormals_double(
	psy_dsp_Operations* self, double* dst, uintptr_t num)
{
	uintptr_t i;

	for (i = 0; i < num; ++i) {
		float val;

		val = (float)dst[i];
		dsp.erase_all_nans_infinities_and_denormals(&val, 1);
		dst[i] = val;
	}
}
#ifdef __cplusplus
}
#endif

#endif /* psy_dsp_OPERATIONS_H */
