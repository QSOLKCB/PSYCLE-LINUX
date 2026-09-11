/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "sequenceentry.h"
/* local */
#include "patterns.h"
/* platform */
#include "../../detail/portable.h"


/* psy_audio_SequenceEntry */

static void psy_audio_static_sequenceentry_dispose(
	psy_audio_SequenceEntry* self)
{
}

static psy_audio_SequenceEntry* psy_audio_static_sequenceentry_clone(
		const psy_audio_SequenceEntry* self)
{
	psy_audio_SequenceEntry* rv;

	assert(self);

	rv = (psy_audio_SequenceEntry*)malloc(sizeof(psy_audio_SequenceEntry));
	if (rv) {
		psy_audio_sequenceentry_init_all(rv, self->type, self->offset);
	}
	return rv;
}

static psy_dsp_beatpos_t psy_audio_static_sequenceentry_length(
	const psy_audio_SequenceEntry* self)
{
	return psy_dsp_beatpos_zero();
}

static void psy_audio_static_sequenceentry_set_length(
	psy_audio_SequenceEntry* self, psy_dsp_beatpos_t length)
{
	assert(self);
}

static const char* psy_audio_static_sequenceentry_label(
	const psy_audio_SequenceEntry* self)
{
	assert(self);

	return "";
}

/* vtable */
static psy_audio_SequenceEntryVtable psy_audio_sequenceentry_vtable;
static bool psy_audio_sequenceentry_vtable_initialized = FALSE;

static void psy_audio_sequenceentry_vtable_init(void)
{
	if (!psy_audio_sequenceentry_vtable_initialized) {
		psy_audio_sequenceentry_vtable.dispose =
			psy_audio_static_sequenceentry_dispose;
		psy_audio_sequenceentry_vtable.clone =			
			psy_audio_static_sequenceentry_clone;
		psy_audio_sequenceentry_vtable.length =
			psy_audio_static_sequenceentry_length;
		psy_audio_sequenceentry_vtable.set_length =
			psy_audio_static_sequenceentry_set_length;
		psy_audio_sequenceentry_vtable.label =
			psy_audio_static_sequenceentry_label;
		psy_audio_sequenceentry_vtable_initialized = TRUE;
	}	
}

/* implementation */
void psy_audio_sequenceentry_init_all(psy_audio_SequenceEntry* self,
	psy_audio_SequenceEntryType type, psy_dsp_beatpos_t offset)
{
	assert(self);

	psy_audio_sequenceentry_vtable_init();
	self->vtable = &psy_audio_sequenceentry_vtable;
	self->type = type;
	self->row = 0;
	assert(offset.res != 0);
	self->offset = offset;
	self->repositionoffset = psy_dsp_beatpos_zero();
	self->selplay = 0;
}


/* psy_audio_SequencePatternEntry */

psy_audio_SequenceEntry* psy_audio_static_sequencepatternentry_clone(
	const psy_audio_SequencePatternEntry* self)
{
	psy_audio_SequencePatternEntry* rv;

	assert(self);

	rv = (psy_audio_SequencePatternEntry*)malloc(
		sizeof(psy_audio_SequencePatternEntry));
	if (rv) {		
		*rv = *self;
		return &rv->entry;
	}	
	return NULL;
}

static psy_dsp_beatpos_t psy_audio_static_sequencepatternentry_length(
	const psy_audio_SequencePatternEntry*);
static void psy_audio_static_sequencepatternentry_set_length(
	psy_audio_SequencePatternEntry*, psy_dsp_beatpos_t length);
static const char* psy_audio_static_sequencepatternentry_label(
	const psy_audio_SequencePatternEntry*);

/* vtable */
static psy_audio_SequenceEntryVtable psy_audio_sequencepatternentry_vtable;
static bool psy_audio_sequencepatternentry_vtable_initialized = FALSE;

