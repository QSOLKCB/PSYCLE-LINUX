// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Project-authored Phase 6C Sampler PS1 non-44.1-kHz pitch witness.
 *
 * The modern C-Psycle save is an intermediate authoring form. A separate
 * fail-closed converter adds a byte-identical legacy WAVE representation for
 * the frozen C++ candidate while retaining the modern SMSB sample metadata for
 * pinned Psycle 1.12.0.
 */
#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <machine.h>
#include <machinefactory.h>
#include <machines.h>
#include <pattern.h>
#include <patterns.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

#define WITNESS_BPM 120.0
#define WITNESS_LPB 4.0
#define WITNESS_TPB 24
#define WITNESS_SAMPLE_RATE 22050.0
#define WITNESS_SAMPLE_FRAMES 11025
#define WITNESS_OUTPUT_RATE 44100
#define WITNESS_NOTE 60
#define WITNESS_PATTERN_BEATS 2.0

static void fail(const char* message) {
	fprintf(stderr, "phase6c_sampler_ps1_pitch_fixture: %s\n", message);
	exit(2);
}

static void install_sampler(psy_audio_Song* song) {
	psy_audio_MachineFactory* factory;
	psy_audio_Machine* sampler;

	factory = psy_audio_song_machine_factory(song);
	sampler = psy_audio_machinefactory_make_machine_from_path(
		factory, psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	if (!sampler) {
		fail("could not allocate built-in PS1 Sampler");
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), 0, sampler);
	psy_audio_machines_connect(
		psy_audio_song_machines(song), 0, psy_audio_MASTER_INDEX);
}

static void install_sample_and_instrument(psy_audio_Song* song) {
	psy_audio_Sample* sample;
	psy_audio_Instrument* instrument;
	psy_audio_InstrumentEntry entry;
	uintptr_t frame;

	sample = psy_audio_sample_allocinit(1);
	if (!sample) {
		fail("could not allocate witness sample");
	}
	psy_audio_sample_set_name(sample, "Phase 6C PS1 22.05 kHz witness");
	psy_audio_sample_set_sample_rate(sample, WITNESS_SAMPLE_RATE);
	psy_audio_sample_set_num_frames(sample, WITNESS_SAMPLE_FRAMES);
	psy_audio_sample_set_volume(sample, 0x80);
	psy_audio_buffer_allocsamples(
		&sample->channels, WITNESS_SAMPLE_FRAMES);
	for (frame = 0; frame < WITNESS_SAMPLE_FRAMES; ++frame) {
		/*
		 * Never-zero deterministic content makes the last active output frame
		 * a robust duration discriminator without requiring FFT estimation.
		 */
		sample->channels.samples[0][frame] =
			10000.0f + (float)(frame % 97u);
	}
	psy_audio_samples_insert(
		psy_audio_song_samples(song),
		sample,
		psy_audio_sampleindex_make(0, 0));

	instrument = psy_audio_instrument_allocinit();
	if (!instrument) {
		fail("could not allocate witness instrument");
	}
	psy_audio_instrument_set_name(
		instrument, "Phase 6C PS1 pitch instrument");
	entry = psy_audio_instrumententry_make();
	entry.sampleindex = psy_audio_sampleindex_make(0, 0);
	psy_audio_instrument_addentry(instrument, entry);
	psy_audio_instruments_insert(
		psy_audio_song_instruments(song), instrument, 0, 0);
}

static void install_pattern(psy_audio_Song* song) {
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_OrderIndex order;

	pattern = psy_audio_pattern_allocinit();
	if (!pattern) {
		fail("could not allocate witness pattern");
	}
	psy_audio_pattern_set_name(pattern, "PS1 22.05 kHz C4");
	psy_audio_pattern_setlength(pattern, WITNESS_PATTERN_BEATS);
	psy_audio_patternevent_clear(&event);
	event.note = WITNESS_NOTE;
	event.inst = 0;
	event.mach = 0;
	psy_audio_pattern_insert(
		pattern, NULL, 0, 0.0, &event);
	psy_audio_patterns_insert(
		psy_audio_song_patterns(song), pattern, 0);

	order = psy_audio_orderindex_make(0, 0);
	psy_audio_sequence_insert(
		psy_audio_song_sequence(song), order, 0);
}

static void verify_song(const psy_audio_Song* song) {
	const psy_audio_Sample* sample;

	sample = psy_audio_samples_at_const(
		psy_audio_song_samples_const(song),
		psy_audio_sampleindex_make(0, 0));
	if (!sample) {
		fail("witness sample missing after construction/load");
	}
	if (psy_audio_sample_num_frames(sample) != WITNESS_SAMPLE_FRAMES) {
		fail("witness sample frame count changed");
	}
	if (fabs(psy_audio_sample_sample_rate(sample) - WITNESS_SAMPLE_RATE) > 0.01) {
		fail("witness sample rate changed");
	}
	if (!psy_audio_machines_at_const(
			psy_audio_song_machines_const(song), 0)) {
		fail("witness Sampler machine missing");
	}
}

int main(int argc, char** argv) {
	psy_audio_Song song;
	psy_audio_Song loaded;
	psy_audio_SongFile songfile;
	psy_audio_SongFile loadfile;
	int status;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_PSY3\n", argv[0]);
		return 64;
	}

	psy_audio_song_init(&song, psy_audio_machinefactory());
	psy_audio_song_set_title(
		&song, "PSYCLE-LINUX Phase 6C Sampler PS1 22.05 kHz pitch witness");
	psy_audio_song_set_bpm(&song, WITNESS_BPM);
	psy_audio_song_set_lpb(&song, WITNESS_LPB);
	psy_audio_song_set_tpb(&song, WITNESS_TPB);
	install_sampler(&song);
	install_sample_and_instrument(&song);
	install_pattern(&song);
	verify_song(&song);

	psy_audio_songfile_init(&songfile);
	psy_audio_songfile_set_song(&songfile, &song);
	status = psy_audio_songfile_save(&songfile, argv[1]);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		psy_audio_song_dispose(&song);
		fail("could not save modern witness");
	}

	psy_audio_song_init(&loaded, psy_audio_machinefactory());
	psy_audio_songfile_init(&loadfile);
	psy_audio_songfile_set_song(&loadfile, &loaded);
	status = psy_audio_songfile_load(&loadfile, argv[1]);
	psy_audio_songfile_dispose(&loadfile);
	if (status != PSY_OK) {
		psy_audio_song_dispose(&loaded);
		psy_audio_song_dispose(&song);
		fail("fresh C-Psycle reload rejected modern witness");
	}
	verify_song(&loaded);

	printf(
		"{\"schema_version\":1,\"phase\":\"6C\","
		"\"contract\":\"sampler-ps1-pitch-runtime\","
		"\"authored_sample_rate\":22050,"
		"\"authored_sample_frames\":11025,"
		"\"note\":60,\"output_rate\":44100}\n");

	psy_audio_song_dispose(&loaded);
	psy_audio_song_dispose(&song);
	return 0;
}
