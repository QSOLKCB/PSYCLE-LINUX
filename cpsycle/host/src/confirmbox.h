/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2024 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(CONFIRMBOX_H)
#define CONFIRMBOX_H

/* host */
#include "viewindex.h"
/* ui */
#include <uibutton.h>
#include <uilabel.h>
#include <uinavhandler.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	CONFIRM_YES = 1,
	CONFIRM_NO = 2,
	CONFIRM_CONTINUE = 4
} ConfirmBoxButton;

typedef struct ConfirmBox {
	/*! @extends  */
	psy_ui_Component component;		
	/* callbacks */
	psy_Slot signal_accept;
	psy_Slot signal_reject;
	psy_Slot signal_continue;
	/*! @internal */
	psy_ui_Component view_;
	psy_ui_Label title_;
	psy_ui_Label header_;
	psy_ui_Component buttons_;
	psy_ui_Button accept_;
	psy_ui_Button reject_;
	psy_ui_Button continue_;	
	psy_ui_NavHandler nav_handler_;
} ConfirmBox;

void confirmbox_init(ConfirmBox*, psy_ui_Component* parent);

void confirmbox_set_labels(ConfirmBox*, const char* title,
	const char* yesstr, const char* nostr);	
void confirmbox_set_callbacks(ConfirmBox*, psy_Slot accept, psy_Slot reject,
	psy_Slot cont);

INLINE psy_ui_Component* confirmbox_base(ConfirmBox* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* CONFIRMBOX */