static void psy_audio_sequencepatternentry_vtable_init(
	psy_audio_SequencePatternEntry* self)
{
	if (!psy_audio_sequencepatternentry_vtable_initialized) {
		psy_audio_sequencepatternentry_vtable = *(self->entry.vtable);
		psy_audio_sequencepatternentry_vtable.clone =
			(psy_audio_fp_sequenceentry_clone)
			psy_audio_static_sequencepatternentry_clone;
		psy_audio_sequencepatternentry_vtable.length =
			(psy_audio_fp_sequenceentry_length)
			psy_audio_static_sequencepatternentry_length;
		psy_audio_sequencepatternentry_vtable.set_length =
			(psy_audio_fp_sequenceentry_set_length)
			psy_audio_static_sequencepatternentry_set_length;
		psy_audio_sequencepatternentry_vtable.label =
			(psy_audio_fp_sequenceentry_label)
			psy_audio_static_sequencepatternentry_label;
		psy_audio_sequencepatternentry_vtable_initialized = TRUE;
	}
	self->entry.vtable = &psy_audio_sequencepatternentry_vtable;
}

/* implementation */
void psy_audio_sequencepatternentry_init(psy_audio_SequencePatternEntry* self,
	uintptr_t patternslot, psy_dsp_beatpos_t offset)
{	
	assert(self);
	assert(offset.res != 0);
	
	psy_audio_sequenceentry_init_all(&self->entry,
		psy_audio_SEQUENCEENTRY_PATTERN, offset);
	psy_audio_sequencepatternentry_vtable_init(self);
	self->patternslot = patternslot;
	self->patterns = NULL;	
}

psy_audio_SequencePatternEntry* psy_audio_sequencepatternentry_alloc(void)
{
	return (psy_audio_SequencePatternEntry*)malloc(sizeof(
		psy_audio_SequencePatternEntry));
}

psy_audio_SequencePatternEntry* psy_audio_sequencepatternentry_alloc_init(
	uintptr_t pattern, psy_dsp_beatpos_t offset)
{
	psy_audio_SequencePatternEntry* rv;

	rv = psy_audio_sequencepatternentry_alloc();
	if (rv) {
		psy_audio_sequencepatternentry_init(rv, pattern, offset);
	}
	return rv;
}

psy_dsp_beatpos_t psy_audio_static_sequencepatternentry_length(
	const psy_audio_SequencePatternEntry* self)
{
	const psy_audio_Pattern* pattern;

	assert(self);

	if (self->patterns) {
		pattern = psy_audio_patterns_at_const(self->patterns,
			psy_audio_sequencepatternentry_patternslot(self));
		if (pattern) {
			return psy_audio_pattern_length(pattern);
		}
	}
	return psy_dsp_beatpos_zero();
}

const char* psy_audio_static_sequencepatternentry_label(
	const psy_audio_SequencePatternEntry* self)
{
	const psy_audio_Pattern* pattern;

	assert(self);

	if (self->patterns) {
		pattern = psy_audio_patterns_at_const(self->patterns,
			psy_audio_sequencepatternentry_patternslot(self));
		if (pattern) {
			return psy_audio_pattern_name(pattern);
		}
	}
	return "Untitled";
}

void psy_audio_static_sequencepatternentry_set_length(
	psy_audio_SequencePatternEntry* self, psy_dsp_beatpos_t length)
{
	psy_audio_Pattern* pattern;

	assert(self);

	if (self->patterns) {
		pattern = psy_audio_patterns_at(self->patterns,
			psy_audio_sequencepatternentry_patternslot(self));
		if (pattern) {			
			psy_audio_pattern_set_length(pattern, length);			
		}
	}
}

struct psy_audio_Pattern* psy_audio_sequencepatternentry_pattern(const
	psy_audio_SequencePatternEntry* self, struct psy_audio_Patterns* patterns)
{
	assert(self);

	if (patterns) {
		return (struct psy_audio_Pattern*)psy_audio_patterns_at(patterns,
			self->patternslot);		
	}
	return NULL;
}


/* psy_audio_SequenceSampleEntry */

psy_audio_SequenceEntry* psy_audio_static_sequencesampleentry_clone(
	const psy_audio_SequenceSampleEntry* self)
{
	psy_audio_SequenceSampleEntry* rv;

	assert(self);

	rv = (psy_audio_SequenceSampleEntry*)malloc(
		sizeof(psy_audio_SequenceSampleEntry));
	if (rv) {
		*rv = *self;
		return &rv->entry;
	}
	return NULL;
}

static psy_dsp_beatpos_t psy_audio_static_sequencesampleentry_length(
	const psy_audio_SequenceSampleEntry*);
