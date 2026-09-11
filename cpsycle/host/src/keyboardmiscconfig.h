/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(KEYBOARDMISCCONFIG_H)
#define KEYBOARDMISCCONFIG_H

/* host */
#include "metronomeconfig.h"
#include "predefsconfig.h"
#include "seqeditconfig.h"
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct KeyboardMiscConfig
*/
typedef struct KeyboardMiscConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal */	
	PredefsConfig predefs_;
	SeqEditConfig seqedit_;
	MetronomeConfig metronome_;
} KeyboardMiscConfig;

void keyboardmiscconfig_init(KeyboardMiscConfig*, struct psy_Property* parent);
void keyboardmiscconfig_dispose(KeyboardMiscConfig*);

INLINE psy_Configuration* keyboardmiscconfig_base(KeyboardMiscConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* KEYBOARDMISCCONFIG_H */
