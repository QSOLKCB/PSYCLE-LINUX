/*
** PSYCLE-LINUX Phase 4 audible sample-workflow compatibility harness.
**
** The fixture WAV is generated at test time. No composition, sample, or
** third-party musical asset is stored in the repository.
*/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <plugincatcher.h>
#include <sample.h>
#include <samples.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define FIXTURE_RATE 44100u
#define FIXTURE_FRAMES 11025u
#define FIXTURE_FREQ 440.0
#define FIXTURE_BPM 135.0
#define FIXTURE_LPB 4
#define FIXTURE_TITLE "PSYCLE-LINUX Phase 4 audible sample fixture"

static int fail(const char* message)
{
	fprintf(stderr, "phase4-sample-workflow: FAIL: %s\n", message);
	return 1;
}

static void write_u16_le(FILE* file, uint16_t value)
{
	unsigned char bytes[2];
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	fwrite(bytes, 1, sizeof(bytes), file);
}

static void write_u32_le(FILE* file, uint32_t value)
{
	unsigned char bytes[4];
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	bytes[2] = (unsigned char)((value >> 16) & 0xffu);
	bytes[3] = (unsigned char)((value >> 24) & 0xffu);
	fwrite(bytes, 1, sizeof(bytes), file);
}

static int write_fixture_wav(const char* path)
{
	FILE* file;
	uint32_t data_bytes;
	uint32_t i;

	file = fopen(path, "wb");
	if (!file) {
		return fail("could not create generated WAV fixture");
	}
	data_bytes = FIXTURE_FRAMES * 2u;
	fwrite("RIFF", 1, 4, file);
	write_u32_le(file, 36u + data_bytes);
	fwrite("WAVE", 1, 4, file);
	fwrite("fmt ", 1, 4, file);
	write_u32_le(file, 16u);
	write_u16_le(file, 1u);
	write_u16_le(file, 1u);
	write_u32_le(file, FIXTURE_RATE);
	write_u32_le(file, FIXTURE_RATE * 2u);
	write_u16_le(file, 2u);
	write_u16_le(file, 16u);
	fwrite("data", 1, 4, file);
	write_u32_le(file, data_bytes);
	for (i = 0; i < FIXTURE_FRAMES; ++i) {
		double phase;
		int16_t sample;

		phase = 2.0 * M_PI * FIXTURE_FREQ * (double)i / (double)FIXTURE_RATE;
		sample = (int16_t)lrint(sin(phase) * 12000.0);
		write_u16_le(file, (uint16_t)sample);
	}
	if (fclose(file) != 0) {
		return fail("could not finalize generated WAV fixture");
	}
	return 0;
}

static psy_audio_SequenceCursor fixture_cursor(psy_audio_Song* song)
{
	psy_audio_SequenceCursor cursor;

	cursor = psy_audio_sequence_cursor(psy_audio_song_sequence(song));
	psy_audio_sequencecursor_set_order_index(&cursor,
		psy_audio_orderindex_make(0, 0));
	psy_audio_sequencecursor_set_channel(&cursor, 0);
	psy_audio_sequencecursor_set_offset(&cursor, psy_dsp_beatpos_zero());
	return cursor;
}

static double sample_energy(const psy_audio_Sample* sample)
{
	uintptr_t i;
	double energy;
	float* channel;

	if (!sample || sample->channels.numchannels == 0 ||
		!sample->channels.samples || !sample->channels.samples[0]) {
		return 0.0;
	}
	channel = sample->channels.samples[0];
	energy = 0.0;
	for (i = 0; i < sample->numframes; ++i) {
		energy += fabs((double)channel[i]);
	}
	return energy;
}

static int verify_sample_song(psy_audio_Song* song, int verify_metadata)
{
	psy_audio_Sample* sample;
	psy_audio_Machine* machine;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_SequenceCursor cursor;

	sample = psy_audio_samples_at(psy_audio_song_samples(song),
		psy_audio_sampleindex_make(0, 0));
	if (!sample) {
		return fail("sample slot 0:0 is missing");
	}
	if (psy_audio_sample_num_frames(sample) != FIXTURE_FRAMES) {
		return fail("sample frame count changed");
	}
	if (fabs(sample->samplerate - (double)FIXTURE_RATE) > 0.5) {
		return fail("sample rate changed");
	}
	if (sample_energy(sample) < 1000.0) {
		return fail("sample PCM is silent or missing");
	}

	machine = psy_audio_machines_at(psy_audio_song_machines(song), 0);
	if (!machine || psy_audio_machine_type(machine) != psy_audio_SAMPLER) {
		return fail("WAV import did not create the built-in sampler");
	}
	if (!psy_audio_machines_connected(psy_audio_song_machines(song),
			psy_audio_wire_make(0, psy_audio_MASTER_INDEX))) {
		return fail("sampler-to-master wire is missing");
	}

	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		return fail("WAV import did not create pattern 0");
	}
	cursor = fixture_cursor(song);
	event = psy_audio_pattern_event_at_cursor(pattern, cursor);
	if (event.note != 48) {
		return fail("WAV import trigger note changed");
	}

	if (verify_metadata) {
		if (strcmp(psy_audio_song_title(song), FIXTURE_TITLE) != 0) {
			return fail("song title did not survive PSY3 reload");
		}
		if (fabs(psy_audio_song_bpm(song) - FIXTURE_BPM) > 0.001) {
			return fail("BPM did not survive PSY3 reload");
		}
		if (psy_audio_song_lpb(song) != FIXTURE_LPB) {
			return fail("LPB did not survive PSY3 reload");
		}
	}
	return 0;
}

static psy_audio_Song* load_song(psy_audio_MachineFactory* factory,
	const char* path)
{
	psy_audio_Song* song;
	psy_audio_SongReader reader;
	int status;

	song = psy_audio_song_alloc_init(factory);
	if (!song) {
		return NULL;
	}
	psy_audio_songreader_init(&reader, song, NULL, FALSE);
	status = psy_audio_songreader_load(&reader, path);
	psy_audio_songreader_dispose(&reader);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-sample-workflow: load failed for %s (%d)\n",
			path, status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-sample-workflow: save failed for %s (%d)\n",
			path, status);
		return 1;
	}
	return 0;
}

int main(int argc, char** argv)
{
	char wav_path[4096];
	char psy_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* imported;
	psy_audio_Song* reloaded;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}
	snprintf(wav_path, sizeof(wav_path), "%s/phase4-generated-sine.wav", argv[1]);
	snprintf(psy_path, sizeof(psy_path), "%s/phase4-sample-workflow.psy", argv[1]);

	if (write_fixture_wav(wav_path) != 0) {
		return 1;
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);

	imported = load_song(&factory, wav_path);
	if (!imported) {
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}
	rc = verify_sample_song(imported, 0);
	if (rc == 0) {
		psy_audio_song_set_title(imported, FIXTURE_TITLE);
		psy_audio_song_set_bpm(imported, FIXTURE_BPM);
		psy_audio_song_set_lpb(imported, FIXTURE_LPB);
		rc = save_song(imported, psy_path);
	}

	reloaded = NULL;
	if (rc == 0) {
		reloaded = load_song(&factory, psy_path);
		if (!reloaded) {
			rc = 1;
		} else {
			rc = verify_sample_song(reloaded, 1);
		}
	}

	if (reloaded) {
		psy_audio_song_deallocate(reloaded);
	}
	psy_audio_song_deallocate(imported);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase4-sample-workflow: PASS\n");
	printf("generated-wav: %s\n", wav_path);
	printf("saved-psy: %s\n", psy_path);
	return 0;
}
