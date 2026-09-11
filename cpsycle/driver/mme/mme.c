/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"

#include "../audiodriver.h"
#include "../audiodriversettings.h"


#include <windows.h>
#include <mmsystem.h>
#include <ks.h>
#include <ksmedia.h>
#include <process.h>
#include <stdio.h>
#include <operations.h>
#include <quantize.h>
#include <hashtbl.h>
#include <logger.h>

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

#define MAX_WAVEOUT_BLOCKS 8

typedef struct {
	HANDLE Handle;
	unsigned char* pData;
	WAVEHDR* pHeader;
	HANDLE HeaderHandle;
	int Prepared;
} CBlock;

typedef struct {
	char* portname;
	int idx;
} PortEnums;

INLINE void portenums_init(PortEnums* self)
{
	self->idx = 0;
	self->portname = strdup("");
}

INLINE void portenums_init_all(PortEnums* self, int idx, const char* portname)
{
	self->idx = idx;
	self->portname = portname ? strdup(portname) : strdup("");
}

INLINE void portenums_dispose(PortEnums* self)
{
	self->idx = 0;
	free(self->portname);
}

INLINE bool portenums_isformatsupported(PortEnums* self, WAVEFORMATEXTENSIBLE* pwfx, bool isInput)
{
	if (isInput) {
		return MMSYSERR_NOERROR == waveInOpen(
			NULL,                   /* ptr can be NULL for query */
			self->idx,              /* the device identifier */
			(WAVEFORMATEX*) (pwfx), /* defines requested format */
			(DWORD_PTR) NULL,       /* no callback */
			(DWORD_PTR) NULL,       /* no instance data */
			WAVE_FORMAT_QUERY);     /* query only, do not open device */
	}
	else {
		return MMSYSERR_NOERROR == waveOutOpen(
			NULL,                   /* ptr can be NULL for query */
			self->idx,              /* the device identifier */
			(WAVEFORMATEX*) (pwfx), /* defines requested format */
			(DWORD_PTR) NULL,       /* no callback */
			(DWORD_PTR) NULL,       /* no instance data */
			WAVE_FORMAT_QUERY);     /* query only, do not open device */
	}
}

typedef struct {
	HWAVEIN _handle;
	int _idx;
	byte* pData;
	WAVEHDR* pHeader;
	HANDLE HeaderHandle;
	CBlock _blocks[MAX_WAVEOUT_BLOCKS];
	bool Prepared;
	float* pleft;
	float* pright;
	uintptr_t _machinepos;
} PortCapt;

INLINE portcapt_init(PortCapt* self)
{
	self->Prepared = FALSE;
	self->pleft = 0;
	self->pright = 0;
	self->_handle = 0;
	self->_idx = 0;
	self->_machinepos = 0;
}

typedef struct MmeDriver {
	psy_AudioDriver driver;
	psy_AudioDriverSettings settings;
	psy_Property* configuration;
	HWAVEOUT _handle;
	int _deviceId;
	int _dither;
	size_t _currentBlock;
	int _running;
	int _stopPolling;	
	unsigned int pollSleep_;
	uintptr_t _writePos;
	/* number of "wraparounds" to compensate the WaveOutGetPosition() call. */
	int m_readPosWraps;
	/* helper variable to detect the previous wraps. */
	int m_lastPlayPos;		
	CBlock _blocks[MAX_WAVEOUT_BLOCKS];
	HANDLE hEvent;
	psy_List* _playEnums;	/* PortEnums */
	psy_List* _capEnums;	/* PortEnums idx : mme input device id */
	psy_Table _capPorts;	/* mme input device id, PortCapt */
} MmeDriver;

static void driver_deallocate(psy_AudioDriver*);
static int driver_init(psy_AudioDriver*);
static int driver_open(psy_AudioDriver*);
static bool driver_opened(const psy_AudioDriver*);
static int driver_close(psy_AudioDriver*);
static int driver_dispose(psy_AudioDriver*);
static void driver_configure(psy_AudioDriver*, const psy_Property*);
static const psy_Property* driver_configuration(const psy_AudioDriver*);
static double samplerate(psy_AudioDriver*);
static const char* capturename(const psy_AudioDriver*, intptr_t index);
static uintptr_t numcaptures(const psy_AudioDriver*);
static const char* playbackname(const psy_AudioDriver*, intptr_t index);
static uintptr_t numplaybacks(const psy_AudioDriver*);
static bool start(MmeDriver*);
static bool stop(MmeDriver*);
static const psy_AudioDriverInfo* driver_info(psy_AudioDriver*);
static uintptr_t playposinsamples(const psy_AudioDriver*);

