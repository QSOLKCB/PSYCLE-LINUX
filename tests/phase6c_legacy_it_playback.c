// SPDX-License-Identifier: GPL-2.0-or-later
/*
** Phase 6C legacy-IT sample-mode playback witness.
**
** Loads the canonical generated IT fixture through C-Psycle, proves that
** sample-mode import materializes a Sampulse instrument/virtual-generator
** route, then renders the first imported note twice from fresh state.  The
** two blocks must be bit-identical and non-silent.
*/
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <audioconfig.h>
#include <buffer.h>
#include <buffercontext.h>
#include <instrument.h>
#include <instruments.h>
#include <machine.h>
#include <machines.h>
#include <pattern.h>
#include <patternevent.h>
#include <patterns.h>
#include <player.h>
#include <properties.h>
#include <song.h>
#include <songio.h>

#define RENDER_FRAMES 512u
#define IT_INSTRUMENT_GROUP 1u

static int fail(const char* message)
{
    fprintf(stderr, "phase6c-legacy-it-playback: FAIL: %s\n", message);
    return 1;
}

static psy_audio_PatternEvent event_at(psy_audio_Song* song, int row)
{
    psy_audio_Pattern* pattern =
        psy_audio_patterns_at(psy_audio_song_patterns(song), 0);
    psy_audio_SequenceCursor cursor =
        psy_audio_sequence_cursor(psy_audio_song_sequence(song));
    psy_audio_sequencecursor_set_order_index(
        &cursor, psy_audio_orderindex_make(0, 0));
    psy_audio_sequencecursor_set_channel(&cursor, 0);
    psy_audio_sequencecursor_set_offset(
        &cursor,
        psy_dsp_beatpos_make_real(
            (double)row / psy_audio_song_lpb(song),
            psy_dsp_DEFAULT_PPQ));
    return psy_audio_pattern_event_at_cursor(pattern, cursor);
}

