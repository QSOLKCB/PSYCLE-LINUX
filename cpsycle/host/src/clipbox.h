/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CLIPBOX_H)
#define CLIPBOX_H

/* ui */
#include <uicomponent.h>

#ifdef __cplusplus
extern "C" {
#endif


struct psy_audio_Machine;

/*!
** @struct ClipBox
** @brief Indicate that clipping has occurred
**
** @detail
** When the peak amplitude goes above 0 dB, the box on the right will turn
** red to indicate that digital clipping has occurred(and that the volume
** needs to be lowered).
**
** Structure:
**  psy_ui_ComponentImp
**          ^
**          |        <<send>>
**       ClipBox  -------------> psy_audio_Machine (buffermemory)
*/
typedef struct ClipBox {
	/*! @extends  */
	psy_ui_Component component;
	/*! @internal */	
	/* references */
	struct psy_audio_Machine* machine_;
} ClipBox;

void clipbox_init(ClipBox*, psy_ui_Component* parent);

INLINE void clipbox_set_machine(ClipBox* self, struct psy_audio_Machine* machine)
{
	assert(self);
	
	self->machine_ = machine;
}

INLINE void clipbox_activate(ClipBox* self)
{
	assert(self);
	
	psy_ui_component_add_style_state(&self->component,
		psy_ui_STYLESTATE_SELECT);
}

INLINE void clipbox_deactivate(ClipBox* self)
{
	assert(self);
	
	psy_ui_component_remove_style_state(&self->component,
		psy_ui_STYLESTATE_SELECT);
}

INLINE bool clipbox_activated(const ClipBox* self)
{
	assert(self);
	
	return ((psy_ui_componentstyle_state(&self->component.style) &
		psy_ui_STYLESTATE_SELECT) == psy_ui_STYLESTATE_SELECT);
}

INLINE psy_ui_Component* clipbox_base(ClipBox* self)
{
	assert(self);
	
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* CLIPBOX_H */
