/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_IMAGE_H
#define psy_ui_IMAGE_H

#include "uibitmap.h"
#include "uicomponent.h"
#include "uisvg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_Image {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */
	psy_ui_Bitmap bitmap_;
	psy_ui_SVG svg_;
	psy_ui_Alignment alignment_;
	psy_ui_Size fixed_icon_size_;
} psy_ui_Image;

void psy_ui_image_init(psy_ui_Image*, psy_ui_Component* parent);
void psy_ui_image_init_resource(psy_ui_Image*, psy_ui_Component* parent,
	uintptr_t resourceid);
void psy_ui_image_init_svg(psy_ui_Image*, psy_ui_Component* parent,
	const psy_ui_SVG*);
void psy_ui_image_init_resource_transparency(psy_ui_Image*,
	psy_ui_Component* parent, uintptr_t resource_id,
	psy_ui_Colour transparency);

void psy_ui_image_set_fixed_icon_size(psy_ui_Image*, psy_ui_Size);
void psy_ui_image_set_bitmap_alignment(psy_ui_Image*, psy_ui_Alignment);
void psy_ui_image_load_resource(psy_ui_Image*, uintptr_t resource_id);
void psy_ui_image_set_svg(psy_ui_Image*, const psy_ui_SVG*);
bool psy_ui_image_empty(const psy_ui_Image*);

INLINE psy_ui_Component* psy_ui_image_base(psy_ui_Image* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_IMAGE_H */
