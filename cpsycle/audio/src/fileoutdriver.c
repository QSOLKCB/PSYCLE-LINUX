/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "fileoutdriver.h"

/* platform */
#include "../../detail/os.h"
/* driver */
#include "../../driver/audiodriver.h"
#include "../../driver/audiodriversettings.h"
/* std */
#include <stdlib.h>
#include <string.h>
/* local */
#include "waveio.h"
#include <fileio.h>
/* dsp */
#include <dither.h>
/* thread_ */
#include <thread.h>

static int WriteMonoSample(psy_audio_WaveFormatChunk*, float SampleData, PsyFile*);
static int WriteStereoSample(psy_audio_WaveFormatChunk*, float LeftSample,
	float RightSample, PsyFile*);

/* FileContext */
typedef struct FileContext {
	PsyFile file;	
	uint32_t begin;
	uint32_t numsamplesbegin;
	uint32_t numsamples;
} FileContext;

/* psy_audio_FileOutDriver */
typedef struct psy_audio_FileOutDriver {
	psy_AudioDriver driver;
	psy_AudioDriverSettings settings;
	psy_Property* configuration;
	int poll_sleep_;
	int stop_polling_;
	bool do_dither_;
	psy_dsp_Dither dither_;
#if defined(DIVERSALIS__OS__MICROSOFT)	
	HANDLE hEvent;
#endif	
	FileContext filecontext;
	psy_audio_WaveFormatChunk format;
	bool write32bitsasint;
	psy_Thread thread_;
} psy_audio_FileOutDriver;

/* prototypes */
static void driver_deallocate(psy_AudioDriver*);
static int fileoutdriver_init(psy_audio_FileOutDriver*);
static int driver_open(psy_AudioDriver*);
static bool driver_opened(const psy_AudioDriver*);
static void driver_refresh_ports(psy_AudioDriver* self) { }
static void driver_configure(psy_audio_FileOutDriver*, psy_Property*);
static const psy_Property* driver_configuration(const psy_AudioDriver*);
static int driver_close(psy_AudioDriver*);
static int driver_dispose(psy_AudioDriver*);
static double samplerate(psy_AudioDriver*);

#if defined DIVERSALIS__OS__MICROSOFT
static unsigned int __stdcall PollerThread(void *psy_audio_FileOutDriver);
#else
static unsigned int PollerThread(void *psy_audio_FileOutDriver);
#endif
static void fileoutdriver_createfile(psy_audio_FileOutDriver*);
static void fileoutdriver_writebuffer(psy_audio_FileOutDriver*, float* pBuf,
	uintptr_t amount);
static void fileoutdriver_closefile(psy_audio_FileOutDriver*);
static uintptr_t playposinsamples(const psy_AudioDriver*);
static void fileoutdriver_make_config(psy_audio_FileOutDriver*);

/* vtable */
static psy_AudioDriverVTable vtable;
static int vtable_initialized = 0;

static void vtable_init(void)
{
	if (!vtable_initialized) {
		vtable.open = driver_open;
		vtable.deallocate = driver_deallocate;
		vtable.opened = driver_opened;
		vtable.refresh_ports = driver_refresh_ports;
		vtable.close = driver_close;
		vtable.dispose = driver_dispose;
		vtable.configure =
			(psy_audiodriver_fp_configure)
			driver_configure;
		vtable.configuration =
			driver_configuration;
		vtable.samplerate =
			(psy_audiodriver_fp_samplerate)samplerate;
		vtable.playposinsamples = playposinsamples;
		vtable_initialized = 1;
	}
}
/* implementation */
psy_AudioDriver* psy_audio_create_fileout_driver(void)
{
	psy_audio_FileOutDriver* out = malloc(sizeof(psy_audio_FileOutDriver));
	if (out) {		
		fileoutdriver_init(out);
		return &out->driver;
	}
	return NULL;
}

void driver_deallocate(psy_AudioDriver* driver)
{
	driver_dispose(driver);
	free(driver);
}