static void preparewaveformat(WAVEFORMATEXTENSIBLE* wf, int channels, int sampleRate, int bits, int validBits);
static void PollerThread(void *pWaveOut);
static void DoBlocksRecording(MmeDriver*, PortCapt*);
static void DoBlocks(MmeDriver* self);
static void init_properties(psy_AudioDriver* self);

static void output_error(MmeDriver* self, const char* msg)
{
	assert(self);

	if (self->driver.logger && msg) {
		psy_logger_error(self->driver.logger, msg);
	}
}

static void readbuffers(MmeDriver*, int idx, float** pleft, float** pright,
	uintptr_t numsamples);
static void refreshavailableports(MmeDriver*);
static void enumerateplaybackports(MmeDriver*);
static void enumerate_capture_ports(MmeDriver*);
static void clearplayenums(MmeDriver*);
static void clearcapenums(MmeDriver*);
static bool createcaptureport(MmeDriver*, PortCapt*);
static bool addcaptureport(MmeDriver*, int idx);
static bool removecaptureport(MmeDriver*, int idx);

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
		vtable.refresh_ports = (psy_audiodriver_fp_refresh_ports)refreshavailableports;
		vtable.configure = driver_configure;
		vtable.configuration = driver_configuration;
		vtable.samplerate = samplerate;
		vtable.addcapture = (psy_audiodriver_fp_addcapture)addcaptureport;
		vtable.removecapture = (psy_audiodriver_fp_removecapture)removecaptureport;
		vtable.readbuffers = (psy_audiodriver_fp_readbuffers)readbuffers;
		vtable.capturename = (psy_audiodriver_fp_capturename)capturename;
		vtable.numcaptures = (psy_audiodriver_fp_numcaptures)numcaptures;
		vtable.playbackname = (psy_audiodriver_fp_playbackname)playbackname;
		vtable.numplaybacks = (psy_audiodriver_fp_numplaybacks)numplaybacks;
		vtable.playposinsamples = playposinsamples;
		vtable.info = (psy_audiodriver_fp_info)driver_info;
		vtable_initialized = 1;
	}
}

EXPORT psy_AudioDriverInfo const * __cdecl GetPsycleDriverInfo(void)
{
	static psy_AudioDriverInfo info;

	info.guid = PSY_AUDIODRIVER_MME_GUID;
	info.Flags = 0;
	info.Name = "Windows MME AudioDriver";
	info.ShortName = "MME";
	info.Version = 0;
	return &info;
}

EXPORT psy_AudioDriver* __cdecl driver_create(void)
{
	MmeDriver* mme = (MmeDriver*) malloc(sizeof(MmeDriver));
	if (mme) {		
		driver_init(&mme->driver);
		return &mme->driver;
	} else {
		return 0;
	}
}

void driver_deallocate(psy_AudioDriver* driver)
{
	driver_dispose(driver);
	free(driver);
}

int driver_init(psy_AudioDriver* driver)
{
	MmeDriver* self = (MmeDriver*)driver;	

	memset(self, 0, sizeof(MmeDriver));
	vtable_init();
	self->driver.vtable = &vtable;
	self->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	
	self->_deviceId = 0;
	self->pollSleep_ = 20;
	self->_dither = 0;
	self->_running = FALSE;

#ifdef PSYCLE_USE_SSE
	psy_dsp_sse2_init(&dsp);
#else
	psy_dsp_noopt_init(&dsp);
#endif
	psy_audiodriversettings_init(&self->settings);
	psy_table_init(&self->_capPorts);
	refreshavailableports(self);
	init_properties(&self->driver);
	return 0;
}

int driver_dispose(psy_AudioDriver* driver)
{
	MmeDriver* self;
	
	self = (MmeDriver*)driver;
	psy_property_deallocate(self->configuration);
	self->configuration = NULL;
	CloseHandle(self->hEvent);
	clearplayenums(self);
	clearcapenums(self);
	psy_table_dispose(&self->_capPorts);
	return 0;
}

