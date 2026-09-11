/*
** This source is free software; you can redistribute itand /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "uiemptyapp.h"
#if PSYCLE_USE_TK == PSYCLE_TK_EMPTY
/* local */
#include "../../uicomponent.h"
#include "uiemptycomponentimp.h"
/* platform */
#include "../../detail/trace.h"

static psy_ui_EmptyApp* emptyapp = NULL;

/* prototypes */
static void psy_ui_emptyapp_onappdefaultschange(psy_ui_EmptyApp*);


static psy_ui_empty_ComponentImp* psy_ui_empty_component_details(psy_ui_Component*
	self)
{
	return (psy_ui_empty_ComponentImp*)self->imp->vtable->dev_platform(self->imp);
}

static void psy_ui_emptyapp_handle_destroy_window(psy_ui_EmptyApp*,
	psy_ui_Component*);

/* virtual */
static void psy_ui_emptyapp_dispose(psy_ui_EmptyApp*);
static int psy_ui_emptyapp_run(psy_ui_EmptyApp*);
static void psy_ui_emptyapp_stop(psy_ui_EmptyApp*);
static void psy_ui_emptyapp_close(psy_ui_EmptyApp*);
static void psy_ui_emptyapp_startmousehook(psy_ui_EmptyApp*);
static void psy_ui_emptyapp_stopmousehook(psy_ui_EmptyApp*);
static void psy_ui_emptyapp_sendevent(psy_ui_EmptyApp*, psy_ui_Component*,
	psy_ui_Event*);
static psy_ui_Component* psy_ui_emptyapp_component(psy_ui_EmptyApp*,
	uintptr_t handle);
psy_ui_empty_ComponentImp* psy_ui_emptyapp_componentimp(psy_ui_EmptyApp*,
	uintptr_t handle);
static psy_List* psy_ui_emptyapp_toplevel(psy_ui_EmptyApp*);
static void psy_ui_emptyimp_register_native(psy_ui_EmptyApp*,
	uintptr_t handle, psy_ui_ComponentImp*, bool top_level);
static void psy_ui_emptyimp_unregister_native(psy_ui_EmptyApp*,
	uintptr_t handle);
static const psy_List* psy_ui_emptyimp_fonts(const psy_ui_EmptyApp*);
static void psy_ui_emptyimp_read_fonts(psy_ui_EmptyApp*);

/* vtable */
static psy_ui_AppImpVTable imp_vtable;
static bool imp_vtable_initialized = FALSE;

static void imp_vtable_init(psy_ui_EmptyApp* self)
{
	assert(self);

	if (!imp_vtable_initialized) {
		imp_vtable = *self->imp.vtable;
		imp_vtable.dev_dispose =
			(psy_ui_fp_appimp_dispose)
			psy_ui_emptyapp_dispose;
		imp_vtable.dev_run =
			(psy_ui_fp_appimp_run)
			psy_ui_emptyapp_run;
		imp_vtable.dev_stop =
			(psy_ui_fp_appimp_stop)
			psy_ui_emptyapp_stop;
		imp_vtable.dev_close =
			(psy_ui_fp_appimp_close)
			psy_ui_emptyapp_close;
		imp_vtable.dev_onappdefaultschange =
			(psy_ui_fp_appimp_onappdefaultschange)
			psy_ui_emptyapp_onappdefaultschange;		
		imp_vtable.dev_startmousehook =
			(psy_ui_fp_appimp_startmousehook)
			psy_ui_emptyapp_startmousehook;
		imp_vtable.dev_stopmousehook =
			(psy_ui_fp_appimp_stopmousehook)
			psy_ui_emptyapp_stopmousehook;
		imp_vtable.dev_sendevent =
			(psy_ui_fp_appimp_sendevent)
			psy_ui_emptyapp_sendevent;
		imp_vtable.dev_component =
			(psy_ui_fp_appimp_component)
			psy_ui_emptyapp_component;		
		imp_vtable.dev_toplevel =
			(psy_ui_fp_appimp_toplevel)
			psy_ui_emptyapp_toplevel;
		imp_vtable.dev_register_native =
			(psy_ui_fp_appimp_register_native)
			psy_ui_emptyimp_register_native;
		imp_vtable.dev_unregister_native =
			(psy_ui_fp_appimp_unregister_native)
			psy_ui_emptyimp_unregister_native;
		imp_vtable.dev_fonts = 
			(psy_ui_fp_appimp_fonts)
			psy_ui_emptyimp_fonts;
		imp_vtable_initialized = TRUE;
	}
	self->imp.vtable = &imp_vtable;
}

