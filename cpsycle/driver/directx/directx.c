/* 
/* This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
/* copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"
#include "../audiodriversettings.h"
#include "avrt.h"

#include <MMReg.h>
#define DIRECTSOUND_VERSION 0x8000
#include <ks.h>
#include <ksmedia.h>

#include <dsound.h>

/* linking */
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winmm.lib")

/* includes */
#include <logger.h>
#include <string.h>
#include "../audiodriver.h"
#include <stdio.h>
#include <quantize.h>
#include <operations.h>
#include <hashtbl.h>
#include "../../detail/trace.h"
#include "../../detail/portable.h"

#undef KSDATAFORMAT_SUBTYPE_PCM
const GUID KSDATAFORMAT_SUBTYPE_PCM = { 0x00000001, 0x0000, 0x0010,
{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };

#undef KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
const GUID KSDATAFORMAT_SUBTYPE_IEEE_FLOAT = { 00000003, 0x0000, 0x0010,
{0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71} };


#define BYTES_PER_SAMPLE 4	/* 2 * 16bits */
#define SHORT_MIN	-32768
#define SHORT_MAX	32767
	

static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);
static BOOL CALLBACK DSCaptureEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);

typedef struct PortEnums {
	char* portname;
	LPGUID guid;
} PortEnums;

INLINE void portenums_init(PortEnums* self)
{
	assert(self);

	self->guid = 0;
	self->portname = psy_strdup("");
}

INLINE void portenums_init_all(PortEnums* self, LPGUID guid, const char* portname)
{
	assert(self);

	self->guid = (guid == NULL) ? (GUID*) &DSDEVID_DefaultPlayback : guid;
	self->portname = portname ? psy_strdup(portname) : psy_strdup("");
}

INLINE void portenums_dispose(PortEnums* self)
{
	assert(self);

	self->guid = 0;
	free(self->portname);
	self->portname = NULL;
}

INLINE PortEnums* portenums_alloc(void)
{
	return (PortEnums*)malloc(sizeof(PortEnums));
}

INLINE bool portenums_isformatsupported(WAVEFORMATEXTENSIBLE* pwfx, bool isInput)
{
	/*! @todo: Implement */
	return TRUE;
}

typedef struct PortCapt {
	LPGUID _pGuid;
	LPDIRECTSOUNDCAPTURE8 _pDs;
	LPDIRECTSOUNDCAPTUREBUFFER8  _pBuffer;
	int _lowMark;
	float* pleft;
	float* pright;
	int _machinepos;
} PortCapt;

INLINE portcapt_init(PortCapt* self)
{
	self->pleft = 0;
	self->pright = 0;
	self->_pGuid = 0;
	self->_pDs = 0;
	self->_pBuffer = 0;
	self->_lowMark = 0;
	self->_machinepos = 0;
}

INLINE void portcap_dispose(PortCapt* self)
{
	assert(self);

	if (self->_pBuffer) {
		IDirectSoundCaptureBuffer_Stop(self->_pBuffer);		
		IDirectSoundCaptureBuffer_Release(self->_pBuffer);
		self->_pBuffer = NULL;
	}	
	if ((self->_pDs) != NULL) {
		IDirectSoundCapture_Release(self->_pDs);
		self->_pDs = NULL;
	}
	dsp.memory_dealloc(self->pleft);
	self->pleft = NULL;
	dsp.memory_dealloc(self->pright);
	self->pright = NULL;
}


/*! @struct DXDriver */
typedef struct DXDriver {
	psy_AudioDriver driver;	
	psy_AudioDriverSettings settings;
	psy_Property* configuration;
	HWND m_hWnd;
	int _dither;	
	unsigned int pollSleep_;	
	
	int _initialized;	
	/* controls if the driver is supposed to be running or not */
	int _running;
	/* informs the real state of the DSound buffer (see the control of buffer play in DoBlocks()) */
	int _playing; 
	/* Controls if we want the thread to be running or not */
	int _threadRun;	
	GUID device_guid_;
	int _deviceIndex;		
	int _currentOffset;
	DWORD _dsBufferSize;
	uint32_t _lowMark;
	uint32_t _highMark;	
	int _buffersToDo;	
	/* number of "wraparounds" to compensate the GetCurrentPosition() call. */
	int m_readPosWraps;
	psy_List* _playEnums;	/* PortEnums */
	psy_List* _capEnums;	/* PortEnums idx : mme input device id */
	psy_Table _capPorts;	/* mme input device id, PortCapt */
	LPDIRECTSOUND8 _pDs;
	LPDIRECTSOUNDBUFFER8 _pBuffer;
	HANDLE hEvent;	
} DXDriver;

static int dxdriver_init(psy_AudioDriver*);
static int dxdriver_open(psy_AudioDriver*);
static bool dxdriver_opened(const psy_AudioDriver*);
static int dxdriver_close(psy_AudioDriver*);
static int dxdriver_dispose(psy_AudioDriver*);
static void dxdriver_refresh_ports(psy_AudioDriver* self) { }
static void dxdriver_configure(psy_AudioDriver*, const psy_Property*);
static const psy_Property* dxdriver_configuration(const psy_AudioDriver*);
static double dxdriver_samplerate(psy_AudioDriver*);
static const char* dxdriver_capturename(const psy_AudioDriver*, intptr_t index);
static uintptr_t dxdriver_numcaptures(const psy_AudioDriver*);
static const char* dxdriver_playbackname(const psy_AudioDriver*, intptr_t index);
static uintptr_t dxdriver_numplaybacks(const psy_AudioDriver*);
static int dxdriver_add_capture_port(DXDriver* self, intptr_t idx);
static int dxdriver_remove_capture_port(DXDriver* self, intptr_t idx);
static bool dxdriver_start(DXDriver*);
static bool dxdriver_stop(DXDriver*);
static void dxdriver_deallocate(psy_AudioDriver*);
static const psy_AudioDriverInfo* dxdriver_info(psy_AudioDriver*);

