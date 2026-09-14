///\file
///\brief Arguru xfilter (crossdelay)

#include "../detail/prefix.h"


#include <psycle/plugin_interface.hpp>
#include <operations.h>
// #include <psycle/helpers/dsp.hpp>
// #include <universalis/stdlib/cstdint.hpp>
// #include <universalis/os/aligned_alloc.hpp>
#include <cstdint>
#include <cstdio>

using namespace psycle::plugin_interface;
// using namespace psycle::helpers;
// using namespace universalis::os;
// using namespace universalis::stdlib;

CMachineParameter const paraDelay = {"Delay time","Delay time",0,88200,MPF_STATE,11025};
CMachineParameter const paraFeedback = {"Feedback","Feedback",0,256,MPF_STATE,128};
CMachineParameter const paraDry = {"Dry","Dry",0,256,MPF_STATE,256};
CMachineParameter const paraWet = {"Wet","Wet",0,256,MPF_STATE,128};
CMachineParameter const paraTickmode = {"Lines mode","Lines mode",0,1,MPF_STATE,0};
CMachineParameter const paraTicktweak = {"Lines","Lines",0,8,MPF_STATE,3};

CMachineParameter const *pParameters[] = {
	&paraDelay,
	&paraFeedback,
	&paraDry,
	&paraWet,
	&paraTickmode,
	&paraTicktweak
};

CMachineInfo const MacInfo (
	MI_VERSION,
	0x0120,
	EFFECT,
	sizeof pParameters / sizeof *pParameters,
	pParameters,
	"Arguru CrossDelay"
	#ifndef NDEBUG
		" (Debug build)"
	#endif
	,
	"CrossDelay",
	"J. Arguelles",
	"About",
	1
);

class mi : public CMachineInterface {
	public:
		mi();
		virtual ~mi();

		virtual void Init();
		virtual void SequencerTick();
		virtual void Work(float *psamplesleft, float *psamplesright , int numsamples, int tracks);
		virtual void ParameterTweak(int par, int val);

		virtual bool DescribeValue(char* txt,int const param, int const value);
		virtual void Command();

	private:
		void SetDelay(int delay);
		void SetDelayTicks(int ticks);
		void AllocateBuffers()
		{
			/* memory_alloc is count x element-size; alignment is supplied by the
			** DSP backend.  CrossDelay historically passed 16 as the count, which
			** over-allocated every float buffer by four times. */
			dbl = (float*)dsp.memory_alloc(max_delay_samples, sizeof(float));
			dbr = (float*)dsp.memory_alloc(max_delay_samples, sizeof(float));
			dsp.clear(dbl, max_delay_samples);
			dsp.clear(dbr, max_delay_samples);
		}
		void DeallocateBuffers() {
			dsp.memory_dealloc(dbl);
			dsp.memory_dealloc(dbr);
		}
		float *dbl;
		float *dbr;
		int32_t dcl,dcr,ccl,ccr;
		int32_t currentSR;
		int32_t currentTL;
		int32_t max_delay_samples;
};

PSYCLE__PLUGIN__INSTANTIATOR(mi, MacInfo)

mi::mi(): dcl(0), dcr(0), ccl(0), ccr(0), currentSR(0), currentTL(0), max_delay_samples(32768) {
	psy_dsp_init();
	Vals = new int[MacInfo.numParameters];
	Vals[4]=0;
	AllocateBuffers();
}

mi::~mi() {
	delete[] Vals;
	DeallocateBuffers();
}

void mi::Init() {
	currentSR = pCB->GetSamplingRate();
	currentTL = pCB->GetTickLength();
}

void mi::SequencerTick() {
	if (currentSR != pCB->GetSamplingRate()) {
		currentSR = pCB->GetSamplingRate();
		if( !Vals[4]) {
			SetDelay(Vals[0]);
		}
	}
	if (currentTL != pCB->GetTickLength()) {
		currentTL = pCB->GetTickLength();
		if( Vals[4]) {
			SetDelayTicks(Vals[5]);
		}
	}
}

void mi::Command() {
	pCB->MessBox("Originally made 18/5/2000 by Juan Antonio Arguelles Rius for Psycl3!","-=<([aRgUrU's Cr0sSdElAy])>=-",0);
}

void mi::ParameterTweak(int par, int val) {
	Vals[par]=val;

	if((par==0 || par == 4 ) && Vals[4]==0) {
		SetDelay(Vals[0]);
	}
	if((par==5 || par == 4) && Vals[4]==1) {
		SetDelayTicks(Vals[5]);
	}
}

