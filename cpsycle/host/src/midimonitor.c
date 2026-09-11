/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "midimonitor.h"
/* host */
#include "styles.h"
/* platform */
#include "../../detail/portable.h"


/* MidiChannelBox */

/* prototypes */
static void midichannelbox_init_channels(MidiChannelBox*);
static bool midichannelbox_channel_active(const MidiChannelBox*,
	uintptr_t ch);
	
/* implementation */
void midichannelbox_init(MidiChannelBox* self,
	psy_ui_Component* parent, uint32_t* channelmap)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);	
	self->channelmap = channelmap;
	midichannelbox_init_channels(self);
}

void midichannelbox_init_channels(MidiChannelBox* self)
{
	uintptr_t ch;	

	assert(self);	

	psy_ui_component_init_align(&self->desc, midichannelbox_base(self),
		NULL, psy_ui_ALIGN_TOP);
	psy_ui_component_init_align(&self->status, midichannelbox_base(self),
		NULL, psy_ui_ALIGN_TOP);
	for (ch = 0; ch < psy_audio_MAX_MIDI_CHANNELS; ++ch) {
		char text[256];

		psy_snprintf(text, 256, "%d", (ch + 1));
		psy_ui_label_init_text(&self->channel_desc[ch], &self->desc, text);
		psy_ui_label_set_char_number(&self->channel_desc[ch], 3.5);
		psy_ui_component_set_align(psy_ui_label_base(&self->channel_desc[ch]),
			psy_ui_ALIGN_LEFT);
		psy_ui_label_init(&self->channel_status[ch], &self->status);
		psy_ui_label_set_char_number(&self->channel_status[ch], 3.5);
		psy_ui_component_set_align(psy_ui_label_base(&self->channel_status[ch]),
			psy_ui_ALIGN_LEFT);
	}
}

void midichannelbox_update(MidiChannelBox* self)
{
	uintptr_t ch;

	assert(self);	

	for (ch = 0; ch < psy_audio_MAX_MIDI_CHANNELS; ++ch) {				
		if (midichannelbox_channel_active(self, ch)) {
			psy_ui_label_set_text(&self->channel_status[ch], ".");
		} else {
			psy_ui_label_set_text(&self->channel_status[ch], "");
		}		
	}
}

bool midichannelbox_channel_active(const MidiChannelBox* self,
	uintptr_t ch)
{
	assert(self);	

	return (self->channelmap && ((*self->channelmap) & (0x01 << ch)));
}


/* MidiClockBox */

/* prototypes */
static void midiclockbox_init_labels(MidiClockBox*);
static void midiclockbox_init_row(MidiClockBox*,
	psy_ui_Component* row, psy_ui_Label* desc, const char* desc_str,
	psy_ui_Label* status);

/* implementation */
void midiclockbox_init(MidiClockBox* self,
	psy_ui_Component* parent, uint32_t* flags)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);	
	self->flags = flags;
	midiclockbox_init_labels(self);
}

void midiclockbox_init_labels(MidiClockBox* self)
{
	assert(self);

	midiclockbox_init_row(self, &self->start, &self->start_desc,
		"MIDI Sync: START", &self->start_status);
	midiclockbox_init_row(self, &self->clock, &self->clock_desc,
		"MIDI Sync: CLOCK", &self->clock_status);
	midiclockbox_init_row(self, &self->stop, &self->stop_desc,
		"MIDI Sync: STOP", &self->stop_status);	
}

void midiclockbox_init_row(MidiClockBox* self, psy_ui_Component* row,
	psy_ui_Label* desc, const char* desc_str, psy_ui_Label* status)
{
	assert(self);

	psy_ui_component_init_align(row, midiclockbox_base(self),
		NULL, psy_ui_ALIGN_TOP);
	psy_ui_label_init_text(desc, row, desc_str);
	psy_ui_label_set_char_number(desc, 20.0);
	psy_ui_component_set_align(psy_ui_label_base(desc), psy_ui_ALIGN_LEFT);
	psy_ui_label_init(status, row);
	psy_ui_label_set_char_number(status, 2.0);
	psy_ui_component_set_align(psy_ui_label_base(status), psy_ui_ALIGN_LEFT);
}

