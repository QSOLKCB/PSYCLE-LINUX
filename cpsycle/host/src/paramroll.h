/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PARAMROLL_H)
#define PARAMROLL_H

#include "../../detail/psyconf.h"

#ifdef PSYCLE_USE_PIANOROLL

/* host */
#include "patternhostcmds.h"
#include "pianogridstate.h"
/* ui */
#include <uiscrollbar.h>
#include <uisvg.h>


#ifdef __cplusplus
extern "C" {
#endif

/*! 
** @struct ParamRuler
*/
typedef struct ParamRuler {
	/*! @extends  */
	psy_ui_Component component;	
} ParamRuler;

void paramruler_init(ParamRuler*, psy_ui_Component* parent);

INLINE psy_ui_Component* paramruler_base(ParamRuler* self)
{
	assert(self);

	return &self->component;
}

/*! 
** @struct ParamDraw
*/
typedef struct ParamDraw {
	/*! @extends  */
	psy_ui_Component component;
	bool tweaking_;
	psy_audio_SequenceCursor tweak_cursor_;
	psy_ui_SVG svg;
	/* references */
	PianoGridState* state_;	
	psy_audio_PatternNode* tweak_node_;
	psy_audio_Pattern* tweak_pattern_;
} ParamDraw;

void paramdraw_init(ParamDraw*, psy_ui_Component* parent,
	PianoGridState*);

INLINE psy_ui_Component* paramdraw_base(ParamDraw* self)
{
	assert(self);

	return &self->component;
}

/*!
** @struct ParamRoll
*/
typedef struct ParamRoll {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	psy_ui_Component left_;
	ParamRuler ruler_;
	psy_ui_Component pane_;
	ParamDraw draw_;
} ParamRoll;

void paramroll_init(ParamRoll*, psy_ui_Component* parent,
	PianoGridState*);

INLINE psy_ui_Component* paramroll_base(ParamRoll* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* PSYCLE_USE_PIANOROLL */

#endif /* PARAMROLL_H */