static void dxdriver_preparewaveformat(WAVEFORMATEXTENSIBLE* wf, int channels, int sampleRate, int bits, int validBits);
static DWORD WINAPI dxdriver_notifythread(void* pDirectSound);
static DWORD WINAPI dxdriver_pollerthread(void* pDirectSound);
static void dxdriver_doblocksrecording(DXDriver*, PortCapt*);
static void dxdriver_doblocks(DXDriver*);
static void dxdriver_init_properties(psy_AudioDriver* self);
static BOOL isvistaorlater(void);
static BOOL dxdriver_wantsmoreblocks(DXDriver*);
static void dxdriver_refresh_available_ports(DXDriver*);
static void dxdriver_clearplayenums(DXDriver*);
static void dxdriver_clearcapenums(DXDriver*);
static void dxdriver_clearcapports(DXDriver*);
static bool dxdriver_createcaptureport(DXDriver*, PortCapt* port);
static void dxdriver_read_buffers(DXDriver* self, intptr_t index,
	float** pleft, float** pright, uintptr_t numsamples);
static uintptr_t dxdriver_playposinsamples(const psy_AudioDriver*);
static void dxdriver_reposition_mark(DXDriver*, int* low, int pos);
static void output_error(DXDriver*, const char* msg);

static psy_AudioDriver* dxdriver_base(DXDriver* self)
{
	return &self->driver;
}

static psy_AudioDriverVTable vtable;
static bool vtable_initialized = FALSE;

static void vtable_init(void)
{
	if (!vtable_initialized) {
		vtable.open = dxdriver_open;
		vtable.opened = dxdriver_opened;
		vtable.deallocate = dxdriver_deallocate;
		vtable.close = dxdriver_close;
		vtable.dispose = dxdriver_dispose;
		vtable.refresh_ports = dxdriver_refresh_ports;
		vtable.configure = dxdriver_configure;
		vtable.configuration = dxdriver_configuration;
		vtable.samplerate = dxdriver_samplerate;
		vtable.addcapture =
			(psy_audiodriver_fp_addcapture)
			dxdriver_add_capture_port;
		vtable.removecapture =
			(psy_audiodriver_fp_removecapture)
			dxdriver_remove_capture_port;
		vtable.readbuffers =
			(psy_audiodriver_fp_readbuffers)
			dxdriver_read_buffers;
		vtable.capturename =
			(psy_audiodriver_fp_capturename)
			dxdriver_capturename;
		vtable.numcaptures =
			(psy_audiodriver_fp_numcaptures)
			dxdriver_numcaptures;
		vtable.playbackname =
			(psy_audiodriver_fp_playbackname)
			dxdriver_playbackname;
		vtable.numplaybacks =
			(psy_audiodriver_fp_numplaybacks)
			dxdriver_numplaybacks;
		vtable.playposinsamples = dxdriver_playposinsamples;
		vtable.info =
			(psy_audiodriver_fp_info)
			dxdriver_info;
		vtable_initialized = TRUE;
	}
}

void output_error(DXDriver* self, const char* msg)
{
	assert(self);

	if (self->driver.logger && msg) {
		psy_logger_error(self->driver.logger, msg);
	}
}

EXPORT psy_AudioDriverInfo const * __cdecl GetPsycleDriverInfo(void)
{
	static psy_AudioDriverInfo info;

	info.guid = PSY_AUDIODRIVER_DIRECTX_GUID;
	info.Flags = 0;
	info.Name = "DirectSound";
	info.ShortName = "DXSound";
	info.Version = 0;
	return &info;
}

EXPORT psy_AudioDriver* __cdecl driver_create(void)
{
	DXDriver* directx;
	
	directx = (DXDriver*)malloc(sizeof(DXDriver));
	if (directx != NULL) {		
		dxdriver_init(&directx->driver);
		return &directx->driver;
	}
	return NULL;
}

void dxdriver_deallocate(psy_AudioDriver* driver)
{
	dxdriver_dispose(driver);
	free(driver);
}

int dxdriver_init(psy_AudioDriver* driver)
{
	DXDriver* self = (DXDriver*) driver;	

	memset(self, 0, sizeof(DXDriver));
	vtable_init();
	self->driver.vtable = &vtable;	
	psy_audiodriversettings_init(&self->settings);
	self->device_guid_ = DSDEVID_DefaultPlayback;
	psy_audiodriversettings_setblockcount(&self->settings, 4);
	psy_audiodriversettings_setblockframes(&self->settings, 768);
	self->_initialized = FALSE;
	self->_running = FALSE;	
	self->_playing = FALSE;
	self->_threadRun = FALSE;
	self->_pDs = NULL;
	self->_pBuffer = NULL;
	self->driver.callback = NULL;
	self->driver.callbackcontext = NULL;
	self->driver.handle = NULL;
	self->m_hWnd = 0;	
	self->_dither = 0;
	self->_playEnums = 0;
	self->_capEnums = 0;
	driver->logger = NULL;
	psy_table_init_keysize(&self->_capPorts, 4);
#ifdef PSYCLE_USE_SSE
	psy_dsp_sse2_init(&dsp);
#else
	psy_dsp_noopt_init(&dsp);
#endif
	SetupAVRT();	
	dxdriver_refresh_available_ports(self);
	dxdriver_init_properties(&self->driver);
	self->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	return 0;
}

