/*
** This source is free software; you can redistribute it and /or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#if !defined(MIDIMONITOR_H)
#define MIDIMONITOR_H

/* host */
#include "labelpair.h"
#include "titlebar.h"
#include "workspace.h"
/* ui */
#include <uibutton.h>
#include <uiscroller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MidiChannelBox {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	psy_ui_Component desc;
	psy_ui_Label channel_desc[psy_audio_MAX_MIDI_CHANNELS];
	psy_ui_Component status;
	psy_ui_Label channel_status[psy_audio_MAX_MIDI_CHANNELS];
	/* references */
	uint32_t* channelmap;
} MidiChannelBox;

void midichannelbox_init(MidiChannelBox*,
	psy_ui_Component* parent, uint32_t* channelmap);

void midichannelbox_update(MidiChannelBox*);

INLINE psy_ui_Component* midichannelbox_base(MidiChannelBox* self)
{
	return &self->component;
}

typedef struct MidiClockBox {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	psy_ui_Component start;
	psy_ui_Label start_desc;
	psy_ui_Label start_status;
	psy_ui_Component clock;
	psy_ui_Label clock_desc;
	psy_ui_Label clock_status;
	psy_ui_Component stop;
	psy_ui_Label stop_desc;
	psy_ui_Label stop_status;
	/* references */
	uint32_t* flags;
} MidiClockBox;

void midiclockbox_init(MidiClockBox*,
	psy_ui_Component* parent, uint32_t* flags);

void midiclockbox_update(MidiClockBox*);

INLINE psy_ui_Component* midiclockbox_base(MidiClockBox* self)
{
	return &self->component;
}

typedef struct MidiFlagsView {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	MidiChannelBox channelmap;
	MidiClockBox clock;
	/* references */
	Workspace* workspace;
} MidiFlagsView;

void midiflagsview_init(MidiFlagsView*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* midiflagsview_base(MidiFlagsView* self)
{
	return &self->component;
}

typedef struct MidiChannelMappingBox {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	double colx_px[4];
	/* references */
	Workspace* workspace;
} MidiChannelMappingBox;

void midichannelmappingbox_init(MidiChannelMappingBox*, psy_ui_Component* parent,
	Workspace*);

INLINE psy_ui_Component* midichannelmappingbox_base(MidiChannelMappingBox* self)
{
	return &self->component;
}

typedef struct MidiChannelMappingView {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	psy_ui_Scroller scroller;
	MidiChannelMappingBox channelmapping;
	psy_ui_Component header;
	psy_ui_Label channel;
	psy_ui_Label mac;
	psy_ui_Label inst;	
	psy_ui_Label note_off;	
	/* references */
	Workspace* workspace;
} MidiChannelMappingView;

void midichannelmappingview_init(MidiChannelMappingView*,
	psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* midichannelmappingview_base(MidiChannelMappingView* self)
{
	return &self->component;
}

typedef struct MidiMonitor {
	/* inherits */
	psy_ui_Component component;
	/* internal */
	psy_ui_Component client;
	TitleBar titlebar;	
	psy_ui_Button configure;	
	psy_ui_Label coretitle;
	psy_ui_Component top;
	psy_ui_Component resources;
	psy_ui_Label resourcestitle;
	LabelPair resources_win;
	LabelPair resources_mem;
	LabelPair resources_swap;
	LabelPair resources_vmem;		
	psy_ui_Component performance;
	LabelPair audiothreads;
	LabelPair totaltime;
	LabelPair machines;
	LabelPair routing;
	psy_ui_Button cpucheck;
	MidiFlagsView flags;
	psy_ui_Label flagtitle;	
	psy_ui_Component topchannelmapping;
	psy_ui_Label channelmappingtitle;
	psy_ui_Button mapconfigure;	
	MidiChannelMappingView channelmapping;	
	psy_ui_Margin topmargin;
	uintptr_t channelmapupdate;
	uint32_t lastchannelmap;
	uintptr_t lastflags;
	int channelstatcounter;
	int flagstatcounter;
	/* references */
	Workspace* workspace;
} MidiMonitor;

void midimonitor_init(MidiMonitor*, psy_ui_Component* parent, Workspace*);

INLINE psy_ui_Component* midimonitor_base(MidiMonitor* self)
{
	return &self->component;
}

#ifdef __cplusplus
}
#endif

#endif /* MIDIMONITOR_H */
