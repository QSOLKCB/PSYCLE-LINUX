// This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
// copyright 2000-2021 members of the psycle project http://psycle.sourceforge.net
#include "plugin_interface.h"

#include "machine.h"
#include "library.h"

#include <cmath>
#include <limits>
#include <string.h>

#include "../../detail/os.h"

#if defined DIVERSALIS__OS__MICROSOFT
#include <windows.h>
#endif

typedef CMachineInfo* (*GETINFO)(void);
typedef CMachineInterface* (*CREATEMACHINE)(void);

class PluginFxCallback : public CFxCallback
{

	inline virtual void MessBox(char const* ptxt, char const* caption, unsigned int type) const
	{
// #if defined DIVERSALIS__OS__MICROSOFT
//		MessageBox(NULL, ptxt, caption, type);
// #endif
		if (callback) {
			callback->vtable->message(callback, ptxt);
		}
	}
	inline virtual int GetTickLength() const {
		/* A freshly initialized MachineCallback has no Player yet. Its timing
		** vtable deliberately returns positive sentinel values (4096 beats per
		** line and 512 beats per sample), so positivity alone cannot identify
		** usable sequencer timing. Only consume the line/sample ratio once a
		** live Player has been attached; headless factory flows must use the
		** historical sample-rate/BPM fallback below. */
		if (callback && callback->player && callback->vtable &&
				callback->vtable->currbeatsperline && callback->vtable->beatspersample) {
			const double beats_per_line = callback->vtable->currbeatsperline(callback);
			const double beats_per_sample = callback->vtable->beatspersample(callback);
			if (beats_per_line > 0.0 && beats_per_sample > 0.0 &&
					std::isfinite(beats_per_line) && std::isfinite(beats_per_sample)) {
				/* Native Psycle machines historically call this a tick length, but
				** the ABI value is the current tracker line duration. Host TPB is a
				** finer transport clock (normally 24) and must not shorten the line
				** seen by plugins (normally LPB 4). Preserve historical truncation
				** for genuinely fractional durations while moving one representable
				** step upward first so an exact integral duration that landed one ULP
				** low (for example 25199.999999999996) is not shortened by a sample.
				** The native ABI returns int, so saturate valid oversized durations
				** at INT_MAX instead of invoking undefined floating-to-int narrowing. */
				const double samples_per_line = beats_per_line / beats_per_sample;
				const double max_tick_length =
					(double)std::numeric_limits<int>::max();
				if (!std::isfinite(samples_per_line) ||
						samples_per_line > max_tick_length) {
					return std::numeric_limits<int>::max();
				}
				const double stabilized_samples = std::nextafter(samples_per_line,
					std::numeric_limits<double>::infinity());
				if (!std::isfinite(stabilized_samples) ||
						stabilized_samples > max_tick_length) {
					return std::numeric_limits<int>::max();
				}
				return (int)stabilized_samples;
			}
		}
		const int samplerate = GetSamplingRate();
		const int bpm = GetBPM();
		const int tpb = GetTPB();
		return (samplerate > 0 && bpm > 0 && tpb > 0)
			? (int)(((double)samplerate * 60.0) / ((double)bpm * (double)tpb))
			: 256;
	}
	inline virtual int GetSamplingRate() const {
		return callback
			? (int)callback->vtable->samplerate(callback)
			: 44100;
	}
	inline virtual int GetBPM() const {
		return callback
			? (int)callback->vtable->bpm(callback)
			: 125;
	}
	inline virtual int GetTPB() const { return 4; }
	virtual int CallbackFunc(int /*cbkID*/, int /*par1*/, int /*par2*/, void* /*par3*/)
	{
		return 0;
	}
	virtual bool FileBox(bool openMode, char filter[], char inoutName[])
	{
		if (callback) {
			return callback->vtable->fileselect_load(callback, filter, inoutName) != FALSE;
		}
		return false;
	}
	/// unused slot kept for binary compatibility for (old) closed-source plugins on msvc++ on mswindows.
	inline virtual float * unused0(int, int) { return NULL;}
	/// unused slot kept for binary compatibility for (old) closed-source plugins on msvc++ on mswindows.
	inline virtual float * unused1(int, int) { return NULL;}