int dxdriver_dispose(psy_AudioDriver* driver)
{
	DXDriver* self = (DXDriver*) driver;

	psy_property_deallocate(self->configuration);
	self->configuration = NULL;
	CloseHandle(self->hEvent);
	dxdriver_clearplayenums(self);
	dxdriver_clearcapenums(self);
	psy_table_dispose(&self->_capPorts);
	CloseAVRT();
	return 0;
}

void dxdriver_refresh_available_ports(DXDriver* self)
{
	assert(self);

	dxdriver_clearplayenums(self);
	dxdriver_clearcapenums(self);
	DirectSoundEnumerate(DSEnumCallback, &self->_playEnums);
	DirectSoundCaptureEnumerate(DSCaptureEnumCallback, &self->_capEnums);
}

BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription,
	LPCSTR lpcstrModule, LPVOID lpContext)
{	
	PortEnums* port;
	
	port = portenums_alloc();
	if (port) {
		psy_List** ports;
		char* utf8str;

		ports = (psy_List**)lpContext;
		utf8str = psy_dos_to_utf8(lpcstrDescription, NULL);
		portenums_init_all(port,
			(lpGuid == NULL)
			? (GUID*)(&DSDEVID_DefaultPlayback)
			: lpGuid,
			utf8str);
		free(utf8str);
		utf8str = NULL;
		psy_list_append(ports, port);
	}
	return TRUE;
}

BOOL CALLBACK DSCaptureEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription,
	LPCSTR lpcstrModule, LPVOID lpContext)
{	
	PortEnums* port;

	port = portenums_alloc();
	if (port) {
		psy_List** ports;
		char* utf8str;				

		ports = (psy_List**)lpContext;
		utf8str = psy_dos_to_utf8(lpcstrDescription, NULL);
		portenums_init_all(port,
			(lpGuid == NULL)
			? (GUID*)(&DSDEVID_DefaultCapture)
			: lpGuid,
			utf8str);
		free(utf8str);
		utf8str = NULL;
		psy_list_append(ports, port);
	}
	return TRUE;	
}

void dxdriver_clearplayenums(DXDriver* self)
{
	psy_List* p;

	for (p = self->_playEnums; p != NULL; p = p->next) {
		PortEnums* port = (PortEnums*) p->entry;
		portenums_dispose(port);
		free(port);
	}
	psy_list_free(self->_playEnums);
	self->_playEnums = 0;	
}

void dxdriver_clearcapenums(DXDriver* self)
{	
	psy_List* p;

	for (p = self->_capEnums; p != NULL; p = p->next) {
		PortEnums* port = (PortEnums*)p->entry;
		portenums_dispose(port);
		free(port);
	}
	psy_list_free(self->_capEnums);
	self->_capEnums = 0;
}

static void dxdriver_init_properties(psy_AudioDriver* driver)
{	
	DXDriver* self = (DXDriver*)driver;
	char key[256];	
	psy_Property* devices;
	psy_Property* indevices;
	psy_List* p;
	int i;		

	psy_snprintf(key, 256, "directx-guid-%d", PSY_AUDIODRIVER_DIRECTX_GUID);
	self->configuration = psy_property_preventtranslate(psy_property_set_text(
		psy_property_allocinit_key(key), "DirectSound"));
	psy_property_hide(
		psy_property_append_int(self->configuration,
			"guid", PSY_AUDIODRIVER_DIRECTX_GUID, 0, 0));
	psy_property_set_text(
		psy_property_setreadonly(
			psy_property_append_str(self->configuration, "name", "DirectSound"),
				TRUE),
			"Name");
	psy_property_set_text(
		psy_property_setreadonly(
		psy_property_append_str(self->configuration, "vendor", "Psycledelics"),
			TRUE),
			"Vendor");
	psy_property_set_text(
		psy_property_setreadonly(
			psy_property_append_str(self->configuration, "version", "1.0"),
			TRUE),
			"Version");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "bitdepth",
			psy_audiodriversettings_bitdepth(&self->settings), 0, 32),
		"Bitdepth");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "samplerate",
			(intptr_t)psy_audiodriversettings_samplespersec(&self->settings),
				0, 0),
		"Samplerate");
	psy_property_set_text(
		psy_property_append_bool(self->configuration, "dither", TRUE),
		"Dither");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "numbuf",
			psy_audiodriversettings_blockcount(&self->settings), 1, 8),
		"Buffer Number");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "numsamples",
			psy_audiodriversettings_blockframes(&self->settings),
				64, 8193),
		"Buffer Samples");
	devices = psy_property_set_text(
		psy_property_append_choice(self->configuration, "device", 0),
		"Output Device");
	psy_property_set_hint(devices, PSY_PROPERTY_HINT_COMBO);
	for (p = self->_playEnums, i = 0; p != NULL; psy_list_next(&p), ++i) {
		PortEnums* port = (PortEnums*)psy_list_entry(p);
		psy_property_append_int(devices, port->portname, i, 0, 0);
	}	
	indevices = psy_property_set_text(
		psy_property_append_choice(self->configuration, "indevice", 0),
		"Standard Input Device (Select different in Recorder)");
	psy_property_set_hint(indevices, PSY_PROPERTY_HINT_COMBO);
	for (p = self->_capEnums, i = 0; p != NULL; psy_list_next(&p), ++i) {
		PortEnums* port = (PortEnums*)psy_list_entry(p);
		psy_property_append_int(indevices, port->portname, i, 0, 0);
	}
}

