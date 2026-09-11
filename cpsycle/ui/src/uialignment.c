/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uialignment.h"

static bool psy_ui_alignment_check(psy_ui_Alignment, psy_ui_Alignment);

psy_ui_RealPoint psy_ui_alignment_offset(psy_ui_Alignment alignment, 	
	psy_ui_RealSize size, psy_ui_RealSize item_size)
{			
	double x;
	double y;
	double width;
	double height;	
	
	width = size.width;
	height = size.height;	
	if (psy_ui_alignment_check(alignment, psy_ui_ALIGNMENT_CENTER_HORIZONTAL)) {
		x = (width - item_size.width) / 2;
	} else if (psy_ui_alignment_check(alignment, psy_ui_ALIGNMENT_RIGHT)) {
		x = width - item_size.width;
	} else {
		x = 0;
	}
	if (psy_ui_alignment_check(alignment, psy_ui_ALIGNMENT_CENTER_VERTICAL)) {
		y = (height - item_size.height) / 2;
	} else if (psy_ui_alignment_check(alignment, psy_ui_ALIGNMENT_BOTTOM)) {
		y = height - item_size.height;
	} else {
		y = 0;
	}
	return psy_ui_realpoint_make(x, y);
}

bool psy_ui_alignment_check(psy_ui_Alignment lhs, psy_ui_Alignment rhs)
{	
	return ((lhs & rhs) == rhs);
}
