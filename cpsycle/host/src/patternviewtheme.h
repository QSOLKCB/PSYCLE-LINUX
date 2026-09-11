/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEWTHEME_H)
#define PATTERNVIEWTHEME_H

/* container */
#include <configuration.h>

#ifdef __cplusplus
extern "C" {
#endif

/*! 
** @struct PatternViewTheme
*/
typedef struct PatternViewTheme {
	/*! @extends */
	psy_CustomConfiguration configuration;
	/*! @internal !*/
	struct psy_Property* theme;
	struct psy_Property* header_skin;
} PatternViewTheme;

void patternviewtheme_init(PatternViewTheme*, struct psy_Property* parent);
void patternviewtheme_dispose(PatternViewTheme*);

#ifdef __cplusplus
}
#endif

#endif /* PATTERNVIEWTHEME_H */
