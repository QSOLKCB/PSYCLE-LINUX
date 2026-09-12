/*
** PSYCLE-LINUX Phase 4 sequencer/transport compatibility harness.
**
** Exercises the production Sequence command/model and Sequencer timing paths
** without depending on X11 input coordinates or a physical audio device.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <player.h>
#include <plugincatcher.h>
#include <sequence.h>
#include <sequencecmds.h>
#include <sequencer.h>
#include <song.h>
#include <songio.h>
#include <undoredo.h>

#define SONG_TITLE "PSYCLE-LINUX Phase 4 sequencer fixture"
#define PATTERN_B_NAME "Bridge"
/* Legacy PSY3 stores BPM as an int32; keep the persisted compatibility
** fixture integer-valued while testing exact sequencer timing at 48 kHz. */
#define SONG_BPM 137.0
#define SONG_LPB 8
#define SAMPLE_RATE 48000.0
#define PATTERN_A_BEATS 4.0
#define PATTERN_B_BEATS 2.0
#define START_POSITION 1.25
#define ADVANCE_BEATS 0.5
#define LOOP_START 0.5
#define LOOP_BEATS 2.0

static int fail(const char* message)
{
	fprintf(stderr, "phase4-sequencer-transport: FAIL: %s\n", message);
	return 1;
}

static int close_enough(double lhs, double rhs, double epsilon)
{
	return fabs(lhs - rhs) <= epsilon;
}

static int save_song(psy_audio_Song* song, const char* path)
{
	psy_audio_SongFile songfile;
	int status;

	psy_audio_songfile_init_song(&songfile, song);
	status = psy_audio_songfile_save(&songfile, path);
	psy_audio_songfile_dispose(&songfile);
	if (status != PSY_OK) {
		fprintf(stderr, "phase4-sequencer-transport: save failed (%d)\n", status);
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
		fprintf(stderr, "phase4-sequencer-transport: load failed (%d)\n", status);
		psy_audio_song_deallocate(song);
		return NULL;
	}
	return song;
}

static int verify_sequence_layout(psy_audio_Song* song)
{
	psy_audio_Sequence* sequence;
	psy_audio_Pattern* pattern_b;
	double offset_b;
	double duration;

	sequence = psy_audio_song_sequence(song);
	if (psy_audio_sequence_num_tracks(sequence) != 2) {
		return fail("sequence track count did not persist");
	}
	if (psy_audio_sequence_track_size(sequence, 0) != 2 ||
			psy_audio_sequence_track_size(sequence, 1) != 1) {
		return fail("sequence order counts did not persist");
	}
	if (psy_audio_sequence_patternindex(sequence,
			psy_audio_orderindex_make(0, 0)) != 0 ||
			psy_audio_sequence_patternindex(sequence,
			psy_audio_orderindex_make(0, 1)) != 1 ||
			psy_audio_sequence_patternindex(sequence,
			psy_audio_orderindex_make(1, 0)) != 0) {
		return fail("sequence pattern assignments did not persist");
	}
	offset_b = psy_dsp_beatpos_real(psy_audio_sequence_offset(sequence,
		psy_audio_orderindex_make(0, 1)));
	if (!close_enough(offset_b, PATTERN_A_BEATS, 0.0001)) {
		return fail("second order offset does not follow pattern A length");
	}
	duration = psy_dsp_beatpos_real(psy_audio_sequence_duration(sequence));
	if (!close_enough(duration, PATTERN_A_BEATS + PATTERN_B_BEATS, 0.0001)) {
		return fail("sequence duration is incorrect");
	}
	pattern_b = psy_audio_patterns_at(psy_audio_song_patterns(song), 1);
	if (!pattern_b || strcmp(psy_audio_pattern_name(pattern_b), PATTERN_B_NAME) != 0 ||
			!close_enough(psy_dsp_beatpos_real(psy_audio_pattern_length(pattern_b)),
				PATTERN_B_BEATS, 0.0001)) {
		return fail("pattern B metadata did not persist");
	}
	if (!close_enough(psy_audio_song_bpm(song), SONG_BPM, 0.0001) ||
			psy_audio_song_lpb(song) != SONG_LPB) {
		return fail("song tempo/LPB did not persist");
	}
	return 0;
}

