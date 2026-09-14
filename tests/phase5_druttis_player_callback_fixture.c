/*
** C-side fixture for the Phase 5C production native callback timing gate.
**
** Psycle's Player/Song headers are C interfaces with legacy declarations that
** are not C++-clean on Linux. Keep construction of the real production
** Song -> Player -> MachineCallback chain in C, then hand the callback to the
** C++ native PluginFxCallback regression.
*/

#include <stdlib.h>

#include <audioconfig.h>
#include <machine.h>
#include <player.h>
#include <properties.h>
#include <song.h>

typedef struct Phase5DruttisPlayerCallbackFixture {
	psy_Property* config;
	psy_audio_AudioConfig audioconfig;
	psy_audio_MachineCallback callback;
	psy_audio_Player player;
	psy_audio_Song* song;
	int audio_initialized;
	int audioconfig_initialized;
	int player_initialized;
} Phase5DruttisPlayerCallbackFixture;

void phase5_druttis_player_callback_fixture_destroy(void* opaque);

void* phase5_druttis_player_callback_fixture_create(void)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)calloc(1, sizeof(*self));
	if (!self) {
		return NULL;
	}

	psy_audio_init();
	self->audio_initialized = 1;
	self->config = psy_property_allocinit_key(NULL);
	if (!self->config) {
		phase5_druttis_player_callback_fixture_destroy(self);
		return NULL;
	}

	psy_audio_audioconfig_init(&self->audioconfig, self->config);
	self->audioconfig_initialized = 1;
	psy_audio_machinecallback_init(&self->callback);
	psy_audio_player_init(&self->player, &self->callback, NULL,
		psy_audio_audioconfig_base(&self->audioconfig),
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	self->player_initialized = 1;

	self->song = psy_audio_song_alloc_init(&self->player.machinefactory);
	if (!self->song) {
		phase5_druttis_player_callback_fixture_destroy(self);
		return NULL;
	}

	psy_audio_song_set_bpm(self->song, 120.0);
	psy_audio_song_set_lpb(self->song, 4);
	psy_audio_song_set_tpb(self->song, 24);
	psy_audio_player_set_song(&self->player, self->song);
	return self;
}

void phase5_druttis_player_callback_fixture_destroy(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	if (!self) {
		return;
	}
	if (self->song) {
		if (self->player_initialized) {
			psy_audio_player_set_empty_song(&self->player);
		}
		psy_audio_song_deallocate(self->song);
		self->song = NULL;
	}
	if (self->player_initialized) {
		psy_audio_player_dispose(&self->player);
	}
	if (self->audioconfig_initialized) {
		psy_audio_audioconfig_dispose(&self->audioconfig);
	}
	if (self->config) {
		psy_property_deallocate(self->config);
	}
	if (self->audio_initialized) {
		psy_audio_dispose();
	}
	free(self);
}

psy_audio_MachineCallback* phase5_druttis_player_callback_fixture_callback(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	return self ? &self->callback : NULL;
}

double phase5_druttis_player_callback_fixture_samplerate(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	return (self && self->callback.vtable && self->callback.vtable->samplerate)
		? self->callback.vtable->samplerate(&self->callback)
		: 0.0;
}

double phase5_druttis_player_callback_fixture_bpm(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	return (self && self->callback.vtable && self->callback.vtable->bpm)
		? self->callback.vtable->bpm(&self->callback)
		: 0.0;
}

unsigned long phase5_druttis_player_callback_fixture_lpb(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	return (self && self->song) ? (unsigned long)psy_audio_song_lpb(self->song) : 0;
}

unsigned long phase5_druttis_player_callback_fixture_tpb(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	return (self && self->song) ? (unsigned long)psy_audio_song_tpb(self->song) : 0;
}

int phase5_druttis_player_callback_fixture_transport_tick_samples(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;
	double beats_per_tick;
	double beats_per_sample;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	if (!self || !self->callback.vtable || !self->callback.vtable->beatspertick ||
			!self->callback.vtable->beatspersample) {
		return -1;
	}
	beats_per_tick = self->callback.vtable->beatspertick(&self->callback);
	beats_per_sample = self->callback.vtable->beatspersample(&self->callback);
	return (beats_per_tick > 0.0 && beats_per_sample > 0.0)
		? (int)(beats_per_tick / beats_per_sample)
		: -1;
}

int phase5_druttis_player_callback_fixture_line_samples(void* opaque)
{
	Phase5DruttisPlayerCallbackFixture* self;
	double beats_per_line;
	double beats_per_sample;

	self = (Phase5DruttisPlayerCallbackFixture*)opaque;
	if (!self || !self->callback.vtable || !self->callback.vtable->currbeatsperline ||
			!self->callback.vtable->beatspersample) {
		return -1;
	}
	beats_per_line = self->callback.vtable->currbeatsperline(&self->callback);
	beats_per_sample = self->callback.vtable->beatspersample(&self->callback);
	return (beats_per_line > 0.0 && beats_per_sample > 0.0)
		? (int)(beats_per_line / beats_per_sample)
		: -1;
}