void midiclockbox_update(MidiClockBox* self)
{
	assert(self);

	if ((*self->flags & FSTAT_FASTART) == FSTAT_FASTART) {
		psy_ui_label_set_text(&self->start_status, ".");
	} else {
		psy_ui_label_set_text(&self->start_status, "");
	}
	if ((*self->flags & FSTAT_F8CLOCK) == FSTAT_F8CLOCK) {
		psy_ui_label_set_text(&self->clock_status, ".");
	} else {
		psy_ui_label_set_text(&self->clock_status, "");
	}
	if ((*self->flags & FSTAT_FCSTOP) == FSTAT_FCSTOP) {
		psy_ui_label_set_text(&self->stop_status, ".");
	} else {
		psy_ui_label_set_text(&self->stop_status, "");
	}
}


/* MidiFlagsView */

/* implementation */
void midiflagsview_init(MidiFlagsView* self, psy_ui_Component* parent,
	Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	self->workspace = workspace;
	midiclockbox_init(&self->clock, &self->component,
		&workspace_player(self->workspace)->midiinput.stats.flags);
	psy_ui_component_set_align(&self->clock.component, psy_ui_ALIGN_TOP);
	midichannelbox_init(&self->channelmap, &self->component,
		&workspace_player(self->workspace)->midiinput.stats.channelmap);
	psy_ui_component_set_align(&self->channelmap.component, psy_ui_ALIGN_TOP);	
}

/* MidiChannelMappingBox */

/* prototypes */
static void midichannelmappingbox_on_draw(MidiChannelMappingBox*,
	psy_ui_Graphics*);
static void midichannelmappingbox_on_align(MidiChannelMappingBox*);

/* vtable */
static psy_ui_ComponentVtable midichannelmappingbox_vtable;
static bool midichannelmappingbox_vtable_initialized = FALSE;

/* implementation */
static void vtable_init(MidiChannelMappingBox* self)
{
	if (!midichannelmappingbox_vtable_initialized) {
		midichannelmappingbox_vtable = *(self->component.vtable);
		midichannelmappingbox_vtable.ondraw =
			(psy_ui_fp_component_ondraw)
			midichannelmappingbox_on_draw;		
		midichannelmappingbox_vtable.onalign =
			(psy_ui_fp_component)
			midichannelmappingbox_on_align;			
		midichannelmappingbox_vtable_initialized = TRUE;
	}
	self->component.vtable = &midichannelmappingbox_vtable;
}

void midichannelmappingbox_init(MidiChannelMappingBox* self,
	psy_ui_Component* parent, Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	vtable_init(self);	
	self->workspace = workspace;
	self->colx_px[0] = 0.0;
	self->colx_px[1] = 100.0;
	self->colx_px[2] = 400.0;
	self->colx_px[3] = 600.0;	
	psy_ui_component_set_preferred_size(&self->component,
		psy_ui_size_make_em(60.0, 20.0));
	psy_ui_component_set_scroll_step_height(&self->component,
		psy_ui_value_make_eh(1.2));
	psy_ui_component_set_wheel_scroll(&self->component, 1);
	psy_ui_component_set_overflow(&self->component,
		psy_ui_OVERFLOW_VSCROLL);
}

