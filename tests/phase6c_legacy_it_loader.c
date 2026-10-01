// SPDX-License-Identifier: GPL-2.0-or-later
/* C-Psycle donor-runtime load probe for the generated Phase 6C IT fixture. */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <audioconfig.h>
#include <machine.h>
#include <pattern.h>
#include <patternevent.h>
#include <patterns.h>
#include <properties.h>
#include <player.h>
#include <sample.h>
#include <samples.h>
#include <samplerdefs.h>
#include <sequence.h>
#include <song.h>
#include <songio.h>

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-legacy-it-loader: FAIL: %s\n", message);
    return 1;
}

static psy_audio_PatternEvent event_at(psy_audio_Song* song, int row)
{
    psy_audio_Pattern* pattern = psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    psy_audio_SequenceCursor cursor = psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(&cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, 0);
    psy_audio_sequencecursor_set_offset(
        &cursor,
        psy_dsp_beatpos_make_real((double)row / psy_audio_song_lpb(song), psy_dsp_DEFAULT_PPQ));
    return psy_audio_pattern_event_at_cursor(pattern, cursor);
}

int main(int argc, char** argv)
{
    psy_audio_MachineCallback callback;
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_Player player;
    psy_audio_Song* song;
    psy_audio_SongReader reader;
    psy_audio_Sample* sample;
    psy_audio_PatternEvent z58;
    psy_audio_PatternEvent cut;
    int status;

    if (argc != 2) {
        fprintf(stderr, "usage: %s FIXTURE.it\n", argv[0]);
        return 64;
    }

    psy_audio_init();
    psy_audio_machinecallback_init(&callback);
    config = psy_property_allocinit_key(NULL);
    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_player_init(
        &player, &callback, NULL, psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = psy_audio_song_alloc_init(&player.machinefactory);
    if (!song) return fail("song allocation failed");
    psy_audio_machinecallback_set_song(&callback, song);
    psy_audio_machinecallback_set_player(&callback, &player);
    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, argv[1]);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) return fail("C-Psycle SongReader rejected generated IT fixture");

    if (strcmp(psy_audio_song_title(song), "PSYCLE IT import witness") != 0)
        return fail("title changed during IT import");
    if (fabs(psy_audio_song_bpm(song) - 140.0) > 0.000001)
        return fail("tempo changed during IT import");
    if (psy_audio_song_lpb(song) != 6)
        return fail("IT speed 4 did not map to the expected LPB 6 donor cadence");

    sample = psy_audio_samples_at(
        psy_audio_song_samples(song), psy_audio_sampleindex_make(0, 0));
    if (!sample || psy_audio_sample_num_frames(sample) != 256)
        return fail("deterministic IT sample was not imported");

    z58 = event_at(song, 5);
    if (z58.cmd != XM_SAMPLER_CMD_MIDI_MACRO || z58.parameter != 0x58)
        return fail("Z58 did not map to the donor XMSampler MIDI-macro command");

    cut = event_at(song, 6);
    if (cut.note != psy_audio_NOTECOMMANDS_RELEASE)
        return fail("current donor note-cut-to-release mapping changed");

    printf(
        "{\"contract\":\"legacy-it-import-donor-runtime\","
        "\"title\":\"%s\",\"bpm\":%.0f,\"lpb\":%d,"
        "\"sample_frames\":256,\"z58\":\"mapped\","
        "\"note_cut_254\":\"release\"}\n",
        psy_audio_song_title(song), psy_audio_song_bpm(song), psy_audio_song_lpb(song));

    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    psy_audio_dispose();
    return 0;
}
