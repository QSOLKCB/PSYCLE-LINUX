/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(TRACKERHEADERSTYLES_H)
#define TRACKERHEADERSTYLES_H

#include "../../detail/psyconf.h"

/* container */
#include <configuration.h>


#ifdef PSYCLE_USE_TRACKERVIEW

/*!
** @struct TrackerHeaderStyles
** @brief Creates and loads the pattern header styles
*/
typedef struct TrackerHeaderStyles {
	/*! @internal */
	psy_Configuration* dir_cfg;	
} TrackerHeaderStyles;

void trackerheaderstyles_init(TrackerHeaderStyles*,
	psy_Configuration* dir_cfg);

void trackerheaderstyles_set_classic(TrackerHeaderStyles*);
void trackerheaderstyles_set_text(TrackerHeaderStyles*);
void trackerheaderstyles_switch(TrackerHeaderStyles*, const int* src);
void trackerheaderstyles_load(TrackerHeaderStyles*,
	const char* header_skin_name);

#endif /* PSYCLE_USE_TRACKERVIEW */

#endif /* TRACKERHEADERSTYLES_H */