int fileoutdriver_init(psy_audio_FileOutDriver* self)
{
	memset(&self->driver, 0, sizeof(psy_AudioDriver));
	vtable_init();
	self->driver.vtable = &vtable;
	psy_audiodriversettings_init(&self->settings);	
	self->write32bitsasint = FALSE;
	psy_signal_init(&self->driver.signal_stop);
	fileoutdriver_make_config(self);
#if defined(DIVERSALIS__OS__MICROSOFT)	
	self->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
#endif	
	self->poll_sleep_ = 0;
	self->stop_polling_ = 0;
	psy_thread_init(&self->thread_);	
	return 0;
}

int driver_dispose(psy_AudioDriver* driver)
{
	psy_audio_FileOutDriver* self;

	self = (psy_audio_FileOutDriver*) driver;
	psy_thread_dispose(&self->thread_);
	psy_property_deallocate(self->configuration);
	self->configuration = NULL;
	psy_signal_dispose(&driver->signal_stop);	
	return 0;
}

int driver_open(psy_AudioDriver* driver)
{	
	psy_audio_FileOutDriver* self;	

	self = (psy_audio_FileOutDriver*)driver;
	self->stop_polling_ = 0;
	self->do_dither_ = psy_property_at_bool(self->configuration,
		"dither.enable", FALSE);
	if (self->do_dither_) {		
		intptr_t dither_depth_mode;
		uintptr_t dither_depth;		
		intptr_t dither_pdf_mode;
		intptr_t dither_pdf;
		intptr_t dither_noise_shape_mode;
		intptr_t dither_noise_shape;
				
		dither_depth_mode = psy_property_at_int(self->configuration,
			"dither.bitdepth", FALSE);
		switch (dither_depth_mode) {
		case 1: dither_depth = 8; break;
		case 2: dither_depth = 16; break;
		case 3: dither_depth = 24; break;
		case 4: dither_depth = 32; break;
		case 5: dither_depth = 32; break;
		default:
			dither_depth = 16;
			break;
		}
		dither_pdf_mode = psy_property_at_int(self->configuration,
			"dither.pdf", FALSE);
		switch (dither_pdf_mode) {
		case 0:
			dither_pdf = psy_dsp_DITHER_PDF_TRIANGULAR;
			break;
		case 1:
			dither_pdf = psy_dsp_DITHER_PDF_RECTANGULAR;
			break;
		case 2:
			dither_pdf = psy_dsp_DITHER_PDF_GAUSSIAN;
			break;
		default:
			dither_pdf = psy_dsp_DITHER_PDF_TRIANGULAR;
			break;
		}
		dither_noise_shape_mode = psy_property_at_int(self->configuration,
			"dither.noiseshape", FALSE);
		switch (dither_noise_shape_mode) {
		case 0:
			dither_noise_shape = psy_dsp_DITHER_NOISESHAPE_NONE;
			break;
		case 1:
			dither_noise_shape = psy_dsp_DITHER_NOISESHAPE_HIGHPASS;
			break;		
		default:
			dither_noise_shape = psy_dsp_DITHER_NOISESHAPE_NONE;
			break;
		}
		psy_dsp_dither_init(&self->dither_);
		psy_dsp_dither_configure(&self->dither_,
			psy_dsp_dithersettings_make(TRUE, dither_depth,
				dither_pdf, dither_noise_shape));
		
	}
#if defined(DIVERSALIS__OS__MICROSOFT)	
	ResetEvent(self->hEvent);
#endif	
	psy_thread_start(&self->thread_, self, PollerThread);
	return 0;
}

bool driver_opened(const psy_AudioDriver* driver)
{
	psy_audio_FileOutDriver* self;	

	self = (psy_audio_FileOutDriver*) driver;
	return TRUE;
}

int driver_close(psy_AudioDriver* driver)
{
	psy_audio_FileOutDriver* self = (psy_audio_FileOutDriver*) driver;

	self->stop_polling_ = 1;
#if defined(DIVERSALIS__OS__MICROSOFT)		
	WaitForSingleObject(self->hEvent, INFINITE);
#endif	
	return 0;
}

