/*
** This source is free software ; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation ; either version 2, or (at your option) any later version.
** copyright 2000-2023 members of the psycle project http://psycle.sourceforge.net
*/

#ifndef psy_audio_PLAYER_H
#define psy_audio_PLAYER_H

/* local */
#include "audiodrivers.h"
#include "eventdrivers.h"
#include "instprev.h"
#include "machinefactory.h"
#include "midiinput.h"
#include "sequencer.h"
#include "song.h"
/* dsp */
#include <dither.h>
/* container */
#include <configuration.h>
#include <signal.h>
#include "../../thread/src/lock.h"

#ifdef __cplusplus
extern "C" {
#endif

void psy_audio_init(void);
void psy_audio_dispose(void);

typedef enum {
	VUMETER_NONE,
	VUMETER_PEAK,
	VUMETER_RMS,	
} VUMeterMode;

typedef struct psy_audio_MachineWork {
	uintptr_t amount;
	uintptr_t slot;
} psy_audio_MachineWork;

typedef struct psy_audio_PatternDefaults {
	psy_audio_Pattern* pattern;
	psy_audio_Patterns patterns;
	psy_audio_Sequence sequence;
} psy_audio_PatternDefaults;

void psy_audio_patterndefaults_init(psy_audio_PatternDefaults*);
void psy_audio_patterndefaults_dispose(psy_audio_PatternDefaults*);

psy_audio_PatternEvent psy_audio_patterndefaults_event(const
	psy_audio_PatternDefaults*, uintptr_t);
psy_audio_PatternEvent psy_audio_patterndefaults_fill_event(const
	psy_audio_PatternDefaults*, uintptr_t track,
	psy_audio_PatternEvent src);


typedef struct WavePrevMachineCallback {
	/* implements */
	psy_audio_MachineCallback machinecallback;
	/* references */	
	psy_audio_Song* song;
} WavePrevMachineCallback;

void waveprevmachinecallback_init(WavePrevMachineCallback*, psy_audio_Song*);
void waveprevmachinecallback_dispose(WavePrevMachineCallback*);

INLINE psy_audio_MachineCallback* waveprevmachinecallback_base(
	WavePrevMachineCallback* self)
{
	return &self->machinecallback;
}

/*! 
** @struct psy_audio_Player
** @brief schedules the processing of machines
*/

typedef struct psy_audio_Player {
	/*! @extends  */
	psy_audio_CustomMachine custommachine;
	/* signals */
	psy_Signal signal_song_changed;
	psy_Signal signal_lpbchanged;
	psy_Signal signal_inputevent;
	psy_Signal signal_stop;
	psy_Signal signal_octavechanged;
	/*! @internal */	
	psy_audio_AudioDrivers audiodrivers;
	psy_audio_Song* song;	
	psy_audio_MachineFactory machinefactory;
	psy_audio_PluginCatcher plugincatcher;
	psy_audio_Song empty_song_; /* dummy song while song switching */
	psy_audio_Sequencer sequencer;		
	psy_audio_EventDrivers eventdrivers;	
	VUMeterMode vumode;
	bool editmode; /* in edit mode? */
	bool follow_song;
	bool recordnoteoff;
	bool multichannelaudition;
	// uint8_t active_note;
	psy_Table notes_to_tracks_;
	psy_Table tracks_to_notes_;	
	psy_audio_PatternDefaults patterndefaults;	
	psy_dsp_Dither dither_;	
	uint8_t octave;
	uintptr_t resyncplayposinsamples;
	double resyncplayposinbeats;
	psy_audio_MidiInput midiinput;
	psy_audio_ActiveChannels playon;
	bool measure_cpu_usage;
	psy_audio_InstPrev inst_prev;	
	uintptr_t thread_count;
	psy_List* threads_;
	bool stop_requested_;
	psy_List* nodes_queue_;
	psy_Lock mutex;
	psy_Lock block;
	uintptr_t waiting;	
	/* parameters */
	psy_audio_CustomMachineParam tempo_param_;
	psy_audio_CustomMachineParam lpb_param_;	
} psy_audio_Player;

void psy_audio_player_init(psy_audio_Player*, psy_audio_MachineCallback*,
	void* systemhandle,
	psy_Configuration*, psy_Configuration* event_input, psy_Configuration* misc,
	psy_Configuration* metronome, psy_Configuration* seqeditor,
	psy_Configuration* compat, psy_Configuration* global,
	psy_Configuration* midi,
	psy_Property* cmddef,
	psy_Logger* logger);
void psy_audio_player_dispose(psy_audio_Player*);

void psy_audio_player_set_song(psy_audio_Player*, psy_audio_Song*);

void psy_audio_player_start_audio(psy_audio_Player*, psy_Logger*);

INLINE psy_audio_Song* psy_audio_player_song(psy_audio_Player* self)
{
	return self->song;
}

void psy_audio_player_set_octave(psy_audio_Player*, uint8_t octave);

INLINE uint8_t psy_audio_player_octave(const psy_audio_Player* self)
{
	return self->octave;
}

void psy_audio_player_set_vu_meter_mode(psy_audio_Player*, VUMeterMode);
VUMeterMode psy_audio_player_vu_meter_mode(psy_audio_Player*);
void psy_audio_player_enable_dither(psy_audio_Player*);
void psy_audio_player_disable_dither(psy_audio_Player*);
void psy_audio_player_configure_dither(psy_audio_Player*, psy_dsp_DitherSettings);
psy_dsp_DitherSettings psy_audio_player_dither_configuration(const psy_audio_Player*);

/* sequencer */
void psy_audio_player_start(psy_audio_Player*);
void psy_audio_player_start_begin(psy_audio_Player*);
void psy_audio_player_start_curr_seq_pos(psy_audio_Player*);
void psy_audio_player_stop(psy_audio_Player*);
void psy_audio_player_pause(psy_audio_Player*);
void psy_audio_player_resume(psy_audio_Player*);
void psy_audio_player_fade_out(psy_audio_Player*);

INLINE int psy_audio_player_playing(const psy_audio_Player* self)
{
	return psy_audio_sequencer_playing(&self->sequencer);
}

INLINE bool psy_audio_player_following_song(const psy_audio_Player* self)
{
	return self->follow_song;
}
	
INLINE void psy_audio_player_set_position(psy_audio_Player* self,
	double offset)
{
	assert(self);

	psy_audio_sequencer_set_position(&self->sequencer, offset);
}

INLINE double psy_audio_player_position(psy_audio_Player* self)
{
	return psy_audio_sequencer_position(&self->sequencer);
}

INLINE void psy_audio_player_set_bpm(psy_audio_Player* self,
	double bpm)
{	
	psy_audio_sequencer_set_bpm(&self->sequencer, bpm);
	if (self->song) {
		psy_audio_song_set_bpm(self->song, psy_audio_sequencer_bpm(
			&self->sequencer));
	}
}

INLINE double psy_audio_player_bpm(const psy_audio_Player* self)
{
	return psy_audio_sequencer_bpm(&self->sequencer);
}

double psy_audio_player_real_bpm(const psy_audio_Player*);

INLINE void psy_audio_player_setticksperbeat(psy_audio_Player* self,
	uintptr_t ticks)
{
	psy_audio_sequencer_set_ticks_per_beat(&self->sequencer, ticks);
	if (self->song) {
		psy_audio_song_set_tpb(self->song, ticks);		
	}
}

INLINE double psy_audio_player_ticks_per_beat(psy_audio_Player* self)
{
	return (double)self->sequencer.tpb;
}

INLINE void psy_audio_player_set_extra_ticks_per_beat(psy_audio_Player* self,
	uintptr_t ticks)
{
	psy_audio_sequencer_set_extra_ticks_per_beat(&self->sequencer, ticks);
	if (self->song) {
		psy_audio_song_set_extra_ticks_per_beat(self->song, ticks);		
	}
}

INLINE double psy_audio_player_extra_ticks_per_beat(
	psy_audio_Player* self)
{
	return (double)self->sequencer.extraticks;
}

INLINE void psy_audio_player_set_sampler_index(psy_audio_Player* self,
	uintptr_t sampler_index)
{
	self->sequencer.sample_event.mach = (uint8_t)sampler_index;
	if (self->song) {
		psy_audio_song_set_sampler_index(self->song, sampler_index);
	}
}

/* \returns lines per beat */
void psy_audio_player_set_lpb(psy_audio_Player*, uintptr_t lpb);

INLINE uintptr_t psy_audio_player_lpb(psy_audio_Player* self)
{
	return psy_audio_sequencer_lpb(&self->sequencer);
}

/* \returns beats per line */
INLINE double psy_audio_player_bpl(psy_audio_Player* self)
{
	return (double)(1.0) /
		psy_audio_sequencer_lpb(&self->sequencer);
}

INLINE double psy_audio_player_sample_rate(psy_audio_Player* self)
{
	return psy_audio_sequencer_sample_rate(&self->sequencer);
}

INLINE psy_dsp_percent_t psy_audio_player_row_progress(
	psy_audio_Player* self, uintptr_t track)
{
	return psy_audio_sequencer_row_progress(&self->sequencer, track);
}

/* cpu measure */
INLINE void psy_audio_player_measure_cpu_usage(psy_audio_Player* self)
{
	self->measure_cpu_usage = TRUE;
}

INLINE void psy_audio_player_stop_measure_cpu_usage(psy_audio_Player* self)
{
	self->measure_cpu_usage = FALSE;
}

INLINE bool psy_audio_player_measuring_cpu_usage(const psy_audio_Player* self)
{
	return self->measure_cpu_usage;
}

/* event recording */
void psy_audio_player_start_edit_mode(psy_audio_Player*);
void psy_audio_player_stop_edit_mode(psy_audio_Player*);
bool psy_audio_player_edit_mode(const psy_audio_Player*);
void psy_audio_player_input_pattern_event(psy_audio_Player*,
	const psy_audio_PatternEvent*);
void psy_audio_player_play_event(psy_audio_Player*,
	const psy_audio_PatternEvent*, uintptr_t track);
psy_audio_PatternEvent psy_audio_player_pattern_event(psy_audio_Player*,
	uint8_t note);
uintptr_t psy_audio_player_multi_channel_audition(psy_audio_Player*,
	const psy_audio_PatternEvent*, uint8_t note_off);

INLINE bool psy_audio_player_recording_noteoff(const psy_audio_Player* self)
{
	return self->recordnoteoff;
}

INLINE psy_audio_SequencerTime* psy_audio_player_sequencer_time(
	psy_audio_Player* self)
{
	return &self->sequencer.seqtime;
}

/* event driver */
psy_EventDriver* psy_audio_player_kbd_driver(psy_audio_Player*);
void psy_audio_player_work_machine(psy_audio_Player*, uintptr_t amount,
	uintptr_t slot);
void psy_audio_player_set_empty_song(psy_audio_Player*);
void psy_audio_player_idle(psy_audio_Player*);
/* metronome */
INLINE void psy_audio_player_activate_metronome(psy_audio_Player* self)
{
	self->sequencer.metronome.active = TRUE;
}

INLINE void psy_audio_player_deactivate_metronome(psy_audio_Player* self)
{
	self->sequencer.metronome.active = FALSE;
}

INLINE psy_audio_Sequencer* psy_audio_player_sequencer(psy_audio_Player* self)
{
	return &self->sequencer;
}

void psy_audio_player_start_threads(psy_audio_Player*, uintptr_t thread_count);
void psy_audio_player_stop_threads(psy_audio_Player*);
uintptr_t psy_audio_player_num_threads(const psy_audio_Player*);
bool psy_audio_player_playing_key(const psy_audio_Player*, uint8_t key);
bool psy_audio_player_enabled(const psy_audio_Player*);
void psy_audio_player_make_controller_save(psy_audio_Player*);
void psy_audio_player_make_controllers(psy_audio_Player*);
void psy_audio_player_wave_prev_play(psy_audio_Player*, const char* path);
void psy_audio_player_wave_prev_play_sample(psy_audio_Player*,
	const psy_audio_Sample*);
void psy_audio_player_wave_prev_stop(psy_audio_Player*);

INLINE psy_audio_MachineFactory* psy_audio_player_machine_factory(psy_audio_Player* self)
{
	return &self->machinefactory;
}
	
#ifdef __cplusplus
}
#endif

#endif /* psy_audio_PLAYER_H */
