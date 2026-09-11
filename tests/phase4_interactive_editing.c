/*
** PSYCLE-LINUX Phase 4 interactive-editing compatibility harness.
**
** This regression exercises the production command/model paths used by
** Machine View and Tracker Grid without depending on X11 pixel coordinates.
** All song data is synthetic and generated at test time.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <machine.h>
#include <machinefactory.h>
#include <machines.h>
#include <pattern.h>
#include <patterns.h>
#include <player.h>
#include <plugincatcher.h>
#include <sequencecmds.h>
#include <song.h>
#include <songio.h>
#include <undoredo.h>
#include <wire.h>

#define SAMPLER_SLOT 0
#define MIXER_SLOT 1
#define SAMPLER_NAME "Loop Sampler"
#define MIXER_NAME "Routing Mixer"
#define SONG_TITLE "PSYCLE-LINUX Phase 4 editing fixture"
#define SONG_BPM 128.0
#define SONG_LPB 4
#define NOTE_A 48
#define NOTE_B 55

static int fail(const char* message)
{
	fprintf(stderr, "phase4-interactive-editing: FAIL: %s\n", message);
	return 1;
}

static psy_audio_SequenceCursor cursor_at(psy_audio_Song* song,
	uintptr_t channel, double beat)
{
	psy_audio_SequenceCursor cursor;

	cursor = psy_audio_sequence_cursor(psy_audio_song_sequence(song));
	psy_audio_sequencecursor_set_order_index(&cursor,
		psy_audio_orderindex_make(0, 0));
	psy_audio_sequencecursor_set_channel(&cursor, channel);
	psy_audio_sequencecursor_set_offset(&cursor,
		psy_dsp_beatpos_make_real(beat, psy_dsp_DEFAULT_PPQ));
	return cursor;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-interactive-editing: save failed (%d)\n", status);
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
		fprintf(stderr, "phase4-interactive-editing: load failed (%d)\n", status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int verify_machine_state(psy_audio_Song* song)
{
	psy_audio_Machines* machines;
	psy_audio_Machine* sampler;
	psy_audio_Machine* mixer;
	double x;
	double y;

	machines = psy_audio_song_machines(song);
	sampler = psy_audio_machines_at(machines, SAMPLER_SLOT);
	mixer = psy_audio_machines_at(machines, MIXER_SLOT);
	if (!sampler || psy_audio_machine_type(sampler) != psy_audio_SAMPLER) {
		return fail("sampler machine missing or wrong type");
	}
	if (!mixer || psy_audio_machine_type(mixer) != psy_audio_MIXER) {
		return fail("mixer machine missing or wrong type");
	}
	if (!psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT))) {
		return fail("sampler-to-mixer wire missing");
	}
	if (!psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX))) {
		return fail("mixer-to-master wire missing");
	}
	if (psy_audio_machines_connected(machines,
			psy_audio_wire_make(SAMPLER_SLOT, psy_audio_MASTER_INDEX))) {
		return fail("unexpected direct sampler-to-master wire");
	}
	if (!psy_audio_machine_muted(sampler)) {
		return fail("sampler mute state did not persist");
	}
	if (!psy_audio_machine_bypassed(mixer)) {
		return fail("mixer bypass state did not persist");
	}
	if (fabs(psy_audio_machine_panning(sampler) - 0.25) > 0.02) {
		return fail("sampler panning did not persist");
	}
	if (!psy_audio_machine_edit_name(sampler) ||
			strcmp(psy_audio_machine_edit_name(sampler), SAMPLER_NAME) != 0) {
		return fail("sampler edit name did not persist");
	}
	if (!psy_audio_machine_edit_name(mixer) ||
			strcmp(psy_audio_machine_edit_name(mixer), MIXER_NAME) != 0) {
		return fail("mixer edit name did not persist");
	}
	psy_audio_machine_position(sampler, &x, &y);
	if (fabs(x - 96.0) > 0.5 || fabs(y - 144.0) > 0.5) {
		return fail("sampler Machine View position did not persist");
	}
	psy_audio_machine_position(mixer, &x, &y);
	if (fabs(x - 320.0) > 0.5 || fabs(y - 144.0) > 0.5) {
		return fail("mixer Machine View position did not persist");
	}
	if (psy_audio_machine_num_parameters(mixer) == 0 ||
			!psy_audio_machine_parameter(mixer, 0)) {
		return fail("mixer parameter surface is not accessible");
	}
	return 0;
}

static int verify_pattern_state(psy_audio_Song* song)
{
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent a;
	psy_audio_PatternEvent b;

	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		return fail("pattern 0 missing");
	}
	a = psy_audio_pattern_event_at_cursor(pattern, cursor_at(song, 0, 0.0));
	b = psy_audio_pattern_event_at_cursor(pattern, cursor_at(song, 1, 0.25));
	if (a.note != NOTE_A || a.inst != 0 || a.mach != SAMPLER_SLOT ||
			a.cmd != 0x0C || a.parameter != 0xA0) {
		return fail("primary tracker event did not persist");
	}
	if (b.note != NOTE_B || b.inst != 0 || b.mach != SAMPLER_SLOT ||
			b.cmd != 0x03 || b.parameter != 0x20) {
		return fail("second tracker/effect-column event did not persist");
	}
	return 0;
}

int main(int argc, char** argv)
{
	char path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_Song* song;
	psy_audio_Song* loaded;
	psy_audio_Machines* machines;
	psy_audio_Machine* sampler;
	psy_audio_Machine* mixer;
	psy_audio_Pattern* pattern;
	psy_audio_PatternEvent event;
	psy_UndoRedo pattern_undo;
	int rc;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}
	if (snprintf(path, sizeof(path), "%s/phase4-interactive-editing.psy",
			argv[1]) >= (int)sizeof(path)) {
		return fail("output path is too long");
	}

	psy_audio_init();
	psy_audio_machinecallback_init(&callback);
	psy_audio_plugincatcher_init(&catcher, NULL);
	psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);
	song = psy_audio_song_alloc_init(&factory);
	if (!song) {
		return fail("could not allocate song");
	}
	psy_audio_song_set_title(song, SONG_TITLE);
	psy_audio_song_set_bpm(song, SONG_BPM);
	psy_audio_song_set_lpb(song, SONG_LPB);
	machines = psy_audio_song_machines(song);

	/*
	** These are the same undoable Machines operations used by Machine View.
	** Exercise insert/connect/delete/rewire plus undo/redo before persisting.
	*/
	sampler = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_SAMPLER, NULL, 0, psy_INDEX_INVALID);
	mixer = psy_audio_machinefactory_make_machine_from_path(&factory,
		psy_audio_MIXER, NULL, 0, psy_INDEX_INVALID);
	if (!sampler || !mixer) {
		psy_audio_song_deallocate(song);
		return fail("could not create built-in editing machines");
	}
	psy_audio_machines_insert(machines, SAMPLER_SLOT, sampler);
	psy_audio_machines_insert(machines, MIXER_SLOT, mixer);
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT));
	psy_audio_machines_connect(machines,
		psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX));

	psy_undoredo_undo(&machines->undoredo);
	if (psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX))) {
		psy_audio_song_deallocate(song);
		return fail("wire undo did not disconnect mixer from master");
	}
	psy_undoredo_redo(&machines->undoredo);
	if (!psy_audio_machines_connected(machines,
			psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX))) {
		psy_audio_song_deallocate(song);
		return fail("wire redo did not restore mixer-to-master");
	}

	psy_audio_machines_remove(machines, MIXER_SLOT, TRUE);
	if (psy_audio_machines_at(machines, MIXER_SLOT) ||
			!psy_audio_machines_connected(machines,
				psy_audio_wire_make(SAMPLER_SLOT, psy_audio_MASTER_INDEX))) {
		psy_audio_song_deallocate(song);
		return fail("delete-with-rewire did not produce sampler-to-master path");
	}
	psy_undoredo_undo(&machines->undoredo);
	if (!psy_audio_machines_at(machines, MIXER_SLOT) ||
			!psy_audio_machines_connected(machines,
				psy_audio_wire_make(SAMPLER_SLOT, MIXER_SLOT)) ||
			!psy_audio_machines_connected(machines,
				psy_audio_wire_make(MIXER_SLOT, psy_audio_MASTER_INDEX))) {
		psy_audio_song_deallocate(song);
		return fail("machine-delete undo did not restore topology");
	}

	sampler = psy_audio_machines_at(machines, SAMPLER_SLOT);
	mixer = psy_audio_machines_at(machines, MIXER_SLOT);
	psy_audio_machines_rename(machines, SAMPLER_SLOT, SAMPLER_NAME);
	psy_audio_machines_rename(machines, MIXER_SLOT, MIXER_NAME);
	psy_audio_machine_set_position(sampler, 96.0, 144.0);
	psy_audio_machine_set_position(mixer, 320.0, 144.0);
	psy_audio_machine_set_panning(sampler, 0.25);
	psy_audio_machine_mute(sampler);
	psy_audio_machine_bypass(mixer);
	if (verify_machine_state(song) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}

	/*
	** Tracker Grid uses InsertCommand through an UndoRedo instance. Exercise a
	** normal note/volume command and a second track/effect-column command, then
	** prove undo/redo before the song is saved.
	*/
	pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern) {
		psy_audio_song_deallocate(song);
		return fail("default pattern is missing");
	}
	psy_undoredo_init(&pattern_undo);

	psy_audio_patternevent_clear(&event);
	event.note = NOTE_A;
	event.inst = 0;
	event.mach = SAMPLER_SLOT;
	event.cmd = 0x0C;
	event.parameter = 0xA0;
	psy_undoredo_execute(&pattern_undo,
		&insertcommand_allocinit(pattern, cursor_at(song, 0, 0.0), event,
			psy_dsp_beatpos_zero(), psy_audio_song_sequence(song))->command);

	psy_audio_patternevent_clear(&event);
	event.note = NOTE_B;
	event.inst = 0;
	event.mach = SAMPLER_SLOT;
	event.cmd = 0x03;
	event.parameter = 0x20;
	psy_undoredo_execute(&pattern_undo,
		&insertcommand_allocinit(pattern, cursor_at(song, 1, 0.25), event,
			psy_dsp_beatpos_zero(), psy_audio_song_sequence(song))->command);

	if (verify_pattern_state(song) != 0) {
		psy_undoredo_dispose(&pattern_undo);
		psy_audio_song_deallocate(song);
		return 1;
	}
	psy_undoredo_undo(&pattern_undo);
	event = psy_audio_pattern_event_at_cursor(pattern, cursor_at(song, 1, 0.25));
	if (event.note == NOTE_B && event.cmd == 0x03 && event.parameter == 0x20) {
		psy_undoredo_dispose(&pattern_undo);
		psy_audio_song_deallocate(song);
		return fail("tracker undo did not remove the second edit");
	}
	psy_undoredo_redo(&pattern_undo);
	if (verify_pattern_state(song) != 0) {
		psy_undoredo_dispose(&pattern_undo);
		psy_audio_song_deallocate(song);
		return 1;
	}
	psy_undoredo_dispose(&pattern_undo);

	if (save_song(song, path) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}
	loaded = load_song(&factory, path);
	if (!loaded) {
		psy_audio_song_deallocate(song);
		return 1;
	}
	if (strcmp(psy_audio_song_title(loaded), SONG_TITLE) != 0 ||
			fabs(psy_audio_song_bpm(loaded) - SONG_BPM) > 0.001 ||
			psy_audio_song_lpb(loaded) != SONG_LPB) {
		psy_audio_song_deallocate(loaded);
		psy_audio_song_deallocate(song);
		return fail("song metadata did not survive reload");
	}
	rc = verify_machine_state(loaded);
	if (rc == 0) {
		rc = verify_pattern_state(loaded);
	}

	psy_audio_song_deallocate(loaded);
	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();

	if (rc != 0) {
		return rc;
	}
	printf("phase4-interactive-editing: PASS\n");
	printf("generated: %s\n", path);
	return 0;
}
