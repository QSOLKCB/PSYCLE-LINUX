/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptycomponentimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY

#include "uiemptyimpfactory.h"
#include "../../uicomponent.h"
#include "uiemptyfontimp.h"
#include "uiemptybitmapimp.h"
#include "../../uiapp.h"
#include "uiemptyapp.h"
#include "uiemptygraphicsimp.h"
/* details */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

static uintptr_t hwnd_counter = 1000;

static void psy_ui_empty_component_create_window(psy_ui_empty_ComponentImp*,
	psy_ui_empty_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand);

/* prototypes */
static void dev_dispose(psy_ui_empty_ComponentImp*);
static void dev_destroy(psy_ui_empty_ComponentImp*);
static void dev_show(psy_ui_empty_ComponentImp*);
static void dev_showstate(psy_ui_empty_ComponentImp*, int state);
static void dev_hide(psy_ui_empty_ComponentImp*);
static int dev_visible(psy_ui_empty_ComponentImp*);
static int dev_draw_visible(psy_ui_empty_ComponentImp*);
static void dev_move(psy_ui_empty_ComponentImp*, psy_ui_Point origin);
static void dev_resize(psy_ui_empty_ComponentImp*, psy_ui_Size);
static void dev_clientresize(psy_ui_empty_ComponentImp*, intptr_t width,
	intptr_t height);
static psy_ui_RealRectangle dev_position(const psy_ui_empty_ComponentImp*);
static psy_ui_Rectangle dev_screenposition(const psy_ui_empty_ComponentImp*);
static void dev_set_position(psy_ui_empty_ComponentImp*, psy_ui_Point topleft,
	psy_ui_Size);
static psy_ui_Size dev_size(const psy_ui_empty_ComponentImp*);
static void dev_updatesize(psy_ui_empty_ComponentImp*);
static void dev_updatetopleft(psy_ui_empty_ComponentImp*);
static psy_ui_Size dev_framesize(psy_ui_empty_ComponentImp*);
static void dev_scrollto(psy_ui_empty_ComponentImp*, psy_ui_RealPoint delta,
	const psy_ui_RealRectangle*);
static psy_ui_Component* dev_parent(psy_ui_empty_ComponentImp*);
static void dev_setparent(psy_ui_empty_ComponentImp*,
	psy_ui_Component* parent);
static void dev_insert(psy_ui_empty_ComponentImp*,
	psy_ui_empty_ComponentImp* child,
	psy_ui_empty_ComponentImp* insertafter);
static void dev_remove(psy_ui_empty_ComponentImp*,
	psy_ui_empty_ComponentImp* child);
static void dev_erase(psy_ui_empty_ComponentImp*,
	psy_ui_empty_ComponentImp* child);
static void dev_setorder(psy_ui_empty_ComponentImp*, psy_ui_empty_ComponentImp*
	insertafter);
static void dev_capture(psy_ui_empty_ComponentImp*);
static void dev_releasecapture(psy_ui_empty_ComponentImp*);
static void dev_invalidate(psy_ui_empty_ComponentImp*);
static void dev_invalidaterect(psy_ui_empty_ComponentImp*,
	const psy_ui_RealRectangle*);
static void dev_update(psy_ui_empty_ComponentImp*);
static void dev_setfont(psy_ui_empty_ComponentImp*, psy_ui_Font*);
static psy_List* dev_children(psy_ui_empty_ComponentImp*, int recursive, int visible);
static void dev_enableinput(psy_ui_empty_ComponentImp*);
static void dev_preventinput(psy_ui_empty_ComponentImp*);
static bool dev_inputprevented(const psy_ui_empty_ComponentImp* self);
static void dev_set_cursor(psy_ui_empty_ComponentImp*, psy_ui_CursorStyle);
static void dev_seticonressource(psy_ui_empty_ComponentImp*, int ressourceid);
static const psy_ui_TextMetric* dev_textmetric(const psy_ui_empty_ComponentImp*);
static void dev_setbackgroundcolour(psy_ui_empty_ComponentImp*, psy_ui_Colour);
static void dev_settitle(psy_ui_empty_ComponentImp*, const char* title);
static const char* dev_title(const psy_ui_empty_ComponentImp*);
static void dev_setfocus(psy_ui_empty_ComponentImp*);
static int dev_hasfocus(psy_ui_empty_ComponentImp*);