static void psy_audio_sequencesampleentry_dispose(
	psy_audio_SequenceSampleEntry*);
static const char* psy_audio_static_sequencesampleentry_label(
	const psy_audio_SequenceSampleEntry*);

/* vtable */
static psy_audio_SequenceEntryVtable psy_audio_sequencesampleentry_vtable;
static bool psy_audio_sequencesampleentry_vtable_initialized = FALSE;

static void psy_audio_sequencesampleentry_vtable_init(
	psy_audio_SequenceSampleEntry* self)
{
	if (!psy_audio_sequencesampleentry_vtable_initialized) {
		psy_audio_sequencesampleentry_vtable = *(self->entry.vtable);
		psy_audio_sequencesampleentry_vtable.dispose =
			(psy_audio_fp_sequenceentry_dispose)
			psy_audio_sequencesampleentry_dispose;
		psy_audio_sequencesampleentry_vtable.clone =
			(psy_audio_fp_sequenceentry_clone)
			psy_audio_static_sequencesampleentry_clone;
		psy_audio_sequencesampleentry_vtable.length =
			(psy_audio_fp_sequenceentry_length)
			psy_audio_static_sequencesampleentry_length;
		psy_audio_sequencesampleentry_vtable.label =
			(psy_audio_fp_sequenceentry_label)
			psy_audio_static_sequencesampleentry_label;
		psy_audio_sequencesampleentry_vtable_initialized = TRUE;
	}
	self->entry.vtable = &psy_audio_sequencesampleentry_vtable;
}

/* implementation */
void psy_audio_sequencesampleentry_init(psy_audio_SequenceSampleEntry* self,	
	psy_dsp_beatpos_t offset, psy_audio_SampleIndex sampleindex)
{
	assert(self);

	psy_audio_sequenceentry_init_all(&self->entry,
		psy_audio_SEQUENCEENTRY_SAMPLE, offset);
	psy_audio_sequencesampleentry_vtable_init(self);	
	self->samples = NULL;
	self->sampleindex = sampleindex;
	self->samplerindex = psy_INDEX_INVALID;
}

void psy_audio_sequencesampleentry_dispose(psy_audio_SequenceSampleEntry* self)
{	
	assert(self);
}

psy_audio_SequenceSampleEntry* psy_audio_sequencesampleentry_alloc(void)
{
	return (psy_audio_SequenceSampleEntry*)malloc(sizeof(
		psy_audio_SequenceSampleEntry));
}

psy_audio_SequenceSampleEntry* psy_audio_sequencesampleentry_alloc_init(
	psy_dsp_beatpos_t offset, psy_audio_SampleIndex sampleindex)
{
	psy_audio_SequenceSampleEntry* rv;

	rv = psy_audio_sequencesampleentry_alloc();
	if (rv) {
		psy_audio_sequencesampleentry_init(rv, offset, sampleindex);
	}
	return rv;
}

psy_dsp_beatpos_t psy_audio_static_sequencesampleentry_length(
	const psy_audio_SequenceSampleEntry* self)
{
	const psy_audio_Sample* sample;

	assert(self);

	if (self->samples) {
		sample = psy_audio_samples_at_const(self->samples,
			psy_audio_sequencesampleentry_samplesindex(self));
		if (sample) {
			double beatspersample;

			beatspersample = (125.0 * 1.0) / (44100.0 * 60.0);
			return psy_dsp_beatpos_make_real(
				psy_audio_sample_num_frames(sample) *
					beatspersample * 44100.0 / sample->samplerate,
				psy_dsp_DEFAULT_PPQ);
		}
	}
	return psy_dsp_beatpos_make_real(16.0, psy_dsp_DEFAULT_PPQ);
}

const char* psy_audio_static_sequencesampleentry_label(
	const psy_audio_SequenceSampleEntry* self)
{
	const psy_audio_Sample* sample;

	assert(self);

	if (self->samples) {
		sample = psy_audio_samples_at_const(self->samples,
			psy_audio_sequencesampleentry_samplesindex(self));
		if (sample) {
			return sample->name;			
		}
	}
	return "";
}

/* psy_audio_SequenceMarkerEntry */

static const char* psy_audio_static_sequencemarkerentry_label(
	const psy_audio_SequenceMarkerEntry*);

