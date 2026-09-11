/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_COMBOBOX_H
#define psy_ui_COMBOBOX_H

/* local */
#include "uibutton.h"
#include "uidropdownbox.h"
#include "uilabel.h"
#include "uilistbox.h"
#include "uitext.h"
/* container */
#include <hashtbl.h>

/*
** ComboBox
** Bridge
** Aim: avoid coupling to one platform (win32, x11, etc)
** Abstraction/Refined  psy_ui_ComboBox
** Implementor			psy_ui_ComponentImp
** Concrete Implementor	psy_ui_win_ComponentImp
**
** psy_ui_Component <>----<> psy_ui_ComponentImp
**         ^                                                        
**         |                                                        
**         |                               
** psy_ui_ComboBox <>----<>  psy_ui_DropDownBox <>----<> psy_ui_ListBox
**
*/

#ifdef __cplusplus
extern "C" {
#endif

struct psy_ui_ComponentImp;

typedef struct psy_ui_ComboBox {
    /*! @extends  */
    psy_ui_Component component;
    /* signals */
    psy_Signal signal_selchanged;
    /*! @internal */
    psy_ui_DropDownBox dropdown_;
    psy_ui_Component pane_;
    psy_ui_ListBox listbox_;
    psy_ui_Component buttons_;
    psy_ui_Button less_;
    psy_ui_Button more_;
    psy_ui_Button expand_;
    psy_ui_Component editpane_;
    psy_ui_Edit text_;    
    bool simple_;
    bool prevent_wheel_select_;
    /* references */
    psy_Property* property_;
} psy_ui_ComboBox;

void psy_ui_combobox_init(psy_ui_ComboBox*, psy_ui_Component* parent);
void psy_ui_combobox_init_simple(psy_ui_ComboBox*, psy_ui_Component* parent);

psy_ui_ComboBox* psy_ui_combobox_alloc(void);
psy_ui_ComboBox* psy_ui_combobox_alloc_init(psy_ui_Component* parent);

void psy_ui_combobox_exchange(psy_ui_ComboBox*, psy_Property*);
uintptr_t psy_ui_combobox_add_text(psy_ui_ComboBox*, const char* text);
void psy_ui_combobox_clear(psy_ui_ComboBox*);
void psy_ui_combobox_select(psy_ui_ComboBox*, uintptr_t index);
uintptr_t psy_ui_combobox_cur_sel(const psy_ui_ComboBox*);
void psy_ui_combobox_set_char_number(psy_ui_ComboBox*, double num);
void psy_ui_combobox_set_item_id(psy_ui_ComboBox*, uintptr_t index, uintptr_t id);
uintptr_t psy_ui_combobox_item_id(psy_ui_ComboBox*, uintptr_t index);
void psy_ui_combobox_text(psy_ui_ComboBox*, char* text);
void psy_ui_combobox_text_at(psy_ui_ComboBox*, char* text, uintptr_t index);
uintptr_t psy_ui_combobox_count(const psy_ui_ComboBox*);

INLINE void psy_ui_combobox_prevent_wheel_select(psy_ui_ComboBox* self)
{
	assert(self);
	
	self->prevent_wheel_select_ = TRUE;
}

INLINE psy_ui_Component* psy_ui_combobox_base(psy_ui_ComboBox* self)
{
    return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_COMBOBOX_H */
