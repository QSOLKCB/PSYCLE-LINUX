/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(STEPSEQUENCERVIEW_H)
#define STEPSEQUENCERVIEW_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_STEPSEQUENCER

/* host */
#include "closebar.h"
#include "knobui.h"
#include "switchui.h"
#include "workspace.h"
/* audio */
#include <pattern.h>
/* ui */
#include <uilabel.h>
#include <uislider.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** StepSequencerView
*/

struct StepSequencerNoteState;

typedef enum StepSequencerShift {
	STEPSEQUENCERSHIFT_SELECT = 1,	
	STEPSEQUENCERSHIFT_LAST_STEP = 3,
	STEPSEQUENCERSHIFT_CLEAR = 12
} StepSequencerShift;


typedef struct LastStepMachineParam {
	psy_audio_MachineParam machineparam;	
	struct StepSequencerLastStepState* state;	
} LastStepMachineParam;

void laststepmachineparam_init(LastStepMachineParam*, struct StepSequencerLastStepState*);
void laststepmachineparam_dispose(LastStepMachineParam*);

INLINE psy_audio_MachineParam* laststepmachineparam_base(LastStepMachineParam* self)
{
	return &(self->machineparam);
}

struct StepsequencerBar;

/* StepSequencerState */

struct StepSequencerState;

typedef void (*fp_stepsequencerstate)(struct StepSequencerState*);
typedef struct StepSequencerState* (*fp_stepsequencerstate_tile_press)(struct StepSequencerState*,
	uintptr_t step);
typedef struct StepSequencerState* (*fp_stepsequencerstate_shift_press)(struct StepSequencerState*);
typedef psy_audio_MachineParam* (*fp_stepsequencerstate_param)(
	struct StepSequencerState*, uintptr_t idx);

typedef struct psy_StepSequencerStateVtable {
	fp_stepsequencerstate dispose;
	fp_stepsequencerstate enter;
	fp_stepsequencerstate_tile_press tile_press;
	fp_stepsequencerstate_shift_press shift_press;
	fp_stepsequencerstate update;
	fp_stepsequencerstate update_playline;	
	fp_stepsequencerstate_param param;	
} StepSequencerStateVtable;

typedef struct StepSequencerState {
	StepSequencerStateVtable* vtable;
} StepSequencerState;

void stepsequencerstate_init(StepSequencerState*);


INLINE void stepsequencerstate_dispose(StepSequencerState* self)
{
	assert(self);
	
	self->vtable->dispose(self);
}

INLINE void stepsequencerstate_enter(StepSequencerState* self)
{
	assert(self);
	
	self->vtable->enter(self);
}

INLINE StepSequencerState* stepsequencerstate_tile_press(
	StepSequencerState* self, uintptr_t step)
{
	assert(self);
	
	return self->vtable->tile_press(self, step);
}

INLINE StepSequencerState* stepsequencerstate_shift_press(
	StepSequencerState* self)
{
	assert(self);
	
	return self->vtable->shift_press(self);
}

INLINE void stepsequencerstate_update(StepSequencerState* self)
{
	assert(self);
	
	self->vtable->update(self);
}

INLINE void stepsequencerstate_update_playline(StepSequencerState* self)
{
	assert(self);
	
	self->vtable->update_playline(self);
}

INLINE psy_audio_MachineParam* stepsequencerstate_param(
	StepSequencerState* self, uintptr_t index)
{
	assert(self);
	
	return self->vtable->param(self, index);
}

struct StepsequencerStates;
struct StepsequencerTiles;

/* StepSequencerNoteState */
typedef struct StepSequencerNoteState {	
	StepSequencerState state;	
	uintptr_t stepgroup;	
	psy_audio_ChoiceMachineParam param_stepgroup;
	psy_audio_IntMachineParam param_note;
	int32_t note;
	/* references */
	Workspace* workspace;
	struct StepsequencerTiles* tiles;
	struct StepsequencerStates* states;
} StepSequencerNoteState;

void stepsequencernotestate_init(StepSequencerNoteState*,
	struct StepsequencerStates* states, Workspace*);

psy_audio_Pattern* stepsequencernotestate_pattern(StepSequencerNoteState*);

/* StepSequencerLastStepState */
typedef struct StepSequencerLastStepState {	
	StepSequencerState state;	
	int32_t stepgroup;	
	psy_audio_ChoiceMachineParam param_stepgroup;	
	LastStepMachineParam param_note;
	/* references */
	Workspace* workspace;
	struct StepsequencerTiles* tiles;
	struct StepsequencerStates* states;
} StepSequencerLastStepState;

void stepsequencerlaststepstate_init(StepSequencerLastStepState*,
	struct StepsequencerStates*, Workspace*);