static void dev_initialized(psy_ui_empty_ComponentImp* self) { }
static uintptr_t dev_platform_handle(psy_ui_empty_ComponentImp* self) {
		return (uintptr_t)self->hwnd; }
static bool dev_issystem(psy_ui_empty_ComponentImp* self) { return TRUE; }

static void dev_setshowfullscreen(psy_ui_empty_ComponentImp*, bool fullscreen);
static void dev_showtaskbar(psy_ui_empty_ComponentImp*, bool show);
static psy_ui_ComponentState dev_component_state(psy_ui_empty_ComponentImp*);
static void dev_set_component_state(psy_ui_empty_ComponentImp*,
	psy_ui_ComponentState);
static psy_ui_RealRectangle dev_restore_position(const
	psy_ui_empty_ComponentImp*);
static void dev_center(psy_ui_empty_ComponentImp*);

/* vtable */
static psy_ui_ComponentImpVTable vtable;
static bool vtable_initialized = FALSE;

static void empty_imp_vtable_init(psy_ui_empty_ComponentImp* self)
{
	if (!vtable_initialized) {
		vtable = *self->imp.vtable;
		vtable.dev_dispose =
			(psy_ui_fp_componentimp_dev_dispose)
			dev_dispose;
		vtable.dev_destroy =
			(psy_ui_fp_componentimp_dev_destroy)
			dev_destroy;
		vtable.dev_show =
			(psy_ui_fp_componentimp_dev_show)
			dev_show;
		vtable.dev_showstate =
			(psy_ui_fp_componentimp_dev_showstate)
			dev_showstate;
		vtable.dev_hide =
			(psy_ui_fp_componentimp_dev_hide)
			dev_hide;
		vtable.dev_visible =
			(psy_ui_fp_componentimp_dev_visible)
			dev_visible;
		vtable.dev_draw_visible =
			(psy_ui_fp_componentimp_dev_draw_visible)
			dev_draw_visible;
		vtable.dev_move =
			(psy_ui_fp_componentimp_dev_move)
			dev_move;
		vtable.dev_resize =
			(psy_ui_fp_componentimp_dev_resize)
			dev_resize;
		vtable.dev_clientresize =
			(psy_ui_fp_componentimp_dev_clientresize)
			dev_clientresize;
		vtable.dev_position =
			(psy_ui_fp_componentimp_dev_position)
			dev_position;
		vtable.dev_restore_position =
			(psy_ui_fp_componentimp_dev_position)
			dev_restore_position;
		vtable.dev_screenposition =
			(psy_ui_fp_componentimp_dev_position)
			dev_screenposition;
		vtable.dev_set_position =
			(psy_ui_fp_componentimp_dev_set_position)
			dev_set_position;
		vtable.dev_size =
			(psy_ui_fp_componentimp_dev_size)
			dev_size;
		vtable.dev_updatesize =
			(psy_ui_fp_componentimp_dev_updatesize)
			dev_updatesize;
		vtable.dev_framesize =
			(psy_ui_fp_componentimp_dev_framesize)
			dev_framesize;
		vtable.dev_scrollto =
			(psy_ui_fp_componentimp_dev_scrollto)
			dev_scrollto;
		vtable.dev_parent =
			(psy_ui_fp_componentimp_dev_parent)
			dev_parent;
		vtable.dev_setparent =
			(psy_ui_fp_componentimp_dev_setparent)
			dev_setparent;
		vtable.dev_insert =
			(psy_ui_fp_componentimp_dev_insert)
			dev_insert;
		vtable.dev_remove =
			(psy_ui_fp_componentimp_dev_remove)
			dev_remove;
		vtable.dev_erase =
			(psy_ui_fp_componentimp_dev_erase)
			dev_erase;
		vtable.dev_capture =
			(psy_ui_fp_componentimp_dev_capture)
			dev_capture;
		vtable.dev_releasecapture =
			(psy_ui_fp_componentimp_dev_releasecapture)
			dev_releasecapture;
		vtable.dev_invalidate =
			(psy_ui_fp_componentimp_dev_invalidate)
			dev_invalidate;
		vtable.dev_invalidaterect =
			(psy_ui_fp_componentimp_dev_invalidaterect)
			dev_invalidaterect;
		vtable.dev_update =
			(psy_ui_fp_componentimp_dev_update)
			dev_update;		
		vtable.dev_setfont =
			(psy_ui_fp_componentimp_dev_setfont)
			dev_setfont;
		vtable.dev_children =
			(psy_ui_fp_componentimp_dev_children)
			dev_children;
		vtable.dev_enableinput =
			(psy_ui_fp_componentimp_dev_enableinput)
			dev_enableinput;
		vtable.dev_preventinput =
			(psy_ui_fp_componentimp_dev_preventinput)
			dev_preventinput;
		vtable.dev_inputprevented =
			(psy_ui_fp_componentimp_dev_inputprevented)
			dev_inputprevented;		
		vtable.dev_set_cursor =
			(psy_ui_fp_componentimp_dev_set_cursor)
			dev_set_cursor;		
		vtable.dev_seticonressource =
			(psy_ui_fp_componentimp_dev_seticonressource)
			dev_seticonressource;
		vtable.dev_textmetric =
			(psy_ui_fp_componentimp_dev_textmetric)
			dev_textmetric;		
		vtable.dev_setbackgroundcolour =
			(psy_ui_fp_componentimp_dev_setbackgroundcolour)
			dev_setbackgroundcolour;
		vtable.dev_settitle =
			(psy_ui_fp_componentimp_dev_settitle)
			dev_settitle;
		vtable.dev_title =
			(psy_ui_fp_componentimp_dev_title)
			dev_title;
		vtable.dev_setfocus =
			(psy_ui_fp_componentimp_dev_setfocus)
			dev_setfocus;
		vtable.dev_hasfocus =
			(psy_ui_fp_componentimp_dev_hasfocus)
			dev_hasfocus;
		vtable.dev_initialized =
			(psy_ui_fp_componentimp_dev_initialized)
			dev_initialized;
		vtable.dev_platform_handle =
			(psy_ui_fp_componentimp_dev_platform_handle)
			dev_platform_handle;
		vtable.dev_issystem =
			(psy_ui_fp_componentimp_dev_issystem)
			dev_issystem;
		vtable.dev_component_state =
			(psy_ui_fp_componentimp_dev_component_state)
			dev_component_state;
		vtable.dev_set_component_state =
			(psy_ui_fp_componentimp_dev_set_component_state)
			dev_set_component_state;
		vtable.dev_center =
			(psy_ui_fp_componentimp_dev_center)
			dev_center;
		vtable_initialized = TRUE;
	}
}
/* implementation */
void psy_ui_empty_componentimp_init(psy_ui_empty_ComponentImp* self,
	psy_ui_Component* component,
	psy_ui_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand)
{	
	psy_ui_empty_ComponentImp* parent_imp;	

	psy_ui_componentimp_init(&self->imp);
	empty_imp_vtable_init(self);
	printf("psy_ui_empty_componentimp_init\n");
	self->imp.vtable = &vtable;
	self->component = component;		
	self->emptyid = -1;
	self->hwnd = 0;	
	self->topleftcachevalid = FALSE;
	self->sizecachevalid = FALSE;	
	self->visible = parent ? TRUE : FALSE;
	self->viewcomponents = NULL;
	self->fullscreen = FALSE;
	self->restore_style = 0;
	self->restore_exstyle = 0;
	self->title = NULL;
	parent_imp = (parent)
		? (psy_ui_empty_ComponentImp*)parent
		: NULL;	
	psy_ui_empty_component_create_window(self, parent_imp, classname, x, y,
		width, height, dwStyle, usecommand);
	if (self->hwnd) {
		
	}
}

