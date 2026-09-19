/* SPDX-License-Identifier: GPL-2.0-or-later
** Project-authored Phase 6C sequence/order fixture generator.
**
** This creates one ordinary legacy PSY3 play-order list. It deliberately avoids
** C-Psycle's later multi-sequence extension so the fixture can be compared
** directly with original Psycle 1.12.0 and the pinned C++ candidate.
*/
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <machinefactory.h>
#include <pattern.h>
#include <patterns.h>
#include <plugincatcher.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

#define TITLE "PSYCLE-LINUX Phase 6C order fixture"
#define BPM 125.0
#define LPB 4

static const uintptr_t EXPECTED_ORDER[] = {0, 2, 1, 2};
static const double PATTERN_BEATS[] = {1.0, 2.0, 3.0};
static const char* PATTERN_NAMES[] = {"Order Alpha", "Order Beta", "Order Gamma"};

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-sequence-order-fixture: FAIL: %s\n", message);
    return 1;
}

static int close_enough(double lhs, double rhs)
{
    return fabs(lhs - rhs) <= 0.0001;
}

static int verify_layout(psy_audio_Song* song)
{
    psy_audio_Sequence* sequence = psy_audio_song_sequence(song);
    double expected_offset = 0.0;

    if (psy_audio_sequence_num_tracks(sequence) != 1) {
        return fail("fixture must contain exactly one sequence track");
    }
    if (psy_audio_sequence_track_size(sequence, 0) !=
            sizeof(EXPECTED_ORDER) / sizeof(EXPECTED_ORDER[0])) {
        return fail("fixture sequence order count is wrong");
    }

    for (uintptr_t order = 0;
         order < sizeof(EXPECTED_ORDER) / sizeof(EXPECTED_ORDER[0]);
         ++order) {
        uintptr_t pattern = psy_audio_sequence_patternindex(
            sequence, psy_audio_orderindex_make(0, order));
        double offset = psy_dsp_beatpos_real(psy_audio_sequence_offset(
            sequence, psy_audio_orderindex_make(0, order)));
        if (pattern != EXPECTED_ORDER[order]) {
            return fail("fixture sequence order differs from expected pattern IDs");
        }
        if (!close_enough(offset, expected_offset)) {
            return fail("fixture sequence offset does not follow pattern lengths");
        }
        expected_offset += PATTERN_BEATS[pattern];
    }

    for (uintptr_t index = 0; index < 3; ++index) {
        psy_audio_Pattern* pattern =
            psy_audio_patterns_at(psy_audio_song_patterns(song), index);
        if (!pattern) {
            return fail("fixture pattern is missing");
        }
        if (strcmp(psy_audio_pattern_name(pattern), PATTERN_NAMES[index]) != 0) {
            return fail("fixture pattern name changed");
        }
        if (!close_enough(
                psy_dsp_beatpos_real(psy_audio_pattern_length(pattern)),
                PATTERN_BEATS[index])) {
            return fail("fixture pattern length changed");
        }
    }

    if (strcmp(psy_audio_song_title(song), TITLE) != 0 ||
        !close_enough(psy_audio_song_bpm(song), BPM) ||
        psy_audio_song_lpb(song) != LPB) {
        return fail("fixture song metadata changed");
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
    psy_audio_Sequence* sequence;
    psy_audio_Pattern* pattern0;
    psy_audio_Pattern* pattern1;
    psy_audio_Pattern* pattern2;

    if (argc != 2) {
        fprintf(stderr, "usage: %s OUTPUT_DIRECTORY\n", argv[0]);
        return 2;
    }
    if (snprintf(path, sizeof(path), "%s/phase6c-sequence-order.psy", argv[1])
            >= (int)sizeof(path)) {
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

    pattern0 = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    if (!pattern0) {
        return fail("default pattern is missing");
    }
    psy_audio_pattern_set_name(pattern0, PATTERN_NAMES[0]);
    psy_audio_pattern_set_length(
        pattern0,
        psy_dsp_beatpos_make_real(PATTERN_BEATS[0], psy_dsp_DEFAULT_PPQ));

    pattern1 = psy_audio_pattern_alloc_init();
    pattern2 = psy_audio_pattern_alloc_init();
    if (!pattern1 || !pattern2) {
        return fail("could not allocate fixture patterns");
    }
    psy_audio_pattern_set_name(pattern1, PATTERN_NAMES[1]);
    psy_audio_pattern_set_length(
        pattern1,
        psy_dsp_beatpos_make_real(PATTERN_BEATS[1], psy_dsp_DEFAULT_PPQ));
    psy_audio_pattern_set_name(pattern2, PATTERN_NAMES[2]);
    psy_audio_pattern_set_length(
        pattern2,
        psy_dsp_beatpos_make_real(PATTERN_BEATS[2], psy_dsp_DEFAULT_PPQ));
    psy_audio_patterns_insert(psy_audio_song_patterns(song), 1, pattern1);
    psy_audio_patterns_insert(psy_audio_song_patterns(song), 2, pattern2);

    sequence = psy_audio_song_sequence(song);
    if (psy_audio_sequence_num_tracks(sequence) != 1 ||
        psy_audio_sequence_track_size(sequence, 0) != 1 ||
        psy_audio_sequence_patternindex(
            sequence, psy_audio_orderindex_make(0, 0)) != 0) {
        return fail("unexpected default sequence layout");
    }

    psy_audio_sequence_insert(sequence, psy_audio_orderindex_make(0, 1), 2);
    psy_audio_sequence_insert(sequence, psy_audio_orderindex_make(0, 2), 1);
    psy_audio_sequence_insert(sequence, psy_audio_orderindex_make(0, 3), 2);

    if (verify_layout(song) != 0) {
        return 1;
    }
    if (save_song(song, path) != 0) {
        return 1;
    }

    psy_audio_machinecallback_init(&loaded_callback);
    psy_audio_plugincatcher_init(&loaded_catcher, NULL);
    psy_audio_machinefactory_init(
        &loaded_factory, &loaded_callback, &loaded_catcher, NULL);
    loaded = load_song(&loaded_factory, path);
    if (!loaded || verify_layout(loaded) != 0) {
        return fail("fresh C-Psycle load did not preserve fixture structure");
    }

    psy_audio_song_deallocate(loaded);
    psy_audio_machinefactory_dispose(&loaded_factory);
    psy_audio_plugincatcher_dispose(&loaded_catcher);
    psy_audio_song_deallocate(song);
    psy_audio_machinefactory_dispose(&factory);
    psy_audio_plugincatcher_dispose(&catcher);
    psy_audio_dispose();

    puts("phase6c-sequence-order-fixture: PASS order=0,2,1,2");
    return 0;
}
