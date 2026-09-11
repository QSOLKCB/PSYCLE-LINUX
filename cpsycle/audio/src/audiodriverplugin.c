/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "audiodriverplugin.h"
/* local */
#include "audioconfig.h"
#include "constants.h"
#include "exclusivelock.h"
#include "silentdriver.h"
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"


/* prototypes */
static int driver_open(psy_audio_AudioDriverPlugin*);
static bool driver_opened(const psy_audio_AudioDriverPlugin*);
static void driver_deallocate(psy_audio_AudioDriverPlugin*);
static int driver_close(psy_audio_AudioDriverPlugin*);
static int driver_dispose(psy_audio_AudioDriverPlugin*);
static void driver_refresh_ports(psy_audio_AudioDriverPlugin*);
static void driver_configure(psy_audio_AudioDriverPlugin*, psy_Property*);
static const psy_Property* driver_configuration(const psy_audio_AudioDriverPlugin*);
static double driver_samplerate(psy_audio_AudioDriverPlugin*);
static bool addcaptureport(psy_audio_AudioDriverPlugin*, int idx);
static bool removecaptureport(psy_audio_AudioDriverPlugin*, int idx);
static void readbuffers(psy_audio_AudioDriverPlugin*, int idx, float** pleft,
	float** pright, uintptr_t numsamples);
static const char* capturename(psy_audio_AudioDriverPlugin*, int index);
static uintptr_t numcaptures(const psy_audio_AudioDriverPlugin*);
static const char* playbackname(psy_audio_AudioDriverPlugin*, int index);
static int numplaybacks(psy_audio_AudioDriverPlugin*);
static uintptr_t playposinsamples(psy_audio_AudioDriverPlugin*);
static const psy_AudioDriverInfo* driver_info(psy_audio_AudioDriverPlugin*);

/* vtable */
static psy_AudioDriverVTable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(psy_audio_AudioDriverPlugin* self)
{
	if (!vtable_initialized) {		
		vtable.open = 
			(psy_audiodriver_fp_open)
			driver_open;
		vtable.opened =
			(psy_audiodriver_fp_opened)
			driver_opened;
		vtable.deallocate =
			(psy_audiodriver_fp_deallocate)
			driver_deallocate;
		vtable.close =
			(psy_audiodriver_fp_close)
			driver_close;
		vtable.dispose = 
			(psy_audiodriver_fp_dispose)
			driver_dispose;
		vtable.refresh_ports =
			(psy_audiodriver_fp_refresh_ports)
			driver_refresh_ports;
		vtable.configure =
			(psy_audiodriver_fp_configure)
			driver_configure;
		vtable.configuration =
			(psy_audiodriver_fp_configuration)
			driver_configuration;
		vtable.samplerate =
			(psy_audiodriver_fp_samplerate)
			driver_samplerate;		
		vtable.addcapture =
			(psy_audiodriver_fp_addcapture)
			addcaptureport;
		vtable.removecapture =
			(psy_audiodriver_fp_removecapture)
			removecaptureport;
		vtable.readbuffers =
			(psy_audiodriver_fp_readbuffers)
			readbuffers;
		vtable.capturename =
			(psy_audiodriver_fp_capturename)
			capturename;
		vtable.numcaptures =
			(psy_audiodriver_fp_numcaptures)
			numcaptures;
		vtable.playbackname =
			(psy_audiodriver_fp_playbackname)
			playbackname;
		vtable.numplaybacks =
			(psy_audiodriver_fp_numplaybacks)
			numplaybacks;
		vtable.playposinsamples =
			(psy_audiodriver_fp_playposinsamples)
			playposinsamples;
		vtable.info =
			(psy_audiodriver_fp_info)
			driver_info;
		vtable_initialized = TRUE;
	}
	self->driver.vtable = &vtable;
}

/* implementation */
void psy_audio_audiodriverplugin_init(psy_audio_AudioDriverPlugin* self, struct psy_Logger* logger)
{
	assert(self);
	
	vtable_init(self);	
	psy_library_init(&self->module);
	self->client = NULL;
	self->driver.logger = logger;
}

