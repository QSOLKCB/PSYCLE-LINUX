/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(ZOOMBOX_H)
#define ZOOMBOX_H

/* ui */
#include <uibutton.h>
#include <uitext.h>


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct ZoomBox
** @brief Displays and sets a zoom factor with buttons or wheel scroll
*/

typedef struct ZoomBox {
	/*! @extends */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_changed;
	/*! @internal */
	psy_ui_Button zoom_in_;
	psy_ui_Edit zoom_;
	psy_ui_Button zoom_out_;
	double zoom_rate_;
	double zoom_step_;
	psy_RealPair range_;
	bool prevent_wheel_select_;
	/* references */
	psy_Property* property_;
} ZoomBox;

void zoombox_init(ZoomBox*, psy_ui_Component* parent);
void zoombox_init_connect(ZoomBox*, psy_ui_Component* parent,
	void* context, void* fp);
void zoombox_init_exchange(ZoomBox*, psy_ui_Component* parent,
	psy_Property*);

ZoomBox* zoombox_alloc(void);
ZoomBox* zoombox_alloc_init(psy_ui_Component* parent);
ZoomBox* zoombox_alloc_init_exchange(psy_ui_Component* parent, psy_Property*);

void zoombox_exchange(ZoomBox*, psy_Property*);
void zoombox_set_rate(ZoomBox*, double);

INLINE double zoombox_rate(const ZoomBox* self)
{
	assert(self);

	return self->zoom_rate_;
}

/* sets the step, the rate is inc-/decremented */
void zoombox_set_step(ZoomBox*, double);
INLINE double zoombox_step(const ZoomBox* self)
{
	assert(self);

	return self->zoom_step_;
}

/* sets min/max-range (including maxval) */
INLINE void zoombox_set_range(ZoomBox* self, psy_RealPair range)
{
	assert(self);

	self->range_ = range;	
	self->range_.second = psy_max(self->range_.first, self->range_.second);
}

INLINE psy_RealPair zoombox_range(const ZoomBox* self)
{
	assert(self);

	return self->range_;
}

INLINE void zoombox_prevent_wheel_select(ZoomBox* self)
{
	assert(self);
	
	self->prevent_wheel_select_ = TRUE;
}

INLINE psy_ui_Component* zoombox_base(ZoomBox* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* ZOOMBOX_H */
