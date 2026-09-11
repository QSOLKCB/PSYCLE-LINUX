/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNHOSTCMDS_H)
#define PATTERNHOSTCMDS_H

/* audio */
#include <pattern.h>
#include <sequencecmds.h>
/* driver */
#include "../../driver/eventdriver.h"

#ifdef __cplusplus
extern "C" {
#endif

struct PatternViewState;
struct psy_DiskOp;

/*!
** @struct PatternCmds
** @brief Maps the InputHandler Host Events and executes Sequence cmds with
**        undoredo
*/

typedef struct PatternCmds {	
	/* references */		
	struct psy_DiskOp* disk_op;
	struct PatternViewState* state;
} PatternCmds;

void patterncmds_init(PatternCmds*, struct PatternViewState*,
	struct psy_DiskOp*);
	
bool patterncmds_handle_edt_command(PatternCmds*, psy_EventDriverCmd);

void patterncmds_swing_fill(PatternCmds*, psy_audio_SwingFill, bool track_mode);

#ifdef __cplusplus
}
#endif

#endif /* PATTERNHOSTCMDS_H */