void psy_ui_empty_component_create_window(psy_ui_empty_ComponentImp* self,
	psy_ui_empty_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand)
{	
	if (parent) {
		self->parent = parent->component;
	} else {
		self->parent = NULL;
	}
	self->hwnd = hwnd_counter;
	++hwnd_counter;	
	self->position = psy_ui_realrectangle_make(
		psy_ui_realpoint_make(x, y),
		psy_ui_realsize_make(width, height));
}

void dev_dispose(psy_ui_empty_ComponentImp* self)
{
	psy_List* p;
	psy_List* q;

	psy_ui_app_unregister_native(psy_ui_app(), (uintptr_t)self->hwnd);
	psy_ui_componentimp_dispose(&self->imp);
	for (p = self->viewcomponents; p != NULL; p = q) {
		psy_ui_Component* component;
		bool deallocate;

		q = p->next;
		component = (psy_ui_Component*)psy_list_entry(p);
		deallocate = component->deallocate;
		psy_ui_component_destroy(component);
		if (deallocate) {
			free(component);
		}
	}
	psy_list_free(self->viewcomponents);
	self->viewcomponents = NULL;
	free(self->title);
	self->title = NULL;
}

psy_ui_empty_ComponentImp* psy_ui_empty_componentimp_alloc(void)
{
	return (psy_ui_empty_ComponentImp*)malloc(
		sizeof(psy_ui_empty_ComponentImp));
}