void midichannelmappingbox_on_draw(MidiChannelMappingBox* self,
	psy_ui_Graphics* g)
{
	psy_audio_MidiInput* midiinput;
	intptr_t ch;
	double cpy;	
	const psy_ui_TextMetric* tm;	
	double line_height;

	assert(self);

	tm = psy_ui_component_textmetric(&self->component);	
	line_height = (int)(tm->tmHeight * 1.2);	
	midiinput = &workspace_player(self->workspace)->midiinput;
	for (ch = 0, cpy = 0; ch < psy_audio_MAX_MIDI_CHANNELS; ++ch,
			cpy += line_height) {
		char text[256];
		uintptr_t selidx;
		int inst;
		psy_audio_Machine* machine;
				
		machine = NULL;	
		if (psy_audio_midiinput_genmap(midiinput, ch) != psy_INDEX_INVALID) {
			psy_ui_graphics_set_text_colour(g, psy_ui_component_colour(
				&self->component));
		} else {
			psy_ui_graphics_set_text_colour(g, psy_ui_colour_make(0x00444444));
		}
		psy_snprintf(text, 256, "Ch %d", (ch + 1));
		psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[0], cpy), text,
			psy_strlen(text));
		/* Generator/effect selector */
		selidx = psy_INDEX_INVALID;
		switch (midiinput->midiconfig.gen_select_with) {
			case psy_audio_MIDICONFIG_MS_USE_SELECTED:
				if (workspace_song(self->workspace)) {
					selidx = psy_audio_machines_selected(
						&workspace_song(self->workspace)->machines_);
				}
				break;
			case psy_audio_MIDICONFIG_MS_BANK:
			case psy_audio_MIDICONFIG_MS_PROGRAM:
				selidx = psy_audio_midiinput_genmap(midiinput, ch);
				break;
			case psy_audio_MIDICONFIG_MS_MIDI_CHAN:
				selidx = ch;
				break;
			default:
				selidx = psy_INDEX_INVALID;
				break;
		}
		if (workspace_song(self->workspace) && selidx != psy_INDEX_INVALID) {
			machine = psy_audio_machines_at(
				&workspace_song(self->workspace)->machines_, selidx);
			if (machine) {
				psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[1], cpy),
					psy_audio_machine_edit_name(machine),
					psy_strlen(psy_audio_machine_edit_name(machine)));
			} else {
				psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[1], cpy),
					"-", psy_strlen("-"));
			}
		} else {
			psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[1], cpy), "-",
				psy_strlen("-"));
		}
		/* instrument selection */
		inst = -1;
		if (inst == -1) {
			switch (midiinput->midiconfig.inst_select_with)
			{
			case psy_audio_MIDICONFIG_MS_USE_SELECTED:
				if (workspace_song(self->workspace)) {
					psy_audio_InstrumentIndex instidx;

					instidx = psy_audio_instruments_selected(
						&workspace_song(self->workspace)->instruments_);
					selidx = instidx.subslot;
				}
				break;
			case psy_audio_MIDICONFIG_MS_BANK:
			case psy_audio_MIDICONFIG_MS_PROGRAM:
				selidx = psy_audio_midiinput_instmap(midiinput, ch);
				break;
			case psy_audio_MIDICONFIG_MS_MIDI_CHAN:
				selidx = ch;
				break;
			}
		} else {
			selidx = inst;
		}
		if (machine && machine_supports(machine, psy_audio_SUPPORTS_INSTRUMENTS)
				&& selidx >= 0 && selidx < MAX_INSTRUMENTS) {
			/* pMachine->NumAuxColumnIndexes()) */
			psy_snprintf(text, 256, "%02X", selidx);
		} else { psy_snprintf(text, 256, "-"); }
		psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[2], cpy), text,
			psy_strlen(text));
		psy_ui_graphics_textout(g, psy_ui_realpoint_make(self->colx_px[3], cpy), "Yes",
			psy_strlen("Yes"));
	}
}

void midichannelmappingbox_on_align(MidiChannelMappingBox* self)
{
	const psy_ui_TextMetric* tm;	
	
	assert(self);

	tm = psy_ui_component_textmetric(&self->component);	
	self->colx_px[0] = 0;
	self->colx_px[1] = floor(tm->tmAveCharWidth * 10.0);	
	self->colx_px[2] = self->colx_px[1] + floor(tm->tmAveCharWidth * 17.0);
	self->colx_px[3] = self->colx_px[2] + floor(tm->tmAveCharWidth * 13.0);
}

/* MidiChannelMappingView */

/* prototypes */
static void midichannelmappingview_init_header(MidiChannelMappingView*);

