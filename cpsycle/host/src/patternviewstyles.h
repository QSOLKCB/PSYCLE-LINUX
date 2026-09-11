/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(PATTERNVIEWSTYLES_H)
#define PATTERNVIEWSTYLES_H

#include "../../detail/psyconf.h"

/* container */
#include <configuration.h>


#ifdef PSYCLE_USE_TRACKERVIEW

/*!
** @struct PatternViewderStyles
** @brief Creates the pattern view styles
*/
typedef struct PatternViewStyles {
	/*! @internal */
	psy_Configuration* dir_cfg;	
} PatternViewStyles;

void patternviewstyles_init(PatternViewStyles*,
	psy_Configuration* dir_cfg);

/*!
** @struct PatternStyleConfigurator
** @brief Updates the pattern grid styles to the pattern view configuration
*/
typedef struct PatternStyleConfigurator {
	/*! @internal */
	PatternViewStyles* styles;
	psy_Configuration* cfg;
} PatternStyleConfigurator;

void patternstyleconfigurator_init(PatternStyleConfigurator*,
	PatternViewStyles*, psy_Configuration* pat_cfg);
	
void patternstyleconfigurator_connect(PatternStyleConfigurator*);
void patternstyleconfigurator_configure(PatternStyleConfigurator*);

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* PATTERNVIEWSTYLES_H */
