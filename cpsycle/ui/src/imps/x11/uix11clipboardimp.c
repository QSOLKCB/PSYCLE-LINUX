/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uix11clipboardimp.h"

#if PSYCLE_USE_TK == PSYCLE_TK_X11

/* local */
#include "../../uiapp.h"
#include "uiapp.h"
#include "uix11app.h"
#include "uix11componentimp.h"
/* x11 */
#include <X11/Xatom.h>
/* file */
#include <encoding.h>
/* platform */
#include "../../detail/portable.h"


/* atoms */
static bool clipboard_atoms_initialized = 0;
static Atom xa_clipboard;
static Atom xa_xsel_data;
static Atom xa_targets;
static Atom xa_text;
static Atom xa_utf8_string;

/* prototypes */
static void psy_ui_x11_clipboardimp_dispose(psy_ui_x11_ClipboardImp*);
static const char* psy_ui_x11_clipboardimp_read_text(psy_ui_x11_ClipboardImp*);
static void psy_ui_x11_clipboardimp_write_text(psy_ui_x11_ClipboardImp*,
	const char* text);

/* vtable */
static psy_ui_ClipboardImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_x11_ClipboardImp* self)
{
	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_clipboard_imp_fp_dispose)
			psy_ui_x11_clipboardimp_dispose;
		imp_vtable.dev_read_text = 
			(psy_ui_clipboard_imp_fp_read_text)
			psy_ui_x11_clipboardimp_read_text;
		imp_vtable.dev_write_text =
			(psy_ui_clipboard_imp_fp_write_text)
			psy_ui_x11_clipboardimp_write_text;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

static void imp_clipboard_atoms_init(void)
{	
	if (!clipboard_atoms_initialized) {
		psy_ui_X11App* x11app;
	
		x11app = (psy_ui_X11App*)psy_ui_app()->imp;	
		xa_clipboard = XInternAtom(x11app->dpy, "CLIPBOARD", 0);
		xa_xsel_data = XInternAtom(x11app->dpy, "XSEL_DATA", 0);
		xa_utf8_string = XInternAtom(x11app->dpy, "UTF8_STRING", 1);
		xa_targets = XInternAtom(x11app->dpy, "TARGETS", 0);		
		xa_text = XInternAtom(x11app->dpy, "TEXT", 1);
		clipboard_atoms_initialized = TRUE;
	}
}

/* implementation */
void psy_ui_x11_clipboardimp_init(psy_ui_x11_ClipboardImp* self)
{	
	assert(self);

	psy_ui_clipboard_imp_init(&self->imp);
	imp_vtable_init(self);
	imp_clipboard_atoms_init();
	self->text = psy_strdup("");	
}

void psy_ui_x11_clipboardimp_dispose(psy_ui_x11_ClipboardImp* self)
{
	assert(self);

	free(self->text);
	self->text = NULL;
}

const char* psy_ui_x11_clipboardimp_read_text(psy_ui_x11_ClipboardImp* self)
{		
	psy_ui_X11App* x11app;
	psy_ui_x11_ComponentImp* main;
	XEvent notification;
	
	assert(self);

	x11app = (psy_ui_X11App*)psy_ui_app()->imp;
	if (!psy_ui_app()->main) {
		return self->text;
	}	
	main = (psy_ui_x11_ComponentImp*)psy_ui_app()->main->imp;	
	if (XGetSelectionOwner(x11app->dpy, xa_clipboard) == main->hwnd) {		
        return self->text;
    }
	XConvertSelection(x11app->dpy, xa_clipboard, xa_utf8_string, xa_xsel_data,
		main->hwnd, CurrentTime);
	while (!XCheckTypedWindowEvent(x11app->dpy, main->hwnd, SelectionNotify,
		&notification))
	{		
		/*! @todo timeout */            
    }
    psy_ui_x11app_handle_event(x11app, &notification);    
	return self->text;
}

void psy_ui_x11_clipboardimp_write_text(psy_ui_x11_ClipboardImp* self,
	const char* text)
{
	psy_ui_X11App* x11app;	
	psy_ui_x11_ComponentImp* main;	

	assert(self);
	
	psy_strreset(&self->text, text);
	x11app = (psy_ui_X11App*)psy_ui_app()->imp;
	if (!psy_ui_app()->main) {
		return;
	}
	main = (psy_ui_x11_ComponentImp*)psy_ui_app()->main->imp;	
	XSetSelectionOwner(x11app->dpy, xa_clipboard, main->hwnd, 0);	
}

void psy_ui_x11_clipboardimp_handle_selection_request(
	psy_ui_x11_ClipboardImp* self, psy_ui_X11App* x11app,
	XEvent* event)
{		
	XSelectionRequestEvent * xsr;
	XSelectionEvent ev;
	int r;
							
	if (event->xselectionrequest.selection != xa_clipboard) {
		return;
	}
	xsr = &event->xselectionrequest;
	memset(&ev, 0, sizeof(ev));
	ev.type = SelectionNotify;
	ev.display = xsr->display;
	ev.requestor = xsr->requestor;
	ev.selection = xsr->selection;
	ev.target = xsr->target;
	ev.time = xsr->time;
	ev.property = xsr->property;			
	r = 0;
	if (ev.target == xa_targets) {
		r = XChangeProperty(ev.display, ev.requestor, ev.property,
				XA_ATOM, 32, PropModeReplace, (unsigned char*)&xa_utf8_string,
				1);			
	} else if (ev.target == XA_STRING || ev.target == xa_text) {
		r = XChangeProperty(ev.display, ev.requestor, ev.property,
				XA_STRING, 8, PropModeReplace, self->text,
				psy_strlen(self->text));			
	} else if (ev.target == xa_utf8_string) {
		r = XChangeProperty(ev.display, ev.requestor, ev.property,
				xa_utf8_string, 8, PropModeReplace, self->text,
				psy_strlen(self->text));			
	} else {
		ev.property = None;
	}
	if ((r & 2) == 0) {			
		XSendEvent(x11app->dpy, ev.requestor, 0, 0, (XEvent*)&ev);			 
	}
}

void psy_ui_x11_clipboardimp_handle_selection_notify(
	psy_ui_x11_ClipboardImp* self, XEvent* event)
{							
	if (event->xselection.selection != xa_clipboard) {
		return;
	}
	if (event->xselection.property) {
		Atom target;		
		char* data;
		int format;
		unsigned long N, size;
				
		XGetWindowProperty(event->xselection.display,
			event->xselection.requestor,
			event->xselection.property, 0L,(~0L), 0,
			AnyPropertyType, &target,
			&format, &size, &N,(unsigned char**)&data);
		if (target == xa_utf8_string || target == XA_STRING) {			
			psy_strreset(&self->text, data);					
			XFree(data);
		}
		XDeleteProperty(event->xselection.display,
			event->xselection.requestor, event->xselection.property);
	}
}

#endif /* PSYCLE_TK_X11 */