void fileoutdriver_make_config(psy_audio_FileOutDriver* self)
{
	psy_AudioDriver* driver = &self->driver;
	psy_Property* samplerate;
	psy_Property* bitdepth;
	psy_Property* channels;		
	psy_Property* dither;
	psy_Property* ditherpdf;
	psy_Property* dithernoiseshape;

	self->configuration = psy_property_set_text(
		psy_property_preventtranslate(psy_property_allocinit_key("fileout")),
		"Output");
	psy_property_hide(psy_property_set_text(
		psy_property_setreadonly(
			psy_property_append_str(self->configuration, "name", "FileOut Driver"),
			TRUE), "Name"));
	psy_property_hide(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "vendor", "Psycledelics"),
		TRUE));
	psy_property_hide(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "version", "1.0"),
		TRUE));
	psy_property_set_text(
		psy_property_append_str(self->configuration, "outputpath",
			"Untitled.wav"),
		"Output to file");	
	samplerate = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(self->configuration, "samplerate", 5),
		"Sample Rate"), /* default 5: 44100*/
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(psy_property_append_int(samplerate, "sr8000",
		8000, 0, 0), "8000 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr11025",
		11025, 0, 0), "11025 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr16000",
		16000, 0, 0), "16000 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr22050",
		22050, 0, 0), "22050 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr32000",
		32000, 0, 0), "32000 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr44100",
		44100, 0, 0), "44100 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr48000",
		48000, 0, 0), "48000 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr88200",
		88200, 0, 0), "88200 hz");
	psy_property_set_text(psy_property_append_int(samplerate, "sr96000",
		96000, 0, 0), "96000 hz");
	bitdepth = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(self->configuration, "bitdepth", 1),
		"Bit Depth"), /* default 1: 16 bit*/
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(psy_property_append_int(bitdepth, "bits8",
		8, 0, 0), "8 bit");
	psy_property_set_text(psy_property_append_int(bitdepth, "bits16",
		16, 0, 0), "16 bit");
	psy_property_set_text(psy_property_append_int(bitdepth, "bits24",
		24, 0, 0), "24 bit");
	psy_property_set_text(psy_property_append_int(bitdepth, "bits32",
		32, 0, 0), "32 bit (int)");
	psy_property_set_text(psy_property_append_int(bitdepth, "bits32f",
		32, 0, 0), "32 bit (float)");
	channels = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(self->configuration, "channels", 3),
		"Channels"), /* default 3: stereo*/
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(psy_property_append_int(channels, "mono-mix",
		0, 0, 0), "Mono (Mix)");
	psy_property_set_text(psy_property_append_int(channels, "mono-left",
		1, 0, 0), "Mono (Left)");
	psy_property_set_text(psy_property_append_int(channels, "mono-right",
		2, 0, 0), "Mono (Right)");
	psy_property_set_text(psy_property_append_int(channels, "stereo",
		3, 0, 0), "Stereo");	
	
	dither = psy_property_set_text(
		psy_property_append_section(self->configuration, "dither"),
		"Dither");
	psy_property_set_text(
		psy_property_append_bool(dither, "enable", FALSE),
		"Enable");
	ditherpdf = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(dither, "pdf", 0),
		"render.pdf"), PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(
		psy_property_append_str(ditherpdf, "triangular", ""),
		"render.triangular");
	psy_property_set_text(
		psy_property_append_str(ditherpdf, "rectangular", ""),
		"render.rectangular");
	psy_property_set_text(
		psy_property_append_str(ditherpdf, "gaussian", ""),
		"render.gaussian");
	dithernoiseshape = psy_property_set_hint(psy_property_set_text(
		psy_property_append_choice(dither, "noiseshape", 0),
		"render.noise-shaping"),
		PSY_PROPERTY_HINT_COMBO);
	psy_property_set_text(
		psy_property_append_str(dithernoiseshape, "none", ""),
		"render.none");
	psy_property_set_text(
		psy_property_append_str(dithernoiseshape, "highpass", ""),
		"render.high-pass-contour");
}