void dxdriver_configure(psy_AudioDriver* driver, const psy_Property* cfg)
{
	DXDriver* self;
	psy_Property* property;

	assert(driver);

	self = (DXDriver*)driver;
	if (self->configuration && cfg) {
		psy_property_sync(self->configuration, cfg);
	}
	psy_audiodriversettings_setvalidbitdepth(&self->settings,
		psy_property_at_int(self->configuration, "bitdepth",
		psy_audiodriversettings_validbitdepth(&self->settings)));
	psy_audiodriversettings_setsamplespersec(&self->settings,
		(double)psy_property_at_int(self->configuration,
			"samplerate",
		(intptr_t)psy_audiodriversettings_samplespersec(&self->settings)));
	psy_audiodriversettings_setblockcount(&self->settings,
		psy_property_at_int(self->configuration, "numbuf",
			psy_audiodriversettings_blockcount(&self->settings)));	
	psy_audiodriversettings_setblockframes(&self->settings,
		psy_property_at_int(self->configuration, "numsamples",
			psy_audiodriversettings_blockframes(&self->settings)));
	if (psy_property_at_bool(self->configuration, "dither", TRUE)) {
		psy_audiodriversettings_enable_dither(&self->settings);
	} else {
		psy_audiodriversettings_disable_dither(&self->settings);
	}
	property = psy_property_at(self->configuration, "device",
		PSY_PROPERTY_TYPE_CHOICE);
	if (property) {
		intptr_t playenumindex;
		psy_List* portnode;
		
		portnode = NULL;
		playenumindex = psy_property_item_int(property);
		if (playenumindex >= 0) {
			portnode = psy_list_at(self->_playEnums, playenumindex);
			if (portnode) {
				PortEnums* port;

				port = (PortEnums*)psy_list_entry(portnode);
				if (port) {
					self->device_guid_ = (*port->guid);
				}
			}
		}
		if (!portnode) {
			self->device_guid_ = DSDEVID_DefaultPlayback;
		}
	}
}

double dxdriver_samplerate(psy_AudioDriver* self)
{
	return psy_audiodriversettings_samplespersec(&((DXDriver*)self)->settings);
}

int dxdriver_open(psy_AudioDriver* driver)
{		
	return dxdriver_start((DXDriver*)driver);
}

bool dxdriver_opened(const psy_AudioDriver* driver)
{
	DXDriver* self;

	self = (DXDriver*)driver;
	return (self->_running != FALSE);
}

bool dxdriver_start(DXDriver* self)
{	
	DSBUFFERDESC desc;
	WAVEFORMATPCMEX format;
	/* WAVEFORMATEX format; */
	LPDIRECTSOUNDBUFFER pBufferGen;
	HRESULT hr;
	DWORD dwThreadId;
	psy_TableIterator port_cap_it;

	/* CSingleLock lock(&_lock, TRUE); */
	if (self->_running) {
		return TRUE;
	}
	self->m_hWnd = self->driver.handle;
	if (self->m_hWnd == NULL) {
		self->m_hWnd = GetDesktopWindow();
	}
	if (self->driver.callback == NULL) {
		return FALSE;
	}
	if (hr = FAILED(DirectSoundCreate8(&self->device_guid_, &self->_pDs, NULL)))
	{
		output_error(self, "Failed to create DirectSound object");		
		return FALSE;
	}		
	if (FAILED(IDirectSound_SetCooperativeLevel(self->_pDs, self->m_hWnd, DSSCL_PRIORITY))) {
			output_error(self, "Failed to set DirectSound cooperative level");
			IDirectSound_Release(self->_pDs);
			self->_pDs = NULL;
			return FALSE;
	}
	self->_dsBufferSize = (DWORD)psy_audiodriversettings_totalbufferbytes(&self->settings);
	dxdriver_preparewaveformat(&format,
		(int)psy_audiodriversettings_numchannels(&self->settings),
		(int)psy_audiodriversettings_samplespersec(&self->settings),
		(int)psy_audiodriversettings_bitdepth(&self->settings),
		(int)psy_audiodriversettings_validbitdepth(&self->settings));
	ZeroMemory(&desc, sizeof(DSBUFFERDESC));
	desc.dwSize = sizeof(DSBUFFERDESC);
	desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = (DWORD)self->_dsBufferSize;
	desc.dwReserved = 0;
	desc.lpwfxFormat = (LPWAVEFORMATEX)&format;
	desc.guid3DAlgorithm = GUID_NULL;	

	if (FAILED(IDirectSound_CreateSoundBuffer(self->_pDs, &desc, &pBufferGen, NULL)))
	{
		output_error(self, "Failed to create DirectSound psy_audio_Buffer(s)");
		IDirectSound_Release(self->_pDs);
		self->_pDs = NULL;
		return FALSE;
	}		
	hr = IDirectSound_QueryInterface(pBufferGen, &IID_IDirectSoundBuffer8, (LPVOID*)&self->_pBuffer);
	IDirectSound_Release(pBufferGen);
	if (FAILED(hr))
	{
		output_error(self, "Failed to obtain version 8 interface for Buffer");
		self->_pBuffer = 0;
		IDirectSound_Release(self->_pDs);
		self->_pDs = 0;
		return FALSE;
	}

	for (port_cap_it = psy_table_begin(&self->_capPorts);
		!psy_tableiterator_equal(&port_cap_it, psy_table_end());
		psy_tableiterator_inc(&port_cap_it)) {
		PortCapt* port_capt;

		port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
		dxdriver_createcaptureport(self, port_capt);
	}
	ResetEvent(self->hEvent);
	self->_threadRun = TRUE;
	self->_playing = FALSE;	
	if (self->driver.logger) {
		psy_logger_output(self->driver.logger, "DirectX Audiodriver started\n");
	}
#define DIRECTSOUND_POLLING P
#ifdef DIRECTSOUND_POLLING
	CreateThread(NULL, 0, dxdriver_pollerthread, self, 0, &dwThreadId);
#else
	CreateThread(NULL, 0, dxdriver_notifythread, self, 0, &dwThreadId);
#endif
	self->_running = TRUE;
	return TRUE;	
}