/* implementation */
void psy_ui_emptyapp_init(psy_ui_EmptyApp* self, psy_ui_App* app,
	uintptr_t instance)
{
	assert(self);

	printf("psy_ui_emptyapp_init\n");
	psy_ui_appimp_init(&self->imp);
	imp_vtable_init(self);	
	emptyapp = self; /* init static reference */
	self->app = app;
	self->instance = instance;
	self->emptyid = 20000;	
	psy_table_init(&self->selfmap);
	psy_table_init(&self->emptyidmap);
	psy_table_init(&self->toplevelmap);	
	self->fonts = NULL;
	psy_ui_emptyimp_read_fonts(self);
}

void psy_ui_emptyapp_dispose(psy_ui_EmptyApp* self)
{	
	printf("psy_ui_emptyapp_dispose\n");
	psy_table_dispose(&self->selfmap);
	psy_table_dispose(&self->emptyidmap);
	psy_table_dispose(&self->toplevelmap);
	psy_list_deallocate(&self->fonts, NULL);	
}

int psy_ui_emptyapp_run(psy_ui_EmptyApp* self) 
{
	printf("psy_ui_emptyapp_run\n");
	return 1;
}

void psy_ui_emptyapp_startmousehook(psy_ui_EmptyApp* self)
{

}

void psy_ui_emptyapp_stopmousehook(psy_ui_EmptyApp* self)
{
	
}

void psy_ui_emptyapp_stop(psy_ui_EmptyApp* self)
{	
	printf("psy_ui_emptyapp_stop\n");
}

void psy_ui_emptyapp_close(psy_ui_EmptyApp* self)
{		
	assert(self);
	
	printf("psy_ui_emptyapp_close\n");
}

void psy_ui_emptyapp_onappdefaultschange(psy_ui_EmptyApp* self)
{
	printf("psy_ui_emptyapp_onappdefaultschange\n");
}

void psy_ui_emptyapp_sendevent(psy_ui_EmptyApp* self, psy_ui_Component* component,
	psy_ui_Event* ev)
{

}

psy_ui_Component* psy_ui_emptyapp_component(psy_ui_EmptyApp* self,
	uintptr_t handle)
{	
	psy_ui_empty_ComponentImp* imp;
	
	imp = psy_ui_emptyapp_componentimp(self, handle);
	if (imp) {
		return imp->component;
	}
	return NULL;
}

psy_ui_empty_ComponentImp* psy_ui_emptyapp_componentimp(psy_ui_EmptyApp* self,
	uintptr_t handle)
{
	return (psy_ui_empty_ComponentImp*)psy_table_at(&emptyapp->selfmap, handle);
}

psy_List* psy_ui_emptyapp_toplevel(psy_ui_EmptyApp* self)
{
	psy_List* rv;
	psy_TableIterator it;	
	
	rv = NULL;
	for (it = psy_table_begin(&self->toplevelmap);
			!psy_tableiterator_equal(&it, psy_table_end());
			psy_tableiterator_inc(&it)) {
		psy_ui_empty_ComponentImp* imp;

		imp = (psy_ui_empty_ComponentImp*)psy_tableiterator_value(&it);
		if (imp->component) {
			psy_list_append(&rv, imp->component);
		}
	}
	return rv;
}

void psy_ui_emptyimp_register_native(psy_ui_EmptyApp* self, uintptr_t handle,
	psy_ui_ComponentImp* imp, bool top_level)
{
	psy_table_insert(&emptyapp->selfmap, handle, imp);
	if (top_level) {
		psy_table_insert(&emptyapp->toplevelmap, handle, imp);
	}
}

void psy_ui_emptyimp_unregister_native(psy_ui_EmptyApp* self, uintptr_t handle)
{	
	psy_table_remove(&emptyapp->selfmap, handle);
	psy_table_remove(&emptyapp->toplevelmap, handle);
}

const psy_List* psy_ui_emptyimp_fonts(const psy_ui_EmptyApp* self)
{
	return self->fonts;
}

void psy_ui_emptyimp_read_fonts(psy_ui_EmptyApp* self)
{
	
}

#endif /* PSYCLE_TK_EMPTY */
