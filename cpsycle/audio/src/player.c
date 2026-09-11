/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#include "../../detail/prefix.h"


#include "player.h"
/* local */
#include "cmdsnotes.h"
#include "constants.h"
#include "exclusivelock.h"
#include "kbddriver.h"
#include "master.h"
#include "plugin_interface.h"
#include "silentdriver.h"
/* dsp */
#include <operations.h>
#include <rms.h>
/* file */
#include <fileio.h>
/* thread */
#include <thread.h>
/* std */
#include "math.h"
#include <time.h>
/* platform */
#include "../../detail/portable.h"
#include "../../detail/trace.h"

void psy_audio_init(void)
{	
	psy_audio_exclusivelock_init();	
	psy_dsp_init();
}

void psy_audio_dispose(void)
{
	psy_audio_exclusivelock_dispose();
}

static float bufferdriver[MAX_SAMPLES_WORKFN];
static void* mainframe;

static uint16_t midi_combinebytes(unsigned char data1, unsigned char data2)
{
	uint16_t rv_14bit;
	rv_14bit = (uint16_t)data2;
	rv_14bit <<= 7;
	rv_14bit |= (uint16_t)data1;
	return rv_14bit;
}

/* psy_audio_PatternDefaults */

void psy_audio_patterndefaults_init(psy_audio_PatternDefaults* self)
{
	self->pattern = psy_audio_pattern_alloc_init();
	psy_audio_pattern_set_length(self->pattern, psy_dsp_beatpos_make_real(
		0.25, psy_dsp_DEFAULT_PPQ));
	psy_audio_patterns_init(&self->patterns);
	psy_audio_patterns_insert(&self->patterns, 0, self->pattern);
	psy_audio_sequence_init(&self->sequence, &self->patterns, NULL);
	psy_audio_sequence_append_track(&self->sequence,
		psy_audio_sequencetrack_alloc_init());
	psy_audio_sequence_insert(&self->sequence,
		psy_audio_orderindex_make(0, 0), 0);
}

void psy_audio_patterndefaults_dispose(psy_audio_PatternDefaults* self)
{
	psy_audio_sequence_dispose(&self->sequence);
	psy_audio_patterns_dispose(&self->patterns);
}

psy_audio_PatternEvent psy_audio_patterndefaults_event(const
	psy_audio_PatternDefaults* self, uintptr_t track)
{	
	return psy_audio_sequence_pattern_event_at_cursor(&self->sequence,
		psy_audio_sequencecursor_make(psy_audio_orderindex_make(0, 0),
		track, psy_dsp_beatpos_zero()));
}

psy_audio_PatternEvent psy_audio_patterndefaults_fill_event(const
	psy_audio_PatternDefaults* self, uintptr_t track,
	psy_audio_PatternEvent src)
{
	psy_audio_PatternEvent rv;
	psy_audio_PatternEvent defaultevent;

	rv = src;
	defaultevent = psy_audio_sequence_pattern_event_at_cursor(&self->sequence,
		psy_audio_sequencecursor_make(psy_audio_orderindex_make(0, 0),
		track, psy_dsp_beatpos_zero()));
	if (defaultevent.note != psy_audio_NOTECOMMANDS_EMPTY) {
		rv.note = defaultevent.note;
	}
	if (defaultevent.inst != psy_audio_NOTECOMMANDS_INST_EMPTY) {
		rv.inst = defaultevent.inst;
	}
	if (defaultevent.mach != psy_audio_NOTECOMMANDS_EMPTY) {
		rv.mach = defaultevent.mach;
	}
	if (defaultevent.vol != psy_audio_NOTECOMMANDS_VOL_EMPTY) {
		rv.vol = defaultevent.vol;
	}
	if (defaultevent.cmd != 0) {
		rv.cmd = defaultevent.cmd;
	}
	if (defaultevent.parameter != 0) {
		rv.parameter = defaultevent.parameter;
	}	
	return rv;
}


/* psy_audio_Player */

/* prototpyes */
static void psy_audio_player_set_config(psy_audio_Player*,
	psy_Configuration* config, 
	psy_Configuration* event_input, psy_Configuration* misc,
	psy_Configuration* metronome, psy_Configuration* seqeditor,
	psy_Configuration* compat, psy_Configuration* global,
	psy_Configuration* midi);
static void psy_audio_player_initkbddriver(psy_audio_Player*);
static void psy_audio_player_initsignals(psy_audio_Player*);
static void psy_audio_player_unloadeventdrivers(psy_audio_Player*);
static float* psy_audio_player_work(psy_audio_Player*, int* numsamples,
	int* stop);
static void psy_audio_player_work_amount(psy_audio_Player*, uintptr_t amount,
	uintptr_t* numsamplex, float** psamples);
static void psy_audio_player_on_event_driver_input(psy_audio_Player*,
	psy_EventDriver* sender);
static void psy_audio_player_work_path(psy_audio_Player*, uintptr_t amount);
static void psy_audio_player_fill_driver(psy_audio_Player*,
	float* buffer, uintptr_t amount);
static void psy_audio_player_dither_buffer(psy_audio_Player*,
	psy_audio_Buffer* buffer, uintptr_t amount);
static void psy_audio_player_resetvumeters(psy_audio_Player*);
static void psy_audio_player_do_stop(psy_audio_Player*);
static void psy_audio_player_record_notes(psy_audio_Player*,
	uintptr_t track, const psy_audio_PatternEvent*);
#if defined DIVERSALIS__OS__MICROSOFT	
static unsigned int __stdcall psy_audio_player_thread_function(
	psy_audio_Player*);
#else
static unsigned int psy_audio_player_thread_function(psy_audio_Player*);
#endif
static void psy_audio_player_process_loop(psy_audio_Player*);
static void psy_audio_player_on_bpm_tweak(psy_audio_Player*,
	psy_audio_MachineParam* sender, double value);
static void psy_audio_player_on_bpm_norm_value(psy_audio_Player*,
	psy_audio_MachineParam* sender, double* rv);