void dxdriver_preparewaveformat(WAVEFORMATEXTENSIBLE* wf, int channels, int sampleRate, int bits, int validBits)
{
	assert(wf);

	/* Set up wave format structure. */
	ZeroMemory(wf, sizeof(WAVEFORMATEX));	
	wf->Format.nChannels = channels;
	wf->Format.wBitsPerSample = bits;
	wf->Format.nSamplesPerSec = sampleRate;
	wf->Format.nBlockAlign = wf->Format.nChannels * wf->Format.wBitsPerSample / 8;
	wf->Format.nAvgBytesPerSec = wf->Format.nSamplesPerSec * wf->Format.nBlockAlign;					
	if(bits <= 16) {
		wf->Format.wFormatTag = WAVE_FORMAT_PCM;
		wf->Format.cbSize = 0;
	} else {
		wf->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wf->Format.cbSize = 0x16;
		wf->Samples.wValidBitsPerSample = validBits;
		if (channels == 2) {
			wf->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		}
		if (validBits == 32) {
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
		} else {
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		}
	}
}

DWORD WINAPI dxdriver_pollerthread(void* self)
{	
	DXDriver* pThis = (DXDriver*)self;
	HANDLE hTask = NULL;
	unsigned int i;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	/*
	** Ask MMCSS to temporarily boost the thread priority
	** to reduce glitches while the low-latency stream plays.
	*/
	if (isvistaorlater())
	{		
		DWORD taskIndex = 0;		
		hTask = pAvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskIndex);
	}
	/* Prefill buffer : */
	pThis->_lowMark = 0;
	pThis->m_readPosWraps = 0;
	pThis->_highMark = (DWORD)psy_audiodriversettings_blockbytes(&pThis->settings);	
	IDirectSoundBuffer8_SetCurrentPosition(pThis->_pBuffer, 0);
	for (i = 0; i < psy_audiodriversettings_blockcount(&pThis->settings); ++i) {
		/* Directsound playback buffer is started here. */
		dxdriver_doblocks(pThis);
	}
	{
		psy_TableIterator port_cap_it;

		for (port_cap_it = psy_table_begin(&pThis->_capPorts);
			!psy_tableiterator_equal(&port_cap_it, psy_table_end());
			psy_tableiterator_inc(&port_cap_it)) {
			PortCapt* port_capt;

			port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
			if (port_capt->_pDs == NULL)
				continue;
			dxdriver_reposition_mark(self, &port_capt->_lowMark, 0);
			IDirectSoundCaptureBuffer_Start(port_capt->_pBuffer, DSCBSTART_LOOPING);
		}
	}
	while (pThis->_threadRun)
	{
		unsigned int runs = 0;

		while (dxdriver_wantsmoreblocks(pThis))
		{
			/* First, run the capture buffers so that audio is available to wavein machines. */
			psy_TableIterator port_cap_it;

			for (port_cap_it = psy_table_begin(&pThis->_capPorts);
					!psy_tableiterator_equal(&port_cap_it, psy_table_end());
					psy_tableiterator_inc(&port_cap_it)) {
				PortCapt* port_capt;

				port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
				if (port_capt->_pDs == NULL) {
					continue;
				}
				dxdriver_doblocksrecording(self, port_capt);
			}
			/* Next, proceeed with the generation of audio */
			dxdriver_doblocks(pThis);
			if (++runs > psy_audiodriversettings_blockcount(&pThis->settings)) {
				break;
			}
		}
		Sleep(1);
	}	
	SetEvent(pThis->hEvent);
	if (hTask != NULL) {
		pAvRevertMmThreadCharacteristics(hTask);
	}
	return 0;
}

DWORD WINAPI notifythread(void* self)
{
	return 0;
}

BOOL dxdriver_wantsmoreblocks(DXDriver* self)
{
	/*
	** [_lowMark, _highMark) is the next buffer to be filled.
	** if (play) pos is still inside, we have to wait.
	*/
	uint32_t pos = 0;
	HRESULT hr;
	
	hr = IDirectSoundBuffer8_GetCurrentPosition(self->_pBuffer, &pos, 0);
	if (FAILED(hr)) return FALSE;
	if (self->_lowMark <= pos && pos < self->_highMark)	return FALSE;
	return TRUE;
}

