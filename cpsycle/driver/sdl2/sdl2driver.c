/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2022 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "../audiodriversettings.h"


#include <SDL2/SDL.h>  
#include "../audiodriver.h"
#include "../../detail/portable.h"


typedef struct {		
	psy_AudioDriver driver;
	psy_AudioDriverSettings settings;
	psy_Property* configuration;
	SDL_AudioDeviceID audio_dev;	
	bool running_;
	int (*error)(int, const char*);	
} Sdl2Driver;

static void driver_deallocate(psy_AudioDriver*);
static int driver_init(psy_AudioDriver*);
static int driver_open(psy_AudioDriver*);
static bool driver_opened(const psy_AudioDriver*);
static int driver_close(psy_AudioDriver*);
static int driver_dispose(psy_AudioDriver*);
static void driver_refresh_ports(psy_AudioDriver* self) { }
static void driver_configure(psy_AudioDriver*, psy_Property*);
static const psy_Property* driver_configuration(const psy_AudioDriver*);
static double driver_samplerate(psy_AudioDriver*);
static const psy_AudioDriverInfo* driver_info(psy_AudioDriver*);
static uint32_t playposinsamples(psy_AudioDriver*);
static bool addcaptureport(psy_AudioDriver* self, int idx) { return FALSE; }
static bool removecaptureport(psy_AudioDriver* self, int idx) { return TRUE; }
static void readbuffers(psy_AudioDriver* self, int idx, float** pleft,
	float** pright,
	uintptr_t numsamples)
{
	*pleft = NULL;
	*pright = NULL;
}
static const char* capturename(psy_AudioDriver* self, int index)
{
	return "undefined";
}
static uintptr_t numcaptures(const psy_AudioDriver* self)
{
	return 0;
}
static const char* playbackname(psy_AudioDriver* self, int index)
{
	return "undefined";
}
static int numplaybacks(psy_AudioDriver* self)
{
	return 0;
}

static void init_configuration(psy_AudioDriver*);
static void driver_fill_soundcard_buffer(void*, uint8_t * stream, int len);
static void do_stop(Sdl2Driver*);
static int on_error(int err, const char* msg);
static void init_output_devices(Sdl2Driver*);
static int driver_sdl2_sample_format(const Sdl2Driver*);
static int driver_sdl2_sample_byte_num(const Sdl2Driver*);

static psy_AudioDriverVTable vtable;
static int vtable_initialized = 0;

static void vtable_init(void)
{
	if (!vtable_initialized) {
		vtable.open = driver_open;
		vtable.opened = driver_opened;
		vtable.deallocate = driver_deallocate;
		vtable.close = driver_close;
		vtable.dispose = driver_dispose;
		vtable.refresh_ports = driver_refresh_ports;
		vtable.configure =
			(psy_audiodriver_fp_configure)
			driver_configure;
		vtable.configuration = driver_configuration;
		vtable.samplerate = driver_samplerate;		
		vtable.addcapture =
			(psy_audiodriver_fp_addcapture)
			addcaptureport;
		vtable.removecapture =
			(psy_audiodriver_fp_removecapture)
			removecaptureport;
		vtable.readbuffers = readbuffers;
		vtable.capturename = capturename;
		vtable.numcaptures = numcaptures;
		vtable.playbackname = playbackname;
		vtable.numplaybacks = numplaybacks;
		vtable.playposinsamples =
			(psy_audiodriver_fp_playposinsamples)
			playposinsamples;
		vtable.info = driver_info;
		vtable_initialized = 1;
	}
}

int on_error(int err, const char* msg)
{
	fprintf(stderr, "Sdl2 driver %s\n", msg);
	return 0;
}

EXPORT psy_AudioDriverInfo const * __cdecl GetPsycleDriverInfo(void)
{
	static psy_AudioDriverInfo info;

	info.guid = PSY_AUDIODRIVER_ALSA_GUID;
	info.Flags = 0;
	info.Name = "SDL2 Audio driver";
	info.ShortName = "sdl2";
	info.Version = 0;
	return &info;
}

EXPORT psy_AudioDriver* __cdecl driver_create(void)
{
	Sdl2Driver* dx = (Sdl2Driver*)malloc(sizeof(Sdl2Driver));
	if (dx != NULL) {		
		driver_init(&dx->driver);
		return &dx->driver;
	}
	return NULL;
}

