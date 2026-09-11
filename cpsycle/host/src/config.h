/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CONFIG_H)
#define CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* host */
#include "globalconfig.h"
#include "keyboardmiscconfig.h"
#include "visualconfig.h"
/* file */
#include <dirconfig.h>
/* audio */
#include <audioconfig.h>
#include <midiconfig.h>
#include <eventdriverconfig.h>
#include <player.h>


struct psy_DiskOp;

/*!
** @struct PsycleConfig
** @brief Configures the psycle host
*/
typedef struct PsycleConfig {
	/*! @implements */
	psy_Configuration configuration;
	/*! @internal */
	psy_Property config;
	GlobalConfig global;		
	VisualConfig visual;
	psy_audio_AudioConfig audio;
	EventDriverConfig input;	
	psy_DirConfig directories;		
	KeyboardMiscConfig misc;
	psy_audio_MidiViewConfig midi;
	char* ini_path;
	/* references */
	struct psy_DiskOp* disk_op;
} PsycleConfig;

void psycleconfig_init(PsycleConfig*, struct psy_DiskOp*);
void psycleconfig_dispose(PsycleConfig*);

void psycleconfig_load(PsycleConfig*);
void psycleconfig_save(PsycleConfig*);

INLINE psy_Configuration* psycleconfig_global(PsycleConfig* self)
{
    return globalconfig_base(&self->global);
}

INLINE psy_Configuration* psycleconfig_general(PsycleConfig* self)
{
    return globalconfig_general(&self->global);
}

INLINE const psy_Configuration* psycleconfig_general_const(
    const PsycleConfig* self)
{
    return globalconfig_general_const(&self->global);
}

INLINE psy_Configuration* psycleconfig_audio(PsycleConfig* self)
{	
	return psy_audio_audioconfig_base(&self->audio);
}

INLINE psy_Configuration* psycleconfig_input(PsycleConfig* self)
{
	return eventdriverconfig_base(&self->input);
}

INLINE psy_Configuration* psycleconfig_language(PsycleConfig* self)
{
	return globalconfig_lang(&self->global);
}

INLINE psy_Configuration* psycleconfig_directories(PsycleConfig* self)
{
	return psy_dirconfig_base(&self->directories);
}

INLINE psy_Configuration* psycleconfig_visual(PsycleConfig* self)
{
	return visualconfig_base(&self->visual);
}

INLINE psy_Configuration* psycleconfig_patview(PsycleConfig* self)
{
	return patternviewconfig_base(&self->visual.patview);
}

INLINE psy_Configuration* psycleconfig_macview(PsycleConfig* self)
{
	return machineviewconfig_base(&self->visual.macview);
}

INLINE psy_Configuration* psycleconfig_macparam(PsycleConfig* self)
{
	return machineparamconfig_base(&self->visual.macparam);
}

INLINE psy_Configuration* psycleconfig_misc(PsycleConfig* self)
{
	return keyboardmiscconfig_base(&self->misc);
}

INLINE psy_Configuration* psycleconfig_midi(PsycleConfig* self)
{
	return psy_audio_midiviewconfig_base(&self->midi);
}

INLINE psy_Configuration* psycleconfig_compat(PsycleConfig* self)
{
	return globalconfig_compat(&self->global);
}

INLINE PredefsConfig* psycleconfig_predefs(PsycleConfig* self)
{
	return &self->misc.predefs_;
}

INLINE psy_Configuration* psycleconfig_metronome(PsycleConfig* self)
{
	return metronomeconfig_base(&self->misc.metronome_);
}

INLINE psy_Configuration* psycleconfig_seqedit(PsycleConfig* self)
{
	return seqeditconfig_base(&self->misc.seqedit_);
}

INLINE psy_Configuration* psycleconfig_base(PsycleConfig* self)
{
	assert(self);

	return &self->configuration;
}

#ifdef __cplusplus
}
#endif

#endif /* CONFIG_H */
