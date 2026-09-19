/* SPDX-License-Identifier: GPL-2.0-or-later
** Project-authored Phase 6C BPM/LPB/tick fixture generator.
**
** This fixture contains no third-party musical material. Four simple note
** markers are placed on consecutive tracker lines so a loader must preserve
** both the song timing metadata and the 1/LPB event spacing.
*/
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <plugincatcher.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

#define TITLE "PSYCLE-LINUX Phase 6C timing fixture"
#define BPM 137.0
#define LPB 8
#define TPB 24
#define EXTRA_TICKS 0
#define EVENT_COUNT 4

static const uint8_t NOTES[EVENT_COUNT] = {48, 50, 52, 53};

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-bpm-lpb-tick-fixture: FAIL: %s\n", message);
    return 1;
}

static int close_enough(double lhs, double rhs)
{
    return fabs(lhs - rhs) <= 0.000001;
}

static psy_audio_SequenceCursor cursor_at(psy_audio_Song* song, double offset)
{
    psy_audio_SequenceCursor cursor =
        psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(
        &cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, 0);
    psy_audio_sequencecursor_set_offset(
        &cursor, psy_dsp_beatpos_make_real(offset, psy_dsp_DEFAULT_PPQ));
    return cursor;
}

static int verify_song(psy_audio_Song* song)
{
    psy_audio_Pattern* pattern;

    if (strcmp(psy_audio_song_title(song), TITLE) != 0) {
        return fail("title changed");
    }
    if (!close_enough(psy_audio_song_bpm(song), BPM)) {
        return fail("BPM changed");
    }
    if (psy_audio_song_lpb(song) != LPB) {
        return fail("LPB changed");
    }
    if (psy_audio_song_tpb(song) != TPB) {
        return fail("ticks-per-beat changed");
    }
    if (psy_audio_song_extra_ticks_per_beat(song) != EXTRA_TICKS) {
        return fail("extra ticks changed");
    }

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern) {
        return fail("pattern 0 is missing");
    }
    if (!close_enough(
            psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)), 1.0)) {
        return fail("pattern length changed");
    }

    for (uintptr_t line = 0; line < EVENT_COUNT; ++line) {
        const double offset = (double)line / (double)LPB;
        psy_audio_SequenceCursor cursor = cursor_at(song, offset);
        psy_audio_PatternEvent event =
            psy_audio_pattern_event_at_cursor(pattern, cursor);
        if (event.note != NOTES[line]) {
            return fail("consecutive line marker changed");
        }
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
    return status == PSY_OK ? 0 : fail("PSY3 save failed");
}

static psy_audio_Song* load_song(
    psy_audio_MachineFactory* factory,
    const char* path)
{
    psy_audio_Song* song = psy_audio_song_alloc_init(factory);
    psy_audio_SongReader reader;
    int status;

    if (!song) {
        return NULL;
    }
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        psy_audio_song_deallocate(song);
        return NULL;
    }
    return song;
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
    psy_audio_Pattern* pattern;

    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    if (snprintf(
            path,
            sizeof(path),
            "%s/phase6c-bpm-lpb-tick.psy",
            argv[1]) >= (int)sizeof(path)) {
        return fail("output path too long");
    }

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    psy_audio_plugincatcher_init(&catcher, NULL);
    psy_audio_machinefactory_init(&factory, &callback, &catcher, NULL);

    song = psy_audio_song_alloc_init(&factory);
    if (!song) {
        return fail("could not allocate song");
    }
    psy_audio_song_set_title(song, TITLE);
    psy_audio_song_set_bpm(song, BPM);
    psy_audio_song_set_lpb(song, LPB);
    psy_audio_song_set_tpb(song, TPB);
    psy_audio_song_set_extra_ticks_per_beat(song, EXTRA_TICKS);

    pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern) {
        return fail("default pattern is missing");
    }
    psy_audio_pattern_set_name(pattern, "Timing Markers");
    psy_audio_pattern_set_length(
        pattern,
        psy_dsp_beatpos_make_real(1.0, psy_dsp_DEFAULT_PPQ));

    for (uintptr_t line = 0; line < EVENT_COUNT; ++line) {
        psy_audio_PatternEvent event;
        psy_audio_SequenceCursor cursor =
            cursor_at(song, (double)line / (double)LPB);
        psy_audio_patternevent_clear(&event);
        event.note = NOTES[line];
        event.inst = 0;
        event.mach = 0;
        if (!psy_audio_pattern_set_event_at_cursor(pattern, cursor, event)) {
            return fail("could not insert timing marker");
        }
    }

    if (verify_song(song) != 0 || save_song(song, path) != 0) {
        return 1;
    }

    psy_audio_machinecallback_init(&loaded_callback);
    psy_audio_plugincatcher_init(&loaded_catcher, NULL);
    psy_audio_machinefactory_init(
        &loaded_factory, &loaded_callback, &loaded_catcher, NULL);
    loaded = load_song(&loaded_factory, path);
    if (!loaded || verify_song(loaded) != 0) {
        return fail("fresh C-Psycle load did not preserve timing fixture");
    }

    psy_audio_song_deallocate(loaded);
    psy_audio_machinefactory_dispose(&loaded_factory);
    psy_audio_plugincatcher_dispose(&loaded_catcher);
    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    psy_audio_dispose();

    puts("phase6c-bpm-lpb-tick-fixture: PASS bpm=137 lpb=8 tpb=24 extra=0 positions=0,0.125,0.25,0.375");
    return 0;
}