void dxdriver_doblocksrecording(DXDriver* self, PortCapt* port)
{
	int* pBlock1, * pBlock2;
	unsigned long blockSize1, blockSize2;
	HRESULT hr;

	/* If directsound capture fails */
	if (port->_pBuffer == NULL) {
		return;
	}		
	hr = IDirectSoundBuffer_Lock(
		port->_pBuffer,
		(DWORD)port->_lowMark,
		(DWORD)psy_audiodriversettings_blockbytes(&self->settings),
		(void**)&pBlock1, (DWORD*)&blockSize1,
		(void**)&pBlock2, (DWORD*)&blockSize2,
		0);
	if (SUCCEEDED(hr)) {
		/* Put the audio in our float buffers. */
		unsigned int _sampleValidBits = (DWORD)psy_audiodriversettings_validbitdepth(&self->settings);
		DWORD numSamples = blockSize1 / (DWORD)psy_audiodriversettings_framebytes(&self->settings);				
		if (numSamples > 0) {
			if (_sampleValidBits == 32) {
				psy_dsp_deinterlacefloat((float*)(pBlock1), port->pleft, port->pright, numSamples);
			} else if (_sampleValidBits == 24) {
				psy_dsp_dequantize32anddeinterlace(pBlock1, port->pleft, port->pright, numSamples);
			} else {
				psy_dsp_dequantize16anddeinterlace((short int*)(pBlock1), port->pleft, port->pright, numSamples);
			}
		}
		port->_lowMark += blockSize1;
		if (blockSize2 > 0)
		{
			numSamples = blockSize2 / (DWORD)psy_audiodriversettings_framebytes(&self->settings);
			if (_sampleValidBits == 32) {
				psy_dsp_deinterlacefloat((float*)(pBlock2), port->pleft + numSamples, port->pright + numSamples, numSamples);				
			}
			else if (_sampleValidBits == 24) {				
				psy_dsp_dequantize32anddeinterlace(pBlock2, port->pleft + numSamples, port->pright + numSamples, numSamples);
			}
			else {
				psy_dsp_dequantize16anddeinterlace((short int*)(pBlock2), port->pleft + numSamples, port->pright + numSamples, numSamples);
			}
			port->_lowMark += blockSize2;
		}
		/* Release the data back to DirectSound. */			
		hr = IDirectSoundBuffer_Unlock(port->_pBuffer, pBlock1, blockSize1, pBlock2, blockSize2);
		if (port->_lowMark >= (int)self->_dsBufferSize) {
			port->_lowMark = 0;
		}
	}
	port->_machinepos = 0;
}

void dxdriver_doblocks(DXDriver* self)
{
	int* pBlock1, * pBlock2;
	unsigned long blockSize1, blockSize2;
	HRESULT hr;
	
	hr = IDirectSoundBuffer_Lock(
		self->_pBuffer,
		(DWORD) self->_lowMark,
		(DWORD) psy_audiodriversettings_blockbytes(&self->settings),
		(void**) &pBlock1, (DWORD*)&blockSize1,
		(void**) &pBlock2, (DWORD*)&blockSize2,
		0);	
	if (hr == DSERR_BUFFERLOST)
	{
		/* If DSERR_BUFFERLOST is returned, restore and retry lock. */
		self->_playing = FALSE;
		IDirectSoundBuffer_Restore(self->_pBuffer);
		if (self->_highMark < self->_dsBufferSize) {
			IDirectSoundBuffer8_SetCurrentPosition(self->_pBuffer, self->_highMark);
		} else {
			IDirectSoundBuffer8_SetCurrentPosition(self->_pBuffer, 0);			
		}
		hr = IDirectSoundBuffer_Lock(
			self->_pBuffer,
			(DWORD)self->_lowMark,
			(DWORD)psy_audiodriversettings_blockbytes(&self->settings),
			(void**)&pBlock1, (DWORD*)&blockSize1,
			(void**)&pBlock2, (DWORD*)&blockSize2,
			0);
	}
	if (SUCCEEDED(hr))
	{
		/* Generate audio and put it into the buffer */
		unsigned int _sampleValidBits = (DWORD)psy_audiodriversettings_validbitdepth(&self->settings);
		DWORD numSamples = blockSize1 / (DWORD)psy_audiodriversettings_framebytes(&self->settings);
		int hostisplaying;
		float* pFloatBlock;
		
		pFloatBlock = psy_audiodriver_work(dxdriver_base(self), &numSamples, &hostisplaying);
		if (pFloatBlock && numSamples > 0) {
			if (_sampleValidBits == 32) {
				dsp.movmul(pFloatBlock, (float*)pBlock1, numSamples * 2, 1.f / 32768.f);
			}
			else if (_sampleValidBits == 24) {
				psy_dsp_quantize24in32bit(pFloatBlock, pBlock1, numSamples);
			}
			else if (_sampleValidBits == 16) {
				if (psy_audiodriversettings_dither(&self->settings)) {
					psy_dsp_quantize16withdither(pFloatBlock, pBlock1, numSamples);
				}
				else {
					psy_dsp_quantize16(pFloatBlock, pBlock1, numSamples);
				}
			}
		}
		self->_lowMark += blockSize1;		
		if (blockSize2 > 0) {
			float* pFloatBlock;

			numSamples = blockSize2 / (DWORD)psy_audiodriversettings_framebytes(&self->settings);
			pFloatBlock = psy_audiodriver_work(dxdriver_base(self), &numSamples, &hostisplaying);
			if (pFloatBlock && numSamples > 0) {
				if (_sampleValidBits == 32) {
					dsp.movmul(pFloatBlock, (float*)pBlock2, numSamples * 2, 1.f / 32768.f);
				} else if (_sampleValidBits == 24) {
					psy_dsp_quantize24in32bit(pFloatBlock, pBlock2, numSamples);
				} else if (_sampleValidBits == 16) {
					if (psy_audiodriversettings_dither(&self->settings)) {
						psy_dsp_quantize16withdither(pFloatBlock, pBlock2, numSamples);
					}
					else {
						psy_dsp_quantize16(pFloatBlock, pBlock2, numSamples);
					}
				}
			}
			self->_lowMark += blockSize2;
		}
		/* Release the data back to DirectSound. */
		hr = IDirectSoundBuffer_Unlock(self->_pBuffer, pBlock1, blockSize1, pBlock2, blockSize2);
		if (self->_lowMark >= self->_dsBufferSize) {
			self->_lowMark -= self->_dsBufferSize;
			self->m_readPosWraps++;
#if _MSC_VER > 1200
			if ((uint64_t)self->m_readPosWraps * 
				(uint64_t)self->_dsBufferSize >= 0x100000000LL)
			{
				self->m_readPosWraps = 0;
				/* PsycleGlobal::midi().ReSync();	// MIDI IMPLEMENTATION */
			}
#else
			if ((uint64_t)self->m_readPosWraps * 
				(uint64_t)self->_dsBufferSize >= 0x100000000L)
			{
				self->m_readPosWraps = 0;
				/* PsycleGlobal::midi().ReSync();	// MIDI IMPLEMENTATION */
			}
#endif
		}
		self->_highMark = self->_lowMark + 
			(DWORD)psy_audiodriversettings_blockbytes(&self->settings);
		if (SUCCEEDED(hr) && !self->_playing)
		{
			IDirectSoundBuffer8_SetCurrentPosition(self->_pBuffer, self->_highMark);						
			IDirectSoundBuffer_Play(self->_pBuffer, 0, 0, DSBPLAY_LOOPING);

			if (SUCCEEDED(hr)) {
				self->_playing = TRUE;
				/* PsycleGlobal::midi().ReSync(); // MIDI IMPLEMENTATION */
			}
		}
	}
}