void driver_configure(psy_audio_FileOutDriver* self, psy_Property* config)
{
	psy_AudioDriver* driver = &self->driver;
	psy_Property* property;	

	if (config) {
		psy_property_sync(self->configuration, config);
	}		
	property = psy_property_at(self->configuration, "samplerate",
		PSY_PROPERTY_TYPE_CHOICE);
	if (property) {
		intptr_t sr;

		property = psy_property_at_choice(property);
		if (property) {
			sr = psy_property_item_int(property);
			psy_audiodriversettings_setsamplespersec(&self->settings,
				(double)sr);
		}
	}
	property = psy_property_at(self->configuration, "bitdepth",
		PSY_PROPERTY_TYPE_CHOICE);
	if (property) {
		intptr_t bitdepth;

		property = psy_property_at_choice(property);
		if (property) {
			self->write32bitsasint = (psy_property_item_int(property) == 3);
			bitdepth = psy_property_item_int(property);
			psy_audiodriversettings_setvalidbitdepth(&self->settings,
				(uintptr_t)bitdepth);			
		}
	}
	property = psy_property_at(self->configuration, "channels",
		PSY_PROPERTY_TYPE_CHOICE);
	if (property) {
		psy_AudioDriverChannelMode channelmode;

		channelmode = (psy_AudioDriverChannelMode)psy_property_item_int(property);
		psy_audiodriversettings_setchannelmode(&self->settings,
			channelmode);
	}	
}

double samplerate(psy_AudioDriver* self)
{
	return (double)44100.0;
}

#if defined DIVERSALIS__OS__MICROSOFT
unsigned int __stdcall PollerThread(void* driver)
#else
unsigned int PollerThread(void* driver)
#endif
{	
	int n;	
	uint32_t blocksize = 2048;
	int hostisplaying = 1;

	psy_audio_FileOutDriver* self = (psy_audio_FileOutDriver*) driver;
#if defined(DIVERSALIS__OS__MICROSOFT)	
	SetThreadPriority(GetCurrentThread(),
		THREAD_PRIORITY_ABOVE_NORMAL);
#endif		
	fileoutdriver_createfile(self);	
	while (!self->stop_polling_ && hostisplaying)
	{
		float *pBuf;		
		
		n = blocksize / 2;
		pBuf = self->driver.callback(self->driver.callbackcontext, &n,
			&hostisplaying);		
		fileoutdriver_writebuffer(self, pBuf, blocksize);
		if (self->poll_sleep_ > 0) {
			psy_sleep_for(self->poll_sleep_);
		}
	}
	fileoutdriver_closefile(self);
#if defined(DIVERSALIS__OS__MICROSOFT)			
	SetEvent(self->hEvent);
#endif	
	psy_signal_emit(&self->driver.signal_stop, self, 0);
	return 0;
}

void fileoutdriver_createfile(psy_audio_FileOutDriver* self)
{		
	int bitspersample = 16;
	uint8_t temp8 = 0;
	uint16_t temp16 = 0;
	uint32_t temp32 = 0;	
	uint32_t begin = 0;
	uint32_t pcmbegin = 0;
	uint32_t channel = 0;
	uint32_t frame = 0;
	uint32_t numsamples = 0;
	PsyFile* file;
	const char* path;

	file = &self->filecontext.file;
	self->filecontext.numsamples = 0;
	path = psy_property_at_str(self->configuration, "outputpath",
		"Untitled.wav");
	if (!psyfile_create(file, path, 1)) {
		return;
	}
	psyfile_write(file, "RIFF", 4);
	begin = psyfile_getpos(file);
	self->filecontext.begin = begin;
	temp32 = 0;
	psyfile_write(file, &temp32, sizeof(temp32));
	/* Write Format Chunk */
	psy_audio_waveformatchunk_config(&self->format,
		(uint32_t)psy_audiodriversettings_samplespersec(&self->settings),
		(uint16_t)psy_audiodriversettings_validbitdepth(&self->settings),
		(uint16_t)psy_audiodriversettings_numchannels(&self->settings),
		FALSE /* isfloat */);
	psyfile_write(file, "WAVEfmt ", 8);
	temp32 = 0;
	pcmbegin = psyfile_getpos(file);
	psyfile_write(file, &temp32, sizeof(temp32));	
	temp16 = self->format.wFormatTag;
	psyfile_write(file, &temp16, sizeof(temp16));
	temp16 = self->format.nChannels;
	psyfile_write(file, &temp16, sizeof(temp16));
	temp32 = self->format.nSamplesPerSec;
	psyfile_write(file, &temp32, sizeof(temp32));
	temp32 = self->format.nAvgBytesPerSec;
	psyfile_write(file, &temp32, sizeof(temp32));
	temp16 = self->format.nBlockAlign;
	psyfile_write(file, &temp16, sizeof(temp16));
	temp16 = self->format.wBitsPerSample;
	psyfile_write(file, &temp16, sizeof(temp16));
	{
		uint32_t pos2;
		uint32_t size;
	
		pos2 = psyfile_getpos(file); 
		size = (pos2 - pcmbegin - 4);
		psyfile_seek(file, pcmbegin);
		psyfile_write(file, &size, sizeof(size));
		psyfile_seek(file, pos2);
	}
	/* Write Data Chunk */
	psyfile_write(file, "data", 4);
	numsamples = 0;
	self->filecontext.numsamplesbegin = psyfile_getpos(file);
	self->filecontext.numsamples = 0;
	temp32 = 0xEDED;
	psyfile_write(file, &temp32, sizeof(temp32));
}

