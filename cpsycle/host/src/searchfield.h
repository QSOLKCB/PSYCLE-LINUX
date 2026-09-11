/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(SEARCHFIELD_H)
#define SEARCHFIELD_H

/* ui */
#include <uibutton.h>
#include <uitext.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
** @struct SearchField
*/
typedef struct SearchField {
	/*! @extends  */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_changed;
	/* intern */
	psy_ui_Button image_;
	psy_ui_Edit edit_;
	bool has_default_text_;
	char* default_text_;
	/* references */
	psy_ui_Component* restore_focus_;
} SearchField;

void searchfield_init(SearchField*, psy_ui_Component* parent,
	psy_ui_Component* restore_focus, const psy_ui_SVG*);

void searchfield_set_default_text(SearchField*, const char* text);
const char* searchfield_text(const SearchField*);
void searchfield_set_restore_focus(SearchField*,
	psy_ui_Component* restore_focus);

INLINE psy_ui_Component* searchfield_base(SearchField* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* SEARCHFIELD_H */