static void init_properties(psy_AudioDriver* driver)
{			
	MmeDriver* self;
	char key[256];
	psy_Property* devices;
	psy_Property* indevices;
	psy_List* p;
	int i;

	self = (MmeDriver*)driver;
	psy_snprintf(key, 256, "mme-guid-%d", PSY_AUDIODRIVER_MME_GUID);
	self->configuration = psy_property_preventtranslate(psy_property_set_text(
		psy_property_allocinit_key(key), "Windows Wave MME"));
	psy_property_hide(psy_property_append_int(self->configuration,
		"guid", PSY_AUDIODRIVER_MME_GUID, 0, 0));
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "name", "Windows Wave MME"),
		TRUE),
		"Driver");
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "vendor", "Psycledelics"),
		TRUE),
		"Vendor");
	psy_property_set_text(psy_property_setreadonly(
		psy_property_append_str(self->configuration, "version", "1.0"),
		TRUE),
		"Version");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "bitdepth",
			psy_audiodriversettings_bitdepth(&self->settings), 0, 32),
		"Bitrate");
	psy_property_set_text(
		psy_property_append_int(self->configuration, "samplerate",
			(intptr_t)psy_audiodriversettings_samplespersec(&self->settings),
				0, 0),
		"Samplerate"
		);
	psy_property_set_text(
		psy_property_append_int(self->configuration, "dither", 0, 0, 1),
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
	psy_property_append_int(devices, "WAVE_MAPPER", -1, 0, 0);
	for (p = self->_playEnums, i = 0; p != NULL; p = p->next, ++i) {
		PortEnums* port = (PortEnums*)p->entry;
		psy_property_append_int(devices, port->portname, i, 0, 0);
	}
	indevices = psy_property_set_text(
		psy_property_append_choice(self->configuration, "indevice", 0),
		"Standard Input Device (Select different in Recorder)");
	psy_property_set_hint(indevices, PSY_PROPERTY_HINT_COMBO);
	for (p = self->_capEnums, i = 0; p != NULL; p = p->next, ++i) {
		PortEnums* port = (PortEnums*)p->entry;
		psy_property_append_int(indevices, port->portname, i, 0, 0);
	}
}

void driver_configure(psy_AudioDriver* driver, const psy_Property* config)
{
	MmeDriver* self;
	psy_Property* property;

	self = (MmeDriver*)driver;
	if (self->configuration && config) {
		psy_property_sync(self->configuration, config);
	}
	property = psy_property_at(self->configuration, "device", PSY_PROPERTY_TYPE_NONE);
	if (property && property->item.typ == PSY_PROPERTY_TYPE_CHOICE) {
		psy_Property* device;

		device = psy_property_at_choice(property);
		if (device) {
			self->_deviceId =  (int)psy_property_item_int(device);
		}
	}	
	psy_audiodriversettings_setvalidbitdepth(&self->settings,
		psy_property_at_int(self->configuration, "bitdepth",
			psy_audiodriversettings_validbitdepth(&self->settings)));
	psy_audiodriversettings_setsamplespersec(&self->settings,
		(double)psy_property_at_int(self->configuration, "samplerate",
			(intptr_t)psy_audiodriversettings_samplespersec(&self->settings)));
	psy_audiodriversettings_setblockcount(&self->settings,
		psy_property_at_int(self->configuration, "numbuf",
			psy_audiodriversettings_blockcount(&self->settings)));
	psy_audiodriversettings_setblockframes(&self->settings,
		psy_property_at_int(self->configuration, "numsamples",
			psy_audiodriversettings_blockframes(&self->settings)));
}

double samplerate(psy_AudioDriver* self)
{
	return psy_audiodriversettings_samplespersec(&((MmeDriver*)self)->settings);
}

int driver_open(psy_AudioDriver* driver)
{
	return start((MmeDriver*)driver);
}

bool driver_opened(const psy_AudioDriver* driver)
{
	MmeDriver* self;

	self = (MmeDriver*)driver;
	return (self->_running != FALSE);
}

