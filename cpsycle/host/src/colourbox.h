/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(COLOURBOX_H)
#define COLOURBOX_H

/* host */
/* ui */
// #include <uitextarea.h>
#include <uibutton.h>
#include <uitext.h>

/* ColourBox */

typedef struct ColourBox {
	/*! @extends  */
	psy_ui_Component component;
	/* inrernal */
	psy_ui_Component colour;	
	psy_ui_Edit edit;
	psy_ui_Button dialog;
	/* references */
	psy_Property* property;
} ColourBox;

void colourbox_init(ColourBox*, psy_ui_Component* parent);

ColourBox* colourbox_alloc(void);
ColourBox* colourbox_alloc_init(psy_ui_Component* parent);

void colourbox_exchange(ColourBox*, psy_Property*);

INLINE psy_ui_Component* colourbox_base(ColourBox* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* COLOURBOX_H */