static int verify_transport(psy_audio_Song* song)
{
	psy_audio_Sequencer sequencer;
	uintptr_t frames;
	double expected_bps;
	double expected_position;

	psy_audio_sequencer_init(&sequencer, psy_audio_song_sequence(song),
		psy_audio_song_machines(song));
	psy_audio_sequencer_setsamplerate(&sequencer, SAMPLE_RATE);
	psy_audio_sequencer_set_bpm(&sequencer, SONG_BPM);
	psy_audio_sequencer_set_lpb(&sequencer, SONG_LPB);

	expected_bps = SONG_BPM / (60.0 * SAMPLE_RATE);
	if (!close_enough(psy_audio_sequencer_beats_per_sample(&sequencer),
			expected_bps, 1e-12)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("BPM/sample-rate timing is incorrect");
	}
	if (!close_enough(psy_audio_sequencer_curr_beats_per_line(&sequencer),
			1.0 / SONG_LPB, 1e-12)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("LPB does not produce the expected line duration");
	}

	psy_audio_sequencer_set_play_mode(&sequencer,
		psy_audio_SEQUENCERPLAYMODE_PLAYALL);
	psy_audio_sequencer_stop_loop(&sequencer);
	psy_audio_sequencer_set_position(&sequencer, START_POSITION);
	psy_audio_sequencer_start(&sequencer);
	if (!psy_audio_sequencer_playing(&sequencer)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("transport did not enter playing state");
	}
	frames = psy_audio_sequencer_frames(&sequencer, ADVANCE_BEATS);
	/* Psycle advances by the previously processed window: the first block
	** primes self->window and the next block advances position by that width. */
	psy_audio_sequencer_frame_tick(&sequencer, frames);
	if (!close_enough(psy_audio_sequencer_position(&sequencer),
			START_POSITION, 0.0001)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("first transport frame window did not preserve pipeline position");
	}
	psy_audio_sequencer_frame_tick(&sequencer, frames);
	expected_position = START_POSITION +
		psy_audio_sequencer_frame_to_offset(&sequencer, frames);
	if (!close_enough(psy_audio_sequencer_position(&sequencer),
			expected_position, 0.0001)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("transport position did not advance by the previous frame window");
	}
	psy_audio_sequencer_stop(&sequencer);
	if (psy_audio_sequencer_playing(&sequencer)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("transport did not stop");
	}

	psy_audio_sequencer_set_position(&sequencer, LOOP_START);
	psy_audio_sequencer_set_num_play_beats(&sequencer,
		psy_dsp_beatpos_make_real(LOOP_BEATS, psy_dsp_DEFAULT_PPQ));
	psy_audio_sequencer_set_play_mode(&sequencer,
		psy_audio_SEQUENCERPLAYMODE_PLAYNUMBEATS);
	psy_audio_sequencer_loop(&sequencer);
	psy_audio_sequencer_start(&sequencer);
	if (!psy_audio_sequencer_playing(&sequencer) ||
			!psy_audio_sequencer_looping(&sequencer) ||
			!close_enough(psy_dsp_beatpos_real(sequencer.playbeatloopstart),
				LOOP_START, 0.0001) ||
			!close_enough(psy_dsp_beatpos_real(sequencer.playbeatloopend),
				LOOP_START + LOOP_BEATS, 0.0001)) {
		psy_audio_sequencer_dispose(&sequencer);
		return fail("bounded loop transport setup is incorrect");
	}
	psy_audio_sequencer_stop(&sequencer);
	psy_audio_sequencer_dispose(&sequencer);
	return 0;
}