/* implementation */
void midichannelmappingview_init(MidiChannelMappingView* self,
	psy_ui_Component* parent, Workspace* workspace)
{
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);	
	midichannelmappingview_init_header(self);
	midichannelmappingbox_init(&self->channelmapping, &self->component, workspace);	
	psy_ui_scroller_init(&self->scroller, &self->component, NULL, NULL);
	psy_ui_scroller_set_client(&self->scroller, &self->channelmapping.component);
	psy_ui_component_set_align(psy_ui_scroller_base(&self->scroller),
		psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_align(&self->channelmapping.component,
		psy_ui_ALIGN_HCLIENT);
}

void midichannelmappingview_init_header(MidiChannelMappingView* self)
{
	assert(self);

	psy_ui_component_init_align(&self->header, &self->component, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_default_align(&self->header,
		psy_ui_ALIGN_LEFT, psy_ui_margin_zero());
	psy_ui_component_set_align_expand(&self->header, psy_ui_HEXPAND);
	psy_ui_label_init_text(&self->channel, &self->header, "Channel");
	psy_ui_label_set_char_number(&self->channel, 10.0);
	psy_ui_label_init_text(&self->mac, &self->header, "Generator/Effect");
	psy_ui_label_set_char_number(&self->mac, 17.0);
	psy_ui_label_init_text(&self->inst, &self->header, "Instrument");
	psy_ui_label_set_char_number(&self->inst, 13.0);
	psy_ui_label_init_text(&self->note_off, &self->header, "Note Off");	
	psy_ui_label_set_char_number(&self->note_off, 10.0);
}


/* MidiMonitor */

/* prototypes */
static void midimonitor_init_core_status(MidiMonitor*);
static void midimonitor_init_title_bar(MidiMonitor*);
static void midimonitor_init_core_status(MidiMonitor*);
static void midimonitor_init_core_status_left(MidiMonitor*);
static void midimonitor_init_core_status_right(MidiMonitor*);
static void midimonitor_initflags(MidiMonitor*);
static void midimonitor_init_channel_mapping(MidiMonitor*);
static void midimonitor_on_song_changed(MidiMonitor*,
	psy_audio_Player* sender);
static void midimonitor_on_machine_slot_change(MidiMonitor* self,
	psy_audio_Machines* sender, uintptr_t slot);
static void midimonitor_on_timer(MidiMonitor*, uintptr_t timerid);
static void midimonitor_update_channel_map(MidiMonitor*);
static void midimonitor_on_configure(MidiMonitor*);
static void midimonitor_on_map_configure(MidiMonitor*);

/* vtable */
static psy_ui_ComponentVtable midimonitor_vtable;
static bool midimonitor_vtable_initialized = FALSE;

static psy_ui_ComponentVtable* midimonitor_vtable_init(MidiMonitor* self)
{
	if (!midimonitor_vtable_initialized) {
		midimonitor_vtable = *(self->component.vtable);		
		midimonitor_vtable.on_timer = (psy_ui_fp_component_on_timer)
			midimonitor_on_timer;	
		midimonitor_vtable_initialized = TRUE;
	}
	return &midimonitor_vtable;
}

/* implementation */
void midimonitor_init(MidiMonitor* self, psy_ui_Component* parent,
	Workspace* workspace)
{	
	assert(self);

	psy_ui_component_init(&self->component, parent, NULL);
	midimonitor_vtable_init(self);	
	self->workspace = workspace;
	self->channelstatcounter = 0;
	psy_ui_component_set_vtable(midimonitor_base(self),
		midimonitor_vtable_init(self));
	midimonitor_init_title_bar(self);
	psy_ui_component_init(&self->client, midimonitor_base(self), NULL);	
	psy_ui_component_set_style_type(&self->client, STYLE_SIDE_VIEW);
	psy_ui_component_set_align(&self->client, psy_ui_ALIGN_CLIENT);
	psy_ui_component_set_padding(&self->client, 
		psy_ui_margin_make_em(0.0, 1.0, 1.0, 1.0));		
	psy_ui_component_init(&self->top, &self->client, NULL);
	psy_ui_margin_init(&self->topmargin);
	psy_ui_component_set_align(&self->top, psy_ui_ALIGN_TOP);	
	//midimonitor_init_core_status(self);
	//midimonitor_init_core_status_left(self);
	//midimonitor_init_core_status_right(self);
	midimonitor_initflags(self);
	midimonitor_init_channel_mapping(self);
	psy_ui_component_start_timer(&self->component, 0, 50);
}

void midimonitor_init_title_bar(MidiMonitor* self)
{
	assert(self);

	titlebar_init(&self->titlebar, &self->component, "Psycle MIDI Monitor");
	closebar_set_property(&self->titlebar.close_bar_,
		psy_configuration_at(
			psycleconfig_general(workspace_cfg(self->workspace)),
			"bench.showmidi"));	
	psy_ui_button_init_text(&self->configure, &self->titlebar.client_,
		"Devices");
	psy_ui_button_set_svg(&self->configure, psy_ui_app_svg(psy_ui_app(), "img.settings"));
	psy_ui_component_set_align(&self->configure.component, psy_ui_ALIGN_LEFT);
	psy_signal_connect(&self->configure.signal_clicked, self, midimonitor_on_configure);			
}

void midimonitor_init_core_status(MidiMonitor* self)
{
	assert(self);

	psy_ui_label_init_text(&self->coretitle, &self->top, "Core Status");
	psy_ui_component_set_minimum_size(&self->coretitle.component,
		psy_ui_size_make_em(0.0, 2.0));
	psy_ui_component_set_align(&self->coretitle.component,
		psy_ui_ALIGN_TOP);
}

void midimonitor_init_core_status_left(MidiMonitor* self)
{	
	psy_ui_component_init(&self->resources, &self->top, NULL);
	psy_ui_component_set_align(&self->resources, psy_ui_ALIGN_LEFT);
	psy_ui_label_init_text(&self->resourcestitle, &self->resources,
		"Core Status");
	labelpair_init(&self->resources_win, &self->resources, "Buffer Used (events)", 25.0);
	labelpair_init(&self->resources_mem, &self->resources, "Buffer capacity (events)", 25.0);
	labelpair_init(&self->resources_swap, &self->resources, "Events lost", 25.0);
	labelpair_init(&self->resources_vmem, &self->resources, "MIDI headroom (ms)", 25.0);
	psy_list_free(psy_ui_components_setalign(
		psy_ui_component_children(&self->resources, psy_ui_NONE_RECURSIVE, FALSE),
		psy_ui_ALIGN_TOP,
		self->topmargin));
}

void midimonitor_init_core_status_right(MidiMonitor* self)
{	
	assert(self);

	psy_ui_component_init(&self->performance, &self->top, NULL);
	psy_ui_component_set_align(&self->performance, psy_ui_ALIGN_LEFT);
	psy_ui_button_init(&self->cpucheck, &self->performance);
	psy_ui_button_set_svg(&self->cpucheck, psy_ui_app_svg(psy_ui_app(), "img.check-off"));
	psy_ui_button_set_svg_selected(&self->cpucheck, psy_ui_app_svg(psy_ui_app(), "img.check-on"));
	labelpair_init(&self->audiothreads, &self->performance, "Internal MIDI Version", 25.0);
	labelpair_init(&self->totaltime, &self->performance, "MIDI clock deviation (ms)", 25.0);
	labelpair_init(&self->machines, &self->performance, "Audio latency (sampl.)", 25.0);
	labelpair_init(&self->routing, &self->performance, "Sync Offset (ms)", 25.0);
	psy_list_free(psy_ui_components_setalign(
		psy_ui_component_children(&self->performance, psy_ui_NONE_RECURSIVE, FALSE),
		psy_ui_ALIGN_TOP,
		self->topmargin));
}

void midimonitor_initflags(MidiMonitor* self)
{	
	self->lastchannelmap = 0;
	self->channelstatcounter = 0;
	psy_ui_label_init_text(&self->flagtitle, &self->client, "Flags");
	psy_ui_component_set_minimum_size(&self->flagtitle.component,
		psy_ui_size_make_em(0.0, 2.0));	
	psy_ui_component_set_align(&self->flagtitle.component,
		psy_ui_ALIGN_TOP);
	midiflagsview_init(&self->flags, &self->client, self->workspace);
	psy_ui_component_set_align(&self->flags.component, psy_ui_ALIGN_TOP);	
}

void midimonitor_init_channel_mapping(MidiMonitor* self)
{	
	assert(self);

	psy_ui_component_init_align(&self->topchannelmapping, &self->client, NULL,
		psy_ui_ALIGN_TOP);
	psy_ui_component_set_margin(&self->topchannelmapping,
		psy_ui_margin_make_em(0.0, 0.0, 1.0, 0.0));	
	psy_ui_label_init_text(&self->channelmappingtitle,
		&self->topchannelmapping, "Channel Mapping");
	psy_ui_component_set_align(&self->channelmappingtitle.component,
		psy_ui_ALIGN_LEFT);
	psy_ui_button_init_resource_connect(&self->mapconfigure, &self->topchannelmapping,
		"img.settings", self, midimonitor_on_map_configure);
	psy_ui_component_set_align(psy_ui_button_base(&self->mapconfigure),
		psy_ui_ALIGN_LEFT);
	midichannelmappingview_init(&self->channelmapping, &self->client,
		self->workspace);
	psy_ui_component_set_align(&self->channelmapping.component, psy_ui_ALIGN_CLIENT);
	psy_signal_connect(&self->workspace->player_.signal_song_changed, self,
		midimonitor_on_song_changed);	
	if (workspace_song(self->workspace)) {
		psy_signal_connect(&workspace_song(self->workspace)->machines_.signal_slotchange,
			self, midimonitor_on_machine_slot_change);
	}
}

void midimonitor_on_song_changed(MidiMonitor* self, psy_audio_Player* sender)
{
	self->channelmapupdate =
		workspace_player(self->workspace)->midiinput.stats.channelmapupdate - 1;
	if (sender->song) {
		psy_signal_connect(&sender->song->machines_.signal_slotchange, self,
			midimonitor_on_machine_slot_change);
	}
}

void midimonitor_on_timer(MidiMonitor* self, uintptr_t timerid)
{
	if (psy_ui_component_visible(&self->component)) {
		if (self->channelstatcounter > 0) {
			--self->channelstatcounter;
			if (self->channelstatcounter == 0) {
				workspace_player(self->workspace)->midiinput.stats.channelmap = 0;
			}
		}
		if (self->flagstatcounter > 0) {
			--self->flagstatcounter;
			if (self->flagstatcounter == 0) {
				workspace_player(self->workspace)->midiinput.stats.flags = 0;
			}
		}
		if (self->channelmapupdate !=
				workspace_player(self->workspace)->midiinput.stats.channelmapupdate) {
			
			psy_ui_component_invalidate(&self->channelmapping.component);			
			self->channelmapupdate =
				workspace_player(self->workspace)->midiinput.stats.channelmapupdate;
		}
		if (self->lastchannelmap != workspace_player(self->workspace)->midiinput.stats.channelmap) {
			self->channelstatcounter = 5;			
			self->lastchannelmap = workspace_player(self->workspace)->midiinput.stats.channelmap;
			midichannelbox_update(&self->flags.channelmap);
		}
		if (self->lastflags != workspace_player(self->workspace)->midiinput.stats.flags) {
			self->flagstatcounter = 5;
			self->lastflags = workspace_player(self->workspace)->midiinput.stats.flags;
			midiclockbox_update(&self->flags.clock);			
		}
	}
}

void midimonitor_on_machine_slot_change(MidiMonitor* self,
	psy_audio_Machines* sender, uintptr_t slot)
{	
	midimonitor_update_channel_map(self);	
}

void midimonitor_update_channel_map(MidiMonitor* self)
{
	self->channelmapupdate =
		workspace_player(self->workspace)->midiinput.stats.channelmapupdate - 1;
}

void midimonitor_on_configure(MidiMonitor* self)
{	
	workspace_select_view(self->workspace,
		viewindex_make_all(VIEW_ID_SETTINGS, 5, 0, psy_INDEX_INVALID));
}

void midimonitor_on_map_configure(MidiMonitor* self)
{
	workspace_select_view(self->workspace, viewindex_make_all(
		VIEW_ID_SETTINGS, 6, 0, psy_INDEX_INVALID));
}
