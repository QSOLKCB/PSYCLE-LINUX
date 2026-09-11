/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiimage.h"
/* local */
#include "uiapp.h"
#include "uialignment.h"


/* prototypes */
static void psy_ui_image_on_destroyed(psy_ui_Image*);
static void psy_ui_image_on_draw(psy_ui_Image*, psy_ui_Graphics*);

/* vtable */
static psy_ui_ComponentVtable psy_ui_image_vtable;
static bool psy_ui_image_vtable_initialized = FALSE;

static void psy_ui_image_vtable_init(psy_ui_Image* self)
{
	assert(self);

	if (!psy_ui_image_vtable_initialized) {
		psy_ui_image_vtable = *(self->component.vtable);
		psy_ui_image_vtable.on_destroyed =
			(psy_ui_fp_component)
			psy_ui_image_on_destroyed;
		psy_ui_image_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			psy_ui_image_on_draw;		
		psy_ui_image_vtable_initialized = TRUE;
	}
	psy_ui_component_set_vtable(&self->component, &psy_ui_image_vtable);
}

/* implementation */
void psy_ui_image_init(psy_ui_Image* self, psy_ui_Component* parent)
{  
	assert(self);

    psy_ui_component_init(&self->component, parent, NULL);
	psy_ui_image_vtable_init(self);
	psy_ui_bitmap_init(&self->bitmap_);
	psy_ui_svg_init(&self->svg_);
	self->alignment_ = psy_ui_ALIGNMENT_CENTER_VERTICAL;
	psy_ui_size_init(&self->fixed_icon_size_);
	psy_ui_size_disable(&self->fixed_icon_size_);
}

void psy_ui_image_init_resource(psy_ui_Image* self, psy_ui_Component* parent,
	uintptr_t resourceid)
{
	assert(self);

	psy_ui_image_init(self, parent);
	psy_ui_image_load_resource(self, resourceid);
}

void psy_ui_image_init_svg(psy_ui_Image* self, psy_ui_Component* parent,
	const psy_ui_SVG* svg)
{
	assert(self);

	psy_ui_image_init(self, parent);	
	psy_ui_svg_copy(&self->svg_, svg);
}

void psy_ui_image_load_resource(psy_ui_Image* self, uintptr_t resource_id)
{
	assert(self);

	psy_ui_bitmap_load_resource(&self->bitmap_, resource_id);
	if (!psy_ui_bitmap_empty(&self->bitmap_)) {
		psy_ui_RealSize size;

		size = psy_ui_bitmap_size(&self->bitmap_);
		psy_ui_component_set_preferred_size(&self->component,
			psy_ui_size_make_px(size.width, size.height));
	}
}

void psy_ui_image_set_svg(psy_ui_Image* self, const psy_ui_SVG* svg)
{
	assert(self);
	
	psy_ui_svg_copy(&self->svg_, svg);
	if (!psy_ui_svg_empty(&self->svg_)) {
		psy_ui_RealSize size;

		size = psy_ui_svg_viewport(&self->svg_);
		psy_ui_component_set_preferred_size(&self->component,
			psy_ui_size_make_px(size.width, size.height));
	}
}

void psy_ui_image_init_resource_transparency(psy_ui_Image* self,
	psy_ui_Component* parent, uintptr_t resource_id, psy_ui_Colour transparency)
{
	assert(self);

	psy_ui_image_init_resource(self, parent, resource_id);
	psy_ui_bitmap_set_transparency(&self->bitmap_, transparency);
}

void psy_ui_image_on_destroyed(psy_ui_Image* self)
{
	assert(self);

	psy_ui_bitmap_dispose(&self->bitmap_);	
	psy_ui_svg_dispose(&self->svg_);
}

void psy_ui_image_set_fixed_icon_size(psy_ui_Image* self, psy_ui_Size size)
{
	assert(self);

	self->fixed_icon_size_ = size;
}

void psy_ui_image_set_bitmap_alignment(psy_ui_Image* self,
	psy_ui_Alignment alignment)
{
	assert(self);

	self->alignment_ = alignment;
}

void psy_ui_image_on_draw(psy_ui_Image* self, psy_ui_Graphics* g)
{
	psy_ui_Size size;
	psy_ui_RealSize item_size;	
	const psy_ui_TextMetric* tm;	

	assert(self);

	size = psy_ui_component_scroll_size(&self->component);	
	tm = psy_ui_component_textmetric(&self->component);	
	if (!psy_ui_bitmap_empty(&self->bitmap_)) {
		item_size = psy_ui_bitmap_size(&self->bitmap_);
		psy_ui_graphics_draw_bitmap(g, &self->bitmap_,
			psy_ui_realrectangle_make(
				psy_ui_alignment_offset(self->alignment_,
					psy_ui_realsize_make(
						psy_ui_value_px(&size.width, tm, NULL),
						psy_ui_value_px(&size.height, tm, NULL)),
					item_size),
				item_size),
			psy_ui_realpoint_zero());
	}
	if (!psy_ui_svg_empty(&self->svg_)) {
		psy_ui_SVGDraw draw;		
		psy_ui_Size icon_size;
										
		if (psy_ui_size_enabled(&self->fixed_icon_size_)) {
			icon_size = self->fixed_icon_size_;
		} else {
			icon_size = size;
		}
		item_size = psy_ui_realsize_make(
			psy_ui_value_px(&icon_size.width, tm, NULL),
			psy_ui_value_px(&icon_size.height, tm, NULL));
		psy_ui_svgdraw_init(&draw, &self->svg_, item_size);
		psy_ui_svgdraw_draw(&draw, g,
			psy_ui_alignment_offset(self->alignment_,
				psy_ui_realsize_make(
					psy_ui_value_px(&size.width, tm, NULL),
					psy_ui_value_px(&size.height, tm, NULL)),
				item_size));
	}
}

bool psy_ui_image_empty(const psy_ui_Image* self)
{
	assert(self);

	return psy_ui_bitmap_empty(&self->bitmap_) &&
		psy_ui_svg_empty(&self->svg_);
}
