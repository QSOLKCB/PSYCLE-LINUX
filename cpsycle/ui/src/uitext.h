/* 
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_ui_TEXT_H
#define psy_ui_TEXT_H

/* local */
#include "uitextdraw.h"
#include "uitextformat.h"
#include "uitextsource.h"


#ifdef __cplusplus
extern "C" {
#endif

/*!
** @struct Edit
** @brief Single line text edit.
*/
typedef struct psy_ui_TextPane {
    /*! @extends */
	psy_ui_Component component;
    /* signals */
    /* emits any change */
    psy_Signal signal_change;
    /*
    ** emits if edit is inputfield and
    ** - return pressed or
    ** - focus lost or
    ** - clicked outside
    */
    psy_Signal signal_accept;
    /* emits if edit is inputfield and esc pressed */
    psy_Signal signal_reject;
    /*! @internal */
	double char_number_;
    bool is_input_field_;        
    psy_ui_TextFormat format_;
    psy_ui_TextSource source_;
    bool prevent_input_;
    bool down_;
    bool sel_starting_;
} psy_ui_TextPane;

void psy_ui_textpane_init(psy_ui_TextPane*, psy_ui_Component* parent);

void psy_ui_textpane_enable_input_field(psy_ui_TextPane*);
void psy_ui_textpane_set_text(psy_ui_TextPane*, const char* text);
void psy_ui_textpane_add_text(psy_ui_TextPane*, const char* text);
const char* psy_ui_textpane_text(const psy_ui_TextPane* self);
void psy_ui_textpane_set_char_number(psy_ui_TextPane*, double number);
void psy_ui_textpane_enable_edit(psy_ui_TextPane*);
void psy_ui_textpane_prevent_edit(psy_ui_TextPane*);
void psy_ui_textpane_set_selection(psy_ui_TextPane*, uintptr_t cpmin,
    uintptr_t cpmax);
uintptr_t psy_ui_textpane_cursor_column(const psy_ui_TextPane*);

INLINE psy_ui_Component* psy_ui_textpane_base(psy_ui_TextPane* self)
{
    return &self->component;
}

/*!
** @struct psy_ui_Edit
** @brief  A single-line text edit.
*/
typedef struct psy_ui_Edit {
    /*! @extends */
    psy_ui_Component component;
    /* signals */
    psy_Signal signal_change;
    /*
    ** emits if edit is inputfield and
    ** - return pressed or
    ** - focus lost or
    ** - clicked outside
    */
    psy_Signal signal_accept;
    /*
    ** emits if edit is inputfield and
    ** - esc pressed
    */
    psy_Signal signal_reject;
    /*! @internal */    
    psy_ui_TextPane pane;
    /* references */
    psy_Property* property_;
} psy_ui_Edit;

void psy_ui_edit_init(psy_ui_Edit*, psy_ui_Component* parent);

psy_ui_Edit* psy_ui_edit_alloc(void);
psy_ui_Edit* psy_ui_edit_allocinit(psy_ui_Component* parent);

void psy_ui_edit_exchange(psy_ui_Edit*, psy_Property*);

INLINE void psy_ui_edit_enable_input_field(psy_ui_Edit* self)
{
    psy_ui_textpane_enable_input_field(&self->pane);
}

INLINE void psy_ui_edit_set_text(psy_ui_Edit* self, const char* text)
{
    psy_ui_textpane_set_text(&self->pane, text);
}

INLINE void psy_ui_edit_add_text(psy_ui_Edit* self, const char* text)
{
    psy_ui_textpane_add_text(&self->pane, text);
}

INLINE const char* psy_ui_edit_text(const psy_ui_Edit* self)
{
    return psy_ui_textpane_text(&self->pane);
}

uintptr_t psy_ui_edit_length(const psy_ui_Edit*);
void psy_ui_edit_range(psy_ui_Edit*, intptr_t start, intptr_t end, char* text);

INLINE void psy_ui_edit_set_char_number(psy_ui_Edit* self, double number)
{
    psy_ui_textpane_set_char_number(&self->pane, number);
}

INLINE void psy_ui_edit_enable(psy_ui_Edit* self)
{
    psy_ui_textpane_enable_edit(&self->pane);
}

INLINE void psy_ui_edit_prevent(psy_ui_Edit* self)
{
    psy_ui_textpane_prevent_edit(&self->pane);
}

INLINE void psy_ui_edit_select(psy_ui_Edit* self,
    uintptr_t cpmin, uintptr_t cpmax)
{
    psy_ui_textpane_set_selection(&self->pane, cpmin, cpmax);
}

uintptr_t psy_ui_edit_cursor_column(const psy_ui_Edit*);

INLINE psy_ui_Component* psy_ui_edit_base(psy_ui_Edit* self)
{
    return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_TEXT_H */