void mi::SetDelay(int delay) {
	int delaySR = (int)(delay*(float)currentSR/44100.0f);
	if (delaySR > max_delay_samples) {
		do {
			max_delay_samples <<=1;
		} while(delaySR > max_delay_samples);
		DeallocateBuffers();
		AllocateBuffers();
	} else {
		dsp.clear(dbl, max_delay_samples);
		dsp.clear(dbr, max_delay_samples);
	}
	ccl=max_delay_samples-8;
	ccr=ccl-(delaySR/2);
	
	dcl=ccl-delaySR;
	dcr=ccr-delaySR;

	if(dcl<0)dcl=0;
	if(dcr<0)dcr=0;
}

void mi::SetDelayTicks(int ticks) {
	/* Lines mode historically follows tracker timing.  The host accepts BPM 32
	** and LPB 1, while CrossDelay exposes up to eight Lines; with the retained
	** x2 stereo-delay convention that makes 30 seconds the largest ordinary
	** Lines delay (8 * 2 * 60 / 32).  Preserve that full supported envelope at
	** the current sample rate and cap only timing stretched beyond it.  Use
	** 64-bit arithmetic before capping so signed-int overflow cannot bypass the
	** resource guard. */
	const int64_t requested_delay =
		(int64_t)ticks * (int64_t)pCB->GetTickLength() * 2;
	const int64_t resource_samplerate = currentSR > 0
		? (int64_t)currentSR
		: (int64_t)44100;
	const int64_t max_resource_delay = resource_samplerate * 30;
	const int64_t bounded_delay = requested_delay > max_resource_delay
		? max_resource_delay
		: requested_delay;
	int delaySR = bounded_delay > 0 ? (int)bounded_delay : 0;
	if (delaySR > max_delay_samples) {
		do {
			max_delay_samples <<=1;
		} while(delaySR > max_delay_samples);
		DeallocateBuffers();
		AllocateBuffers();
	} else {
		dsp.clear(dbl, max_delay_samples);
		dsp.clear(dbr, max_delay_samples);
	}
	ccl=max_delay_samples-8;
	ccr=ccl-(delaySR/2);

	dcl=ccl-delaySR;
	dcr=ccr-delaySR;

	if(dcl<0)dcl=0;
	if(dcr<0)dcr=0;
}


void mi::Work(float *psamplesleft, float *psamplesright , int numsamples, int tracks) {
	float const fbc=Vals[1]*0.00390625f;
	float const cdry=Vals[2]*0.00390625f;
	float const cwet=Vals[3]*0.00390625f;
	do {
		float const il=*psamplesleft;
		float const ir=*psamplesright;

		dbl[ccl]=il+dbl[dcl]*fbc;
		dbr[ccr]=ir+dbr[dcr]*fbc;

		*psamplesleft =il*cdry+dbl[dcl]*cwet;
		*psamplesright =ir*cdry+dbr[dcl]*cwet;
		
		if(++ccl==max_delay_samples)ccl=0;
		if(++ccr==max_delay_samples)ccr=0;
		if(++dcl==max_delay_samples)dcl=0;
		if(++dcr==max_delay_samples)dcr=0;

		++psamplesleft;
		++psamplesright;
	} while(--numsamples);
}

bool mi::DescribeValue(char* txt,int const param, int const value) {
	switch(param) {
		case 0:
			{
				if(Vals[4]) { std::sprintf(txt,"--"); }
				else {
					float const spt=(float)pCB->GetTickLength()*2;
					std::sprintf(txt,"%.3f (%.fms)",(float)value/spt, value/88.2);
				}
				return true;
			}
		case 1:
			{
				std::sprintf(txt,"%.1f%%",(float)value*0.390625f);
				return true;
			}
		case 2: //fallthrough
		case 3:
			{
				float coef=value*0.00390625f;
				if(coef>0.0f)
					std::sprintf(txt,"%.1f dB",20.0f * log10(coef));
				else
					std::sprintf(txt,"-Inf. dB");				
				return true;
			}
		case 4:
			{
				if(value==0)
					std::sprintf(txt,"Off");
				else
					std::sprintf(txt,"On");
				return true;
			}
		case 5:
			{
				if(!Vals[4]) { std::sprintf(txt,"--"); return true; }
			}
		default: return false;
	}
}