static void psy_audio_player_on_bpm_describe(psy_audio_Player*,
	psy_audio_MachineParam* sender, int* active, char* rv);
static void psy_audio_player_on_lpb_tweak(psy_audio_Player*,
	psy_audio_MachineParam* sender, double value);
static void psy_audio_player_on_lpb_norm_value(psy_audio_Player*,
	psy_audio_MachineParam* sender, double* rv);
static void psy_audio_player_on_lpb_describe(psy_audio_Player*,
	psy_audio_MachineParam* sender, int* active, char* rv);
static void psy_audio_player_on_set_num_threads(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_multi_key(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_record_noteoff(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_notes_to_effect(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_metronome_note_changed(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_metronome_machine_changed(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_on_sampler_machine_changed(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_on_blitz_compatibility(psy_audio_Player*,
	psy_Property* sender);
static void psy_audio_player_on_enable_audio(psy_audio_Player*,
	psy_Property* sender);

/* implementation */
void psy_audio_player_init(psy_audio_Player* self,
	psy_audio_MachineCallback* callback, void* handle,
	psy_Configuration* config, psy_Configuration* event_input, 
	psy_Configuration* misc, psy_Configuration* metronome,
	psy_Configuration* seqeditor, psy_Configuration* compat,
	psy_Configuration* global, psy_Configuration* midi,
	psy_Property* cmddef, psy_Logger* logger)
{
	assert(self);

	psy_audio_plugincatcher_init(&self->plugincatcher, logger);
	psy_audio_machinefactory_init(&self->machinefactory, callback,
		&self->plugincatcher, logger);
	if (callback) {
		psy_audio_machinecallback_set_player(callback, self);		
	}
	psy_audio_custommachine_init(&self->custommachine, callback);	
	psy_audio_song_init(&self->empty_song_, &self->machinefactory);	
	self->song = &self->empty_song_;	
	self->editmode = TRUE;
	self->follow_song = FALSE;
	self->recordnoteoff = FALSE;
	self->multichannelaudition = FALSE;	
	self->octave = 4;
	self->resyncplayposinsamples = 0;
	self->resyncplayposinbeats = 0.0;
	self->measure_cpu_usage = FALSE;
	self->thread_count = 1;
	self->threads_ = NULL;
	self->waiting = 0;
	self->stop_requested_ = FALSE;
	self->nodes_queue_ = NULL;
	// self->active_note = psy_audio_NOTECOMMANDS_EMPTY;
	self->vumode = VUMETER_RMS;	
	psy_lock_init(&self->mutex);
	psy_lock_init(&self->block);
	psy_dsp_dither_init(&self->dither_);	
	psy_audio_sequencer_init(&self->sequencer, psy_audio_song_sequence(self->song),
		psy_audio_song_machines(self->song));
	mainframe = handle;
	psy_audio_midiinput_init(&self->midiinput, self->song, midi);
	psy_audio_activechannels_init(&self->playon);	
	psy_audio_player_initsignals(self);	
	psy_table_init(&self->notes_to_tracks_);
	psy_table_init(&self->tracks_to_notes_);	
	psy_audio_patterndefaults_init(&self->patterndefaults);	
	psy_audio_audiodrivers_init(&self->audiodrivers, handle, &self->sequencer,
		config, self, (AUDIODRIVERWORKFN)psy_audio_player_work, logger);
	psy_audio_eventdrivers_init(&self->eventdrivers, handle, event_input, cmddef);
	psy_signal_connect(&self->eventdrivers.signal_input, self,
		psy_audio_player_on_event_driver_input);
	/* wave prev */
	psy_audio_instprev_init(&self->inst_prev);	
	/* parameters */
	psy_audio_custommachineparam_init(&self->tempo_param_,
		"Tempo (Bpm)", "BPM", MPF_STATE | MPF_SMALL, 0, 999);	
	psy_signal_connect(&self->tempo_param_.machineparam.signal_tweak, self,
		psy_audio_player_on_bpm_tweak);
	psy_signal_connect(&self->tempo_param_.machineparam.signal_normvalue, self,
		psy_audio_player_on_bpm_norm_value);
	psy_signal_connect(&self->tempo_param_.machineparam.signal_describe, self,
		psy_audio_player_on_bpm_describe);
	psy_audio_custommachineparam_init(&self->lpb_param_,
		"Lines per Beat", "LPB", MPF_STATE | MPF_SMALL, 1, 99);	
	psy_signal_connect(&self->lpb_param_.machineparam.signal_tweak, self,
		psy_audio_player_on_lpb_tweak);
	psy_signal_connect(&self->lpb_param_.machineparam.signal_normvalue, self,
		psy_audio_player_on_lpb_norm_value);
	psy_signal_connect(&self->lpb_param_.machineparam.signal_describe, self,
		psy_audio_player_on_lpb_describe);
	psy_audio_player_set_config(self, config, event_input, misc,
		metronome, seqeditor, compat, global, midi);	
}

void psy_audio_player_initsignals(psy_audio_Player* self)
{
	assert(self);

	psy_signal_init(&self->signal_song_changed);
	psy_signal_init(&self->signal_lpbchanged);
	psy_signal_init(&self->signal_inputevent);
	psy_signal_init(&self->signal_octavechanged);
}

void psy_audio_player_dispose(psy_audio_Player* self)
{
	assert(self);
	
	psy_audio_instprev_dispose(&self->inst_prev);	
	psy_audio_audiodrivers_dispose(&self->audiodrivers);	
	psy_audio_eventdrivers_dispose(&self->eventdrivers);
	self->stop_requested_ = TRUE;
	psy_list_deallocate(&self->threads_, (psy_fp_disposefunc)
		psy_thread_dispose);
	psy_list_deallocate(&self->nodes_queue_, NULL);
	psy_lock_dispose(&self->mutex);
	psy_lock_dispose(&self->block);
	psy_signal_dispose(&self->signal_song_changed);
	psy_signal_dispose(&self->signal_lpbchanged);
	psy_signal_dispose(&self->signal_inputevent);
	psy_signal_dispose(&self->signal_octavechanged);
	psy_audio_activechannels_dispose(&self->playon);
	psy_audio_sequencer_dispose(&self->sequencer);
	psy_table_dispose(&self->notes_to_tracks_);
	psy_table_dispose(&self->tracks_to_notes_);	
	psy_audio_patterndefaults_dispose(&self->patterndefaults);
	psy_dsp_dither_dispose(&self->dither_);
	psy_audio_midiinput_dispose(&self->midiinput);
	psy_audio_song_dispose(&self->empty_song_);
	psy_audio_machinefactory_dispose(&self->machinefactory);
	psy_audio_plugincatcher_save(&self->plugincatcher);	
	psy_audio_plugincatcher_dispose(&self->plugincatcher);
	psy_audio_custommachineparam_dispose(&self->tempo_param_);
	psy_audio_custommachineparam_dispose(&self->lpb_param_);
	psy_audio_custommachine_dispose(&self->custommachine);
	psy_audio_dispose_seqiterator();
}

/*
** audio driver work callback
**
** - splits work to psy_audio_MAX_STREAM_SIZE parts or to let work begin on a
**   line tick
** - player_workamount processes each spltted part
** - updates the sequencer line tick count
*/
float* psy_audio_player_work(psy_audio_Player* self, int* numsamples,
	int* hostisplaying)
{	
	uintptr_t maxamount;
	uintptr_t amount;
	uintptr_t numsamplex;
	float* samples;
	
	assert(self);

	samples = bufferdriver;
	numsamplex = *numsamples;
	maxamount = (numsamplex > psy_audio_MAX_STREAM_SIZE)
		? psy_audio_MAX_STREAM_SIZE
		: numsamplex;
	psy_audio_exclusivelock_enter();
	self->resyncplayposinsamples = psy_audiodriver_playpos_in_samples(
		psy_audio_audiodriverplugin_base(&self->audiodrivers.driver_plugin));
	self->resyncplayposinbeats = psy_audio_player_position(self);
	do {		
		amount = maxamount;
		if (amount > numsamplex) {
			amount = numsamplex;
		}							
		if (self->sequencer.linetickcount <= psy_audio_sequencer_frame_to_offset(
				&self->sequencer, amount)) {
			if (self->sequencer.linetickcount > 0.0) {
				uintptr_t pre;

				pre = psy_audio_sequencer_frames(&self->sequencer,
					self->sequencer.linetickcount);
				if (pre) {
					pre--;
					if (pre) {
						psy_audio_player_work_amount(self, pre, &numsamplex,
							&samples);
						amount -= pre;
						self->sequencer.linetickcount -= 							
								psy_audio_sequencer_frame_to_offset(
									&self->sequencer, pre);
					}
				}
			}		
			psy_audio_sequencer_notify_newline(&self->sequencer);	
			psy_audio_sequencer_on_newline(&self->sequencer);
		}		
		if (amount > 0) {			
			psy_audio_player_work_amount(self, amount, &numsamplex, &samples);
			self->sequencer.linetickcount -=
				psy_audio_sequencer_frame_to_offset(&self->sequencer, amount);			
		}		
	} while (numsamplex > 0);
	psy_audio_exclusivelock_leave();
	*hostisplaying = psy_audio_sequencer_playing(&self->sequencer);
	return bufferdriver;
}

void psy_audio_player_work_amount(psy_audio_Player* self, uintptr_t amount,
	uintptr_t* numsamplex, float** psamples)
{		
	assert(self);

	if (self->song) {		
		uintptr_t numsamples;		

		numsamples = amount;
		while (numsamples > 0) {
			uintptr_t worked;

			worked = psy_audio_sequencer_frame_tick(&self->sequencer,
				numsamples);			
			if (worked > 0) {
				if (worked > numsamples) {
					worked = numsamples;
				}
				psy_audio_player_work_path(self, worked);
				numsamples -= worked;
			}
		}			
	}
	psy_audio_player_fill_driver(self, *psamples, amount);
	*numsamplex -= amount;
	*psamples += (amount * 2);
}

void psy_audio_player_work_path(psy_audio_Player* self, uintptr_t amount)
{
	MachinePath* path;
	uintptr_t waiting;

	assert(self);
	
	path = psy_audio_machines_path(psy_audio_song_machines(self->song));
	if (path) {		
		for ( ; path != 0; path = path->next) {
			size_t slot;			
			psy_audio_MachineWork* work;
			
			slot = (size_t)path->entry;
			if (self->thread_count < 2) {
				if (slot != psy_INDEX_INVALID) {
					if (!psy_audio_machines_is_mixer_send(psy_audio_song_machines(self->song),
							slot)) {
						psy_audio_player_work_machine(self, amount, slot);
					}
				}
			} else if (slot != psy_INDEX_INVALID) {
				psy_lock_enter(&self->mutex);
				work = (psy_audio_MachineWork*)malloc(sizeof(
					psy_audio_MachineWork));
				work->amount = amount;
				work->slot = slot;
				psy_list_append(&self->nodes_queue_, (void*)work);
				psy_lock_leave(&self->mutex);
			} else {
				bool waiting;

				do {
					psy_lock_enter(&self->mutex);
					waiting = psy_list_size(self->nodes_queue_) > 0;
					psy_lock_leave(&self->mutex);
				} while (waiting);				
			}
		}
	}	
	do {
		psy_lock_enter(&self->mutex);
		waiting = psy_list_size(self->nodes_queue_) > 0;
		psy_lock_leave(&self->mutex);
	} while (waiting);	
	do {
		psy_lock_enter(&self->block);
		waiting = self->waiting;
		psy_lock_leave(&self->block);
	} while (waiting > 0);	
}

void psy_audio_player_work_machine(psy_audio_Player* self, uintptr_t amount,
	uintptr_t slot)
{
	psy_audio_Machine* machine;

	assert(self);

	machine = psy_audio_machines_at(psy_audio_song_machines(self->song), slot);
	if (machine) {
		psy_audio_Buffer* output;

		output = psy_audio_machine_mix(machine, slot, amount,
			psy_audio_connections_at(&psy_audio_song_machines(self->song)->connections, slot),
			psy_audio_song_machines(self->song), self);
		if (output) {
			psy_audio_BufferContext bc;
			psy_List* events;
			psy_List* p;
			
			events = psy_audio_sequencer_timed_events(&self->sequencer,
				slot, amount);		
			if (events || self->sequencer.metronome.precounting) {
				/* update playon */
				for (p = events; p != NULL; psy_list_next(&p)) {
					psy_audio_PatternEntry* patternentry;
					
					patternentry = (psy_audio_PatternEntry*)psy_list_entry(p);					
					psy_audio_activechannels_write(&self->playon,
						psy_audio_patternentry_track(patternentry),
						psy_audio_patternentry_front(patternentry));
				}
			}
			psy_audio_buffercontext_init(&bc, events, output, output, amount,
				(self->song && !self->sequencer.metronome.active)
				? psy_audio_song_num_song_tracks(self->song)
				: MAX_TRACKS);			
			psy_audio_buffer_scale(output, psy_audio_machine_amp_range(machine),
				amount);
			if (self->measure_cpu_usage) {
				psy_audio_cputimeclock_measure(&machine->cpu_time);
			}
			psy_audio_machine_work(machine, &bc);
			if (psy_audio_buffercontext_out_events_begin(&bc)) {
				for (p = psy_audio_buffercontext_out_events_begin(&bc); p != NULL; psy_list_next(&p)) {
					psy_EventDriverMidiData* midiev;
					psy_audio_PatternEvent ev;

					midiev = (psy_EventDriverMidiData*)psy_list_entry(p);
					psy_audio_patternevent_clear(&ev);
					if (psy_audio_midiinput_work_input(&self->midiinput,
							*midiev, psy_audio_song_machines(self->song), &ev)) {
						psy_audio_sequencer_add_input_event(&self->sequencer,
							&ev, 0);
					}					
				}
				psy_list_deallocate(&bc.out_events_, NULL);
			}
			psy_audio_buffer_pan(output, psy_audio_machine_panning(machine),
				amount);			
			psy_audio_machine_update_memory(machine, &bc);
			psy_signal_emit(&machine->signal_worked, machine, 2, slot, &bc);
			if (self->measure_cpu_usage) {
				psy_audio_cputimeclock_stop(&machine->cpu_time);
				psy_audio_cputimeclock_update(&machine->cpu_time,
					amount, psy_audio_sequencer_sample_rate(&self->sequencer));
			}									
			psy_list_free(events);			
		}
	}
}

void psy_audio_player_fill_driver(psy_audio_Player* self, float* buffer,
	uintptr_t amount)
{
	psy_audio_Buffer* masteroutput;

	assert(self);

	if (psy_audio_instprev_playing(&self->inst_prev)) {
		psy_audio_instprev_work(&self->inst_prev, psy_audio_machines_outputs(
			psy_audio_song_machines(self->song), psy_audio_MASTER_INDEX), amount);
	}
	masteroutput = psy_audio_machines_outputs(psy_audio_song_machines(self->song),
		psy_audio_MASTER_INDEX);
	if (masteroutput) {		
		psy_audio_buffer_scale(masteroutput, PSY_DSP_AMP_RANGE_NATIVE, amount);
		if (self->dither_.settings.enabled) {
			psy_audio_player_dither_buffer(self, masteroutput, amount);			
		}		
		dsp.interleave(buffer, masteroutput->samples[0],
			masteroutput->samples[1], amount);
	}
}

void psy_audio_player_dither_buffer(psy_audio_Player* self, psy_audio_Buffer*
	buffer, uintptr_t amount)
{
	uintptr_t channel;

	assert(self);

	/* dither needs PSY_DSP_AMP_RANGE_NATIVE */
	for (channel = 0; channel < psy_audio_buffer_num_channels(buffer);
			++channel) {
		psy_dsp_dither_process(&self->dither_,
			psy_audio_buffer_at(buffer, channel), amount);
	}
}

void psy_audio_player_on_event_driver_input(psy_audio_Player* self,
	psy_EventDriver* sender)
{
	psy_EventDriverCmd cmd;	
	uintptr_t track = 0;

	assert(self);
	
	cmd = psy_eventdriver_getcmd(sender, NULL);
	/* only midi handled here, keyboard cmds handled in mainframe_on_notes */
	if (cmd.type == psy_EVENTDRIVER_MIDI) {
		psy_audio_PatternEvent ev;
		uintptr_t track;
						
		psy_audio_patternevent_clear(&ev);
		if (!psy_audio_midiinput_work_input(&self->midiinput, cmd.midi,
			psy_audio_song_machines(self->song), &ev)) {
			return;
		}
		if (ev.note >= psy_audio_NOTECOMMANDS_MIDI_SPP &&
				ev.note <= psy_audio_NOTECOMMANDS_MIDI_SYNC) {
			switch (ev.note) {
				case psy_audio_NOTECOMMANDS_MIDI_SPP: {
					uint16_t midibeats;

					midibeats = midi_combinebytes(ev.cmd, ev.parameter);
					psy_audio_sequencer_set_position(&self->sequencer,
						midibeats * 1/16.0);
					break; }
				case psy_audio_NOTECOMMANDS_MIDI_CLK_START:
					psy_audio_sequencer_clock_start(&self->sequencer);
					break;
				case psy_audio_NOTECOMMANDS_MIDI_CLK:
					psy_audio_sequencer_clock(&self->sequencer);
					break;
				case psy_audio_NOTECOMMANDS_MIDI_CLK_CONT:
					psy_audio_sequencer_clock_continue(&self->sequencer);
					break;
				case psy_audio_NOTECOMMANDS_MIDI_CLK_STOP:
					psy_audio_sequencer_clock_stop(&self->sequencer);
					break;
				default:
					break;
			}
			return;
		}
		track = psy_audio_player_multi_channel_audition(self, &ev, psy_audio_NOTECOMMANDS_EMPTY);
		psy_audio_sequencer_add_input_event(&self->sequencer, &ev, track);
		if (self->editmode && psy_audio_player_playing(self)) {
			psy_audio_player_record_notes(self, 0, &ev);
		}		
	}
}

void psy_audio_player_play_event(psy_audio_Player* self,
	const psy_audio_PatternEvent* ev, uintptr_t track)
{	
	psy_audio_sequencer_add_input_event(&self->sequencer, ev, track);	
}

void psy_audio_player_input_pattern_event(psy_audio_Player* self,
	const psy_audio_PatternEvent* ev)
{
	uintptr_t track;

	track = psy_audio_player_multi_channel_audition(self, ev, psy_audio_NOTECOMMANDS_EMPTY);
	psy_audio_sequencer_add_input_event(&self->sequencer, ev, track);
	if (self->editmode && psy_audio_player_playing(self)) {
		psy_audio_player_record_notes(self, 0, ev);
	} else {
		psy_signal_emit(&self->signal_inputevent, self, 1, ev);
	}
}

psy_audio_PatternEvent psy_audio_player_pattern_event(psy_audio_Player* self,
	uint8_t note)
{	
	psy_audio_PatternEvent rv;
	uintptr_t mac;
	psy_audio_Machine* machine;
	bool useaux;
	uint8_t ev_note;
	uint16_t ev_inst;
	uint8_t ev_mac;
	uint8_t ev_vol;	
	bool is_tweak;
	 
	mac = psy_audio_machines_selected(psy_audio_song_machines(self->song));
	machine = psy_audio_machines_at(psy_audio_song_machines(self->song), mac);
	useaux = machine && (psy_audio_machine_num_aux_columns(machine) > 0);
	is_tweak = (note >= psy_audio_NOTECOMMANDS_TWEAK &&
		note <= psy_audio_NOTECOMMANDS_TWEAK_SLIDE);
	/* note */
	if (note < psy_audio_NOTECOMMANDS_RELEASE) {
		ev_note = (uint8_t)note + (uint8_t)self->octave * 12;
	} else {
		ev_note = (uint8_t)note;
	}
	/* inst */
	if (is_tweak) {
		ev_inst	= (uint16_t)psy_audio_machines_paramselected(
			psy_audio_song_machines(self->song));
	} else {
		if (useaux) {
			ev_inst = (uint16_t)psy_audio_machine_aux_column_selected(machine);
		} else if (machine && machine_supports(machine,
				psy_audio_SUPPORTS_INSTRUMENTS)) {
			ev_inst = (uint16_t)psy_audio_instruments_selected(
				psy_audio_song_instruments(self->song)).subslot;
		} else {
			ev_inst = psy_audio_NOTECOMMANDS_INST_EMPTY;
		}
	}
	if (is_tweak && (ev_inst == psy_audio_NOTECOMMANDS_INST_EMPTY)) {
		ev_inst = 0;
	}
	/* mac */			
	ev_mac = (uint8_t)mac;
	if (is_tweak && ev_mac == 0xFF) {
		ev_mac = 0;		
	}
	/* vol */
	ev_vol = (uint8_t)psy_audio_NOTECOMMANDS_VOL_EMPTY;	
	/* event */
	psy_audio_patternevent_init_all(&rv, ev_note, ev_inst, ev_mac, ev_vol,
		0, 0);
	return rv;
}

uintptr_t psy_audio_player_multi_channel_audition(psy_audio_Player* self,
	const psy_audio_PatternEvent* ev, uint8_t note_off)
{
	uintptr_t track = 0;

	if (self->multichannelaudition) {
		if (ev->note < psy_audio_NOTECOMMANDS_RELEASE) {
			if (psy_table_exists(&self->notes_to_tracks_, ev->note)) {
				track = (uintptr_t)psy_table_at(&self->notes_to_tracks_,
					ev->note);
			} else {
				while (psy_table_exists(&self->tracks_to_notes_, track)) {
					++track;
				}
				psy_table_insert(&self->notes_to_tracks_, ev->note,
					(void*)track);
				psy_table_insert(&self->tracks_to_notes_, track,
					(void*)(uintptr_t)ev->note);
			}
		} else if (ev->note == psy_audio_NOTECOMMANDS_RELEASE) {
			if (psy_table_exists(&self->notes_to_tracks_, note_off)) {
				track = (uintptr_t)psy_table_at(&self->notes_to_tracks_,
					note_off);
				psy_table_remove(&self->notes_to_tracks_, note_off);
				psy_table_remove(&self->tracks_to_notes_, track);
			}
		}
	} else {
		if (ev->note < psy_audio_NOTECOMMANDS_RELEASE) {
			// self->active_note = ev->note;			
		} else if (ev->note == psy_audio_NOTECOMMANDS_RELEASE) {			
			// self->active_note = psy_audio_NOTECOMMANDS_EMPTY;			
		}
	}
	return track;
}

void psy_audio_player_record_notes(psy_audio_Player* self,
	uintptr_t track, const psy_audio_PatternEvent* ev)
{
	double offset;

	offset = psy_audio_sequencer_frame_to_offset(&self->sequencer,
		psy_audiodriver_playpos_in_samples(psy_audio_audiodriverplugin_base(
			&self->audiodrivers.driver_plugin)) - self->resyncplayposinsamples);
	if (offset < 0.0) {
		offset = 0.0;
	}
	if (self->recordnoteoff || ev->note != psy_audio_NOTECOMMANDS_RELEASE) {
		psy_audio_sequencer_record_input_event(&self->sequencer, ev, track,
			psy_dsp_beatpos_make_real(self->resyncplayposinbeats,
				psy_dsp_DEFAULT_PPQ));
	}
}

/* properties */
void psy_audio_player_set_song(psy_audio_Player* self, psy_audio_Song* song)
{
	assert(self);

	if (self->song == song) {
		return;
	}
	psy_audio_player_stop(self);
	dsp.clear(bufferdriver, MAX_SAMPLES_WORKFN);
	self->song = song;
	psy_audio_midiinput_setsong(&self->midiinput, song);
	if (self->song) {		
		psy_audio_SequencerMetronome restore_metronome;
		psy_audio_SequencerPlayMode restore_play_mode;
		bool restore_loop;
		
		if (self->machinefactory.machinecallback) {
			psy_audio_machinecallback_set_song(
				self->machinefactory.machinecallback, self->song);
		}
		restore_metronome = self->sequencer.metronome;		
		restore_play_mode = self->sequencer.mode;
		restore_loop = self->sequencer.looping;
		psy_audio_sequencer_reset(&self->sequencer, psy_audio_song_sequence(song),
			psy_audio_song_machines(song), psy_audiodriver_samplerate(
				psy_audio_audiodriverplugin_base(
					&self->audiodrivers.driver_plugin)));
		if (restore_play_mode == psy_audio_SEQUENCERPLAYMODE_PLAYFILES) {
			self->sequencer.mode = psy_audio_SEQUENCERPLAYMODE_PLAYFILES;
		}
		self->sequencer.auto_note_off = self->song->properties_.auto_note_off_;
		psy_audio_player_set_bpm(self, psy_audio_song_bpm(self->song));
		psy_audio_player_set_lpb(self, psy_audio_song_lpb(self->song));
		psy_audio_player_set_octave(self, psy_audio_song_octave(self->song));
		self->sequencer.metronome = restore_metronome;
		self->sequencer.looping = restore_loop;
		psy_audio_player_set_sampler_index(self,
			psy_audio_song_sampler_index(self->song));
		psy_audio_player_set_position(self, 0.0);	
	}
	psy_signal_emit(&self->signal_song_changed, self, 0);
}

void psy_audio_player_start_audio(psy_audio_Player* self, psy_Logger* logger)
{
	assert(self);

	self->audiodrivers.driver_plugin.driver.logger = logger;
	if (self->audiodrivers.driver_plugin.client) {		
		self->audiodrivers.driver_plugin.client->logger = logger;
	}
	psy_audiodriver_open(psy_audio_audiodriverplugin_base(
		&self->audiodrivers.driver_plugin));
	psy_audio_player_stop_threads(self);
	self->eventdrivers.logger = logger;
	psy_audio_eventdrivers_load_actives(&self->eventdrivers);
	psy_audio_eventdrivers_update_active(&self->eventdrivers);
	if (self->audiodrivers.config_) {
		psy_audio_player_start_threads(self,
			psy_configuration_value_int(self->audiodrivers.config_, "threads", 0));
	} else {
		psy_audio_player_start_threads(self, 0);
	}
}

void psy_audio_player_set_octave(psy_audio_Player* self, uint8_t octave)
{
	assert(self);

	if (octave >= 0 && octave <= 8) {
		self->octave = octave;
		if (self->song) {
			psy_audio_song_set_octave(self->song, octave);
			psy_signal_emit(&self->signal_octavechanged, self, 1, octave);
		}
	}	
}

void psy_audio_player_set_vu_meter_mode(psy_audio_Player* self,
	VUMeterMode mode)
{
	assert(self);

	self->vumode = mode;
}

VUMeterMode psy_audio_player_vu_meter_mode(psy_audio_Player* self)
{
	assert(self);

	return self->vumode;
}

void psy_audio_player_enable_dither(psy_audio_Player* self)
{
	assert(self);

	self->dither_.settings.enabled = TRUE;	
}

void psy_audio_player_disable_dither(psy_audio_Player* self)
{
	assert(self);

	self->dither_.settings.enabled = FALSE;
}

void psy_audio_player_configure_dither(psy_audio_Player* self,
	psy_dsp_DitherSettings settings)
{
	assert(self);

	psy_dsp_dither_configure(&self->dither_, settings);	
}

psy_dsp_DitherSettings psy_audio_player_dither_configuration(
	const psy_audio_Player* self)
{
	assert(self);

	return self->dither_.settings;	
}

void psy_audio_player_start(psy_audio_Player* self)
{
	assert(self);

	psy_audio_activechannels_reset(&self->playon);
	psy_audio_sequencer_start(&self->sequencer);
}

void psy_audio_player_start_begin(psy_audio_Player* self)
{
	psy_audio_player_set_position(self, 0.0);
	psy_audio_player_start(self);
}

void psy_audio_player_start_curr_seq_pos(psy_audio_Player* self)
{
	assert(self);

	if (self->song) {
		psy_audio_player_set_position(self,
			psy_dsp_beatpos_real(psy_audio_sequence_offset(
				psy_audio_song_sequence(self->song),
				psy_audio_sequencecursor_order_index(
					&psy_audio_song_sequence(self->song)->cursor))));
		psy_audio_player_start(self);
	}
}

void psy_audio_player_stop(psy_audio_Player* self)
{	
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_sequencer_stop(&self->sequencer);
	psy_audio_player_do_stop(self);
	psy_audio_exclusivelock_leave();
}

void psy_audio_player_pause(psy_audio_Player* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	psy_audio_sequencer_stop(&self->sequencer);	
	psy_audio_exclusivelock_leave();
}

void psy_audio_player_resume(psy_audio_Player* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();
	/* force regeneration of trackiterators */
	psy_audio_sequencer_set_position(&self->sequencer,
		psy_audio_sequencer_position(&self->sequencer));
	psy_audio_sequencer_start(&self->sequencer);
	psy_audio_exclusivelock_leave();
}

#define FADE_OUT_WAIT 500000

void psy_audio_player_fade_out(psy_audio_Player* self)
{
	assert(self);

	if (self->song) {
		psy_audio_PatternEvent ev;

		psy_audio_patternevent_init(&ev);
		ev.note = psy_audio_NOTECOMMANDS_RELEASE;
		ev.mach = psy_audio_MASTER_INDEX;
		psy_audio_machine_seq_tick(psy_audio_machines_master(
			psy_audio_song_machines(self->song)), 0, &ev);
		psy_sleep_for(FADE_OUT_WAIT);
	}
}

void psy_audio_player_do_stop(psy_audio_Player* self)
{
	assert(self);

	if (self->song) {
		psy_TableIterator it;
		
		for (it = psy_audio_machines_begin(psy_audio_song_machines(self->song));
				!psy_tableiterator_equal(&it, psy_table_end());
				psy_tableiterator_inc(&it)) {			
			psy_audio_machine_stop((psy_audio_Machine*)
				psy_tableiterator_value(&it));					
		}
		psy_audio_player_set_bpm(self, psy_audio_song_bpm(self->song));
		psy_audio_player_set_lpb(self, psy_audio_song_lpb(self->song));
		psy_audio_activechannels_reset(&self->playon);
	}
}

void psy_audio_player_set_lpb(psy_audio_Player* self, uintptr_t lpb)
{	
	assert(self);	

	if (lpb > 31) {
		lpb = 31;
	}
	if (lpb == 0) {
		return;
	}	
	if (self->song) {
		psy_audio_song_set_lpb(self->song, lpb);
	}
	psy_audio_sequencer_set_lpb(&self->sequencer, lpb);	
	/*if (self->song) {
		psy_audio_SequenceCursor cursor;
		
		cursor = psy_audio_sequence_cursor(&self->song->sequence);
		psy_audio_sequencecursor_set_lpb(&cursor, lpb);
		printf("set cursor\n");
		psy_audio_sequence_set_cursor(&self->song->sequence, cursor);		
	}*/
	psy_signal_emit(&self->signal_lpbchanged, self, 1, lpb);
}

/* Event recording */
void psy_audio_player_start_edit_mode(psy_audio_Player* self)
{
	assert(self);

	self->editmode = TRUE;
	if (self->song) {
		psy_signal_emit(
			&psy_audio_song_sequence(self->song)->signal_cursor_changed,
			psy_audio_song_sequence(self->song), 0);
	}
}

void psy_audio_player_stop_edit_mode(psy_audio_Player* self)
{
	assert(self);

	self->editmode = FALSE;
	if (self->song) {
		psy_signal_emit(
			&psy_audio_song_sequence(self->song)->signal_cursor_changed,
			psy_audio_song_sequence(self->song), 0);
	}
}

bool psy_audio_player_edit_mode(const psy_audio_Player* self)
{
	assert(self);

	return self->editmode;
}

psy_EventDriver* psy_audio_player_kbd_driver(psy_audio_Player* self)
{
	assert(self);

	return self->eventdrivers.kbddriver;
}

void psy_audio_player_set_empty_song(psy_audio_Player* self)
{
	assert(self);

	psy_audio_exclusivelock_enter();	
	psy_audio_player_set_song(self, &self->empty_song_);
	dsp.clear(bufferdriver, MAX_SAMPLES_WORKFN);
	psy_audio_exclusivelock_leave();
}

void psy_audio_player_idle(psy_audio_Player* self)
{
	psy_audio_eventdrivers_idle(&self->eventdrivers);	
}

double psy_audio_player_real_bpm(const psy_audio_Player* self)
{
	return psy_audio_player_bpm(self) * psy_audio_sequencer_speed(
		&self->sequencer);
}

void psy_audio_player_start_threads(psy_audio_Player* self,
	uintptr_t thread_count)
{
	uintptr_t numthreads;
	uintptr_t i;

	if (psy_list_size(self->threads_) > 0) {
		/* scheduler threads are already running */
		return;
	}
	if (self->thread_count == 0) {
		numthreads = psy_thread_hardware_concurrency();	
	} else {
		numthreads = thread_count;
	}
	self->thread_count = numthreads;
	if (numthreads < 2) {
		/* don't create any thread, will use a single-threaded */
		return;
	}
	self->stop_requested_ = FALSE;		
	for (i = 0; i < numthreads; ++i) {
		psy_Thread* thread;

		/* start the scheduling threads */
		thread = (psy_Thread*)malloc(sizeof(psy_Thread));
		psy_thread_init_start(thread, self,
			(psy_fp_thread_callback)psy_audio_player_thread_function);
		psy_list_append(&self->threads_, thread);			
	}
}

#if defined DIVERSALIS__OS__MICROSOFT
unsigned int __stdcall psy_audio_player_thread_function(psy_audio_Player* self)
#else
unsigned int psy_audio_player_thread_function(psy_audio_Player* self)
#endif
{
	psy_audio_player_process_loop(self);
	return 0;
}

void psy_audio_player_process_loop(psy_audio_Player* self)
{
	while (!self->stop_requested_) {
		psy_audio_MachineWork* work;

		work = NULL;
		psy_lock_enter(&self->mutex);
		if (psy_list_size(self->nodes_queue_) > 0) {
			work = (psy_audio_MachineWork*)
				psy_list_last(self->nodes_queue_)->entry;
			psy_list_remove(&self->nodes_queue_, self->nodes_queue_->tail);
		}
		psy_lock_leave(&self->mutex);
		if (work != NULL) {
			if (!psy_audio_machines_is_mixer_send(psy_audio_song_machines(self->song),
				work->slot)) {
				psy_lock_enter(&self->block);
				++self->waiting;
				psy_lock_leave(&self->block);
				psy_audio_player_work_machine(self, work->amount, work->slot);
				psy_lock_enter(&self->block);
				--self->waiting;
				psy_lock_leave(&self->block);
			}
			free(work);
		}
	}
}

void psy_audio_player_stop_threads(psy_audio_Player* self)
{
	self->stop_requested_ = TRUE;
	psy_list_deallocate(&self->threads_,
		(psy_fp_disposefunc)psy_thread_dispose);
	psy_list_deallocate(&self->nodes_queue_, NULL);
}

uintptr_t psy_audio_player_num_threads(const psy_audio_Player* self)
{
	if (self->thread_count < 2) {
		return 1;
	}
	return psy_list_size(self->threads_);
}

bool psy_audio_player_playing_key(const psy_audio_Player* self , uint8_t key)
{
	assert(self);

	return psy_table_exists(&self->notes_to_tracks_, key);
}

/* bool psy_audio_player_is_active_key(const psy_audio_Player* self, uint8_t key)
{
	return (self->active_note == key);
} */

bool psy_audio_player_enabled(const psy_audio_Player* self)
{
	assert(self);
	
	return psy_audio_audiodrivers_enabled(&self->audiodrivers);	
}

void psy_audio_player_on_bpm_tweak(psy_audio_Player* self,
	psy_audio_MachineParam* sender, double value)
{
	psy_audio_sequencer_set_bpm(&self->sequencer, value * 999.0);
}

void psy_audio_player_on_bpm_norm_value(psy_audio_Player* self,
	psy_audio_MachineParam* sender, double* rv)
{
	*rv = psy_audio_sequencer_bpm(&self->sequencer) / 999.0;
}

void psy_audio_player_on_bpm_describe(psy_audio_Player* self,
	psy_audio_MachineParam* sender, int* active, char* rv)
{
	assert(self);
		
	psy_snprintf(rv, 64, "%d (%.2f)", (int)psy_audio_player_bpm(self),
		psy_audio_player_real_bpm(self));
	*active = TRUE;
}

void psy_audio_player_on_lpb_tweak(psy_audio_Player* self,
	psy_audio_MachineParam* sender, double value)
{
	psy_audio_player_set_lpb(self, (uintptr_t)(value * 99));
}

void psy_audio_player_on_lpb_norm_value(psy_audio_Player* self,
	psy_audio_MachineParam* sender, double* rv)
{	
	*rv = (float)psy_audio_player_lpb(self) / 99.f;
}

void psy_audio_player_on_lpb_describe(psy_audio_Player* self,
	psy_audio_MachineParam* sender, int* active, char* rv)
{
	uintptr_t lpb;	

	assert(self);

	lpb = psy_audio_player_lpb(self);	
	psy_snprintf(rv, 64, "%d", (int)lpb);
	*active = TRUE;
}

void psy_audio_player_set_config(psy_audio_Player* self,
	psy_Configuration* config, psy_Configuration* event_config,
	psy_Configuration* misc, psy_Configuration* metronome,
	psy_Configuration* seqeditor, psy_Configuration* compat,
	psy_Configuration* global, psy_Configuration* midi)
{
	assert(self);	
	
	if (config) {
		psy_configuration_connect(config, "threads.num",
			self, psy_audio_player_on_set_num_threads);
	}
	if (misc) {
		psy_configuration_connect(misc, "multikey",
			self, psy_audio_player_on_multi_key);
		psy_configuration_connect(misc, "recordnoteoff",
			self, psy_audio_player_on_record_noteoff);
		psy_configuration_connect(misc, "notestoeffects",
			self, psy_audio_player_on_notes_to_effect);
	}
	if (metronome) {
		psy_configuration_connect(metronome, "note",
			self, psy_audio_player_on_metronome_note_changed);
		psy_configuration_connect(metronome, "machine",
		self, psy_audio_player_on_metronome_machine_changed);
	}
	if (seqeditor) {
		psy_configuration_connect(seqeditor, "machine",
			self, psy_audio_on_sampler_machine_changed);
		psy_configuration_configure(seqeditor, "machine");		
	}
	if (compat) {
		psy_configuration_connect(compat, "loadnewgamefxblitz",
			self, psy_audio_on_blitz_compatibility);		
		psy_configuration_connect(compat, "enableaudio",
			self, psy_audio_player_on_enable_audio);
	}	
}

void psy_audio_player_on_set_num_threads(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);

	psy_audio_player_stop_threads(self);
}

void psy_audio_player_on_multi_key(psy_audio_Player* self, psy_Property* sender)
{
	assert(self);
	
	self->multichannelaudition = psy_property_item_bool(sender);	
}

void psy_audio_player_on_record_noteoff(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);

	self->recordnoteoff = psy_property_item_bool(sender);		
}

void psy_audio_player_on_notes_to_effect(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);
	
}

void psy_audio_player_on_metronome_note_changed(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);

	self->sequencer.metronome_event.note = (uint8_t)psy_property_item_int(
		sender);
}

void psy_audio_player_on_metronome_machine_changed(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);

	self->sequencer.metronome_event.mach = (uint8_t)psy_property_item_int(
		sender);
}

void psy_audio_on_sampler_machine_changed(psy_audio_Player* self, psy_Property*
	sender)
{
	assert(self);

	psy_audio_player_set_sampler_index(self, psy_property_item_int(sender));
}

void psy_audio_on_blitz_compatibility(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);
	
	bool mode;

	mode = psy_property_item_bool(sender);
	if (mode != FALSE) {
		psy_audio_machinefactory_loadnewgamefxandblitzifversionunknown(
			&self->machinefactory);
	} else {
		psy_audio_machinefactory_loadoldgamefxandblitzifversionunknown(
			&self->machinefactory);
	}
}

void psy_audio_player_on_enable_audio(psy_audio_Player* self,
	psy_Property* sender)
{
	assert(self);

	if (psy_property_item_bool(sender)) {		
		psy_audiodriver_open(psy_audio_audiodriverplugin_base(
			&self->audiodrivers.driver_plugin));
	} else {
		psy_audiodriver_close(psy_audio_audiodriverplugin_base(
			&self->audiodrivers.driver_plugin));
	}
}

void psy_audio_player_wave_prev_play(psy_audio_Player* self, const char* path)
{
	assert(self);

	psy_audio_instprev_play(&self->inst_prev, psy_audio_machines_outputs(
		psy_audio_song_machines(self->song), psy_audio_MASTER_INDEX), path);
}

void psy_audio_player_wave_prev_play_sample(psy_audio_Player* self,
	const psy_audio_Sample* sample)
{
	assert(self);

	psy_audio_instprev_play_sample(&self->inst_prev, psy_audio_machines_outputs(
		psy_audio_song_machines(self->song), psy_audio_MASTER_INDEX), sample);
}

void psy_audio_player_wave_prev_stop(psy_audio_Player* self)
{	
	assert(self);

	psy_audio_instprev_stop(&self->inst_prev);
}
