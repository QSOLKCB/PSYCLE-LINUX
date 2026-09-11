/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(NUMBEREDIT_H)
#define NUMBEREDIT_H

/* ui */
#include "uibutton.h"
#include "uilabel.h"
#include "uitext.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum psy_ui_NumberEditType {
	psy_ui_NUMBEREDIT_TYPE_INT,
	psy_ui_NUMBEREDIT_TYPE_REAL
} psy_ui_NumberEditType;

typedef enum psy_ui_NumberEditFormat {
	psy_ui_NUMBEREDIT_FORMAT_DEC,
	psy_ui_NUMBEREDIT_FORMAT_HEX
} psy_ui_NumberEditFormat;

typedef struct psy_ui_NumberEdit {
	/*! @extends  */
	psy_ui_Component component;
	/* signals */
	psy_Signal signal_changed;
	/*! @internal */
	psy_ui_Label desc;
	psy_ui_Edit edit;
	psy_ui_Button less;
	psy_ui_Button more;	
	double minval;
	double maxval;
	double value;
	double restore;
	psy_ui_NumberEditType type;
	psy_ui_NumberEditFormat format;
} psy_ui_NumberEdit;

void psy_ui_numberedit_init(psy_ui_NumberEdit*, psy_ui_Component* parent,
	const char* desc, double value, double minval, double maxval);
void psy_ui_numberedit_init_connect(psy_ui_NumberEdit*, psy_ui_Component* parent,
	const char* desc, double value, double minval, double maxval,
	void* context, void* fp);
	
void psy_ui_numberedit_set_type(psy_ui_NumberEdit*, psy_ui_NumberEditType);
void psy_ui_numberedit_set_format(psy_ui_NumberEdit*, psy_ui_NumberEditFormat);
double psy_ui_numberedit_value(psy_ui_NumberEdit*);
void psy_ui_numberedit_set_value(psy_ui_NumberEdit*, double value);
void psy_ui_numberedit_enable(psy_ui_NumberEdit*);
void psy_ui_numberedit_prevent(psy_ui_NumberEdit*);
void psy_ui_numberedit_set_edit_char_number(psy_ui_NumberEdit*, int charnumber);
void psy_ui_numberedit_set_desc_char_number(psy_ui_NumberEdit*, int charnumber);

INLINE psy_ui_Component* psy_ui_numberedit_base(psy_ui_NumberEdit* self)
{
	assert(self);

	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* NUMBEREDIT_H */