void fileoutdriver_writebuffer(psy_audio_FileOutDriver* self, float* pBuf, uintptr_t amount)
{
	uintptr_t i;
	float* currbuf = pBuf;	
	
	amount /= 2;	
	
	switch (psy_audiodriversettings_channelmode(&self->settings)) {
	case psy_AUDIODRIVERCHANNELMODE_MONO_MIX: /* mono mix */
		for (i = 0; i < amount; i++) {
			float l, r;

			l = *currbuf;
			++currbuf;
			r = *currbuf;
			++currbuf;						
			/* argh! dithering both channels and then mixing.. we'll have to sum the arrays before-hand, and then dither. */
			WriteMonoSample(&self->format, ((l + r) * 0.5f), &self->filecontext.file);
		}
	break;
	case psy_AUDIODRIVERCHANNELMODE_MONO_LEFT: /* mono L */
		for (i = 0; i < amount; ++i) {
			WriteMonoSample(&self->format, *currbuf, &self->filecontext.file);
			currbuf++;
			currbuf++; /* skip right channel */
		}	
	break;
	case psy_AUDIODRIVERCHANNELMODE_MONO_RIGHT: /* mono R */
		for (i = 0; i < amount; ++i) {
			currbuf++; /* skip left channel */
			WriteMonoSample(&self->format, *currbuf, &self->filecontext.file);
			currbuf++;
		}
		break;
	default: /* psy_AUDIODRIVERCHANNELMODE_STEREO */
		for (i = 0; i < amount; ++i) {
			float l;
			float r;

			l = *currbuf;
			++currbuf;
			r = *currbuf;
			++currbuf;
			if(self->do_dither_) {
				psy_dsp_dither_process(&self->dither_, &l, 1);				
				psy_dsp_dither_process(&self->dither_, &r, 1);
			}
			WriteStereoSample(&self->format, l, r, &self->filecontext.file);
		}
		break;
	}
}

void fileoutdriver_closefile(psy_audio_FileOutDriver* self)
{
	uint32_t pos2;
	uint32_t size;
	PsyFile* file;

	file = &self->filecontext.file;
	{
		uint32_t temp32;

		pos2 = psyfile_getpos(file);
		psyfile_seek(file, self->filecontext.numsamplesbegin);
		if (pos2 >= self->filecontext.numsamplesbegin + sizeof(temp32)) {
			temp32 = pos2 - self->filecontext.numsamplesbegin - sizeof(temp32);
		} else {
			temp32 = 0;
		}
		psyfile_write(file, &temp32, sizeof(temp32));
		psyfile_seek(file, pos2);	
	}	
	pos2 = psyfile_getpos(file); 
	size = (pos2 - self->filecontext.begin - 4);
	psyfile_seek(file, self->filecontext.begin);
	psyfile_write(file, &size, sizeof(size));
	psyfile_seek(file, pos2);	
	psyfile_close(file);
}

