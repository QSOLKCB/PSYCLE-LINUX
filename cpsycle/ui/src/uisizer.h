/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_SIZER_H
#define psy_ui_SIZER_H

#include "uicomponent.h"
#include "uiimage.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_Sizer {
	/*! @extends */	
	psy_ui_Image img;
	/*! @internal */	
	psy_ui_RealPoint frame_drag_offset_;
	bool dragging_;
	/* references */
	psy_ui_Component* resize_component_;
} psy_ui_Sizer;

void psy_ui_sizer_init(psy_ui_Sizer*, psy_ui_Component* parent);

void psy_ui_sizer_set_resize_component(psy_ui_Sizer*, psy_ui_Component*);

INLINE psy_ui_Component* psy_ui_sizer_base(psy_ui_Sizer* self)
{
	return psy_ui_image_base(&self->img);
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_SIZER_H */