int dxdriver_close(psy_AudioDriver* driver)
{
	int status;

	assert(driver);
	
	status = dxdriver_stop((DXDriver*)driver);
	return status;
}

bool dxdriver_stop(DXDriver* self)
{
	psy_TableIterator port_cap_it;

	assert(self);

	if (!self->_running) {
		return TRUE;
	}
	self->_threadRun = FALSE;
	WaitForSingleObject(self->hEvent, INFINITE);
	/* Once we get here, the PollerThread should have stopped */
	if (self->_playing) {
		IDirectSoundBuffer_Stop(self->_pBuffer);		
		self->_playing = FALSE;
	}
	IDirectSoundBuffer_Release(self->_pBuffer);	
	self->_pBuffer = NULL;
	IDirectSound_Release(self->_pDs);
	self->_pDs = NULL;
	for (port_cap_it = psy_table_begin(&self->_capPorts);
		!psy_tableiterator_equal(&port_cap_it, psy_table_end());
		psy_tableiterator_inc(&port_cap_it)) {
		PortCapt* port_capt;

		port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
		if (port_capt) {
			portcap_dispose(port_capt);			
		}
	}
	self->_running = FALSE;
	return TRUE;
}

BOOL isvistaorlater(void)
{
#if _MSC_VER > 1200
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;
	int op = VER_GREATER_EQUAL;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 0;
	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, op);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, op);
	/* Perform the test. */
	return VerifyVersionInfo(
		&osvi, VER_MAJORVERSION | VER_MINORVERSION,
		dwlConditionMask);
#else
	return 0;
#endif
}
bool dxdriver_createcaptureport(DXDriver* self, PortCapt* port)
{
	HRESULT hr;
	/* WAVEFORMATPCMEX wf; */
	WAVEFORMATEX wf;
	DSCBUFFERDESC dscbd;
	LPDIRECTSOUNDCAPTUREBUFFER cb;

	/* avoid opening a port twice */
	if (port->_pDs) {
		return TRUE;
	}
	port->_machinepos = 0;
	/* Create IDirectSoundCapture using the selected capture device */
	if (FAILED(hr = DirectSoundCaptureCreate8(
			port->_pGuid, &port->_pDs, NULL))) {
		output_error(self, "Failed to create Capture DirectSound Device");
		return FALSE;
	}
	/* Create the capture buffer */
	dxdriver_preparewaveformat((WAVEFORMATEXTENSIBLE*)&wf,
		(int)psy_audiodriversettings_numchannels(&self->settings),
		(int)psy_audiodriversettings_samplespersec(&self->settings),
		(int)psy_audiodriversettings_bitdepth(&self->settings),
		(int)psy_audiodriversettings_validbitdepth(&self->settings));
	ZeroMemory(&dscbd, sizeof(DSCBUFFERDESC));
	dscbd.dwSize = sizeof(DSCBUFFERDESC);
	dscbd.dwBufferBytes = self->_dsBufferSize;
	dscbd.lpwfxFormat = (LPWAVEFORMATEX)(&wf);	
	if (FAILED(hr = IDirectSoundCapture_CreateCaptureBuffer(port->_pDs, &dscbd,
			&cb, NULL))) {
		output_error(self, "Failed to create Capture DirectSound Buffer");
		if (port->_pDs) {
			IDirectSoundCapture_Release(port->_pDs);
			port->_pDs = 0;
		}
		return FALSE;
	}
	if (FAILED(hr = IDirectSoundCapture_QueryInterface(cb,
			&IID_IDirectSoundCaptureBuffer8, (void**)&port->_pBuffer))) {
		output_error(self, "Failed to create Interface for Capture DirectSound Buffer(s)");
		if (cb) {
			IDirectSoundCaptureBuffer_Release(cb);
			cb = 0;
		}
		if (port->_pDs) {
			IDirectSoundCapture_Release(port->_pDs);
			port->_pDs = 0;
		}
		return FALSE;
	}
	/* 2 * is a safety measure(Haven't been able to dig out why it crashes if it is exactly the size) */	
	port->pleft = (float*)dsp.memory_alloc(2 * psy_audiodriversettings_blockbytes(&self->settings) * 16, 1);
	port->pright = (float*)dsp.memory_alloc(2 * psy_audiodriversettings_blockbytes(&self->settings) * 16, 1);
	dsp.clear(port->pleft, psy_audiodriversettings_blockbytes(&self->settings));
	dsp.clear(port->pright, psy_audiodriversettings_blockbytes(&self->settings));	
	return TRUE;
}

