/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEWCONFIG_H)
#define PATTERNVIEWCONFIG_H

#include "patternviewtheme.h"
/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct PatternViewConfig
*/
typedef struct PatternViewConfig {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal */	
	PatternViewTheme theme;	
	bool singlemode;
} PatternViewConfig;

void patternviewconfig_init(PatternViewConfig*, struct psy_Property* parent);
void patternviewconfig_dispose(PatternViewConfig*);

INLINE psy_Configuration* patternviewconfig_base(PatternViewConfig* self)
{
	assert(self);

	return psy_customconfiguration_base(&self->configuration);
}

#ifdef __cplusplus
}
#endif

#endif /* PATTERNVIEWCONFIG_H */