	public: ///\todo private:
		psy_audio_MachineCallback* callback;
};

void mi_resetcallback(CMachineInterface* mi)
{
	mi->pCB = 0;
}

void mi_setcallback(CMachineInterface* mi, struct psy_audio_MachineCallback* callback)
{
	PluginFxCallback* pCB;

	if (mi->pCB == 0) {
		mi->pCB = new PluginFxCallback;
	}	
	pCB = dynamic_cast<PluginFxCallback*>(mi->pCB);
	if (pCB) {
		pCB->callback = callback;		
	}
}

void mi_init(CMachineInterface* mi)
{
	mi->Init();	
}

void mi_dispose(CMachineInterface* mi)
{
	delete mi->pCB;
}

void mi_sequencertick(CMachineInterface* mi)
{
	mi->SequencerTick();
}		

void mi_parametertweak(CMachineInterface* mi, int par, int val)
{
	mi->ParameterTweak(par, val);
}		

void mi_work(CMachineInterface* mi, float * psamplesleft, float * psamplesright, int numsamples, int tracks)
{
	mi->Work(psamplesleft, psamplesright, numsamples, tracks);
}

void mi_stop(CMachineInterface* mi)
{
	mi->Stop();
}

void mi_putdata(CMachineInterface* mi, void * data)
{
   mi->PutData(data);
}

void mi_getdata(CMachineInterface* mi, void * data)
{
	int size;

	size = mi->GetDataSize();
	if (data && size > 0) {
		/* Some historical native machines deliberately skip pointer-sized
		** compatibility slots while serializing opaque state. Callers allocate
		** an uninitialized buffer, so zero it first to make those reserved bytes
		** deterministic without changing the historical state layout. */
		memset(data, 0, (size_t)size);
	}
	mi->GetData(data);
}

int mi_getdatasize(CMachineInterface* mi)
{
	return mi->GetDataSize();
}

void mi_command(CMachineInterface* mi)
{
	mi->Command();
}

void mi_unused0(CMachineInterface* mi, int track)
{
	mi->unused0(track);
}

int mi_unused1(CMachineInterface* mi, int track)
{
	return mi->unused1(track);
}

void mi_midievent(CMachineInterface* mi, int channel, int midievent, int value)
{
	mi->MidiEvent(channel, midievent, value);
}

void mi_unused2(CMachineInterface* mi, unsigned int const data)
{
	mi->unused2(data);
}
		
int mi_describevalue(CMachineInterface* mi, char* txt, int const param, int const value)
{
	return mi->DescribeValue(txt, param, value); 
}

int mi_hostevent(CMachineInterface* mi, int const eventNr, int const val1, float const val2)
{
	return mi->HostEvent(eventNr, val1, val2);
}

void mi_seqtick(CMachineInterface* mi, int channel, int note, int ins, int cmd, int val)
{
	mi->SeqTick(channel, note, ins, cmd, val);
}

void mi_unused3(CMachineInterface* mi)
{
	mi->unused3();
}

CMachineInterface* mi_create(void* module)
{
	CMachineInterface* mi;
	CREATEMACHINE GetInterface;
	psy_Library library;
	
	library.module = module;
	library.err = 0;
	
	mi = 0;
	GetInterface = (CREATEMACHINE) psy_library_functionpointer(&library, "CreateMachine");
	if (GetInterface != NULL)
	{		
		mi = GetInterface();			
	}
	return mi;
}

int mi_val(CMachineInterface* mi, int param)
{
	return mi->Vals[param];
}

void mi_setval(CMachineInterface* mi, int param, int val)
{
	mi->Vals[param] = val;
}