static int render_once(
    const char* path,
    float* left_out,
    float* right_out,
    double* peak_out,
    uintptr_t* nonzero_out,
    uintptr_t* virtual_slot_out)
{
    psy_audio_MachineCallback callback;
    psy_Property* config;
    psy_audio_AudioConfig audioconfig;
    psy_audio_Player player;
    psy_audio_Song* song;
    psy_audio_SongReader reader;
    psy_audio_Instrument* instrument;
    psy_audio_Machine* sampler;
    psy_audio_Machine* virtual_generator;
    psy_audio_PatternEvent note;
    psy_audio_Buffer buffer;
    psy_audio_BufferContext bc;
    float* left;
    float* right;
    uintptr_t frame;
    double peak = 0.0;
    uintptr_t nonzero = 0;
    int status;
    int rc = 0;

    psy_audio_machinecallback_init(&callback);
    config = psy_property_allocinit_key(NULL);
    psy_audio_audioconfig_init(&audioconfig, config);
    psy_audio_player_init(
        &player, &callback, NULL, psy_audio_audioconfig_base(&audioconfig),
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

    song = psy_audio_song_alloc_init(&player.machinefactory);
    if (!song) {
        psy_audio_player_dispose(&player);
        psy_audio_audioconfig_dispose(&audioconfig);
        psy_property_deallocate(config);
        return fail("song allocation failed");
    }
    psy_audio_machinecallback_set_song(&callback, song);
    psy_audio_machinecallback_set_player(&callback, &player);

    psy_audio_songreader_init(&reader, song, NULL, FALSE);
    status = psy_audio_songreader_load(&reader, path);
    psy_audio_songreader_dispose(&reader);
    if (status != PSY_OK) {
        rc = fail("SongReader rejected canonical IT fixture");
        goto cleanup_song;
    }

    instrument = psy_audio_instruments_at(
        psy_audio_song_instruments(song),
        psy_audio_instrumentindex_make(IT_INSTRUMENT_GROUP, 0));
    if (!instrument) {
        rc = fail("sample-mode import did not materialize Sampulse instrument 0");
        goto cleanup_song;
    }

    note = event_at(song, 0);
    if (note.note > psy_audio_NOTECOMMANDS_B9) {
        rc = fail("row-0 imported note is not playable");
        goto cleanup_song;
    }
    if (note.mach == 0 || note.mach == 255) {
        rc = fail("row-0 note was not routed through a virtual generator");
        goto cleanup_song;
    }

    sampler = psy_audio_machines_at(psy_audio_song_machines(song), 0);
    virtual_generator =
        psy_audio_machines_at(psy_audio_song_machines(song), note.mach);
    if (!sampler || psy_audio_machine_type(sampler) != psy_audio_XMSAMPLER) {
        rc = fail("imported Sampulse machine missing");
        goto cleanup_song;
    }
    if (!virtual_generator ||
        psy_audio_machine_type(virtual_generator) != psy_audio_VIRTUALGENERATOR) {
        rc = fail("sample-mode virtual generator missing");
        goto cleanup_song;
    }

    psy_audio_buffer_init(&buffer, 2);
    psy_audio_buffer_allocsamples(&buffer, RENDER_FRAMES);
    left = psy_audio_buffer_at(&buffer, 0);
    right = psy_audio_buffer_at(&buffer, 1);
    if (!left || !right) {
        psy_audio_buffer_dispose(&buffer);
        rc = fail("playback buffer allocation failed");
        goto cleanup_song;
    }
    psy_audio_buffer_clearsamples(&buffer, RENDER_FRAMES);
    psy_audio_buffercontext_init(
        &bc, NULL, &buffer, &buffer, RENDER_FRAMES, 1);

    psy_audio_machine_seq_tick(virtual_generator, 0, &note);
    psy_audio_machine_work(sampler, &bc);

    for (frame = 0; frame < RENDER_FRAMES; ++frame) {
        double a = fabs((double)left[frame]);
        double b = fabs((double)right[frame]);
        if (a > peak) peak = a;
        if (b > peak) peak = b;
        if (left[frame] != 0.0f || right[frame] != 0.0f)
            ++nonzero;
        left_out[frame] = left[frame];
        right_out[frame] = right[frame];
    }

    psy_audio_buffercontext_dispose(&bc);
    psy_audio_buffer_dispose(&buffer);

    if (nonzero == 0 || peak <= 0.0) {
        rc = fail("sample-mode playback witness is silent");
        goto cleanup_song;
    }

    *peak_out = peak;
    *nonzero_out = nonzero;
    *virtual_slot_out = note.mach;

cleanup_song:
    psy_audio_song_deallocate(song);
    psy_audio_player_dispose(&player);
    psy_audio_audioconfig_dispose(&audioconfig);
    psy_property_deallocate(config);
    return rc;
}

int main(int argc, char** argv)
{
    float left_a[RENDER_FRAMES];
    float right_a[RENDER_FRAMES];
    float left_b[RENDER_FRAMES];
    float right_b[RENDER_FRAMES];
    double peak_a = 0.0;
    double peak_b = 0.0;
    uintptr_t nonzero_a = 0;
    uintptr_t nonzero_b = 0;
    uintptr_t virtual_a = 0;
    uintptr_t virtual_b = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s FIXTURE.it\n", argv[0]);
        return 64;
    }

    psy_audio_init();
    if (render_once(
            argv[1], left_a, right_a, &peak_a, &nonzero_a, &virtual_a) != 0) {
        psy_audio_dispose();
        return 1;
    }
    if (render_once(
            argv[1], left_b, right_b, &peak_b, &nonzero_b, &virtual_b) != 0) {
        psy_audio_dispose();
        return 1;
    }
    psy_audio_dispose();

    if (virtual_a != virtual_b)
        return fail("fresh imports selected different virtual-generator slots");
    if (nonzero_a != nonzero_b || peak_a != peak_b)
        return fail("fresh playback witnesses disagree on output statistics");
    if (memcmp(left_a, left_b, sizeof(left_a)) != 0 ||
        memcmp(right_a, right_b, sizeof(right_a)) != 0)
        return fail("fresh playback witnesses are not bit-identical");

    printf(
        "{\"schema_version\":1,"
        "\"contract\":\"legacy-it-sample-mode-playback-donor\","
        "\"frames\":%u,"
        "\"virtual_generator\":%lu,"
        "\"nonzero_frames\":%lu,"
        "\"peak\":%.9g,"
        "\"repeat_render_bit_identical\":true,"
        "\"non_silent\":true}\n",
        (unsigned)RENDER_FRAMES,
        (unsigned long)virtual_a,
        (unsigned long)nonzero_a,
        peak_a);
    return 0;
}
