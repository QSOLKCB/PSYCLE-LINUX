/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_MIDICONFIG_H
#define psy_audio_MIDICONFIG_H

#include "../../detail/psydef.h"


/* container */
#include <configuration.h>
#include <list.h>
#include <properties.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	psy_audio_MIDICONFIG_MS_USE_SELECTED = 0,
	psy_audio_MIDICONFIG_MS_BANK,
	psy_audio_MIDICONFIG_MS_PROGRAM,
	psy_audio_MIDICONFIG_MS_MIDI_CHAN
} psy_audio_midiconfig_selector_t;

enum {
	psy_audio_MIDICONFIG_T_COMMAND = 0,
	psy_audio_MIDICONFIG_T_TWEAK,
	psy_audio_MIDICONFIG_T_TWEAKSLIDE,
	psy_audio_MIDICONFIG_T_MCM,
	psy_audio_MIDICONFIG_NUM_TYPES
};

enum {
	psy_audio_MIDICONFIG_GT_VELOCITY,
	psy_audio_MIDICONFIG_GT_PITCHWHEEL,
	psy_audio_MIDICONFIG_GT_CUSTOM		
};


typedef struct psy_audio_MidiConfigGroup {
	intptr_t grouptype;
	bool record;
	intptr_t type;
	intptr_t command;
	intptr_t from;
	intptr_t to;
	intptr_t message;
	char encode[64];
} psy_audio_MidiConfigGroup;

void psy_audio_midiconfiggroup_init(psy_audio_MidiConfigGroup*, intptr_t grouptype,
	intptr_t command);

psy_audio_MidiConfigGroup* psy_audio_midiconfiggroup_alloc(void);
psy_audio_MidiConfigGroup* psy_audio_midiconfiggroup_allocinit(intptr_t grouptype,
	intptr_t command);

INLINE bool psy_audio_midiconfiggroup_record(const psy_audio_MidiConfigGroup* self)
{
	assert(self);

	return self->record;
}

INLINE intptr_t psy_audio_midiconfiggroup_type(const psy_audio_MidiConfigGroup* self)
{
	assert(self);

	return self->type;
}

INLINE intptr_t psy_audio_midiconfiggroup_command(const psy_audio_MidiConfigGroup* self)
{ 
	assert(self);

	return self->command;
}
INLINE intptr_t psy_audio_midiconfiggroup_from(const psy_audio_MidiConfigGroup* self)
{
	assert(self);

	return self->from;
}

INLINE intptr_t psy_audio_midiconfiggroup_to(const psy_audio_MidiConfigGroup* self)
{
	assert(self);

	return self->to;
}

const char* psy_audio_midiconfiggroup_tostring(const psy_audio_MidiConfigGroup* self);


struct MidiViewConfig;

typedef struct psy_audio_MidiConfig {
	psy_List* groups;	
	bool raw;
	psy_audio_midiconfig_selector_t	gen_select_with;
	psy_audio_midiconfig_selector_t	inst_select_with;
	psy_Configuration* cfg;
} psy_audio_MidiConfig;

void psy_audio_midiconfig_init(psy_audio_MidiConfig*, psy_Configuration*);
void psy_audio_midiconfig_dispose(psy_audio_MidiConfig*);
void psy_audio_midiconfig_reset(psy_audio_MidiConfig*);

void psy_audio_midiconfig_add_controller(psy_audio_MidiConfig*,
	psy_audio_MidiConfigGroup group);
void psy_audio_midiconfig_remove_controller(psy_audio_MidiConfig* self, intptr_t id);
char_dyn_t* psy_audio_midiconfig_controllers_to_string(const psy_audio_MidiConfig* self);

INLINE uintptr_t psy_audio_midiconfig_numcontrollers(const
	psy_audio_MidiConfig* self)
{
	assert(self);

	return psy_list_size(self->groups);
}

INLINE psy_audio_MidiConfigGroup* psy_audio_midiconfig_at(const
	psy_audio_MidiConfig* self, uintptr_t index)
{
	psy_List* i;

	assert(self);

	i = psy_list_at(self->groups, index);
	if (i) {
		return (psy_audio_MidiConfigGroup*)psy_list_entry(i);
	}
	return NULL;
}

bool psy_audio_midiconfig_read_controllers(psy_audio_MidiConfig*);
void psy_audio_midiconfig_write_controllers(psy_audio_MidiConfig* self);

/*!
** @struct psy_audio_MidiViewConfig
*/
typedef struct psy_audio_MidiViewConfig {
	/*! extends */
	psy_CustomConfiguration configuration;
	/*! @internal */
	psy_Property* controllers;	
} psy_audio_MidiViewConfig;

void psy_audio_midiviewconfig_init(psy_audio_MidiViewConfig*, psy_Property* parent);
void psy_audio_midiviewconfig_dispose(psy_audio_MidiViewConfig*);

INLINE psy_Configuration* psy_audio_midiviewconfig_base(psy_audio_MidiViewConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_audio_MIDICONFIG */