void psy_audio_audiodriverplugin_dispose(psy_audio_AudioDriverPlugin* self)
{
	assert(self);
	
	psy_audio_audiodriverplugin_clear(self);
	psy_library_dispose(&self->module);
}

void psy_audio_audiodriverplugin_load(psy_audio_AudioDriverPlugin* self,
	const char* path)
{
	bool silent;
	
	assert(self);
	
	silent = TRUE;
	psy_audio_audiodriverplugin_clear(self);
	if (psy_strlen(path) > 0 && strcmp(path, "silent") != 0) {
		psy_library_load(&self->module, path);	
		if (!psy_library_empty(&self->module)) {
			pfndriver_create fpdrivercreate;

			fpdrivercreate = (pfndriver_create)psy_library_functionpointer(
				&self->module, "driver_create");			
			if (fpdrivercreate) {
				self->client = fpdrivercreate();
				self->client->logger = self->driver.logger;
				silent = FALSE;				
				psy_audio_exclusivelock_enable();
			}		
		}
	}
	if (silent) {
		self->client = psy_audio_create_silent_driver();
		self->client->logger = self->driver.logger;
		psy_audio_exclusivelock_disable();
	}
}

void psy_audio_audiodriverplugin_clear(psy_audio_AudioDriverPlugin* self)
{
	assert(self);

	if (self->client) {
		psy_audiodriver_close(self->client);
		psy_audiodriver_deallocate(self->client);		
		self->client = NULL;
	}
	if (!psy_library_empty(&self->module)) {
		psy_library_unload(&self->module);
	}
}

void psy_audio_audiodriverplugin_connect(psy_audio_AudioDriverPlugin* self,
	void* platform_handle, void* context, AUDIODRIVERWORKFN fp)
{
	assert(self);
	
	if (self->client) {
		psy_audiodriver_connect(self->client, context, fp, platform_handle);
	}
}


int driver_open(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_open(self->client);
	}
	return 0;
}

bool driver_opened(const psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return  psy_audiodriver_opened(self->client);	
	}
	return FALSE;
}

void driver_deallocate(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		psy_audiodriver_deallocate(self->client);
	}
}

int driver_close(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_close(self->client);
	}
	return 0;
}

int driver_dispose(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_dispose(self->client);
	}
	return 0;
}

void driver_refresh_ports(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		psy_audiodriver_refresh_ports(self->client);
	}
}

void driver_configure(psy_audio_AudioDriverPlugin* self, psy_Property* configuration)
{
	if (self->client) {
		psy_audiodriver_configure(self->client, configuration);
	}
}

const psy_Property* driver_configuration(const psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_configuration(self->client);
	}
	return NULL;
}

double driver_samplerate(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_samplerate(self->client);
	}
	return (double)44100.0;
}

bool addcaptureport(psy_audio_AudioDriverPlugin* self, int idx)
{
	if (self->client) {
		return  psy_audiodriver_addcapture(self->client, idx);
	}
	return FALSE;
}

bool removecaptureport(psy_audio_AudioDriverPlugin* self, int idx)
{
	if (self->client) {
		return psy_audiodriver_removecapture(self->client, idx);
	}
	return FALSE;
}

void readbuffers(psy_audio_AudioDriverPlugin* self, int idx, float** pleft,
	float** pright, uintptr_t numsamples)
{
	if (self->client) {
		psy_audiodriver_read_buffers(self->client, idx, pleft, pright,
			numsamples);
	}
}

const char* capturename(psy_audio_AudioDriverPlugin* self, int index)
{
	if (self->client) {
		return psy_audiodriver_capturename(self->client, index);
	}
	return "";
}

uintptr_t numcaptures(const psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_numcaptures(self->client);
	}
	return 0;
}

const char* playbackname(psy_audio_AudioDriverPlugin* self, int index)
{
	if (self->client) {
		return psy_audiodriver_playbackname(self->client, index);
	}
	return "";
}

int numplaybacks(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_numplaybacks(self->client);
	}
	return 0;
}

uintptr_t playposinsamples(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_playpos_in_samples(self->client);
	}
	return 0;
}

const psy_AudioDriverInfo* driver_info(psy_audio_AudioDriverPlugin* self)
{
	if (self->client) {
		return psy_audiodriver_info(self->client);
	}
	return NULL;
}
