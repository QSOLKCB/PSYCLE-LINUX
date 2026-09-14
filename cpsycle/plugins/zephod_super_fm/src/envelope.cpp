#include "../detail/prefix.h"

#include "envelope.hpp"

envelope::envelope()
{
	a=1;
	d=1;
	s=0;
	r=1;
	envstate=ENV_NONE;
	envvol=0.0f;
	susvol=0.0f;
	envcoef=0.0f;
	suscounter=0;
}

envelope::~envelope()
{
}

void envelope::reset()
{
	/* A stopped or zero-level envelope must start a fresh attack.  The retained
	** code used the anti-click path whenever envstate was non-NONE, but at zero
	** level that produces envcoef == 0 and can leave the first/retriggered note
	** stuck in ENV_ANTICLICK forever. */
	if(envstate==ENV_NONE || envvol<=0.0f)
	{
		envstate=ENV_ATT;
		envcoef=1.0f/(float)a;
	}
	else
	{
		envstate=ENV_ANTICLICK;
		envcoef=envvol/32.0f;
	}
}

void envelope::attack(int newv)
{
	a=newv;
}
void envelope::decay(int newv)
{
	d=newv;
}
void envelope::sustain(int newv)
{
	s=newv;
}
void envelope::release(int newv)
{
	r=newv;
}
void envelope::sustainv(float newv)
{
	susvol=newv;
}
void envelope::stop()
{
	envvol=0;
	envstate=ENV_NONE;
}

void envelope::noteoff()
{
	if (envstate!=ENV_NONE)
	{
		envstate=ENV_REL;
		envcoef=envvol/(float)r;
	}
}