void driver_deallocate(psy_AudioDriver* driver)
{
	driver_dispose(driver);
	free(driver);
}

int driver_init(psy_AudioDriver* driver)
{
	Sdl2Driver* self = (Sdl2Driver*) driver;		

	memset(self, 0, sizeof(Sdl2Driver));
	vtable_init();
	self->driver.vtable = &vtable;
	self->error = on_error;			
	self->running_ = FALSE;	
	psy_audiodriversettings_init(&self->settings);
	psy_audiodriversettings_setblockcount(&self->settings, 1);
	psy_audiodriversettings_setblockframes(&self->settings, 2048);
	if (SDL_Init(SDL_INIT_AUDIO) < 0) {
		return 1;
	}
	init_configuration(driver);
	return 0;
}

int driver_dispose(psy_AudioDriver* driver)
{
	Sdl2Driver* self = (Sdl2Driver*)driver;
		
	psy_property_deallocate(self->configuration);
	self->configuration = NULL;
	SDL_Quit();
	return 0;
}

void init_configuration(psy_AudioDriver* driver)
{	
	Sdl2Driver* self;
	psy_Property* property;	
	char key[256];

	self = (Sdl2Driver*)driver;
	psy_snprintf(key, 256, "sdl2-guid-%d", PSY_AUDIODRIVER_SDL2_GUID);
	self->configuration = psy_property_preventtranslate(
		psy_property_set_text(
		psy_property_allocinit_key(key), "Linux Sdl2 Interface"));	
	psy_property_hide(psy_property_append_int(self->configuration,
		"guid", PSY_AUDIODRIVER_SDL2_GUID, 0, 0));
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "name", "sdl"),
		TRUE), "driver.name");
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "vendor",
		"Psycedelics"),
		TRUE), "driver.vendor");
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "version", "1.0"),
		TRUE), "driver.version");
	init_output_devices(self);
	psy_property_set_text(psy_property_append_int(self->configuration,
		"bitdepth", psy_audiodriversettings_validbitdepth(&self->settings),
			0, 32), "driver.bitdepth");
	psy_property_set_text(psy_property_append_int(self->configuration,
		"samplerate", psy_audiodriversettings_samplespersec(&self->settings),
		0, 0), "driver.rate");	
	psy_property_set_text(psy_property_append_int(self->configuration,
		"buffersize", psy_audiodriversettings_blockframes(&self->settings),
		0, MAX_SAMPLES_WORKFN), "Framesize");
}

void init_output_devices(Sdl2Driver* self)
{
	char **hints;
	char** p;
	psy_Property* devices;	
	int err;
	int i;
	int count;
		
	devices = psy_property_set_text(
		psy_property_append_choice(self->configuration, "output-device", 1),
		"driver.outdevice");
	psy_property_set_hint(devices, PSY_PROPERTY_HINT_COMBO);	
	count = SDL_GetNumAudioDevices(0);	
	for (i = 0; i < count; ++i) {	
		const char* sdl_name;
		const char* name;
		char key[64];		
			
		psy_snprintf(key, 64, "%d", i);
		sdl_name = SDL_GetAudioDeviceName(i, FALSE /* iscapture */);
		if (sdl_name) {		
			name = sdl_name;
		} else {
			name = key;
		}
		psy_property_prevent_save(psy_property_preventtranslate(
			psy_property_set_text(psy_property_append_str(
			devices, key, name), name)));		
	}	
}

void driver_configure(psy_AudioDriver* driver, psy_Property* config)
{
	Sdl2Driver* self;
	
	self = (Sdl2Driver*)driver;
	if (self->configuration && config) {
		psy_property_sync(self->configuration, config);
	}
	psy_audiodriversettings_setsamplespersec(&self->settings,
		psy_property_at_int(self->configuration, "samplerate",
			44100));
	psy_audiodriversettings_setblockframes(&self->settings,
		psy_property_at_int(self->configuration, "buffersize",
			2048));
}

double driver_samplerate(psy_AudioDriver* driver)
{	
	Sdl2Driver* self;
	
	self = (Sdl2Driver*)driver;
	return psy_audiodriversettings_samplespersec(&self->settings);
}

