/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(psy_ui_VIEWFRAME_H)
#define psy_ui_VIEWFRAME_H

/* local */
#include "uiframe.h"
#include "uiterminal.h"
#include "uisplitbar.h"
#include "uisizer.h"
/* container */
#include <properties.h>
/* event driver */
#include "../../driver/eventdriver.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct psy_ui_FrameDrag {
	psy_ui_Component* frame;
	psy_List* accept;
	psy_ui_RealPoint frame_drag_offset;
	bool allow_frame_move;
} psy_ui_FrameDrag;

void psy_ui_framedrag_init(psy_ui_FrameDrag*, psy_ui_Component* frame);
void psy_ui_framedrag_add(psy_ui_FrameDrag*, const psy_ui_Component*);
void psy_ui_framedrag_dispose(psy_ui_FrameDrag*);

/*
** psy_ui_EmptyViewPage
*/

typedef struct psy_ui_EmptyViewPage {
	/*! @extends  */
	psy_ui_Component component;
	psy_ui_Label label;
} psy_ui_EmptyViewPage;

void psy_ui_emptyviewpage_init(psy_ui_EmptyViewPage*, psy_ui_Component* parent);


/* ViewFrame */
typedef struct psy_ui_ViewFrame {
	psy_ui_Component component;
	psy_ui_Component pane;
	psy_ui_Component* view;	
	psy_ui_Component* splitter;
	psy_ui_Component* icons;
	psy_ui_Component* close;
	psy_ui_Sizer* resizer;
	psy_ui_Component* restore_parent;
	psy_ui_AlignType restore_align;
	uintptr_t restore_section;
	psy_EventDriver* kbd;
	psy_ui_Size frame_size;
	bool center;
} psy_ui_ViewFrame;

void psy_ui_viewframe_init(psy_ui_ViewFrame*, psy_ui_Component* parent,
	psy_ui_Component* page, psy_ui_Component* splitter,
	psy_ui_Component* icons, psy_ui_Component* close,
	psy_ui_Sizer* resizer, psy_EventDriver* kbd,
	psy_ui_Size frame_size, bool center);

psy_ui_ViewFrame* psy_ui_viewframe_alloc(void);
psy_ui_ViewFrame* psy_ui_viewframe_alloc_init(psy_ui_Component* parent,
	psy_ui_Component* page, psy_ui_Component* splitter,
	psy_ui_Component* icons, psy_ui_Component* close,
	psy_ui_Sizer* resizer, psy_EventDriver* kbd,
	psy_ui_Size frame_size, bool center);

void psy_ui_viewframe_float(psy_ui_ViewFrame*, psy_ui_Component* component);
void psy_ui_viewframe_dock(psy_ui_ViewFrame*);

INLINE psy_ui_Component* psy_ui_viewframe_base(psy_ui_ViewFrame* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* psy_ui_VIEWFRAME_H */