bool start(MmeDriver* self)
{
	WAVEFORMATEX format;
	CBlock* pBlock;
	SIZE_T _blockSizeBytes;
	uintptr_t _numBlocks;
	psy_TableIterator port_cap_it;

	/*	CSingleLock lock(&_lock, TRUE); */
	if (self->_running) return TRUE;
	if (self->driver.callback == NULL) {
		return FALSE;
	}
	preparewaveformat((WAVEFORMATEXTENSIBLE*)&format,
		(int)psy_audiodriversettings_numchannels(&self->settings),
		(int)psy_audiodriversettings_samplespersec(&self->settings),
		(int)psy_audiodriversettings_bitdepth(&self->settings),
		(int)psy_audiodriversettings_validbitdepth(&self->settings));
	if (waveOutOpen(&self->_handle, self->_deviceId, &format, 0UL, 0,
		CALLBACK_NULL) != MMSYSERR_NOERROR) {
		output_error(self, "waveOutOpen() failed");
		return FALSE;
	}
	self->_currentBlock = 0;
	self->_writePos = 0;
	self->m_readPosWraps = 0;
	self->m_lastPlayPos = 0;

	/* allocate blocks */
	_blockSizeBytes = psy_audiodriversettings_blockbytes(&self->settings);
	_numBlocks = psy_audiodriversettings_blockcount(&self->settings);
	for (pBlock = self->_blocks; pBlock < self->_blocks + _numBlocks;
		++pBlock) {
		pBlock->Handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, _blockSizeBytes);
		if (pBlock->Handle) {
			pBlock->pData = (byte*)GlobalLock(pBlock->Handle);
		}
		else {
			pBlock->pData = 0;
		}
	}
	/* allocate block headers */
	for (pBlock = self->_blocks; pBlock < self->_blocks + _numBlocks; ++pBlock) {
		WAVEHDR* ph;

		pBlock->HeaderHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
		if (pBlock->HeaderHandle) {
			pBlock->pHeader = (WAVEHDR*)GlobalLock(pBlock->HeaderHandle);

			ph = pBlock->pHeader;
			if (ph) {
				ph->lpData = (char*)pBlock->pData;
				ph->dwBufferLength = (DWORD)_blockSizeBytes;
				ph->dwFlags = WHDR_DONE;
				ph->dwLoops = 0;
			}
		}
		pBlock->Prepared = FALSE;
	}

	for (port_cap_it = psy_table_begin(&self->_capPorts);
			!psy_tableiterator_equal(&port_cap_it, psy_table_end());
			psy_tableiterator_inc(&port_cap_it)) {
		PortCapt* port_capt;

		port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
		createcaptureport(self, port_capt);
	}
	self->_stopPolling = FALSE;
	ResetEvent(self->hEvent);
	if (self->driver.logger) {
		psy_logger_output(self->driver.logger, "Mme Audiodriver started\n");
	}
	_beginthread(PollerThread, 0, self);
	self->_running = TRUE;
	/* PsycleGlobal::midi().ReSync();	// MIDI IMPLEMENTATION */
	return TRUE;
}

void preparewaveformat(WAVEFORMATEXTENSIBLE* wf, int channels, int sampleRate, int bits, int validBits)
{
	/* Set up wave format structure. */
	ZeroMemory(wf, sizeof(WAVEFORMATEX));
	wf->Format.nChannels = channels;
	wf->Format.wBitsPerSample = bits;
	wf->Format.nSamplesPerSec = sampleRate;
	wf->Format.nBlockAlign = wf->Format.nChannels * wf->Format.wBitsPerSample / 8;
	wf->Format.nAvgBytesPerSec = wf->Format.nSamplesPerSec * wf->Format.nBlockAlign;
	if (bits <= 16) {
		wf->Format.wFormatTag = WAVE_FORMAT_PCM;
		wf->Format.cbSize = 0;
	}
	else {
		wf->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		wf->Format.cbSize = 0x16;
		wf->Samples.wValidBitsPerSample = validBits;
		if (channels == 2) {
			wf->dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		}
		if (validBits == 32) {
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
		}
		else {
			wf->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
		}
	}
}

