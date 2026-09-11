/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "eventdriverplugin.h"
/* local */
// #include "eventconfig.h"
#include "constants.h"
#include "exclusivelock.h"
#include "kbddriver.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* prototypes */
static void driver_deallocate(psy_audio_EventDriverPlugin*);
static int driver_open(psy_audio_EventDriverPlugin*);
static int driver_onerror(int err, char*);
static int driver_close(psy_audio_EventDriverPlugin*);
static int driver_dispose(psy_audio_EventDriverPlugin*);
static const psy_EventDriverInfo* driver_info(psy_audio_EventDriverPlugin*);
static void driver_configure(psy_audio_EventDriverPlugin*, const psy_Property*);
static const psy_Property* driver_configuration(const
	psy_audio_EventDriverPlugin*);
static void driver_write(psy_audio_EventDriverPlugin*, psy_EventDriverInput);
static void driver_cmd(psy_audio_EventDriverPlugin*, const char* section,
	psy_EventDriverInput, psy_EventDriverCmd*);
static psy_EventDriverCmd driver_getcmd(psy_audio_EventDriverPlugin*,
	const char* section);
static void driver_setcmddef(psy_audio_EventDriverPlugin*, const psy_Property*);
static void driver_idle(psy_audio_EventDriverPlugin*);
static psy_EventDriverInput driver_input(psy_audio_EventDriverPlugin*);

/* vtable */
static psy_EventDriverVTable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_audio_EventDriverPlugin* self)
{
	if (!vtable_initialized) {		
		vtable.open =
			(psy_eventdriver_fp_open)
			driver_open;
		vtable.deallocate =
			(psy_eventdriver_fp_deallocate)
			driver_deallocate;		
		vtable.close =
			(psy_eventdriver_fp_close)
			driver_close;
		vtable.dispose =
			(psy_eventdriver_fp_dispose)
			driver_dispose;
		vtable.info =
			(psy_eventdriver_fp_info)
			driver_info;
		vtable.configure =
			(psy_eventdriver_fp_configure)
			driver_configure;
		vtable.configuration =
			(psy_eventdriver_fp_configuration)
			driver_configuration;
		vtable.error =
			(psy_eventdriver_fp_error)
			driver_onerror;
		vtable.write =
			(psy_eventdriver_fp_write)
			driver_write;
		vtable.cmd =
			(psy_eventdriver_fp_cmd)
			driver_cmd;
		vtable.getcmd =
			(psy_eventdriver_fp_getcmd)
			driver_getcmd;		
		vtable.setcmddef =
			(psy_eventdriver_fp_setcmddef)
			driver_setcmddef;
		vtable.idle =
			(psy_eventdriver_fp_idle)
			driver_idle;
		vtable.input =
			(psy_eventdriver_fp_input)
			driver_input;
		vtable_initialized = TRUE;		
	}
	self->driver.vtable = &vtable;
}

/* implementation */
void psy_audio_eventdriverplugin_init(psy_audio_EventDriverPlugin* self)
{
	assert(self);
	
	vtable_init(self);	
	psy_library_init(&self->module);
	self->client = NULL;
}

void psy_audio_eventdriverplugin_dispose(psy_audio_EventDriverPlugin* self)
{
	assert(self);
	
	psy_audio_eventdriverplugin_clear(self);
	psy_library_dispose(&self->module);
}

psy_audio_EventDriverPlugin* psy_audio_eventdriverplugin_alloc(void)
{
	return (psy_audio_EventDriverPlugin*)
		malloc(sizeof(psy_audio_EventDriverPlugin));
}

psy_audio_EventDriverPlugin* psy_audio_eventdriverplugin_alloc_init(void)
{
	psy_audio_EventDriverPlugin* rv;
	
	rv = psy_audio_eventdriverplugin_alloc();
	if (rv) {
		psy_audio_eventdriverplugin_init(rv);
	}
	return rv;
}


