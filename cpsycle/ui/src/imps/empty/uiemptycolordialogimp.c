/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
**  copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptycolordialogimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "../../uiapp.h"
#include <stdlib.h>
#include "../../uiapp.h"
#include <stdlib.h>
#include "../../detail/portable.h"

/* prototypes */
static void dev_dispose(psy_ui_empty_ColourDialogImp*);
static int dev_execute(psy_ui_empty_ColourDialogImp*);
static const char* dev_path(psy_ui_empty_ColourDialogImp*);
static psy_ui_Colour dev_colour(psy_ui_empty_ColourDialogImp*);
static void dev_setcolour(psy_ui_empty_ColourDialogImp*, psy_ui_Colour);

/* vtable */
static psy_ui_ColourDialogImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static psy_ui_ColourDialogImpVTable* imp_vtable_init(psy_ui_empty_ColourDialogImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose = (psy_ui_fp_colourdialogimp_dev_dispose)dev_dispose;
		imp_vtable.dev_execute = (psy_ui_fp_colourdialogimp_dev_execute)dev_execute;		
		imp_vtable.dev_colour = (psy_ui_fp_colourdialogimp_dev_colour)dev_colour;
		imp_vtable.dev_setcolour = (psy_ui_fp_colourdialogimp_dev_setcolour)dev_setcolour;
		imp_vtable_initialized = TRUE;
	}
	return &imp_vtable;
}
/* implementation */
void psy_ui_empty_colourdialogimp_init(psy_ui_empty_ColourDialogImp* self)
{
	psy_ui_colourdialogimp_init(&self->imp);
	self->imp.vtable = imp_vtable_init(self);	
	self->colour = psy_ui_colour_make_rgb(0x80, 0x80, 0x80);
}

void dev_dispose(psy_ui_empty_ColourDialogImp* self)
{
}

int dev_execute(psy_ui_empty_ColourDialogImp* self)
{	
	return FALSE;
}

psy_ui_Colour dev_colour(psy_ui_empty_ColourDialogImp* self)
{
	return self->colour;	
}

void dev_setcolour(psy_ui_empty_ColourDialogImp* self, psy_ui_Colour colour)
{
	self->colour = colour;
}

#endif /* PSYCLE_TK_EMPTY */