psy_ui_empty_ComponentImp* psy_ui_empty_componentimp_allocinit(
	struct psy_ui_Component* component,
	psy_ui_ComponentImp* parent,
	const char* classname,
	int x, int y, int width, int height,
	uint32_t dwStyle,
	int usecommand)
{
	psy_ui_empty_ComponentImp* rv;

	rv = psy_ui_empty_componentimp_alloc();
	if (rv) {
		psy_ui_empty_componentimp_init(rv,
			component,
			parent,
			classname,
			x, y, width, height,
			dwStyle,
			usecommand
		);
	}	
	return rv;
}

void dev_destroy(psy_ui_empty_ComponentImp* self)
{	
	
}

void dev_show(psy_ui_empty_ComponentImp* self)
{
	self->visible = TRUE;	
}

void dev_showstate(psy_ui_empty_ComponentImp* self, int state)
{
	self->visible = TRUE;	
}

void dev_setshowfullscreen(psy_ui_empty_ComponentImp* self, bool fullscreen)	
{
	
}

void dev_center(psy_ui_empty_ComponentImp* self)
{
	
}

void dev_showtaskbar(psy_ui_empty_ComponentImp* self, bool show)
{

}

void dev_hide(psy_ui_empty_ComponentImp* self)
{
	self->visible = FALSE;	
}

int dev_visible(psy_ui_empty_ComponentImp* self)
{
	return self->visible;	
}

int dev_draw_visible(psy_ui_empty_ComponentImp* self)
{
	return 1;
}

void dev_move(psy_ui_empty_ComponentImp* self, psy_ui_Point topleft)
{
	self->topleftcachevalid = FALSE;	
	self->topleftcache = topleft;
	self->topleftcachevalid = TRUE;
}

void dev_resize(psy_ui_empty_ComponentImp* self, psy_ui_Size size)
{	
	self->sizecachevalid = FALSE;	
	self->sizecache = size;
	self->sizecachevalid = TRUE;
}

void dev_clientresize(psy_ui_empty_ComponentImp* self, intptr_t width,
	intptr_t height)
{
	
}


psy_ui_RealRectangle dev_position(const psy_ui_empty_ComponentImp* self)
{	
	return self->position;		
}

psy_ui_Rectangle dev_screenposition(const psy_ui_empty_ComponentImp* self)
{	
	return psy_ui_rectangle_make(psy_ui_point_zero(),
            psy_ui_component_scroll_size(self->component));
}

void dev_set_position(psy_ui_empty_ComponentImp* self, psy_ui_Point topleft,
	psy_ui_Size size)
{
	const psy_ui_TextMetric* tm;	
	
	tm = dev_textmetric(self);

	dev_updatesize(self);
	dev_updatetopleft(self);
}

psy_ui_Size dev_size(const psy_ui_empty_ComponentImp* self)
{
	if (!self->sizecachevalid) {
	}
	return self->sizecache;
}

void dev_updatesize(psy_ui_empty_ComponentImp* self)
{	
	self->sizecachevalid = TRUE;
}

void dev_updatetopleft(psy_ui_empty_ComponentImp* self)
{	
	
	// self->topleftcache = psy_ui_point_make_px((double)pt.x,
	// (double)pt.y);
	self->topleftcachevalid = TRUE;
}

psy_ui_Size dev_framesize(psy_ui_empty_ComponentImp* self)
{		
	return psy_ui_size_make_px(0, 0);
}

void dev_scrollto(psy_ui_empty_ComponentImp* self, psy_ui_RealPoint delta,
	const psy_ui_RealRectangle* r)
{
	
}

psy_ui_Component* dev_parent(psy_ui_empty_ComponentImp* self)
{
	return 0;
}