int driver_open(psy_AudioDriver* driver)
{
    Sdl2Driver* self = (Sdl2Driver*) driver;
	SDL_AudioSpec want, have;	
	
	if (self->running_) {
		return TRUE;
	}
	if (driver->callback == NULL) {
		return FALSE;
	}	
	SDL_memset(&want, 0, sizeof(want)); /* or SDL_zero(want) */
	want.userdata = self;
	want.freq = psy_audiodriversettings_samplespersec(&self->settings);	
	want.format = driver_sdl2_sample_format(self);
	want.channels = 2;
	want.samples = psy_audiodriversettings_blockframes(&self->settings);
	want.callback = driver_fill_soundcard_buffer;
	self->audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
		SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (self->audio_dev > 0) {
		self->running_ = TRUE;		
	} else {
		self->running_ = FALSE;
		printf("%s\n", SDL_GetError());
	}
	if (self->audio_dev > 0) {
		printf("sdl2 audio started\n");		
		SDL_PauseAudioDevice(self->audio_dev, 0);
	}
	
	return self->running_;
}

int driver_sdl2_sample_format(const Sdl2Driver* self)
{
	switch (psy_audiodriversettings_validbitdepth(&self->settings)) {
	case 8:
		/* signed 8-bit samples */
		return AUDIO_S8;		
	case 16:
		/* signed 16-bit samples in little-endian byte order */
		return AUDIO_S16;
	/* no 24 bit format, will default to 16 */
	case 32:
		/* 32-bit integer samples in little-endian byte order */
		return AUDIO_S32;		
	default:		
		return AUDIO_S16;
	}
}

int driver_sdl2_sample_byte_num(const Sdl2Driver* self)
{
	switch (psy_audiodriversettings_validbitdepth(&self->settings)) {
	case 8:
		/* signed 8-bit samples */
		return 1;
	case 16:
		/* signed 16-bit samples in little-endian byte order */
		return 2;
	/* no 24 bit format, will default to 16 */
	case 32:
		/* 32-bit integer samples in little-endian byte order */
		return 4;		
	default:		
		return 2;
	}
}

bool driver_opened(const psy_AudioDriver* driver)
{
	Sdl2Driver* self;

	self = (Sdl2Driver*)driver;
	return (self->running_ != FALSE);
}


void do_stop(Sdl2Driver* self)
{
	/* return immediatly if not running */
	if (!self->running_) return;	
	
	SDL_LockAudioDevice(self->audio_dev);
	SDL_PauseAudioDevice(self->audio_dev, 1);
	SDL_UnlockAudioDevice(self->audio_dev);
	SDL_CloseAudioDevice(self->audio_dev);	
	self->running_ = FALSE;	
}

int driver_close(psy_AudioDriver* driver)
{	
	bool update;
	
	Sdl2Driver* self = (Sdl2Driver*)driver;
    do_stop(self);    
    update = FALSE;	
	return TRUE;
}

/* callback by sdl2 */
void driver_fill_soundcard_buffer(void* driver, uint8_t * stream, int len)
{
	Sdl2Driver* self;	
    float* host_input;
    int num;
    int frames_to_write;    
    int hostisplaying;
 
	self = (Sdl2Driver*)driver;
	frames_to_write = len / (driver_sdl2_sample_byte_num(self) * 
		psy_audiodriversettings_numchannels(&self->settings));
	num = frames_to_write;
	host_input = psy_audiodriver_work(&self->driver, &num, &hostisplaying);
	switch (driver_sdl2_sample_format(self)) {	
	case AUDIO_S16: {
		Sint16* soundcard_buffer;
		int frame;
		
		soundcard_buffer = (Sint16*)stream;			
		for (frame = 0; frame < frames_to_write; ++frame) {						
			*soundcard_buffer++ = (int16_t)(*host_input++); /* left channel */			
			*soundcard_buffer++ = (int16_t)(*host_input++); /* right channel */
		}
		break; }			
	default:
		break;
	}	
}

const psy_AudioDriverInfo* driver_info(psy_AudioDriver* self)
{
	return GetPsycleDriverInfo();
}

const psy_Property* driver_configuration(const psy_AudioDriver* driver)
{
	Sdl2Driver* self = (Sdl2Driver*)driver;
	
	return self->configuration;
}

uint32_t playposinsamples(psy_AudioDriver* driver)
{
	Sdl2Driver* self = (Sdl2Driver*)driver;

	return 0;
}

