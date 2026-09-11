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

#include <instruments.h>
#include <machine.h>
#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <player.h>
#include <plugincatcher.h>
#include <sample.h>
#include <samples.h>
#include <sequence.h>
#include <sequenceentry.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define FIXTURE_RATE 44100u
#define FIXTURE_FRAMES 11025u
#define FIXTURE_FREQ 440.0
#define FIXTURE_BPM 135.0
#define FIXTURE_LPB 4
#define FIXTURE_NOTE 48
#define FIXTURE_INSTRUMENT 0
#define FIXTURE_MACHINE 0
#define FIXTURE_AMPLITUDE 12000.0
#define FIXTURE_PCM_TOLERANCE 0.5
#define FIXTURE_TITLE "PSYCLE-LINUX Phase 4 audible sample fixture"

static int fail(const char* message)
{
	fprintf(stderr, "phase4-sample-workflow: FAIL: %s\n", message);
	return 1;
}

static int16_t fixture_pcm_value(uintptr_t frame)
{
	double phase;

	phase = 2.0 * M_PI * FIXTURE_FREQ * (double)frame /
		(double)FIXTURE_RATE;
	return (int16_t)lrint(sin(phase) * FIXTURE_AMPLITUDE);
}

static int write_bytes(FILE* file, const void* data, size_t size)
{
	return fwrite(data, 1, size, file) == size ? 0 : -1;
}

static int write_u16_le(FILE* file, uint16_t value)
{
	unsigned char bytes[2];
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	return write_bytes(file, bytes, sizeof(bytes));
}

static int write_u32_le(FILE* file, uint32_t value)
{
	unsigned char bytes[4];
	bytes[0] = (unsigned char)(value & 0xffu);
	bytes[1] = (unsigned char)((value >> 8) & 0xffu);
	bytes[2] = (unsigned char)((value >> 16) & 0xffu);
	bytes[3] = (unsigned char)((value >> 24) & 0xffu);
	return write_bytes(file, bytes, sizeof(bytes));
}

static int write_fixture_wav(const char* path)
{
	FILE* file;
	uint32_t data_bytes;
	uint32_t i;
	int failed;

	file = fopen(path, "wb");
	if (!file) {
		return fail("could not create generated WAV fixture");
	}
	data_bytes = FIXTURE_FRAMES * 2u;
	failed = 0;
	failed |= write_bytes(file, "RIFF", 4);
	failed |= write_u32_le(file, 36u + data_bytes);
	failed |= write_bytes(file, "WAVE", 4);
	failed |= write_bytes(file, "fmt ", 4);
	failed |= write_u32_le(file, 16u);
	failed |= write_u16_le(file, 1u);
	failed |= write_u16_le(file, 1u);
	failed |= write_u32_le(file, FIXTURE_RATE);
	failed |= write_u32_le(file, FIXTURE_RATE * 2u);
	failed |= write_u16_le(file, 2u);
	failed |= write_u16_le(file, 16u);
	failed |= write_bytes(file, "data", 4);
	failed |= write_u32_le(file, data_bytes);
	for (i = 0; i < FIXTURE_FRAMES && !failed; ++i) {
		failed |= write_u16_le(file, (uint16_t)fixture_pcm_value(i));
	}
	if (failed || ferror(file)) {
		fclose(file);
		return fail("short write while generating WAV fixture");
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

static int verify_sample_pcm(const psy_audio_Sample* sample)
{
	uintptr_t frame;
	const float* channel;

	if (!sample || sample->channels.numchannels != 1 ||
		!sample->channels.samples || !sample->channels.samples[0]) {
		return fail("expected mono sample PCM is missing");
	}
	channel = sample->channels.samples[0];
	for (frame = 0; frame < FIXTURE_FRAMES; ++frame) {
		double expected;
		double actual;

		expected = (double)fixture_pcm_value(frame);
		actual = (double)channel[frame];
		if (fabs(actual - expected) > FIXTURE_PCM_TOLERANCE) {
			fprintf(stderr,
				"phase4-sample-workflow: FAIL: PCM mismatch at frame %lu: expected %.1f, got %.3f\n",
				(unsigned long)frame, expected, actual);
			return 1;
		}
	}
	return 0;
}

static int verify_sample_song(psy_audio_Song* song, int verify_metadata)
{
	psy_audio_Sample* sample;
	psy_audio_Instrument* instrument;
	psy_audio_Machine* machine;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_SequenceCursor cursor;
	psy_audio_SequenceEntry* sequence_entry;

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
	if (verify_sample_pcm(sample) != 0) {
		return 1;
	}

	instrument = psy_audio_instruments_at(psy_audio_song_instruments(song),
		psy_audio_instrumentindex_make(0, FIXTURE_INSTRUMENT));
	if (!instrument) {
		return fail("sample instrument 0:0 is missing");
	}

	machine = psy_audio_machines_at(psy_audio_song_machines(song),
		FIXTURE_MACHINE);
	if (!machine || psy_audio_machine_type(machine) != psy_audio_SAMPLER) {
		return fail("WAV import did not create the built-in sampler");
	}
	if (!psy_audio_machines_connected(psy_audio_song_machines(song),
			psy_audio_wire_make(FIXTURE_MACHINE, psy_audio_MASTER_INDEX))) {
		return fail("sampler-to-master wire is missing");
	}

	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		return fail("WAV import did not create pattern 0");
	}
	sequence_entry = psy_audio_sequence_entry(psy_audio_song_sequence(song),
		psy_audio_orderindex_make(0, 0));
	if (!sequence_entry || sequence_entry->type != psy_audio_SEQUENCEENTRY_PATTERN) {
		return fail("sequence order 0:0 is not a pattern entry");
	}
	if (psy_audio_sequencepatternentry_patternslot(
			(const psy_audio_SequencePatternEntry*)sequence_entry) != 0) {
		return fail("sequence order 0:0 does not reference pattern 0");
	}

	cursor = fixture_cursor(song);
	event = psy_audio_pattern_event_at_cursor(pattern, cursor);
	if (event.note != FIXTURE_NOTE) {
		return fail("WAV import trigger note changed");
	}
	if (event.inst != FIXTURE_INSTRUMENT) {
		return fail("WAV import trigger does not select instrument 0");
	}
	if (event.mach != FIXTURE_MACHINE) {
		return fail("WAV import trigger does not target sampler machine 0");
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
	rc = snprintf(wav_path, sizeof(wav_path), "%s/phase4-generated-sine.wav",
		argv[1]);
	if (rc < 0 || rc >= (int)sizeof(wav_path)) {
		return fail("output WAV path is too long");
	}
	rc = snprintf(psy_path, sizeof(psy_path), "%s/phase4-sample-workflow.psy",
		argv[1]);
	if (rc < 0 || rc >= (int)sizeof(psy_path)) {
		return fail("output PSY path is too long");
	}

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