void dev_setparent(psy_ui_empty_ComponentImp* self, psy_ui_Component* parent)
{
	
}

void dev_insert(psy_ui_empty_ComponentImp* self,
	psy_ui_empty_ComponentImp* child,
	psy_ui_empty_ComponentImp* insertafter)
{
	assert(child);

	if ((child->imp.vtable->dev_flags(&child->imp) &
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) ==
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) {
		psy_list_append(&self->viewcomponents, child->component);
	} else {
		if (insertafter) {			
			
						
		}
	}
}

void dev_remove(psy_ui_empty_ComponentImp* self, psy_ui_empty_ComponentImp*
	child)
{
	if ((child->imp.vtable->dev_flags(&child->imp) &
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) ==
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) {
		psy_List* p;

		p = psy_list_find_entry(self->viewcomponents, child->component);
		if (p) {
			psy_list_remove(&self->viewcomponents, p);
			if (child->component->deallocate) {
				psy_ui_component_deallocate(child->component);
			} else {
				psy_ui_component_dispose(child->component);
			}
		}		
	} else {
		assert(0);
		/* todo */
	}
}

void dev_erase(psy_ui_empty_ComponentImp* self, psy_ui_empty_ComponentImp*
	child)
{
	if ((child->imp.vtable->dev_flags(&child->imp) & 
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) ==
			psy_ui_COMPONENTIMPFLAGS_HANDLECHILDREN) {
		psy_List* p;

		p = psy_list_find_entry(self->viewcomponents, child->component);
		if (p) {			
			psy_list_remove(&self->viewcomponents, p);			
		}
	} else {
		assert(0);
		/* todo */
	}
}

void dev_setorder(psy_ui_empty_ComponentImp* self, psy_ui_empty_ComponentImp*
	insertafter)
{

}

void dev_capture(psy_ui_empty_ComponentImp* self)
{
	
}

void dev_releasecapture(psy_ui_empty_ComponentImp* self)
{

}

void dev_invalidate(psy_ui_empty_ComponentImp* self)
{

}

void dev_invalidaterect(psy_ui_empty_ComponentImp* self,
	const psy_ui_RealRectangle* r)
{
	
}

void dev_update(psy_ui_empty_ComponentImp* self)
{
	
}

void dev_setfont(psy_ui_empty_ComponentImp* self, psy_ui_Font* source)
{
		
}

psy_List* dev_children(psy_ui_empty_ComponentImp* self, int recursive, int visible)
{	
	return NULL;
}

void dev_enableinput(psy_ui_empty_ComponentImp* self)
{
	
}

void dev_preventinput(psy_ui_empty_ComponentImp* self)
{
	
}

bool dev_inputprevented(const psy_ui_empty_ComponentImp* self)
{
	return FALSE;
}

const psy_ui_TextMetric* dev_textmetric(const psy_ui_empty_ComponentImp* self)
{	
	return NULL;
}

void dev_set_cursor(psy_ui_empty_ComponentImp* self, psy_ui_CursorStyle
	cursorstyle)
{	
}

void dev_seticonressource(psy_ui_empty_ComponentImp* self, int ressourceid)
{

}

void dev_setbackgroundcolour(psy_ui_empty_ComponentImp* self,
	psy_ui_Colour colour)
{	

}

void dev_settitle(psy_ui_empty_ComponentImp* self, const char* title)
{
	psy_strreset(&self->title, title);
}

const char* dev_title(const psy_ui_empty_ComponentImp* self)
{	
	if (self->title) {
		return self->title;
	}
	return "";
}

void dev_setfocus(psy_ui_empty_ComponentImp* self)
{
	
}

int dev_hasfocus(psy_ui_empty_ComponentImp* self)
{	
	return FALSE;
}

psy_ui_ComponentState dev_component_state(psy_ui_empty_ComponentImp* self)
{
	return 0;
}

void dev_set_component_state(psy_ui_empty_ComponentImp* self,
	psy_ui_ComponentState state)
{
	
}

psy_ui_RealRectangle dev_restore_position(const psy_ui_empty_ComponentImp* self)
{
	psy_ui_RealRectangle rv;

	rv = psy_ui_realrectangle_zero();
	
	return rv;
}

#endif /* PSYCLE_TK_EMPTY */
