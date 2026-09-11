/*
** PSYCLE-LINUX Phase 4 synthetic core-workflow compatibility harness.
**
** This file is project-authored regression code. It intentionally generates
** its .psy fixtures at test time; no upstream demo song, composition, sample,
** or other third-party musical content is embedded or redistributed here.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <midiinput.h>
#include <pattern.h>
#include <patterns.h>
#include <player.h>
#include <plugincatcher.h>
#include <song.h>
#include <songio.h>
#include <wire.h>

#define FIXTURE_TITLE "PSYCLE-LINUX Phase 4 synthetic fixture"
#define FIXTURE_CREDITS "PSYCLE-LINUX regression harness"
#define FIXTURE_COMMENTS "Synthetic compatibility data; not a demo song."
#define FIXTURE_BPM 147.0
#define FIXTURE_LPB 4
#define FIXTURE_NOTE 60
#define FIXTURE_VELOCITY 100
#define FIXTURE_MACHINE 0

static int fail(const char* message)
{
	fprintf(stderr, "phase4-core-workflow: FAIL: %s\n", message);
	return 1;
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

static int verify_song(psy_audio_Song* song)
{
	psy_audio_Machine* machine;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_SequenceCursor cursor;

	if (strcmp(psy_audio_song_title(song), FIXTURE_TITLE) != 0) {
		return fail("song title did not survive round trip");
	}
	if (strcmp(psy_audio_song_credits(song), FIXTURE_CREDITS) != 0) {
		return fail("song credits did not survive round trip");
	}
	if (strcmp(psy_audio_song_comments(song), FIXTURE_COMMENTS) != 0) {
		return fail("song comments did not survive round trip");
	}
	if (fabs(psy_audio_song_bpm(song) - FIXTURE_BPM) > 0.001) {
		return fail("BPM did not survive round trip");
	}
	if (psy_audio_song_lpb(song) != FIXTURE_LPB) {
		return fail("LPB did not survive round trip");
	}

	machine = psy_audio_machines_at(psy_audio_song_machines(song),
		FIXTURE_MACHINE);
	if (!machine) {
		return fail("sampler machine missing after load");
	}
	if (psy_audio_machine_type(machine) != psy_audio_SAMPLER) {
		return fail("machine 0 is not the expected Psycle sampler");
	}
	if (!psy_audio_machines_connected(psy_audio_song_machines(song),
			psy_audio_wire_make(FIXTURE_MACHINE, psy_audio_MASTER_INDEX))) {
		return fail("sampler-to-master wire did not survive round trip");
	}

	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		return fail("pattern 0 missing after load");
	}
	cursor = fixture_cursor(song);
	event = psy_audio_pattern_event_at_cursor(pattern, cursor);
	if (event.note != FIXTURE_NOTE) {
		return fail("MIDI-derived note did not survive round trip");
	}
	if (event.inst != 0) {
		return fail("MIDI-derived instrument mapping did not survive round trip");
	}
	if (event.mach != FIXTURE_MACHINE) {
		return fail("MIDI-derived machine mapping did not survive round trip");
	}
	if (event.cmd != 0x0C || event.parameter != FIXTURE_VELOCITY * 2) {
		return fail("MIDI-derived velocity command did not survive round trip");
	}

	return 0;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-core-workflow: save failed for %s (%d)\n",
			path, status);
		return 1;
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
		fprintf(stderr, "phase4-core-workflow: load failed for %s (%d)\n",
			path, status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

int main(int argc, char** argv)
{
	char first_path[4096];
	char second_path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_MidiInput midiinput;
	psy_audio_Machine* sampler;
	psy_audio_Song* song;
	psy_audio_Song* loaded;
	psy_audio_Song* reloaded;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_audio_SequenceCursor cursor;
	psy_EventDriverMidiData midi;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}

	snprintf(first_path, sizeof(first_path), "%s/phase4-first.psy", argv[1]);
	snprintf(second_path, sizeof(second_path), "%s/phase4-roundtrip.psy", argv[1]);

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);

	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		return fail("could not allocate synthetic song");
	}
	psy_audio_song_set_title(song, FIXTURE_TITLE);
	psy_audio_song_set_credits(song, FIXTURE_CREDITS);
	psy_audio_song_set_comments(song, FIXTURE_COMMENTS);
	psy_audio_song_set_bpm(song, FIXTURE_BPM);
	psy_audio_song_set_lpb(song, FIXTURE_LPB);

	sampler = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	if (!sampler) {
		psy_audio_song_deallocate(song);
		return fail("could not create built-in sampler machine");
	}
	psy_audio_machines_insert(psy_audio_song_machines(song), FIXTURE_MACHINE,
		sampler);
	psy_audio_machines_connect(psy_audio_song_machines(song),
		psy_audio_wire_make(FIXTURE_MACHINE, psy_audio_MASTER_INDEX));
	psy_audio_machines_select(psy_audio_song_machines(song), FIXTURE_MACHINE);

	/*
	** Exercise the same MIDI translator used by trackergrid_on_midi_cmds().
	** A MIDI Note On on channel 1 is converted into Psycle pattern data using
	** the selected sampler as the generator. The harness then performs the
	** pattern insertion directly so this core test does not require X11 widgets.
	*/
	psy_audio_midiinput_init(&midiinput, song, NULL);
	midi.byte0 = 0x90;
	midi.byte1 = FIXTURE_NOTE;
	midi.byte2 = FIXTURE_VELOCITY;
	psy_audio_patternevent_clear(&event);
	if (!psy_audio_midiinput_work_input(&midiinput, midi,
			psy_audio_song_machines(song), &event)) {
		psy_audio_midiinput_dispose(&midiinput);
		psy_audio_song_deallocate(song);
		return fail("MIDI Note On was not translated to a Psycle pattern event");
	}
	if (event.note != FIXTURE_NOTE || event.mach != FIXTURE_MACHINE) {
		psy_audio_midiinput_dispose(&midiinput);
		psy_audio_song_deallocate(song);
		return fail("MIDI translator returned unexpected pattern data");
	}
	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		psy_audio_midiinput_dispose(&midiinput);
		psy_audio_song_deallocate(song);
		return fail("synthetic song has no default pattern");
	}
	cursor = fixture_cursor(song);
	if (!psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)) {
		psy_audio_midiinput_dispose(&midiinput);
		psy_audio_song_deallocate(song);
		return fail("could not insert MIDI-derived event into pattern 0");
	}
	psy_audio_midiinput_dispose(&midiinput);

	if (verify_song(song) != 0 || save_song(song, first_path) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}

	loaded = load_song(&factory, first_path);
	if (!loaded) {
		psy_audio_song_deallocate(song);
		return 1;
	}
	if (verify_song(loaded) != 0 || save_song(loaded, second_path) != 0) {
		psy_audio_song_deallocate(loaded);
		psy_audio_song_deallocate(song);
		return 1;
	}

	reloaded = load_song(&factory, second_path);
	if (!reloaded) {
		psy_audio_song_deallocate(loaded);
		psy_audio_song_deallocate(song);
		return 1;
	}
	rc = verify_song(reloaded);

	psy_audio_song_deallocate(reloaded);
	psy_audio_song_deallocate(loaded);
	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase4-core-workflow: PASS\n");
	printf("generated: %s\n", first_path);
	printf("roundtrip: %s\n", second_path);
	return 0;
}
