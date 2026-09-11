/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/psyconf.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
#ifndef psy_ui_empty_GRAPHICSIMP_H
#define psy_ui_empty_GRAPHICSIMP_H

#include "../../uigraphics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct psy_ui_empty_GraphicsImp {
	psy_ui_GraphicsImp imp;	
	uintptr_t hdc;
	bool shareddc;
	psy_ui_RealPoint org;
	psy_ui_RealRectangle clip;
	psy_ui_Colour colour;
	psy_Encoding encoding;
	/* reference */
	const psy_ui_Font* font;
} psy_ui_empty_GraphicsImp;

void psy_ui_empty_graphicsimp_init(psy_ui_empty_GraphicsImp*,
	uintptr_t hdc);
void psy_ui_empty_graphicsimp_init_bitmap(psy_ui_empty_GraphicsImp*,
	psy_ui_Bitmap*);

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_empty_GRAPHICSIMP_H */
#endif /* PSYCLE_TK_EMPTY */