void PollerThread(void* self)
{
	uintptr_t _numBlocks;

	/* universalis::os::thread_name thread_name("mme wave out"); */
	/* universalis::cpu::exceptions::install_handler_in_thread(); */
	MmeDriver* pThis = (MmeDriver*)self;
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
	_numBlocks = psy_audiodriversettings_blockcount(&pThis->settings);
	while (!pThis->_stopPolling)
	{
		CBlock* pb = pThis->_blocks + pThis->_currentBlock;
		unsigned int underruns = 0;
		while (pb->pHeader->dwFlags & WHDR_DONE)
		{
			psy_TableIterator port_cap_it;

			for (port_cap_it = psy_table_begin(&pThis->_capPorts);
				!psy_tableiterator_equal(&port_cap_it, psy_table_end());
				psy_tableiterator_inc(&port_cap_it)) {
				PortCapt* port_capt;

				port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
				if (port_capt->_handle == NULL) {
					continue;
				}
				DoBlocksRecording(pThis, port_capt);
			}
			DoBlocks(pThis);
			++pb;
			if (pb == pThis->_blocks + _numBlocks) pb = pThis->_blocks;
			if (pb->pHeader->dwFlags & WHDR_DONE) {
				++underruns;
				if (underruns > _numBlocks)
				{
					/*
					** Audio dropout most likely happened
					** (There's a possibility a dropout didn't happen, but the cpu usage
					** is almost at 100%, so we force an exit of the loop for a "Sleep()" call,
					** preventing psycle from being frozen.
					*/
					break;
				}
			}
			pThis->_currentBlock = pb - pThis->_blocks;
		}
		Sleep(pThis->pollSleep_);
	}
	SetEvent(pThis->hEvent);
	_endthread();
}

