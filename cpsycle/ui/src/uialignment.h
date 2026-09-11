/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_ALIGNMENT_H
#define psy_ui_ALIGNMENT_H

/* local */
#include "uidef.h"

#ifdef __cplusplus
extern "C" {
#endif

psy_ui_RealPoint psy_ui_alignment_offset(psy_ui_Alignment, psy_ui_RealSize, 
	psy_ui_RealSize item_size);


#ifdef __cplusplus
}
#endif

#endif /* psy_ui_ALIGNMENT_H */