int main(int argc, char** argv)
{
	char path[4096];
	psy_audio_MachineCallback callback;
	psy_audio_PluginCatcher catcher;
	psy_audio_MachineFactory factory;
	psy_audio_MachineCallback loaded_callback;
	psy_audio_PluginCatcher loaded_catcher;
	psy_audio_MachineFactory loaded_factory;
	psy_audio_Song* song;
	psy_audio_Song* loaded;
	psy_audio_Sequence* sequence;
	psy_audio_Pattern* pattern_a;
	psy_audio_Pattern* pattern_b;
	psy_audio_SequenceEntry* entry_a;
	psy_audio_SequenceEntry* entry_b;
	psy_audio_SequenceSelection play_selection;
	psy_UndoRedo sequence_undo;

	if (argc != 2) {
		fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
		return 2;
	}
	if (snprintf(path, sizeof(path), "%s/phase4-sequencer-transport.psy",
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
	sequence = psy_audio_song_sequence(song);

	pattern_a = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
	if (!pattern_a) {
		psy_audio_song_deallocate(song);
		return fail("default pattern is missing");
	}
	psy_audio_pattern_set_length(pattern_a,
		psy_dsp_beatpos_make_real(PATTERN_A_BEATS, psy_dsp_DEFAULT_PPQ));
	pattern_b = psy_audio_pattern_alloc_init();
	if (!pattern_b) {
		psy_audio_song_deallocate(song);
		return fail("could not allocate pattern B");
	}
	psy_audio_pattern_set_name(pattern_b, PATTERN_B_NAME);
	psy_audio_pattern_set_length(pattern_b,
		psy_dsp_beatpos_make_real(PATTERN_B_BEATS, psy_dsp_DEFAULT_PPQ));
	psy_audio_patterns_insert(psy_audio_song_patterns(song), 1, pattern_b);

	/* Sequence View inserts after the selected order through this command. */
	psy_audio_sequenceselection_select_first(&sequence->selection,
		psy_audio_orderindex_make(0, 0));
	psy_undoredo_init(&sequence_undo);
	psy_undoredo_execute(&sequence_undo,
		&psy_audio_sequenceinsertcommand_alloc(sequence, &sequence->selection,
			psy_audio_orderindex_make(0, 0), 1)->command);
	if (psy_audio_sequence_track_size(sequence, 0) != 2 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 1)) != 1) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence insert command did not append pattern B");
	}
	psy_undoredo_undo(&sequence_undo);
	if (psy_audio_sequence_track_size(sequence, 0) != 1 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 0)) != 0) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence insert undo did not restore the original order list");
	}
	psy_undoredo_redo(&sequence_undo);
	if (psy_audio_sequence_track_size(sequence, 0) != 2 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 1)) != 1) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence insert redo did not restore pattern B");
	}
	psy_undoredo_dispose(&sequence_undo);

	/* Sequence View deletion uses SequenceRemoveCommand, whose snapshot must
	** restore the actual source tracks across undo/redo. */
	psy_audio_sequenceselection_select_first(&sequence->selection,
		psy_audio_orderindex_make(0, 1));
	psy_undoredo_init(&sequence_undo);
	psy_undoredo_execute(&sequence_undo,
		&psy_audio_sequenceremovecommand_alloc(sequence,
			&sequence->selection)->command);
	if (psy_audio_sequence_track_size(sequence, 0) != 1 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 0)) != 0) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence remove command did not remove pattern B");
	}
	psy_undoredo_undo(&sequence_undo);
	if (psy_audio_sequence_track_size(sequence, 0) != 2 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 1)) != 1) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence remove undo did not restore pattern B");
	}
	psy_undoredo_redo(&sequence_undo);
	if (psy_audio_sequence_track_size(sequence, 0) != 1) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("sequence remove redo did not remove pattern B again");
	}
	psy_undoredo_undo(&sequence_undo);
	if (psy_audio_sequence_track_size(sequence, 0) != 2 ||
			psy_audio_sequence_patternindex(sequence,
				psy_audio_orderindex_make(0, 1)) != 1) {
		psy_undoredo_dispose(&sequence_undo);
		psy_audio_song_deallocate(song);
		return fail("second sequence remove undo did not restore pattern B");
	}
	psy_undoredo_dispose(&sequence_undo);

	/* Multi-sequence layout: a second sequence track independently plays A. */
	psy_audio_sequence_append_track(sequence, psy_audio_sequencetrack_alloc_init());
	psy_audio_sequence_insert(sequence, psy_audio_orderindex_make(1, 0), 0);
	if (verify_sequence_layout(song) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}

	/* Play-selection state is what PLAYSEL uses to skip unselected orders. */
	psy_audio_sequenceselection_init(&play_selection);
	psy_audio_sequenceselection_select_first(&play_selection,
		psy_audio_orderindex_make(0, 1));
	psy_audio_sequence_set_play_selection(sequence, &play_selection);
	entry_a = psy_audio_sequence_entry(sequence, psy_audio_orderindex_make(0, 0));
	entry_b = psy_audio_sequence_entry(sequence, psy_audio_orderindex_make(0, 1));
	if (!entry_a || !entry_b || entry_a->selplay || !entry_b->selplay) {
		psy_audio_sequenceselection_dispose(&play_selection);
		psy_audio_song_deallocate(song);
		return fail("sequence play selection did not isolate pattern B");
	}
	psy_audio_sequence_clear_play_selection(sequence);
	if (entry_a->selplay || entry_b->selplay) {
		psy_audio_sequenceselection_dispose(&play_selection);
		psy_audio_song_deallocate(song);
		return fail("sequence play selection did not clear");
	}
	psy_audio_sequenceselection_dispose(&play_selection);

	if (verify_transport(song) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}
	if (save_song(song, path) != 0) {
		psy_audio_song_deallocate(song);
		return 1;
	}

	psy_audio_machinecallback_init(&loaded_callback);
	psy_audio_plugincatcher_init(&loaded_catcher, NULL);
	psy_audio_machinefactory_init(&loaded_factory, &loaded_callback,
		&loaded_catcher, NULL);
	loaded = load_song(&loaded_factory, path);
	if (!loaded) {
		psy_audio_machinefactory_dispose(&loaded_factory);
		psy_audio_plugincatcher_dispose(&loaded_catcher);
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}
	if (strcmp(psy_audio_song_title(loaded), SONG_TITLE) != 0 ||
			verify_sequence_layout(loaded) != 0 || verify_transport(loaded) != 0) {
		psy_audio_song_deallocate(loaded);
		psy_audio_machinefactory_dispose(&loaded_factory);
		psy_audio_plugincatcher_dispose(&loaded_catcher);
		psy_audio_song_deallocate(song);
		psy_audio_machinefactory_dispose(&factory);
		psy_audio_plugincatcher_dispose(&catcher);
		psy_audio_dispose();
		return 1;
	}

	psy_audio_song_deallocate(loaded);
	psy_audio_machinefactory_dispose(&loaded_factory);
	psy_audio_plugincatcher_dispose(&loaded_catcher);
	psy_audio_song_deallocate(song);
	psy_audio_machinefactory_dispose(&factory);
	psy_audio_plugincatcher_dispose(&catcher);
	psy_audio_dispose();
	puts("phase4-sequencer-transport: PASS");
	return 0;
}