void DoBlocksRecording(MmeDriver* self, PortCapt* port)
{
	CBlock* pb = port->_blocks + self->_currentBlock;

	if (pb->Prepared)
	{
		if (waveInUnprepareHeader(port->_handle, pb->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
			/* Error("waveInUnprepareHeader() failed"); */
		}
		pb->Prepared = FALSE;
	}

	WAVEHDR* ph = pb->pHeader;
	ph->lpData = (char*)pb->pData;
	ph->dwBufferLength = (DWORD)psy_audiodriversettings_blockbytes(&self->settings);
	ph->dwFlags = WHDR_DONE;
	ph->dwLoops = 0;

	if (waveInPrepareHeader(port->_handle, pb->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		/* Error("waveInPrepareHeader() failed"); */
	}
	pb->Prepared = TRUE;
	waveInAddBuffer(port->_handle, pb->pHeader, sizeof(WAVEHDR));
	/* Put the audio in our float buffers. */
	int numSamples = (int)psy_audiodriversettings_blockframes(&self->settings);
	unsigned int _sampleValidBits = (unsigned int)psy_audiodriversettings_validbitdepth(&self->settings);
	if (_sampleValidBits == 32) {		
		psy_dsp_deinterlacefloat((float*)pb->pData, port->pleft, port->pright, numSamples);
	} else if (_sampleValidBits == 24) {		
		psy_dsp_dequantize32anddeinterlace((int*)pb->pData, port->pleft, port->pright, numSamples);
	} else {
		psy_dsp_dequantize16anddeinterlace((short int*)pb->pData, port->pleft, port->pright, numSamples);		
	}
	port->_machinepos = 0;
}

void DoBlocks(MmeDriver* self)
{
	CBlock* pb = self->_blocks + self->_currentBlock;
	int* pBlock;
	uintptr_t numSamples;
	int n;
	unsigned int _sampleValidBits;
	int hostisplaying;
	float* pFloatBlock;

	if (pb->Prepared)
	{
		while (waveOutUnprepareHeader(self->_handle, pb->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
		{
		}
		pb->Prepared = FALSE;
	}
	pBlock = (int*)pb->pData;
	numSamples = psy_audiodriversettings_blockframes(&self->settings);
	_sampleValidBits = (int)psy_audiodriversettings_validbitdepth(&self->settings);
	n = (int)numSamples;
	pFloatBlock = self->driver.callback(self->driver.callbackcontext, &n, &hostisplaying);
	if (_sampleValidBits == 32) {
		dsp.movmul(pFloatBlock, (float*)(pBlock), numSamples * 2, 1.f / 32768.f);
	}
	else if (_sampleValidBits == 24) {		
		psy_dsp_quantize24in32bit(pFloatBlock, pBlock, (int)numSamples);
	}
	else if (_sampleValidBits == 16) {
		/*
		** if (settings_->dither()) Quantize16WithDither(pFloatBlock, pBlock, numSamples);
		** else Quantize16(pFloatBlock, pBlock, numSamples);
		*/
		psy_dsp_quantize16(pFloatBlock, pBlock, (int)numSamples);
	}

	self->_writePos += numSamples;

	pb->pHeader->lpData = (char*)pb->pData;
	pb->pHeader->dwBufferLength = (DWORD)psy_audiodriversettings_blockbytes(&self->settings);
	pb->pHeader->dwFlags = 0;
	pb->pHeader->dwLoops = 0;

	if (waveOutPrepareHeader(self->_handle, pb->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveOutPrepareHeader() failed");
	}
	pb->Prepared = TRUE;
	if (waveOutWrite(self->_handle, pb->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveOutWrite() failed");
	}
}

int driver_close(psy_AudioDriver* driver)
{
	int status;
	psy_TableIterator port_cap_it;

	MmeDriver* self = (MmeDriver*)driver;
	status = stop(self);
	for (port_cap_it = psy_table_begin(&self->_capPorts);
			!psy_tableiterator_equal(&port_cap_it, psy_table_end());
			psy_tableiterator_inc(&port_cap_it)) {
		PortCapt* port_capt;

		port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
		free(port_capt);
	}
	psy_table_clear(&self->_capPorts);
	return status;
}

bool stop(MmeDriver* self)
{
	size_t _numBlocks;
	CBlock* pBlock;
	psy_TableIterator port_cap_it;

	if (!self->_running) {
		return TRUE;
	}
	self->_stopPolling = TRUE;
	_numBlocks = psy_audiodriversettings_blockcount(&self->settings);
	WaitForSingleObject(self->hEvent, INFINITE);
	/*
	** Once we get here, the PollerThread should have stopped
	** or we hang in deadlock
	*/
	if (waveOutReset(self->_handle) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveOutReset() failed");
		return FALSE;
	}
	for (;;)
	{
		BOOL alldone = TRUE;
		CBlock* pBlock;
		for (pBlock = self->_blocks; pBlock < self->_blocks + _numBlocks; pBlock++)
		{
			if ((pBlock->pHeader->dwFlags & WHDR_DONE) == 0) alldone = FALSE;
		}
		if (alldone) break;
		Sleep(10);
	}
	for (pBlock = self->_blocks; pBlock < self->_blocks + _numBlocks; pBlock++)
	{
		if (pBlock->Prepared)
		{
			if (waveOutUnprepareHeader(self->_handle, pBlock->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			{
				output_error(self, "waveOutUnprepareHeader() failed");
			}
		}
	}
	if (waveOutClose(self->_handle) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveOutClose() failed");
		return FALSE;
	}
	for (pBlock = self->_blocks; pBlock < self->_blocks + _numBlocks; pBlock++)
	{
		GlobalUnlock(pBlock->Handle);
		GlobalFree(pBlock->Handle);
		GlobalUnlock(pBlock->HeaderHandle);
		GlobalFree(pBlock->HeaderHandle);
	}
	for (port_cap_it = psy_table_begin(&self->_capPorts);
		!psy_tableiterator_equal(&port_cap_it, psy_table_end());
		psy_tableiterator_inc(&port_cap_it)) {
		PortCapt* port_capt;

		port_capt = (PortCapt*)psy_tableiterator_value(&port_cap_it);
		if (port_capt->_handle == NULL)
			continue;
		if (waveInReset(port_capt->_handle) != MMSYSERR_NOERROR)
		{
			output_error(self, "waveInReset() failed");
			return FALSE;
		}
		/*! @todo: wait until WHDR_DONE like with waveout? */
		for (pBlock = port_capt->_blocks; pBlock < port_capt->_blocks + _numBlocks; pBlock++)
		{
			if (pBlock->Prepared)
			{
				if (waveInUnprepareHeader(port_capt->_handle, pBlock->pHeader, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
				{
					output_error(self, "waveInUnprepareHeader() failed");
				}
			}
		}
		waveInClose(port_capt->_handle);
		port_capt->_handle = NULL;
		dsp.memory_dealloc(port_capt->pleft);
		dsp.memory_dealloc(port_capt->pright);
		for (pBlock = port_capt->_blocks; pBlock < port_capt->_blocks + _numBlocks; pBlock++)
		{
			GlobalUnlock(pBlock->Handle);
			GlobalFree(pBlock->Handle);
			GlobalUnlock(pBlock->HeaderHandle);
			GlobalFree(pBlock->HeaderHandle);
		}
	}
	self->_running = FALSE;
	return TRUE;
}

void refreshavailableports(MmeDriver* self)
{
	enumerateplaybackports(self);
	enumerate_capture_ports(self);
}

void enumerateplaybackports(MmeDriver* self)
{
	unsigned int i;

	clearplayenums(self);
	for (i = 0; i < waveOutGetNumDevs(); ++i) {
		PortEnums* port;
		WAVEOUTCAPS caps;

		waveOutGetDevCaps(i, &caps, sizeof(WAVEOUTCAPS));
		port = (PortEnums*) malloc(sizeof(PortEnums));
		if (port) {
			char* utf8str;
			
			utf8str = psy_dos_to_utf8(caps.szPname, NULL);
			portenums_init_all(port, i, utf8str);
			free(utf8str);
			utf8str = NULL;
			psy_list_append(&self->_playEnums, port);
		}
	}
}

void enumerate_capture_ports(MmeDriver* self)
{
	unsigned int i;

	clearcapenums(self);
	for (i = 0; i < waveInGetNumDevs(); ++i) {
		PortEnums* port;
		WAVEINCAPS caps;
		waveInGetDevCaps(i, &caps, sizeof(WAVEINCAPS));		
		port = (PortEnums*) malloc(sizeof(PortEnums));
		if (port) {
			char* utf8str;

			utf8str = psy_dos_to_utf8(caps.szPname, NULL);
			portenums_init_all(port, i, utf8str);
			free(utf8str);
			utf8str = NULL;
			psy_list_append(&self->_capEnums, port);
		}
	}
}

void clearplayenums(MmeDriver* self)
{
	psy_List* p;

	for (p = self->_playEnums; p != NULL; p = p->next) {
		PortEnums* port = (PortEnums*)p->entry;
		portenums_dispose(port);
		free(port);
	}
	psy_list_free(self->_playEnums);
	self->_playEnums = 0;
}

void clearcapenums(MmeDriver* self)
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

bool createcaptureport(MmeDriver* self, PortCapt* port)
{
	HRESULT hr;
	WAVEFORMATEX format;
	uintptr_t _blockSizeBytes;
	uintptr_t _numBlocks;
	CBlock* pBlock;

	/* avoid opening a port twice */
	if (port->_handle) {
		return TRUE;
	}
	preparewaveformat((WAVEFORMATEXTENSIBLE*)&format,
		(int)psy_audiodriversettings_numchannels(&self->settings),
		(int)psy_audiodriversettings_samplespersec(&self->settings),
		(int)psy_audiodriversettings_bitdepth(&self->settings),
		(int)psy_audiodriversettings_validbitdepth(&self->settings));
	_blockSizeBytes = psy_audiodriversettings_blockbytes(&self->settings);
	_numBlocks = psy_audiodriversettings_blockcount(&self->settings);
	if ((hr = waveInOpen(&port->_handle, port->_idx, (LPWAVEFORMATEX)(&format),
		(DWORD_PTR)NULL, (DWORD_PTR)NULL, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveInOpen() failed");
		return FALSE;
	}
	/* allocate blocks */
	for (pBlock = port->_blocks; pBlock < port->_blocks + _numBlocks; ++pBlock)
	{
		pBlock->Handle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, _blockSizeBytes);
		if (pBlock->Handle) {
			pBlock->pData = (byte*) GlobalLock(pBlock->Handle);
		}
	}
	/* allocate block headers */
	for (pBlock = port->_blocks; pBlock < port->_blocks + _numBlocks; ++pBlock)
	{		
		pBlock->HeaderHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof(WAVEHDR));
		if (pBlock->HeaderHandle) {
			pBlock->pHeader = (WAVEHDR*)GlobalLock(pBlock->HeaderHandle);
			if (pBlock->pHeader) {
				WAVEHDR* ph;
				ph = pBlock->pHeader;
				ph->lpData = (char*)pBlock->pData;
				ph->dwBufferLength = (DWORD)_blockSizeBytes;
				ph->dwFlags = WHDR_DONE;
				ph->dwLoops = 0;
			}
		}
		pBlock->Prepared = FALSE;
	}
	port->pleft = dsp.memory_alloc(_blockSizeBytes * sizeof(float), 1);
	port->pright = dsp.memory_alloc(_blockSizeBytes * sizeof(float), 1);

	ZeroMemory(port->pleft, 2 * _blockSizeBytes);
	ZeroMemory(port->pright, 2 * _blockSizeBytes);
	port->_machinepos = 0;

	waveInStart(port->_handle);

	return TRUE;
}

bool addcaptureport(MmeDriver* self, int idx)
{
	PortCapt* port;
	bool isplaying = self->_running;

	if (idx >= (int) psy_list_size(self->_capEnums)) return FALSE;
	if (psy_table_exists(&self->_capPorts, idx)) {
		return TRUE;
	}
	if (isplaying) stop(self);
	
	port = malloc(sizeof(PortCapt));
	if (port) {
		portcapt_init(port);
		port->_idx = idx;
		psy_table_insert(&self->_capPorts, idx, port);
		if (isplaying) return start(self);
	} else {
		return FALSE;
	}
	return TRUE;
}

bool removecaptureport(MmeDriver* self, int idx)
{
	bool isplaying = self->_running;
	PortCapt* port_capt;

	port_capt = (PortCapt*)psy_table_at(&self->_capPorts, idx);
	if (!port_capt) {
		return FALSE;
	}
	if (isplaying) {
		stop(self);
	}

	free(port_capt);
	psy_table_remove(&self->_capPorts, idx);
	if (isplaying) {
		start(self);
	}
	return TRUE;
}

void readbuffers(MmeDriver* self, int idx, float** pleft, float** pright, uintptr_t numsamples)
{
	PortCapt* port_capt;
	uintptr_t mpos;

	*pleft = 0;
	*pright = 0;
	port_capt = (PortCapt*)psy_table_at(&self->_capPorts, idx);
	if (!self->_running || !port_capt || !port_capt->_handle) {
		return;
	}
	mpos = port_capt->_machinepos;
	*pleft = port_capt->pleft + mpos;
	*pright = port_capt->pright + mpos;
	port_capt->_machinepos += numsamples;
}

const char* capturename(const psy_AudioDriver* driver, intptr_t index)
{
	const MmeDriver* self = (const MmeDriver*) driver;
	const psy_List* pPort;

	if (self->_capEnums) {
		pPort = psy_list_at_const(self->_capEnums, index);
		if (pPort) {
			return ((const PortEnums*)(pPort->entry))->portname;
		}
	}
	return "";
}

uintptr_t numcaptures(const psy_AudioDriver* driver)
{
	MmeDriver* self = (MmeDriver*)driver;

	return psy_list_size(self->_capEnums);
}

const char* playbackname(const psy_AudioDriver* driver, intptr_t index)
{
	const MmeDriver* self = (const MmeDriver*)driver;
	const psy_List* pPort;

	if (self->_playEnums) {
		pPort = psy_list_at_const(self->_playEnums, index);
		if (pPort) {
			return ((const PortEnums*)(pPort->entry))->portname;
		}
	}
	return "";
}

uintptr_t numplaybacks(const psy_AudioDriver* driver)
{
	const MmeDriver* self = (const MmeDriver*)driver;
	
	return (uintptr_t)psy_list_size(self->_playEnums);
}

const psy_AudioDriverInfo* driver_info(psy_AudioDriver* self)
{
	return GetPsycleDriverInfo();
}

const psy_Property* driver_configuration(const psy_AudioDriver* driver)
{
	MmeDriver* self = (MmeDriver*)driver;

	return self->configuration;
}

uintptr_t playposinsamples(const psy_AudioDriver* driver)
{
	uint32_t retval;
	MMTIME time;

	MmeDriver* self = (MmeDriver*)driver;
	/* WARNING!waveOutGetPosition in TIME_SAMPLES has max of 0x7FFFFF for 16bit stereo signals. */
	if (self->_stopPolling) {
		return 0;
	}
	time.wType = TIME_SAMPLES;
	if (waveOutGetPosition(self->_handle, &time, sizeof(MMTIME)) != MMSYSERR_NOERROR)
	{
		output_error(self, "waveOutGetPosition() failed");
	}
	if (time.wType != TIME_SAMPLES)
	{
		output_error(self, "waveOutGetPosition() doesn't support TIME_SAMPLES");
	}

	retval = time.u.sample;
	/* sample counter wrap around ? */
	if (self->m_lastPlayPos > (int)retval)
	{
		self->m_readPosWraps++;
		if (self->m_lastPlayPos > (int)self->_writePos) {
			self->m_readPosWraps = 0;
			/* PsycleGlobal::midi().ReSync();	// MIDI IMPLEMENTATION */
		}
	}
	self->m_lastPlayPos = retval;
	return retval + (self->m_readPosWraps * 0x800000);
}