int dxdriver_add_capture_port(DXDriver* self, intptr_t idx)
{
	PortCapt* port;
	bool isplaying = self->_running;

	assert(self);

	if (idx >= (int)psy_list_size(self->_capEnums)) return FALSE;
	if (psy_table_exists(&self->_capPorts, idx)) {
		return TRUE;
	}
	if (isplaying) dxdriver_stop(self);

	port = malloc(sizeof(PortCapt));
	if (port) {
		PortEnums* port_enums;


		port_enums = (PortEnums*)psy_list_entry_at(self->_capEnums, idx);
		portcapt_init(port);
		port->_pGuid = port_enums->guid;
		psy_table_insert(&self->_capPorts, idx, port);
		if (isplaying) {
			return dxdriver_start(self);
		}
	}
	else {
		return FALSE;
	}
	return TRUE;
}

int dxdriver_remove_capture_port(DXDriver* self, intptr_t idx)
{
	bool isplaying = self->_running;
	PortCapt* port_capt;

	assert(self);

	port_capt = (PortCapt*)psy_table_at(&self->_capPorts, idx);
	if (!port_capt) {
		return FALSE;
	}
	if (isplaying) {
		dxdriver_stop(self);
	}
	free(port_capt);
	port_capt = NULL;
	psy_table_remove(&self->_capPorts, idx);
	if (isplaying) {
		dxdriver_start(self);
	}
	return TRUE;
}

void dxdriver_read_buffers(DXDriver* self, intptr_t idx, float** pleft, float** pright,
	uintptr_t numsamples)
{
	PortCapt* port_capt;
	int mpos;
	
	port_capt = (PortCapt*)psy_table_at(&self->_capPorts, idx);	
	if (!self->_running || !port_capt || !port_capt->_pDs) {
		*pleft = 0;
		*pright = 0;
		return;
	}
	mpos = port_capt->_machinepos;	
	*pleft = port_capt->pleft + mpos;
	*pright = port_capt->pright + mpos;
	port_capt->_machinepos += (int)numsamples;
}

const char* dxdriver_capturename(const psy_AudioDriver* driver, intptr_t index)
{
	DXDriver* self = (DXDriver*)driver;
	psy_List* pPort;

	if (self->_capEnums) {
		pPort = psy_list_at(self->_capEnums, index);
		if (pPort) {
			return ((PortEnums*)(pPort->entry))->portname;
		}
	}
	return "";
}

uintptr_t dxdriver_numcaptures(const psy_AudioDriver* driver)
{
	DXDriver* self = (DXDriver*)driver;

	return psy_list_size(self->_capEnums);
}

const char* dxdriver_playbackname(const psy_AudioDriver* driver, intptr_t index)
{
	const DXDriver* self = (const DXDriver*)driver;
	const psy_List* pPort;

	if (self->_playEnums) {
		pPort = psy_list_at_const(self->_playEnums, index);
		if (pPort) {
			return ((const PortEnums*)(pPort->entry))->portname;
		}
	}
	return "";
}

uintptr_t dxdriver_numplaybacks(const psy_AudioDriver* driver)
{
	DXDriver* self = (DXDriver*)driver;

	return psy_list_size(self->_playEnums);
}

const psy_AudioDriverInfo* dxdriver_info(psy_AudioDriver* self)
{
	return GetPsycleDriverInfo();
}

const psy_Property* dxdriver_configuration(const psy_AudioDriver* driver)
{
	DXDriver* self = (DXDriver*)driver;

	return self->configuration;
}

uintptr_t dxdriver_playposinsamples(const psy_AudioDriver* driver)
{
	const DXDriver* self = (const DXDriver*)driver;
	HRESULT hr;
	DWORD playPos, writePos;

	/*
	** http://msdn.microsoft.com/en-us/library/ee418744%28v=VS.85%29.aspx
	** When a buffer is created, the play cursor is set to 0. As the buffer is played,
	** the cursor moves and always points to the next byte of data to be played.
	** When the buffer is stopped, the cursor remains where it is.
	*/
	if (!self->_threadRun) {
		return 0;
	}	
	hr = IDirectSoundBuffer8_GetCurrentPosition(self->_pBuffer, &playPos, &writePos);
	if (FAILED(hr))
	{
		output_error(self, "DirectSoundBuffer::GetCurrentPosition failed");
		return 0;
	}
	if (playPos < writePos && self->_lowMark > writePos) {
		return (playPos + self->m_readPosWraps * self->_dsBufferSize) /
			psy_audiodriversettings_framebytes(&self->settings);
	} else {
		return (playPos + (self->m_readPosWraps - 1) * self->_dsBufferSize) /
			psy_audiodriversettings_framebytes(&self->settings);
	}
}

/* Reposition the write block before the play cursor. */
void dxdriver_reposition_mark(DXDriver* self , int* low, int pos)
{
	uintptr_t _blockSizeBytes;

	_blockSizeBytes = psy_audiodriversettings_blockcount(&self->settings);
	if (pos < _blockSizeBytes) {
		*low = (int)self->_dsBufferSize - (int)_blockSizeBytes;
	}
	else {
		*low = (pos - (int)_blockSizeBytes) / (int)_blockSizeBytes * (int)_blockSizeBytes;
	}
}