int WriteMonoSample(psy_audio_WaveFormatChunk* wave_format, float SampleData,
	PsyFile* fp)
{
	int32_t d;

	switch (wave_format->wFormatTag)
	{
	case 1: /* Integer PCM */
		if (SampleData > 32767.0f) SampleData = 32767.0f;
		else if (SampleData < -32768.0f) SampleData = -32768.0f;
		switch (wave_format->wBitsPerSample) {
		case 8:
			/* pcm_data.ckSize += 1; */
			d = (int32_t)(SampleData / 256.0f);
			d += 128;
			return psyfile_write(fp, &d, 1);
		case 16:
			/* pcm_data.ckSize += 2; */
			d = (int32_t)(SampleData);
			return psyfile_write(fp, &d, 2);
		case 24:
			/* pcm_data.ckSize += 3; */
			d = (int32_t)(SampleData * 256.0f);
			return psyfile_write(fp, &d, 3);
		case 32:
			/* pcm_data.ckSize += 4;
			   d = (int32_t)(SampleData * 65536.0f);
			*/
			return psyfile_write(fp, &SampleData, 4);
		default:
			break;
		}
		break;
	case 3: /* IEEE float PCM */
		if (wave_format->wBitsPerSample == 32)
		{
			/* pcm_data.ckSize += 4; */
			const float f = SampleData * 0.000030517578125f;
			return psyfile_write(fp, &f, 4);
		}
	default:
		break;
	}
	return PSY_ERRFILE;
}

int WriteStereoSample(psy_audio_WaveFormatChunk* wave_format, float LeftSample, float RightSample,
	PsyFile* fp)
{
	int retcode;
	int32_t l, r;
	float f;


	retcode = PSY_OK;
	switch (wave_format->wFormatTag)
	{
	case 1: /* Integer PCM */
		if (LeftSample > 32767.0f) LeftSample = 32767.0f;
		else if (LeftSample < -32768.0f) LeftSample = -32768.0f;
		if (RightSample > 32767.0f) RightSample = 32767.0f;
		else if (RightSample < -32768.0f) RightSample = -32768.0f;
		switch (wave_format->wBitsPerSample) {
		case 8:
			l = (int32_t)(LeftSample / 256.0f);
			r = (int32_t)(RightSample / 256.0f);
			l += 128;
			r += 128;
			retcode = psyfile_write(fp, &l, 1);
			if (retcode == PSY_OK) {
				retcode = psyfile_write(fp, &r, 1);
				/* if (retcode == DDC_SUCCESS) pcm_data.ckSize += 2; */
			}
			break;
		case 16:
			l = (int32_t)(LeftSample);
			r = (int32_t)(RightSample);
			retcode = psyfile_write(fp, &l, 2);
			if (retcode == PSY_OK) {
				retcode = psyfile_write(fp, &r, 2);
				/* if (retcode == DDC_SUCCESS) pcm_data.ckSize += 4; */
			}
			break;
		case 24:
			l = (int32_t)(LeftSample * 256.0f);
			r = (int32_t)(RightSample * 256.0f);
			retcode = psyfile_write(fp, &l, 3);
			if (retcode == PSY_OK) {
				retcode = psyfile_write(fp, &r, 3);
				/* if (retcode == DDC_SUCCESS) pcm_data.ckSize += 6; */
			}
			break;
		case 32:
			l = (int32_t)(LeftSample * 65536.0f);
			r = (int32_t)(RightSample * 65536.0f);
			retcode = psyfile_write(fp, &l, 4);
			if (retcode == PSY_OK) {
				retcode = psyfile_write(fp, &r, 4);
				/* if (retcode == DDC_SUCCESS) pcm_data.ckSize += 8; */
			}
			break;
		default:
			retcode = PSY_ERRFILE;
		}
		break;
	case 3: /* IEEE float PCM */
		if (wave_format->wBitsPerSample == 32)
		{
			f = LeftSample * 0.000030517578125f;
			retcode = psyfile_write(fp, &f, 4);
			if (retcode == PSY_OK)
			{
				f = RightSample * 0.000030517578125f;
				retcode = psyfile_write(fp, &f, 4);
				/* if (retcode == DDC_SUCCESS) pcm_data.ckSize += 8; */
			}
		}
	default:
		break;
	}
	return retcode;
}

const psy_Property* driver_configuration(const psy_AudioDriver* driver)
{
	psy_audio_FileOutDriver* self = (psy_audio_FileOutDriver*)driver;

	return self->configuration;
}

uintptr_t playposinsamples(const psy_AudioDriver* driver)
{
	return 0;
}