static void psy_audio_static_sequencemarkerentry_dispose(
	psy_audio_SequenceMarkerEntry*);

static psy_audio_SequenceEntry* psy_audio_static_sequencemarkerentry_clone(
	const psy_audio_SequenceMarkerEntry* self)
{
	psy_audio_SequenceMarkerEntry* rv;

	assert(self);

	rv = (psy_audio_SequenceMarkerEntry*)malloc(sizeof(
		psy_audio_SequenceMarkerEntry));
	if (rv) {
		*rv = *self;
		rv->text = psy_strdup(self->text);
		return &rv->entry;
	}
	return NULL;
}

static psy_dsp_beatpos_t psy_audio_static_sequencemarkerentry_length(
		const psy_audio_SequenceMarkerEntry* self)
{
	assert(self);

	return self->length;
}

static void psy_audio_static_sequencemarkerentry_set_length(
		psy_audio_SequenceMarkerEntry* self, psy_dsp_beatpos_t length)
{
	assert(self);

	self->length = length;
}

/* vtable */
static psy_audio_SequenceEntryVtable psy_audio_sequencemarkerentry_vtable;
static bool psy_audio_sequencemarkerentry_vtable_initialized = FALSE;

static void psy_audio_sequencemarkerentry_vtable_init(
	psy_audio_SequenceMarkerEntry* self)
{
	if (!psy_audio_sequencemarkerentry_vtable_initialized) {
		psy_audio_sequencemarkerentry_vtable = *(self->entry.vtable);
		psy_audio_sequencemarkerentry_vtable.dispose =
			(psy_audio_fp_sequenceentry_dispose)
			psy_audio_static_sequencemarkerentry_dispose;
		psy_audio_sequencemarkerentry_vtable.clone =
			(psy_audio_fp_sequenceentry_clone)
			psy_audio_static_sequencemarkerentry_clone;
		psy_audio_sequencemarkerentry_vtable.length =
			(psy_audio_fp_sequenceentry_length)
			psy_audio_static_sequencemarkerentry_length;
		psy_audio_sequencemarkerentry_vtable.set_length =
			(psy_audio_fp_sequenceentry_set_length)
			psy_audio_static_sequencemarkerentry_set_length;
		psy_audio_sequencemarkerentry_vtable.label =
			(psy_audio_fp_sequenceentry_label)
			psy_audio_static_sequencemarkerentry_label;
		psy_audio_sequencemarkerentry_vtable_initialized = TRUE;
	}
	self->entry.vtable = &psy_audio_sequencemarkerentry_vtable;
}
/* implementation */
void psy_audio_sequencemarkerentry_init(psy_audio_SequenceMarkerEntry* self,
	psy_dsp_beatpos_t offset, const char* text)
{
	assert(self);

	psy_audio_sequenceentry_init_all(&self->entry,
		psy_audio_SEQUENCEENTRY_MARKER, offset);
	psy_audio_sequencemarkerentry_vtable_init(self);
	self->text = psy_strdup(text);
	self->length = psy_dsp_beatpos_make_real(16.0, psy_dsp_DEFAULT_PPQ);
}

void psy_audio_static_sequencemarkerentry_dispose(
	psy_audio_SequenceMarkerEntry* self)
{
	assert(self);

	free(self->text);
	self->text = NULL;
}

psy_audio_SequenceMarkerEntry* psy_audio_sequencemarkerentry_alloc(void)
{
	return (psy_audio_SequenceMarkerEntry*)malloc(sizeof(
		psy_audio_SequenceMarkerEntry));
}

psy_audio_SequenceMarkerEntry* psy_audio_sequencemarkerentry_alloc_init(
	psy_dsp_beatpos_t offset, const char* text)
{
	psy_audio_SequenceMarkerEntry* rv;

	rv = psy_audio_sequencemarkerentry_alloc();
	if (rv) {
		psy_audio_sequencemarkerentry_init(rv, offset, text);
	}
	return rv;
}

const char* psy_audio_static_sequencemarkerentry_label(
	const psy_audio_SequenceMarkerEntry* self)
{
	assert(self);

	return self->text;
}

void psy_audio_sequencemarkerentry_set_text(psy_audio_SequenceMarkerEntry* self,
	const char* text)
{
	assert(self);

	psy_strreset(&self->text, text);
}