psy_audio_Pattern* stepsequencerlaststepstate_pattern(StepSequencerLastStepState*);

/* StepSequencerClearState */
typedef struct StepSequencerClearState {	
	StepSequencerState state;	
	uintptr_t stepgroup;	
	uintptr_t start;	
	psy_audio_SequenceCursor start_cursor;	
	psy_audio_ChoiceMachineParam param_stepgroup;	
	// ClearMachineParam param_note;
	/* references */
	Workspace* workspace;
	struct StepsequencerTiles* tiles;
	struct StepsequencerStates* states;
} StepSequencerClearState;

void stepsequencerclearstate_init(StepSequencerClearState*,
	struct StepsequencerStates*, Workspace*);

psy_audio_Pattern* stepsequencerclearstate_pattern(StepSequencerClearState*);


/* StepSequencerShiftState */
typedef struct StepSequencerShiftState {
	/*! @extends  */
	StepSequencerState state;			
	psy_audio_IntMachineParam param_note;
	/* references */
	Workspace* workspace;
	struct StepsequencerTiles* tiles;
	struct StepsequencerStates* states;
} StepSequencerShiftState;

void stepsequencershiftstate_init(StepSequencerShiftState*,
	struct StepsequencerStates*, Workspace*);

/* StepSequencerTile */
typedef struct StepSequencerTile {
	/*! @extends  */
	psy_ui_Component component;
	psy_ui_Component client;
	psy_ui_Component button;
	psy_ui_Label label;
} StepSequencerTile;

void stepsequencertile_init(StepSequencerTile*, psy_ui_Component* parent);

StepSequencerTile* stepsequencertile_alloc(void);
StepSequencerTile* stepsequencertile_allocinit(
	psy_ui_Component* parent);

void stepsequencertile_turn_on(StepSequencerTile*);
void stepsequencertile_turn_off(StepSequencerTile*);
void stepsequencertile_play(StepSequencerTile*);
void stepsequencertile_reset_play(StepSequencerTile*);
void stepsequencertile_set_text(StepSequencerTile*, const char* text);

INLINE psy_ui_Component* stepsequencertile_base(StepSequencerTile* self)
{
	return &self->component;
}

/* StepsequencerTiles */

struct StepsequencerView;

typedef struct StepsequencerTiles {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	psy_Table tiles;	
	/* references */
	StepSequencerState* state;	
	struct StepsequencerView* view;
} StepsequencerTiles;

void stepsequencertiles_init(StepsequencerTiles*, psy_ui_Component* parent,
	struct StepsequencerView*, StepSequencerState*);

void stepsequencertiles_enable_shift(StepsequencerTiles*);
void stepsequencertiles_disable_shift(StepsequencerTiles*);
	
INLINE void stepsequencertiles_set_state(StepsequencerTiles* self,
	StepSequencerState* state)
{
	self->state = state;
}

INLINE psy_ui_Component* stepsequencertiles_base(StepsequencerTiles* self)
{
	return &self->component;
}


/* StepSequencerOptions */
typedef struct StepSequencerOptions {
	/*! @extends  */	
	psy_ui_Component component;	
	psy_ui_Component left;
	psy_ui_Component client;
	psy_ui_Button shift;
	KnobUi note;
	KnobUi stepgroup;	
} StepSequencerOptions;

void stepsequenceroptions_init(StepSequencerOptions*, psy_ui_Component* parent,
	StepSequencerState*);
	
void stepsequenceroptions_set_state(StepSequencerOptions*, StepSequencerState*);

INLINE psy_ui_Component* stepsequenceroptions_base(StepSequencerOptions* self)
{
	return &self->component;
}

typedef struct StepsequencerStates {
	StepSequencerOptions options;
	StepSequencerNoteState note_state;
	StepSequencerShiftState shift_state;
	StepSequencerLastStepState laststep_state;
	StepSequencerClearState clear_state;
} StepsequencerStates;

void stepsequencerstates_init(StepsequencerStates*, Workspace*);

/*!
** @struct StepsequencerView
*/
typedef struct StepsequencerView {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Component tiles_row_;
	CloseBar close_bar_;
	StepsequencerTiles tiles_;
	StepSequencerOptions options_;
	StepsequencerStates states;
	StepSequencerState* state_;	
} StepsequencerView;

void stepsequencerview_init(StepsequencerView*, psy_ui_Component* parent,
	Workspace*);
void stepsequencerview_enter_state(StepsequencerView*, StepSequencerState*);

INLINE psy_ui_Component* stepsequencerview_base(StepsequencerView* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_STEPSEQUENCER */

#endif /* STEPSEQUENCERVIEW_H */