void psy_audio_eventdriverplugin_load(psy_audio_EventDriverPlugin* self,
	const char* path)
{
	bool kbd;
	
	assert(self);
	
	kbd = TRUE;
	psy_audio_eventdriverplugin_clear(self);
	if (psy_strlen(path) > 0 && strcmp(path, "kbd") != 0) {
		psy_library_load(&self->module, path);	
		if (!psy_library_empty(&self->module)) {
			pfneventdriver_create fpdrivercreate;

			fpdrivercreate = (pfneventdriver_create)psy_library_functionpointer(
				&self->module, "psy_eventdriver_create");			
			if (fpdrivercreate) {
				self->client = fpdrivercreate();			
				kbd = FALSE;			
			}		
		}
	}
	if (kbd) {
		self->client = psy_audio_kbddriver_create();				
	}
	if (self->client) {
		self->client->logger = self->driver.logger;
	}
}

void psy_audio_eventdriverplugin_clear(psy_audio_EventDriverPlugin* self)
{
	assert(self);

	if (self->client) {		
		psy_eventdriver_close(self->client);
		psy_eventdriver_release(self->client);		
		self->client = NULL;
	}
	if (!psy_library_empty(&self->module)) {
		psy_library_unload(&self->module);
	}
}

void psy_audio_eventdriverplugin_connect(psy_audio_EventDriverPlugin* self,
	void* context, EVENTDRIVERWORKFN fp)
{
	assert(self);
	
	if (self->client) {
		psy_eventdriver_connect(self->client, context, fp);
	}
}

void driver_deallocate(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		psy_eventdriver_close(self->client);
	}
#if defined _CRTDBG_MAP_ALLOC
	if (self->client) {		
		psy_eventdriver_dispose(self->client);
		free(self->client);
	}		
#else
	if (self->client) {
		psy_eventdriver_deallocate(self->client);
	}
#endif
	self->client = NULL;		
	if (!psy_library_empty(&self->module)) {
		psy_library_unload(&self->module);
	}
	free(self);
}

int driver_open(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		return psy_eventdriver_open(self->client);
	}
	return 0;
}

int driver_close(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		return psy_eventdriver_close(self->client);
	}
	return 0;
}

int driver_dispose(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		return psy_eventdriver_dispose(self->client);
	}
	return 0;
}

const psy_EventDriverInfo* driver_info(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		return psy_eventdriver_info(self->client);
	}
	return NULL;
}

void driver_configure(psy_audio_EventDriverPlugin* self, const psy_Property*
	configuration)
{
	if (self->client) {
		psy_eventdriver_configure(self->client, configuration);
	}
}

const psy_Property* driver_configuration(const psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		return psy_eventdriver_configuration(self->client);
	}
	return NULL;
}

void driver_write(psy_audio_EventDriverPlugin* self, psy_EventDriverInput input)
{
	if (self->client) {
		psy_eventdriver_write(self->client, input);
	}
}

void driver_cmd(psy_audio_EventDriverPlugin* self, const char* section,
	psy_EventDriverInput input, psy_EventDriverCmd* cmd)
{
	if (self->client) {
		psy_eventdriver_cmd(self->client, section, input, cmd);
	}
}

psy_EventDriverCmd driver_getcmd(psy_audio_EventDriverPlugin* self,
	const char* section)
{	
	psy_EventDriverCmd cmd;
	
	if (self->client) {
		return psy_eventdriver_getcmd(self->client, section);
	}
	cmd.id = -1;
	return cmd;
}

void driver_setcmddef(psy_audio_EventDriverPlugin* self,
	const psy_Property* cmds)
{
	if (self->client) {
		psy_eventdriver_setcmddef(self->client, cmds);
	}
}

void driver_idle(psy_audio_EventDriverPlugin* self)
{
	if (self->client) {
		psy_eventdriver_idle(self->client);
	}
}

int driver_onerror(int err, char* text)
{	
	return 0;
}

psy_EventDriverInput driver_input(psy_audio_EventDriverPlugin* self)
{
	psy_EventDriverInput rv;
	
	if (self->client) {
		return psy_eventdriver_input(self->client);
	}
	rv = psy_eventdriverinput_make(0, 0, 0);
	return rv;	
}
