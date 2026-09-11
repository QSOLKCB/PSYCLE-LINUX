/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_SPLITTER_H
#define psy_ui_SPLITTER_H

/* local */
#include "uicomponent.h"
#include "uibutton.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct psy_ui_Splitter
** @brief The Splitter divides a component into resizable panes
** 
** @detail
** The Splitter is itself implemented as a component. The aligntype defines if
** the splitter is vertical or horizontal. More then one splitter in the parent
** component are possible.
**
**  psy_ui_Component <>----<> psy_ui_ComponentImp
**         ^
**         |
**         |
**  psy_ui_Splitter
*/
typedef struct psy_ui_Splitter {
	/*! @extends */
	psy_ui_Component component;
	/*! @internal */
	double drag_offset_;
	bool resize_;
	psy_ui_Size restore_size_;
	bool has_restore_;
	double thumb_size_;
	bool is_vertical_;
	psy_ui_Component* toggle_;
	psy_ui_Button* button_;
	bool direct_;
} psy_ui_Splitter;

void psy_ui_splitter_init(psy_ui_Splitter*, psy_ui_Component* parent);

psy_ui_Splitter* psy_ui_splitter_alloc(void);
psy_ui_Splitter* psy_ui_splitter_allocinit(psy_ui_Component* parent);

INLINE bool psy_ui_splitter_is_vertical(const psy_ui_Splitter* self)
{
	return self->is_vertical_;
}

void psy_ui_splitter_set_toggle(psy_ui_Splitter*, psy_ui_Component* toggle);
void psy_ui_splitter_set_button(psy_ui_Splitter*, psy_ui_Button*);

INLINE psy_ui_Component* psy_ui_splitter_base(psy_ui_Splitter* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_SPLITTER_H */
