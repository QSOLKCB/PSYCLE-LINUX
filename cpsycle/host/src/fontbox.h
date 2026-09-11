/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(FONTBOX_H)
#define FONTBOX_H

/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uicombobox.h>
#include <uinumberedit.h>

/* FontBox */

typedef struct FontBox {
	/*! @extends  */
	psy_ui_Component component;
	/* internal */
	psy_ui_ComboBox family_;
	psy_ui_NumberEdit size_;
	psy_ui_Button bold_;
	psy_ui_Button underline_;
	psy_ui_Button italic_;
	psy_ui_Button dialog_;
	/* references */
	psy_Property* property_;
} FontBox;

void fontbox_init(FontBox*, psy_ui_Component* parent);

FontBox* fontbox_alloc(void);
FontBox* fontbox_alloc_init(psy_ui_Component* parent);

void fontbox_exchange(FontBox*, psy_Property*);

INLINE psy_ui_Component* fontbox_base(FontBox* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* FONTBOX_H */
